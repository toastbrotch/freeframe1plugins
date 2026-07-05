// ============================================================
//  NDIOut.cpp
//  FreeFrame 1.0 plugin — sends the current clip as an NDI
//  source on the local network.
//
//  Resolume unloads and reloads the DLL on each slot assignment.
//  Globals reset to zero on each reload, but the NDI runtime stays
//  resident in the process — so the previous sender's name is still
//  registered and NDIlib_send_create would fail with the same name.
//
//  Solution: store the sender handle in a PID-keyed named file
//  mapping that survives DLL reload.  On the next load we retrieve
//  it and call NDIlib_send_destroy in a safe context (not during
//  DLL unload where the loader lock would cause a crash), freeing
//  the name before creating a fresh sender.
//
//  Parameters:
//    Active : 0.0 = NDI output off, 1.0 = on (default on)
//
//  NDI source name: "Resolume 2.41"
// ============================================================

#include "NDIOut.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

// ============================================================
//  NDI function pointer types
// ============================================================
typedef bool                   (*pfn_ndi_init)   (void);
typedef NDIlib_send_instance_t (*pfn_send_create)(const NDIlib_send_create_t*);
typedef void                   (*pfn_send_video_v2)(NDIlib_send_instance_t, const NDIlib_video_frame_v2_t*);
typedef void                   (*pfn_send_destroy)(NDIlib_send_instance_t);

// ============================================================
//  Global NDI state (reset to zero on every DLL reload)
// ============================================================
static HMODULE                g_NDILib        = NULL;
static bool                   g_NDILoaded     = false;
static pfn_ndi_init           g_FnInit        = NULL;
static pfn_send_create        g_FnCreate      = NULL;
static pfn_send_video_v2      g_FnSendVideo   = NULL;
static pfn_send_destroy       g_FnSendDestroy = NULL;
static NDIlib_send_instance_t g_Sender        = NULL;

static bool  g_NDIReady    = false;
static float g_ActiveNorm  = DEFAULT_ACTIVE_NORM;
static BYTE* g_ConvBuf     = NULL;
static DWORD g_ConvBufSize = 0;
static DWORD g_FrameCount  = 0;
static int   g_ErrCode     = 0;   // 0=ok 1=no env 2=no dll 3=no func 4=no sender

// ============================================================
//  Process-global shared memory
//  Stores the sender handle so it survives a DLL reload and can
//  be properly destroyed at the start of the next session.
// ============================================================
struct NDIOutPersist { UINT64 senderPtr; };
static HANDLE g_hSharedMem = NULL;

// Open (or create) the named mapping and steal the old sender handle.
// Returns NULL if there is no handle from a previous session.
static NDIlib_send_instance_t stealOldSender()
{
    char name[64];
    _snprintf(name, sizeof(name), "Local\\NDIOut241-%lu",
              (unsigned long)GetCurrentProcessId());

    g_hSharedMem = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
                                      PAGE_READWRITE, 0,
                                      sizeof(NDIOutPersist), name);
    if (!g_hSharedMem) return NULL;

    bool existed = (GetLastError() == ERROR_ALREADY_EXISTS);

    NDIOutPersist* p = (NDIOutPersist*)MapViewOfFile(
        g_hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(NDIOutPersist));
    if (!p) return NULL;

    NDIlib_send_instance_t old = NULL;
    if (existed && p->senderPtr) {
        old = (NDIlib_send_instance_t)(UINT_PTR)p->senderPtr;
        p->senderPtr = 0;
    }
    UnmapViewOfFile(p);
    // g_hSharedMem intentionally NOT closed: keeps the mapping alive
    // across DLL reloads so the next session can find the handle.
    return old;
}

// Save the current sender handle so the next DLL load can clean it up.
static void persistSender()
{
    if (!g_hSharedMem || !g_Sender) return;
    NDIOutPersist* p = (NDIOutPersist*)MapViewOfFile(
        g_hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(NDIOutPersist));
    if (p) {
        p->senderPtr = (UINT64)(UINT_PTR)g_Sender;
        UnmapViewOfFile(p);
    }
}

// ============================================================
//  ensureNDI
// ============================================================
static bool ensureNDI()
{
    if (g_NDILoaded) return (g_Sender != NULL);
    g_NDILoaded = true;

    // 1. Load the NDI runtime DLL.
    const char* ndiDir = getenv("NDI_RUNTIME_DIR_V6");
    if (!ndiDir) { g_ErrCode = 1; return false; }

    char path[512];
    _snprintf(path, sizeof(path), "%s\\%s", ndiDir, NDILIB_LIBRARY_NAME);
    g_NDILib = LoadLibraryA(path);
    if (!g_NDILib) { g_ErrCode = 2; return false; }

    g_FnInit        = (pfn_ndi_init)      GetProcAddress(g_NDILib, "NDIlib_initialize");
    g_FnCreate      = (pfn_send_create)   GetProcAddress(g_NDILib, "NDIlib_send_create");
    g_FnSendVideo   = (pfn_send_video_v2) GetProcAddress(g_NDILib, "NDIlib_send_send_video_v2");
    g_FnSendDestroy = (pfn_send_destroy)  GetProcAddress(g_NDILib, "NDIlib_send_destroy");

    if (!g_FnInit || !g_FnCreate || !g_FnSendVideo || !g_FnSendDestroy) {
        g_FnCreate = NULL; g_ErrCode = 3; return false;
    }

    // Returns false when already initialized — that's fine, runtime is usable.
    g_FnInit();

    // 2. Destroy the sender left over from the previous DLL load.
    //    We do it here (safe context) instead of at DLL-unload time
    //    (which would crash due to the Windows loader lock).
    NDIlib_send_instance_t old = stealOldSender();
    if (old) g_FnSendDestroy(old);

    // 3. Create a fresh sender.
    NDIlib_send_create_t desc = {};
    desc.p_ndi_name  = "Resolume 2.41";
    desc.clock_video = false;
    desc.clock_audio = false;
    g_Sender = g_FnCreate(&desc);

    if (!g_Sender) { g_ErrCode = 4; return false; }

    // Save handle so the next reload can find and destroy it.
    persistSender();
    g_FrameCount = 0;
    return true;
}

// ============================================================
//  Pixel conversion — BGR24 → BGRA32 (alpha = 255)
// ============================================================
void NDIOutPlugin::convertBGRtoBGRA(const BYTE* src, BYTE* dst, DWORD numPixels)
{
    for (DWORD i = 0; i < numPixels; i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 255;
        src += 3;
        dst += 4;
    }
}

// ============================================================
//  Constructor / Destructor
// ============================================================
NDIOutPlugin::NDIOutPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_INPLACE);
    SetMinInputs(1);
    SetMaxInputs(1);
    SetParamInfo(PARAM_ACTIVE, "Active", FF_TYPE_STANDARD, DEFAULT_ACTIVE_NORM);
    mDisplayBuf[0] = '\0';
    g_NDIReady = false;  // ensure ensureNDI() runs on first ProcessFrame
}

NDIOutPlugin::~NDIOutPlugin()
{
    // Global state is managed via shared memory — nothing to clean up here.
}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD NDIOutPlugin::ProcessFrame(void* pFrame)
{
    if (!g_NDIReady) {
        g_NDIReady = true;
        ensureNDI();
    }

    DWORD W     = GetFrameWidth();
    DWORD H     = GetFrameHeight();
    DWORD depth = GetFrameDepth();

    if (W == 0 || H == 0) return FF_SUCCESS;

    DWORD numPixels = W * H;
    DWORD bufBytes  = numPixels * 4;

    g_FrameCount++;

    if (g_Sender && g_ActiveNorm >= 0.5f) {
        if (bufBytes > g_ConvBufSize) {
            delete[] g_ConvBuf;
            g_ConvBuf     = new BYTE[bufBytes];
            g_ConvBufSize = bufBytes;
        }

        if (depth == FF_DEPTH_24) {
            convertBGRtoBGRA((const BYTE*)pFrame, g_ConvBuf, numPixels);
        } else if (depth == FF_DEPTH_32) {
            memcpy(g_ConvBuf, pFrame, bufBytes);
        } else {
            memset(g_ConvBuf, 0, bufBytes);
        }

        NDIlib_video_frame_v2_t frame = {};
        frame.xres                 = (int)W;
        frame.yres                 = (int)H;
        frame.FourCC               = NDIlib_FourCC_type_BGRA;
        frame.line_stride_in_bytes = (int)(W * 4);
        frame.p_data               = g_ConvBuf;
        frame.frame_rate_N         = 30000;
        frame.frame_rate_D         = 1001;
        frame.picture_aspect_ratio = (float)W / (float)H;
        frame.frame_format_type    = NDIlib_frame_format_type_progressive;
        frame.timecode             = NDIlib_send_timecode_synthesize;

        g_FnSendVideo(g_Sender, &frame);
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD NDIOutPlugin::GetParameter(DWORD index)
{
    if (index != PARAM_ACTIVE) return FF_FAIL;
    DWORD dwRet;
    memcpy(&dwRet, &g_ActiveNorm, 4);
    return dwRet;
}

DWORD NDIOutPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (!pParam || pParam->ParameterNumber != PARAM_ACTIVE) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    g_ActiveNorm = val;
    return FF_SUCCESS;
}

char* NDIOutPlugin::GetParameterDisplay(DWORD index)
{
    if (index != PARAM_ACTIVE) return NULL;
    if (g_ActiveNorm < 0.5f)
        sprintf(mDisplayBuf, "off");
    else if (!g_Sender)
        sprintf(mDisplayBuf, "E%d", g_ErrCode);
    else
        sprintf(mDisplayBuf, "on");
    return mDisplayBuf;
}

// ============================================================
//  Factory method
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new NDIOutPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

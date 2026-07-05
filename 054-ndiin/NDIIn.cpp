// ============================================================
//  NDIIn.cpp
//  FreeFrame 1.0 source plugin — receives one NDI stream and
//  outputs it as a video source inside Resolume.
//
//  The NDI runtime is loaded dynamically via NDI_RUNTIME_DIR_V6.
//  A single Source slider (0-1) selects among all currently
//  visible NDI sources on the network.  The source list is
//  refreshed every ~2 seconds automatically.
//
//  Resolume reloads the DLL on each slot assignment; all state is
//  global so it resets cleanly on each load — no shared-memory
//  tricks needed (receivers don't register global names).
//
//  Parameters:
//    Source : 0.0–1.0 — selects the NDI source by position in
//             the discovered source list (sorted alphabetically)
// ============================================================

#include "NDIIn.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

// ============================================================
//  NDI function pointer types
// ============================================================
typedef bool                   (*pfn_ndi_init)    (void);
typedef NDIlib_find_instance_t (*pfn_find_create) (const NDIlib_find_create_t*);
typedef void                   (*pfn_find_destroy)(NDIlib_find_instance_t);
typedef const NDIlib_source_t* (*pfn_find_sources)(NDIlib_find_instance_t, uint32_t*);
typedef NDIlib_recv_instance_t (*pfn_recv_create) (const NDIlib_recv_create_v3_t*);
typedef void                   (*pfn_recv_destroy)(NDIlib_recv_instance_t);
typedef NDIlib_frame_type_e    (*pfn_recv_capture)(NDIlib_recv_instance_t,
                                                   NDIlib_video_frame_v2_t*,
                                                   NDIlib_audio_frame_v3_t*,
                                                   NDIlib_metadata_frame_t*,
                                                   uint32_t);
typedef void                   (*pfn_recv_free_video)(NDIlib_recv_instance_t,
                                                      const NDIlib_video_frame_v2_t*);

// ============================================================
//  Global NDI state — resets to zero on every DLL reload
// ============================================================
static HMODULE                g_NDILib          = NULL;
static bool                   g_NDILoaded       = false;
static pfn_ndi_init           g_FnInit          = NULL;
static pfn_find_create        g_FnFindCreate    = NULL;
static pfn_find_destroy       g_FnFindDestroy   = NULL;
static pfn_find_sources       g_FnFindSources   = NULL;
static pfn_recv_create        g_FnRecvCreate    = NULL;
static pfn_recv_destroy       g_FnRecvDestroy   = NULL;
static pfn_recv_capture       g_FnRecvCapture   = NULL;
static pfn_recv_free_video    g_FnRecvFreeVideo = NULL;

static NDIlib_find_instance_t g_Finder          = NULL;
static NDIlib_recv_instance_t g_Receiver        = NULL;

// ============================================================
//  Source list — copied from the finder, refreshed periodically
// ============================================================
#define MAX_SOURCES 32
static char  g_SrcNames[MAX_SOURCES][256];
static char  g_SrcUrls [MAX_SOURCES][256];
static DWORD g_SrcCount         = 0;
static DWORD g_ConnectedSrcIdx  = 0xFFFFFFFF; // index of the active receiver
static bool  g_NeedFirstFrame   = false;       // wait longer for first frame after connect

// ============================================================
//  Working state
// ============================================================
static bool  g_NDIReady    = false;
static float g_SourceNorm  = DEFAULT_SOURCE_NORM;
static BYTE* g_LastFrame   = NULL;   // BGR24 cache of last received frame
static DWORD g_LastFrameSz = 0;
static DWORD g_FrameNum    = 0;      // counts ProcessFrame calls for refresh timing
static int   g_ErrCode     = 0;      // 0=ok 1=no env 2=no dll 3=no func

// ============================================================
//  Helpers
// ============================================================

static DWORD sourceIndexFromNorm(float norm)
{
    if (g_SrcCount == 0) return 0xFFFFFFFF;
    DWORD idx = (DWORD)(norm * (float)g_SrcCount);
    if (idx >= g_SrcCount) idx = g_SrcCount - 1;
    return idx;
}

static void refreshSources()
{
    if (!g_Finder || !g_FnFindSources) return;
    uint32_t count = 0;
    const NDIlib_source_t* srcs = g_FnFindSources(g_Finder, &count);
    DWORD n = (count < MAX_SOURCES) ? (DWORD)count : MAX_SOURCES;
    for (DWORD i = 0; i < n; i++) {
        strncpy(g_SrcNames[i], srcs[i].p_ndi_name  ? srcs[i].p_ndi_name  : "", 255);
        strncpy(g_SrcUrls [i], srcs[i].p_url_address ? srcs[i].p_url_address : "", 255);
        g_SrcNames[i][255] = g_SrcUrls[i][255] = '\0';
    }
    g_SrcCount = n;
}

static void connectToSource(DWORD idx)
{
    if (g_Receiver && g_FnRecvDestroy) {
        g_FnRecvDestroy(g_Receiver);
        g_Receiver = NULL;
    }
    g_ConnectedSrcIdx = 0xFFFFFFFF;

    if (idx >= g_SrcCount || !g_FnRecvCreate) return;

    NDIlib_recv_create_v3_t desc = {};
    desc.source_to_connect_to.p_ndi_name   = g_SrcNames[idx];
    desc.source_to_connect_to.p_url_address = g_SrcUrls[idx][0] ? g_SrcUrls[idx] : NULL;
    desc.color_format       = NDIlib_recv_color_format_BGRX_BGRA;
    desc.bandwidth          = NDIlib_recv_bandwidth_highest;
    desc.allow_video_fields = false;
    desc.p_ndi_recv_name    = "Resolume NDIIn";

    g_Receiver = g_FnRecvCreate(&desc);
    if (g_Receiver) {
        g_ConnectedSrcIdx = idx;
        g_NeedFirstFrame  = true;  // block on first capture to establish stream
    }
}

// ============================================================
//  ensureNDI — called once per DLL load on first ProcessFrame
// ============================================================
static bool ensureNDI()
{
    if (g_NDILoaded) return (g_Finder != NULL);
    g_NDILoaded = true;

    const char* ndiDir = getenv("NDI_RUNTIME_DIR_V6");
    if (!ndiDir) { g_ErrCode = 1; return false; }

    char path[512];
    _snprintf(path, sizeof(path), "%s\\%s", ndiDir, NDILIB_LIBRARY_NAME);
    g_NDILib = LoadLibraryA(path);
    if (!g_NDILib) { g_ErrCode = 2; return false; }

    g_FnInit         = (pfn_ndi_init)       GetProcAddress(g_NDILib, "NDIlib_initialize");
    g_FnFindCreate   = (pfn_find_create)    GetProcAddress(g_NDILib, "NDIlib_find_create_v2");
    g_FnFindDestroy  = (pfn_find_destroy)   GetProcAddress(g_NDILib, "NDIlib_find_destroy");
    g_FnFindSources  = (pfn_find_sources)   GetProcAddress(g_NDILib, "NDIlib_find_get_current_sources");
    g_FnRecvCreate   = (pfn_recv_create)    GetProcAddress(g_NDILib, "NDIlib_recv_create_v3");
    g_FnRecvDestroy  = (pfn_recv_destroy)   GetProcAddress(g_NDILib, "NDIlib_recv_destroy");
    g_FnRecvCapture  = (pfn_recv_capture)   GetProcAddress(g_NDILib, "NDIlib_recv_capture_v3");
    g_FnRecvFreeVideo= (pfn_recv_free_video)GetProcAddress(g_NDILib, "NDIlib_recv_free_video_v2");

    if (!g_FnInit || !g_FnFindCreate || !g_FnFindSources ||
        !g_FnRecvCreate || !g_FnRecvDestroy ||
        !g_FnRecvCapture || !g_FnRecvFreeVideo) {
        g_ErrCode = 3; return false;
    }

    g_FnInit(); // safe to call even if already initialised; ignore return value

    NDIlib_find_create_t fc = {};
    fc.show_local_sources = true;
    fc.p_groups           = NULL;
    fc.p_extra_ips        = NULL;
    g_Finder = g_FnFindCreate(&fc);

    return (g_Finder != NULL);
}

// ============================================================
//  Constructor / Destructor
// ============================================================
NDIInPlugin::NDIInPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_INPLACE);
    SetMinInputs(0);
    SetMaxInputs(0);
    SetParamInfo(PARAM_SOURCE, "Source", FF_TYPE_STANDARD, DEFAULT_SOURCE_NORM);
    mDisplayBuf[0] = '\0';
    g_NDIReady = false;
}

NDIInPlugin::~NDIInPlugin()
{
    // Globals are intentionally not cleaned up — the DLL is about to unload
    // and calling NDI functions here risks a crash (loader lock).
}

// ============================================================
//  ProcessFrame  — output buffer is provided by Resolume (source mode)
// ============================================================
DWORD NDIInPlugin::ProcessFrame(void* pFrame)
{
    if (!g_NDIReady) {
        g_NDIReady = true;
        ensureNDI();
    }

    DWORD W = GetFrameWidth();
    DWORD H = GetFrameHeight();
    if (W == 0 || H == 0) return FF_SUCCESS;

    DWORD outBytes = W * H * 3;   // BGR24 output
    BYTE* out = (BYTE*)pFrame;

    g_FrameNum++;

    // Refresh source list every ~2 seconds (60 frames).
    if (g_FrameNum % 60 == 1) {
        refreshSources();
    }

    // Reconnect receiver when selected source changes.
    if (g_Finder) {
        DWORD wantIdx = sourceIndexFromNorm(g_SourceNorm);
        if (wantIdx != g_ConnectedSrcIdx) {
            connectToSource(wantIdx);
        }
    }

    // Capture a video frame.
    // After each reconnect we wait up to 500 ms for the first frame so the
    // receiver has time to establish the stream.  After that we use timeout=0
    // (non-blocking) and fall back to the last-frame cache so the render
    // thread is never stalled.
    if (g_Receiver && g_FnRecvCapture) {
        uint32_t timeout_ms = g_NeedFirstFrame ? 500 : 0;
        NDIlib_video_frame_v2_t vf = {};
        NDIlib_frame_type_e ft = g_FnRecvCapture(g_Receiver, &vf, NULL, NULL, timeout_ms);

        if (ft == NDIlib_frame_type_video) {
            g_NeedFirstFrame = false;
            // Convert BGRA/BGRX → BGR24 with nearest-neighbour scale so any
            // NDI resolution maps to the FreeFrame canvas size.
            if (vf.p_data && vf.xres > 0 && vf.yres > 0) {
                const BYTE* srcData = vf.p_data;
                BYTE*       dstRow  = out;
                for (DWORD y = 0; y < H; y++) {
                    DWORD srcY = (DWORD)((float)y * (float)vf.yres / (float)H);
                    if (srcY >= (DWORD)vf.yres) srcY = (DWORD)vf.yres - 1;
                    const BYTE* srcRow = srcData + (size_t)srcY * vf.line_stride_in_bytes;
                    BYTE*       dst    = dstRow;
                    for (DWORD x = 0; x < W; x++) {
                        DWORD srcX = (DWORD)((float)x * (float)vf.xres / (float)W);
                        if (srcX >= (DWORD)vf.xres) srcX = (DWORD)vf.xres - 1;
                        const BYTE* src = srcRow + srcX * 4;
                        dst[0] = src[0]; // B
                        dst[1] = src[1]; // G
                        dst[2] = src[2]; // R
                        dst += 3;
                    }
                    dstRow += W * 3;
                }
                // Cache converted frame for use when no new NDI frame arrives.
                if (outBytes > g_LastFrameSz) {
                    delete[] g_LastFrame;
                    g_LastFrame   = new BYTE[outBytes];
                    g_LastFrameSz = outBytes;
                }
                memcpy(g_LastFrame, out, outBytes);
            }
            g_FnRecvFreeVideo(g_Receiver, &vf);
            return FF_SUCCESS;
        }
    }

    // No new frame this call — output the last received frame (avoids black flash).
    if (g_LastFrame && g_LastFrameSz == outBytes) {
        memcpy(out, g_LastFrame, outBytes);
    } else {
        memset(out, 0, outBytes);
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD NDIInPlugin::GetParameter(DWORD index)
{
    if (index != PARAM_SOURCE) return FF_FAIL;
    DWORD dwRet;
    memcpy(&dwRet, &g_SourceNorm, 4);
    return dwRet;
}

DWORD NDIInPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (!pParam || pParam->ParameterNumber != PARAM_SOURCE) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    g_SourceNorm = val;
    return FF_SUCCESS;
}

char* NDIInPlugin::GetParameterDisplay(DWORD index)
{
    if (index != PARAM_SOURCE) return NULL;
    if (!g_Finder) {
        sprintf(mDisplayBuf, "E%d", g_ErrCode);
    } else if (g_SrcCount == 0) {
        sprintf(mDisplayBuf, "no sources");
    } else {
        DWORD idx = sourceIndexFromNorm(g_SourceNorm);
        // Show last 14 chars of name so it fits in 16-char display buffer.
        const char* name = g_SrcNames[idx];
        size_t len = strlen(name);
        if (len > 14) name += len - 14;
        sprintf(mDisplayBuf, "%s", name);
    }
    return mDisplayBuf;
}

// ============================================================
//  Factory method
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new NDIInPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

#include "ISFEffect.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <commctrl.h>

// ------------------------------------------------------------------
// GL 2.0 extension types & pointers
// ------------------------------------------------------------------
typedef char GLchar;
typedef GLuint (WINAPI *PFNGLCREATESHADERPROC)(GLenum);
typedef void  (WINAPI *PFNGLSHADERSOURCEPROC)(GLuint, GLsizei, const GLchar**, const GLint*);
typedef void  (WINAPI *PFNGLCOMPILESHADERPROC)(GLuint);
typedef void  (WINAPI *PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef void  (WINAPI *PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint(WINAPI *PFNGLCREATEPROGRAMPROC)();
typedef void  (WINAPI *PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void  (WINAPI *PFNGLLINKPROGRAMPROC)(GLuint);
typedef void  (WINAPI *PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void  (WINAPI *PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void  (WINAPI *PFNGLUSEPROGRAMPROC)(GLuint);
typedef GLint (WINAPI *PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void  (WINAPI *PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void  (WINAPI *PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void  (WINAPI *PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void  (WINAPI *PFNGLDELETESHADERPROC)(GLuint);
typedef void  (WINAPI *PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void  (WINAPI *PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei, GLuint*);
typedef void  (WINAPI *PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum, GLuint);
typedef void  (WINAPI *PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum, GLenum, GLenum, GLuint, GLint);
typedef GLenum(WINAPI *PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum);
typedef void  (WINAPI *PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei, const GLuint*);
typedef void  (WINAPI *PFNGLACTIVETEXTUREPROC)(GLenum);

static PFNGLCREATESHADERPROC           glCreateShader_          = NULL;
static PFNGLSHADERSOURCEPROC           glShaderSource_          = NULL;
static PFNGLCOMPILESHADERPROC          glCompileShader_         = NULL;
static PFNGLGETSHADERIVPROC            glGetShaderiv_           = NULL;
static PFNGLGETSHADERINFOLOGPROC       glGetShaderInfoLog_      = NULL;
static PFNGLCREATEPROGRAMPROC          glCreateProgram_         = NULL;
static PFNGLATTACHSHADERPROC           glAttachShader_          = NULL;
static PFNGLLINKPROGRAMPROC            glLinkProgram_           = NULL;
static PFNGLGETPROGRAMIVPROC           glGetProgramiv_          = NULL;
static PFNGLGETPROGRAMINFOLOGPROC      glGetProgramInfoLog_     = NULL;
static PFNGLUSEPROGRAMPROC             glUseProgram_            = NULL;
static PFNGLGETUNIFORMLOCATIONPROC     glGetUniformLocation_    = NULL;
static PFNGLUNIFORM1FPROC              glUniform1f_             = NULL;
static PFNGLUNIFORM1IPROC              glUniform1i_             = NULL;
static PFNGLUNIFORM2FPROC              glUniform2f_             = NULL;
static PFNGLDELETESHADERPROC           glDeleteShader_          = NULL;
static PFNGLDELETEPROGRAMPROC          glDeleteProgram_         = NULL;
static PFNGLGENFRAMEBUFFERSEXTPROC     glGenFramebuffersEXT_    = NULL;
static PFNGLBINDFRAMEBUFFEREXTPROC     glBindFramebufferEXT_    = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT_ = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT_ = NULL;
static PFNGLDELETEFRAMEBUFFERSEXTPROC  glDeleteFramebuffersEXT_ = NULL;
static PFNGLACTIVETEXTUREPROC          glActiveTexture_         = NULL;

#define GL_FRAMEBUFFER_EXT              0x8D40
#define GL_COLOR_ATTACHMENT0_EXT        0x8CE0
#define GL_FRAMEBUFFER_COMPLETE_EXT     0x8CD5
#define GL_FRAGMENT_SHADER              0x8B30
#define GL_VERTEX_SHADER                0x8B31
#define GL_COMPILE_STATUS               0x8B81
#define GL_LINK_STATUS                  0x8B82
#define GL_TEXTURE0                     0x84C0
#define GL_CLAMP_TO_EDGE                0x812F
#define GL_RGBA16F                      0x881A

#define ERROR_LOG "C:\\isf_effect_error.txt"

#define LOADEXT(T, name) \
    name##_ = (T)wglGetProcAddress(#name); \
    if (!name##_) return false;

// ------------------------------------------------------------------
// Panel constants
// ------------------------------------------------------------------
#define IDC_LISTBOX    101
#define IDC_LBL_NAME   110
// Each block needs room for MAX_ALL_INPUTS ids (and the slot buttons for
// MAX_ALL_INPUTS*NUM_FF_PARAMS), so the blocks must not overlap: 200.., 300.., 400..527.
#define IDC_LBL_P0     200   // IDC_LBL_P0 + i  (i = 0..MAX_ALL_INPUTS-1)
#define IDC_TRACK_P0   300   // IDC_TRACK_P0 + i
// One BS_AUTORADIOBUTTON|BS_PUSHLIKE per (input row, slot) pair, so the
// currently-bound slot renders visibly pressed/highlighted. Row i's 4
// buttons are IDC_BTN_SLOT0 + i*NUM_FF_PARAMS + s (s = 0..NUM_FF_PARAMS-1).
#define IDC_BTN_SLOT0  400

#define TRACK_RANGE    1000

// Counts plugin instances created this process, so simultaneously-loaded
// copies (e.g. two ISF Effect layers) still get distinct seeds even when
// GetTickCount() ties between them.
static volatile LONG s_isfEffInstanceCounter = 0;

#define CLR_BG        RGB(14,  14,  14)
#define CLR_LIST_BG   RGB( 6,   6,   6)
#define CLR_TEXT      RGB(237,   5, 165)  // old-Resolume magenta (#ed05a5), was greenish
#define CLR_DIM       RGB(142,   3,  99)  // ~60% of CLR_TEXT, same ratio the old dim green used
#define CLR_LABEL     RGB(200, 200, 200)  // light grey: list + labels (accent kept for buttons/shader name)

// ------------------------------------------------------------------
// CreateInstance
// ------------------------------------------------------------------
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new ISFEffectPlugin();
    return (*ppInstance) ? FF_SUCCESS : FF_FAIL;
}

// ------------------------------------------------------------------
// ISF JSON parsing
// ------------------------------------------------------------------
const char* ISFEffectPlugin::findKeyVal(const char* json, const char* key)
{
    char search[80];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char* p = strstr(json, search);
    if (!p) return NULL;
    p += strlen(search);
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

float ISFEffectPlugin::parseFloatAt(const char* p, float def)
{
    if (!p) return def;
    char* end;
    float v = strtof(p, &end);
    return (end == p) ? def : v;
}

bool ISFEffectPlugin::parseStringAt(const char* p, char* out, int maxLen)
{
    if (!p || *p != '"') return false;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxLen - 1) out[i++] = *p++;
    out[i] = '\0';
    return i > 0;
}

bool ISFEffectPlugin::parseISFInputs(const char* header,
                                       ISFInput* out, int* count, int maxCount)
{
    *count = 0;
    const char* p = strstr(header, "\"INPUTS\"");
    if (!p) return false;
    p += 8;
    while (*p && *p != '[') p++;
    if (!*p) return false;
    p++;

    while (*p && *count < maxCount) {
        while (*p && *p != '{' && *p != ']') p++;
        if (!*p || *p == ']') break;

        const char* blockStart = p;
        int depth = 0;
        const char* q = p;
        while (*q) {
            if (*q == '{') depth++;
            else if (*q == '}') { depth--; if (depth == 0) { q++; break; } }
            q++;
        }

        std::string block(blockStart, (size_t)(q - blockStart));
        const char* b = block.c_str();

        char typeStr[32] = "float";
        const char* tp = findKeyVal(b, "TYPE");
        if (tp) parseStringAt(tp, typeStr, sizeof(typeStr));
        if (strcmp(typeStr, "float") != 0 &&
            strcmp(typeStr, "bool")  != 0 &&
            strcmp(typeStr, "long")  != 0) { p = q; continue; }

        ISFInput& inp = out[*count];
        strcpy(inp.name, "p");
        inp.minVal = 0.0f; inp.maxVal = 1.0f; inp.defaultVal = 0.0f;

        parseStringAt(findKeyVal(b, "NAME"),    inp.name, sizeof(inp.name));
        inp.minVal     = parseFloatAt(findKeyVal(b, "MIN"),     0.0f);
        inp.maxVal     = parseFloatAt(findKeyVal(b, "MAX"),     1.0f);
        inp.defaultVal = parseFloatAt(findKeyVal(b, "DEFAULT"), 0.0f);

        (*count)++;
        p = q;
    }
    return true;
}

void ISFEffectPlugin::parseISFExtras(const char* header,
                                       ISFExtraUniform* out, int* count, int maxCount)
{
    *count = 0;
    const char* p = strstr(header, "\"INPUTS\"");
    if (!p) return;
    p += 8;
    while (*p && *p != '[') p++;
    if (!*p) return;
    p++;
    while (*p && *count < maxCount) {
        while (*p && *p != '{' && *p != ']') p++;
        if (!*p || *p == ']') break;
        const char* blockStart = p;
        int depth = 0; const char* q = p;
        while (*q) {
            if (*q == '{') depth++;
            else if (*q == '}') { if (--depth == 0) { q++; break; } }
            q++;
        }
        std::string block(blockStart, (size_t)(q - blockStart));
        const char* b = block.c_str();
        char typeStr[32] = "float";
        const char* tp = findKeyVal(b, "TYPE");
        if (tp) parseStringAt(tp, typeStr, sizeof(typeStr));
        ISFExtraType et;
        if      (strcmp(typeStr, "color")    == 0) et = ISF_EXTRA_COLOR;
        else if (strcmp(typeStr, "point2D")  == 0) et = ISF_EXTRA_POINT2D;
        else if (strcmp(typeStr, "event")    == 0) et = ISF_EXTRA_EVENT;
        else if (strcmp(typeStr, "image")    == 0 ||
                 strcmp(typeStr, "audio")    == 0 ||
                 strcmp(typeStr, "audioFFT") == 0) et = ISF_EXTRA_SAMPLER;
        else { p = q; continue; }
        ISFExtraUniform& eu = out[*count];
        eu.type = et; eu.loc = -1;
        strcpy(eu.name, "extra");
        const char* np = findKeyVal(b, "NAME");
        if (np) parseStringAt(np, eu.name, sizeof(eu.name));
        (*count)++;
        p = q;
    }
}

// Reads the ISF "PASSES" array: one entry per pass, each with an optional
// TARGET (named buffer), FLOAT and FILTER. Targets are deduplicated by name;
// PERSISTENT is implied (every target keeps its content between frames, as in
// the ffgl ISF Effect). WIDTH/HEIGHT expressions are ignored - full size only.
void ISFEffectPlugin::parsePasses(const char* header)
{
    mPassCount = 0;
    mTargetCount = 0;
    const char* p = strstr(header, "\"PASSES\"");
    if (!p) return;
    p += 8;
    while (*p && *p != '[') p++;
    if (!*p) return;
    p++;
    while (*p && mPassCount < MAX_PASSES) {
        while (*p && *p != '{' && *p != ']') p++;
        if (!*p || *p == ']') break;
        const char* blockStart = p;
        int depth = 0; const char* q = p;
        while (*q) {
            if (*q == '{') depth++;
            else if (*q == '}') { if (--depth == 0) { q++; break; } }
            q++;
        }
        std::string block(blockStart, (size_t)(q - blockStart));
        const char* b = block.c_str();

        char target[64] = "";
        parseStringAt(findKeyVal(b, "TARGET"), target, sizeof(target));
        int ti = -1;
        if (target[0]) {
            for (int t = 0; t < mTargetCount; ++t)
                if (strcmp(mTargets[t].name, target) == 0) ti = t;
            if (ti < 0 && mTargetCount < MAX_TARGETS) {
                ISFTarget& T = mTargets[mTargetCount];
                strcpy(T.name, target);
                const char* fp = findKeyVal(b, "FLOAT");
                T.isFloat = fp && strncmp(fp, "true", 4) == 0;
                char filt[16] = "";
                parseStringAt(findKeyVal(b, "FILTER"), filt, sizeof(filt));
                T.nearest = strcmp(filt, "NEAREST") == 0;
                T.tex[0] = T.tex[1] = T.fbo[0] = T.fbo[1] = 0;
                T.cur = 0;
                T.loc = -1;
                ti = mTargetCount++;
            }
        }
        mPassTarget[mPassCount++] = ti;
        p = q;
    }
}

static std::string stripPrecisionLines(const char* src)
{
    std::string out;
    const char* p = src;
    while (*p) {
        const char* lineEnd = p;
        while (*lineEnd && *lineEnd != '\n') lineEnd++;
        const char* lp = p;
        while (*lp == ' ' || *lp == '\t') lp++;
        bool skip = (strncmp(lp, "precision", 9) == 0 &&
                     (lp[9] == ' ' || lp[9] == '\t' || lp[9] == '\n' || !lp[9]));
        if (!skip) {
            out.append(p, lineEnd - p);
            if (*lineEnd == '\n') out += '\n';
        }
        p = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
    }
    return out;
}

// ------------------------------------------------------------------
// Panel helpers (free functions — need access to public members)
// ------------------------------------------------------------------
static void buildParamLabel(char* buf, int len,
                             ISFEffectPlugin* self, int i)
{
    float t = self->mAllParams[i];
    if (i < self->mInputCount) {
        float val = self->mInputs[i].minVal
                  + t * (self->mInputs[i].maxVal - self->mInputs[i].minVal);
        snprintf(buf, len, "%-14s  %.3f  [%.1f – %.1f]",
                 self->mInputs[i].name, val,
                 self->mInputs[i].minVal, self->mInputs[i].maxVal);
    } else {
        snprintf(buf, len, "—");
    }
}

static void syncTrackFromAllParams(HWND hwnd, ISFEffectPlugin* self, int i)
{
    HWND ht  = GetDlgItem(hwnd, IDC_TRACK_P0 + i);
    int  pos = (int)(self->mAllParams[i] * TRACK_RANGE + 0.5f);
    int  cur = (int)SendMessageA(ht, TBM_GETPOS, 0, 0);
    if (abs(cur - pos) > 2)
        SendMessageA(ht, TBM_SETPOS, TRUE, (LPARAM)pos);
}

// Which Resolume slot (0..NUM_FF_PARAMS-1), if any, is currently bound to
// ISF input i. Returns -1 if input i isn't bound to any slot.
static int slotForInput(ISFEffectPlugin* self, int i)
{
    for (int s = 0; s < NUM_FF_PARAMS; ++s)
        if (self->mSlotToInput[s] == i) return s;
    return -1;
}

static int slotBtnId(int i, int s) { return IDC_BTN_SLOT0 + i*NUM_FF_PARAMS + s; }

// Forces a repaint of row i's 4 (owner-drawn) slot buttons so WM_DRAWITEM
// re-reads mSlotToInput and reflects the current binding.
static void syncSlotButtons(HWND hwnd, ISFEffectPlugin* /*self*/, int i)
{
    for (int s = 0; s < NUM_FF_PARAMS; ++s)
        InvalidateRect(GetDlgItem(hwnd, slotBtnId(i, s)), NULL, FALSE);
}

// Binds ISF input i to Resolume slot `slot` (or unbinds it if slot < 0),
// evicting whichever input previously held that slot — each slot can only
// point at one input at a time. Pushes the input's current value into
// mParam right away so the Resolume UI doesn't show a stale number.
static void assignSlot(ISFEffectPlugin* self, int i, int slot)
{
    int oldSlot = slotForInput(self, i);
    if (oldSlot >= 0) self->mSlotToInput[oldSlot] = -1;
    if (slot >= 0) {
        self->mSlotToInput[slot] = i;  // overwrite implicitly evicts any prior owner
        self->mParam[slot] = self->mAllParams[i];
    }
}

// ------------------------------------------------------------------
// Panel — DPI / frame helpers
// ------------------------------------------------------------------
// Resolume 2.41 is not DPI-aware, so Windows bitmap-stretches its windows
// (a 300px panel shows up 375px wide at 125% scaling). Opting just the panel
// thread into per-monitor awareness makes its pixel sizes real screen pixels.
// Loaded dynamically: SetThreadDpiAwarenessContext needs Windows 10 1607+.
#define PANEL_VISIBLE_W 300
#define PANEL_ONE_COLUMN_MAX 16   // more inputs than this -> two columns of rows, so the
                                  // panel stays shorter than a screen with up to MAX_ALL_INPUTS

static void makeThreadDpiAware()
{
    typedef HANDLE (WINAPI *SetCtxFn)(HANDLE);
    HMODULE u32 = GetModuleHandleA("user32.dll");
    SetCtxFn fn = u32 ? (SetCtxFn)GetProcAddress(u32, "SetThreadDpiAwarenessContext") : NULL;
    if (!fn) return;
    if (!fn((HANDLE)(LONG_PTR)-4))   // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        fn((HANDLE)(LONG_PTR)-3);    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE
}

// Width of the invisible resize borders: GetWindowRect minus the visible
// frame reported by DWM (DWMWA_EXTENDED_FRAME_BOUNDS). 0 if DWM unavailable.
static int invisibleFrameW(HWND hwnd, const RECT& wr)
{
    typedef HRESULT (WINAPI *DwmGetAttrFn)(HWND, DWORD, PVOID, DWORD);
    static DwmGetAttrFn fn = NULL;
    static bool tried = false;
    if (!tried) {
        tried = true;
        HMODULE dwm = LoadLibraryA("dwmapi.dll");
        if (dwm) fn = (DwmGetAttrFn)GetProcAddress(dwm, "DwmGetWindowAttribute");
    }
    RECT vis;
    if (!fn || fn(hwnd, 9 /* DWMWA_EXTENDED_FRAME_BOUNDS */, &vis, sizeof(vis)) != S_OK)
        return 0;
    int d = (wr.right - wr.left) - (vis.right - vis.left);
    return d > 0 ? d : 0;
}

// ------------------------------------------------------------------
// Panel — window procedure
// ------------------------------------------------------------------
LRESULT CALLBACK ISFEffectPlugin::panelWndProc(HWND hwnd, UINT msg,
                                                  WPARAM wp, LPARAM lp)
{
    static HBRUSH hBrBg     = NULL;
    static HBRUSH hBrList   = NULL;
    static HBRUSH hBrAccent = NULL;  // active slot-button fill (CLR_TEXT)
    static HFONT  hFont     = NULL;

    ISFEffectPlugin* self =
        (ISFEffectPlugin*)GetWindowLongA(hwnd, GWL_USERDATA);

    switch (msg) {

    case WM_CREATE: {
        self = (ISFEffectPlugin*)((CREATESTRUCTA*)lp)->lpCreateParams;
        SetWindowLongA(hwnd, GWL_USERDATA, (LONG)(LONG_PTR)self);

        if (!hBrBg)     hBrBg     = CreateSolidBrush(CLR_BG);
        if (!hBrList)   hBrList   = CreateSolidBrush(CLR_LIST_BG);
        if (!hBrAccent) hBrAccent = CreateSolidBrush(CLR_TEXT);
        if (!hFont)     hFont     = CreateFontA(14, 0, 0, 0, FW_NORMAL,
                                    FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                    FIXED_PITCH | FF_MODERN, "Courier New");

        // File listbox
        HWND lb = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            8, 8, 284, 240, hwnd, (HMENU)IDC_LISTBOX, NULL, NULL);
        SendMessageA(lb, WM_SETFONT, (WPARAM)hFont, FALSE);

        int fc = self->mSharedFileCount;
        for (int i = 0; i < fc; ++i)
            SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)self->mSharedFileLabels[i]);
        int ci = self->mSharedCurrentIdx;
        if (ci >= 0 && ci < fc)
            SendMessageA(lb, LB_SETCURSEL, (WPARAM)ci, 0);

        // Shader name label
        HWND ln = CreateWindowExA(0, "STATIC", self->mSharedShaderName,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            8, 256, 284, 16, hwnd, (HMENU)IDC_LBL_NAME, NULL, NULL);
        SendMessageA(ln, WM_SETFONT, (WPARAM)hFont, FALSE);

        // Pre-create MAX_ALL_INPUTS label+trackbar rows, each with 4 slot
        // radio-buttons (P1-P4), hidden initially.
        char buf[80];
        for (int i = 0; i < MAX_ALL_INPUTS; ++i) {
            buildParamLabel(buf, sizeof(buf), self, i);
            HWND lbl = CreateWindowExA(0, "STATIC", buf,
                WS_CHILD | SS_LEFT,   // not WS_VISIBLE yet
                8, 0, 174, 15,
                hwnd, (HMENU)(UINT_PTR)(IDC_LBL_P0 + i), NULL, NULL);
            SendMessageA(lbl, WM_SETFONT, (WPARAM)hFont, FALSE);

            // BS_OWNERDRAW: exclusivity is our own bookkeeping (mSlotToInput),
            // not the OS's radio-group logic — needed so WM_DRAWITEM can
            // paint the active slot with a solid green fill + white text
            // (plain BS_PUSHBUTTON/BS_PUSHLIKE controls ignore WM_CTLCOLORBTN
            // and can't be given a custom face color any other way).
            for (int s = 0; s < NUM_FF_PARAMS; ++s) {
                char cap[4]; snprintf(cap, sizeof(cap), "%d", s + 1);
                HWND sb = CreateWindowExA(0, "BUTTON", cap,
                    WS_CHILD | BS_OWNERDRAW,   // not WS_VISIBLE yet
                    188 + s*26, 0, 24, 18,
                    hwnd, (HMENU)(UINT_PTR)slotBtnId(i, s), NULL, NULL);
                SendMessageA(sb, WM_SETFONT, (WPARAM)hFont, FALSE);
            }

            HWND trk = CreateWindowExA(0, TRACKBAR_CLASSA, NULL,
                WS_CHILD | TBS_HORZ | TBS_NOTICKS,   // not WS_VISIBLE yet
                8, 0, 284, 22,
                hwnd, (HMENU)(UINT_PTR)(IDC_TRACK_P0 + i), NULL, NULL);
            SendMessageA(trk, TBM_SETRANGE, FALSE, MAKELPARAM(0, TRACK_RANGE));
            int pos = (int)(self->mAllParams[i] * TRACK_RANGE + 0.5f);
            SendMessageA(trk, TBM_SETPOS, TRUE, (LPARAM)pos);
        }

        SetTimer(hwnd, 1, 150, NULL);
        return 0;
    }

    case WM_TIMER: {
        if (!self) return 0;
        HWND lb = GetDlgItem(hwnd, IDC_LISTBOX);

        // Rebuild listbox if count changed
        int fc  = self->mSharedFileCount;
        if ((int)SendMessageA(lb, LB_GETCOUNT, 0, 0) != fc) {
            SendMessageA(lb, LB_RESETCONTENT, 0, 0);
            for (int i = 0; i < fc; ++i)
                SendMessageA(lb, LB_ADDSTRING, 0,
                             (LPARAM)self->mSharedFileLabels[i]);
            self->mPanelSeenIdx = -1;   // selection was lost, re-apply below
        }

        // Safety net for a user click: LBN_SELCHANGE already requested the
        // shader; re-read lb.cursel here in case it wasn't committed yet.
        if (self->mUserClicked) {
            int lbSel = (int)SendMessageA(lb, LB_GETCURSEL, 0, 0);
            if (lbSel != LB_ERR && lbSel >= 0 && lbSel != self->mSharedCurrentIdx)
                self->mRequestedIdx = lbSel;
            self->mUserClicked = false;
        }

        // UI first: the listbox selection belongs to the user. Only follow
        // the loaded shader when it actually changes (initial load, rebuild),
        // never just because the render thread hasn't picked up a click yet,
        // and never mid-click (the listbox selects on mouse-down but only
        // sends LBN_SELCHANGE on mouse-up; syncing in between reverted it).
        int ci = self->mSharedCurrentIdx;
        if (ci != self->mPanelSeenIdx && GetKeyState(VK_LBUTTON) >= 0) {
            self->mPanelSeenIdx = ci;
            if (self->mRequestedIdx < 0 && ci >= 0 &&
                (int)SendMessageA(lb, LB_GETCURSEL, 0, 0) != ci)
                SendMessageA(lb, LB_SETCURSEL, (WPARAM)ci, 0);
        }

        SetWindowTextA(GetDlgItem(hwnd, IDC_LBL_NAME), self->mSharedShaderName);

        // Show/hide + reposition controls based on current input count.
        int ic = self->mInputCount;

        if (ic != self->mPanelLastIC) {
            // Resize window to fit: PANEL_VISIBLE_W wide per column as seen on
            // screen, i.e. excluding the invisible resize borders Windows 10+
            // adds around WS_SIZEBOX windows (DWM extended frame bounds).
            // Past PANEL_ONE_COLUMN_MAX inputs the rows go into two columns
            // (filled top-to-bottom, then the second column) and the file list
            // gets shorter, so 32 inputs still fit on a 1080p screen.
            int cols    = (ic > PANEL_ONE_COLUMN_MAX) ? 2 : 1;
            int rows    = (ic + cols - 1) / cols;
            int listH   = (cols == 2) ? 160 : 240;
            int nameY   = 8 + listH + 8;
            int top     = nameY + 22;
            int clientH = (ic > 0) ? (top + rows * 44 + 10) : 290;
            RECT cr; GetClientRect(hwnd, &cr);
            RECT wr; GetWindowRect(hwnd, &wr);
            int chromaH = (wr.bottom - wr.top) - (cr.bottom - cr.top);
            int outerW  = cols * PANEL_VISIBLE_W - (cols - 1) * 8 + invisibleFrameW(hwnd, wr);
            SetWindowPos(hwnd, NULL, 0, 0,
                         outerW, clientH + chromaH,
                         SWP_NOMOVE | SWP_NOZORDER);

            // Lay out controls against the resulting client width
            GetClientRect(hwnd, &cr);
            int cw   = cr.right - cr.left;
            int full = cw - 16;          // 8px margin each side
            int colW = (cw - 8 * (cols + 1)) / cols;
            SetWindowPos(GetDlgItem(hwnd, IDC_LISTBOX),  NULL, 8, 8,     full, listH, SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd, IDC_LBL_NAME), NULL, 8, nameY, full, 16,    SWP_NOZORDER);

            for (int i = 0; i < MAX_ALL_INPUTS; ++i) {
                HWND lbl = GetDlgItem(hwnd, IDC_LBL_P0   + i);
                HWND trk = GetDlgItem(hwnd, IDC_TRACK_P0 + i);
                if (i < ic) {
                    int x    = 8 + (i / rows) * (colW + 8);
                    int y    = top + (i % rows) * 44;
                    int btnX = x + colW - 104;   // 4 slot buttons, right-aligned in the column
                    SetWindowPos(lbl, NULL, x, y,      btnX - x - 6, 15, SWP_NOZORDER);
                    for (int s = 0; s < NUM_FF_PARAMS; ++s)
                        SetWindowPos(GetDlgItem(hwnd, slotBtnId(i, s)), NULL,
                                     btnX + s*26, y - 1, 24, 18, SWP_NOZORDER);
                    SetWindowPos(trk, NULL, x, y + 17, colW, 22, SWP_NOZORDER);
                    ShowWindow(lbl, SW_SHOW);
                    for (int s = 0; s < NUM_FF_PARAMS; ++s)
                        ShowWindow(GetDlgItem(hwnd, slotBtnId(i, s)), SW_SHOW);
                    ShowWindow(trk, SW_SHOW);
                } else {
                    ShowWindow(lbl, SW_HIDE);
                    for (int s = 0; s < NUM_FF_PARAMS; ++s)
                        ShowWindow(GetDlgItem(hwnd, slotBtnId(i, s)), SW_HIDE);
                    ShowWindow(trk, SW_HIDE);
                }
            }

            self->mPanelLastIC = ic;
        }

        // Update labels + slot-button pressed-state + sync trackbars
        char buf[80];
        for (int i = 0; i < ic; ++i) {
            buildParamLabel(buf, sizeof(buf), self, i);
            SetWindowTextA(GetDlgItem(hwnd, IDC_LBL_P0 + i), buf);
            syncSlotButtons(hwnd, self, i);
            syncTrackFromAllParams(hwnd, self, i);
        }
        return 0;
    }

    case WM_HSCROLL: {
        HWND hCtrl = (HWND)lp;
        int  id    = GetDlgCtrlID(hCtrl);
        if (self && id >= IDC_TRACK_P0 && id < IDC_TRACK_P0 + MAX_ALL_INPUTS) {
            int  i   = id - IDC_TRACK_P0;
            int  pos = (int)SendMessageA(hCtrl, TBM_GETPOS, 0, 0);
            float v  = (float)pos / (float)TRACK_RANGE;
            self->mAllParams[i] = v;
            // Keep the Resolume slider in sync, if this input is bound to one
            int slot = slotForInput(self, i);
            if (slot >= 0) self->mParam[slot] = v;
            // Update label immediately
            char buf[80];
            buildParamLabel(buf, sizeof(buf), self, i);
            SetWindowTextA(GetDlgItem(hwnd, IDC_LBL_P0 + i), buf);
        }
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_LISTBOX && HIWORD(wp) == LBN_SELCHANGE && self) {
            // Request the shader right away; WM_TIMER re-reads lb.cursel
            // once more (mUserClicked) in case it wasn't committed yet.
            int sel = (int)SendMessageA((HWND)lp, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel != self->mSharedCurrentIdx)
                self->mRequestedIdx = sel;
            self->mUserClicked = true;
        } else if (self && HIWORD(wp) == BN_CLICKED &&
                   LOWORD(wp) >= IDC_BTN_SLOT0 &&
                   LOWORD(wp) < IDC_BTN_SLOT0 + MAX_ALL_INPUTS*NUM_FF_PARAMS) {
            // Direct per-slot buttons (PARAM_SHADER is just slot 0 here —
            // shader switching lives in the file list now). Clicking the
            // already-bound slot again disconnects it; clicking any other
            // slot binds there, evicting whichever row previously held it.
            int id = LOWORD(wp) - IDC_BTN_SLOT0;
            int i  = id / NUM_FF_PARAMS;
            int s  = id % NUM_FF_PARAMS;
            bool wasActive = (slotForInput(self, i) == s);
            assignSlot(self, i, wasActive ? -1 : s);

            // The click target's own group auto-updates its pressed look,
            // but an evicted row elsewhere needs its button un-pressed too —
            // cheapest correct fix is to just resync every visible row.
            int ic = self->mInputCount;
            for (int k = 0; k < ic; ++k)
                syncSlotButtons(hwnd, self, k);
        }
        return 0;

    case WM_ERASEBKGND:
        if (hBrBg) {
            RECT r; GetClientRect(hwnd, &r);
            FillRect((HDC)wp, &r, hBrBg);
            return 1;
        }
        break;

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;
        int id = (int)dis->CtlID;
        if (!self || id < IDC_BTN_SLOT0 || id >= IDC_BTN_SLOT0 + MAX_ALL_INPUTS*NUM_FF_PARAMS)
            break;
        int rel = id - IDC_BTN_SLOT0;
        int i   = rel / NUM_FF_PARAMS;
        int s   = rel % NUM_FF_PARAMS;
        bool active = (slotForInput(self, i) == s);

        FillRect(dis->hDC, &dis->rcItem, active ? hBrAccent : hBrList);
        DrawEdge(dis->hDC, &dis->rcItem, active ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);

        char cap[4];
        GetWindowTextA(dis->hwndItem, cap, sizeof(cap));
        SetBkMode(dis->hDC, TRANSPARENT);
        SetTextColor(dis->hDC, active ? CLR_LABEL : CLR_DIM);
        RECT r = dis->rcItem;
        DrawTextA(dis->hDC, cap, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, CLR_LABEL);
        SetBkColor(hdc, CLR_LIST_BG);
        return (LRESULT)hBrList;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        bool isName = (GetDlgCtrlID((HWND)lp) == IDC_LBL_NAME);
        SetTextColor(hdc, isName ? CLR_TEXT : CLR_LABEL);
        SetBkColor(hdc, CLR_BG);
        return (LRESULT)hBrBg;
    }

    case WM_APP:
        // Plugin destructor is running — clear spawn flag so destructor's
        // WaitForSingleObject sees a clean state, then close.
        if (self) { self->mPanelSpawned = false; self->mPanelHWND = NULL; }
        DestroyWindow(hwnd);
        return 0;

    case WM_CLOSE:
        // Swallow if a listbox click is in flight (mUserClicked set by
        // LBN_SELCHANGE, cleared by WM_TIMER ~150 ms later) — that close
        // is spurious, caused by the ISF shader switch interaction.
        if (self && self->mUserClicked)
            return 0;
        // Intentional close (user clicked X, or Resolume alt+click removal).
        // Clear the HWND so nothing tries to post to the dead window, but
        // leave mPanelSpawned = true so ProcessFrame's panelGone evaluates
        // false and does NOT immediately respawn a new window.
        if (self) self->mPanelHWND = NULL;
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (self) {
            self->mPanelHWND   = NULL;   // ensure cleared (may already be)
            self->mPanelLastIC = -1;     // force layout refresh on next spawn
            self->mPanelSeenIdx = -1;
            // mPanelSpawned is intentionally NOT cleared here.  WM_APP owns
            // that when the destructor fires; WM_CLOSE keeps it true so
            // ProcessFrame does not respawn after an intentional close.
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// ------------------------------------------------------------------
// Panel — thread
// ------------------------------------------------------------------
DWORD WINAPI ISFEffectPlugin::panelThreadProc(LPVOID param)
{
    ISFEffectPlugin* self = (ISFEffectPlugin*)param;

    makeThreadDpiAware();
    InitCommonControls();

    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = panelWndProc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = "ISFEffPanel";
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    RegisterClassExA(&wc);

    // Initial size: will be resized by WM_TIMER once input count is known
    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "ISFEffPanel", "ISF Effect",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
        40, 40, 316, 330,
        NULL, NULL, GetModuleHandleA(NULL), self);

    if (!hwnd) return 1;
    self->mPanelHWND = hwnd;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

// ------------------------------------------------------------------
// Constructor
// ------------------------------------------------------------------
ISFEffectPlugin::ISFEffectPlugin()
    : mInputCount(0)
    , mCurrentIdx(-1), mNeedReload(false)
    , mHWND(NULL), mHDC(NULL), mHGLRC(NULL)
    , mVS(0), mFS(0), mProg(0)
    , mFBO(0), mColorTex(0), mFBOW(0), mFBOH(0)
    , mLocTIME(-1), mLocRENDERSIZE(-1), mLocTIMEDELTA(-1), mLocFRAMEINDEX(-1)
    , mLocInstanceSeed(-1)
    , mLocPASSINDEX(-1), mInTex(0), mUpBuf(NULL)
    , mPassCount(0), mTargetCount(0), mTargetW(0), mTargetH(0), mLastTime(0.0)
    , mInstanceSeed(0.0f)
    , mRowBuf(NULL), mTime(0.0f), mFrameIdx(0), mGLReady(false)
    , mPanelThread(NULL), mPanelHWND(NULL), mPanelSpawned(false), mPanelLastIC(-1), mPanelSeenIdx(-1)
    , mRequestedIdx(-1), mUserClicked(false)
    , mSharedCurrentIdx(-1), mSharedFileCount(0)
{
    SetMinInputs(1);
    SetMaxInputs(1);
    QueryPerformanceFrequency(&mQpcFreq);
    QueryPerformanceCounter(&mQpcStart);

    mParam[PARAM_SHADER] = 0.5f;
    mParam[PARAM_P1]     = 0.5f;
    mParam[PARAM_P2]     = 0.5f;
    mParam[PARAM_P3]     = 0.5f;
    for (int i = 0; i < NUM_FF_PARAMS; ++i) mSlotToInput[i] = i;  // default: slot i <- input i

    for (int i = 0; i < MAX_ALL_INPUTS; ++i) mAllParams[i]  = 0.5f;
    for (int i = 0; i < MAX_ALL_INPUTS; ++i) mLocP[i]       = -1;
    mExtraCount = 0;
    mLastFrameTick = 0;

    // Bounded on purpose: shaders feed this into sin()-based hashes, and a
    // raw GetTickCount() value is too large for that to stay precise.
    LONG instNum = InterlockedIncrement(&s_isfEffInstanceCounter);
    mInstanceSeed = (float)(GetTickCount() % 100000) * 0.01f + (float)instNum * 3.371f;

    strcpy(mShaderLabel,     "none");
    strcpy(mSharedShaderName,"none");
    for (int i = 0; i < NUM_FF_PARAMS; ++i)
        snprintf(mSharedParamLabels[i], sizeof(mSharedParamLabels[i]), "P%d —", i+1);
    mDisplayBuf[0] = '\0';

    // All 4 are now uniform, freely-assignable ISF-input sliders (see
    // mSlotToInput); shader switching lives entirely in the panel's file
    // list, so none of these names are "special" anymore.
    // 1-indexed to match the popup's "1/2/3/4" slot buttons exactly.
    SetParamInfo(PARAM_SHADER, "P1", FF_TYPE_STANDARD, 0.5f);
    SetParamInfo(PARAM_P1,     "P2", FF_TYPE_STANDARD, 0.5f);
    SetParamInfo(PARAM_P2,     "P3", FF_TYPE_STANDARD, 0.5f);
    SetParamInfo(PARAM_P3,     "P4", FF_TYPE_STANDARD, 0.5f);

    scanFiles();
    if (!mFsFiles.empty()) {
        mCurrentIdx = 0;
        mNeedReload = true;
    }
}

// ------------------------------------------------------------------
// Destructor
// ------------------------------------------------------------------
ISFEffectPlugin::~ISFEffectPlugin()
{
    HWND hw = (HWND)mPanelHWND;
    if (hw) PostMessageA(hw, WM_APP, 0, 0);
    if (mPanelThread) {
        WaitForSingleObject(mPanelThread, 3000);
        CloseHandle(mPanelThread);
        mPanelThread = NULL;
    }
    teardownGL();
    delete[] mRowBuf;
    mRowBuf = NULL;
    delete[] mUpBuf;
    mUpBuf = NULL;
}

// ------------------------------------------------------------------
// spawnPanel
// ------------------------------------------------------------------
void ISFEffectPlugin::spawnPanel()
{
    mPanelThread = CreateThread(NULL, 0, panelThreadProc, this, 0, NULL);
}

// ------------------------------------------------------------------
// scanFiles
// ------------------------------------------------------------------
void ISFEffectPlugin::scanFiles()
{
    mFsFiles.clear();

    HMODULE hMod = NULL;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)CreateInstance, &hMod);

    char dllPath[MAX_PATH] = {};
    GetModuleFileNameA(hMod, dllPath, MAX_PATH);
    char* lastSep = strrchr(dllPath, '\\');
    if (!lastSep) return;
    lastSep[1] = '\0';
    std::string dir(dllPath);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA((dir + "*.fs").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            mFsFiles.push_back(dir + fd.cFileName);
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
    std::sort(mFsFiles.begin(), mFsFiles.end());

    int n = (int)mFsFiles.size();
    if (n > MAX_FS_FILES) n = MAX_FS_FILES;
    for (int i = 0; i < n; ++i) {
        const std::string& p = mFsFiles[i];
        size_t sep = p.rfind('\\');
        size_t dot = p.rfind('.');
        std::string base = (sep != std::string::npos)
                           ? p.substr(sep + 1, dot - sep - 1)
                           : p.substr(0, dot);
        strncpy(mSharedFileLabels[i], base.c_str(),
                sizeof(mSharedFileLabels[i]) - 1);
        mSharedFileLabels[i][sizeof(mSharedFileLabels[i]) - 1] = '\0';
    }
    mSharedFileCount = n;
}

// ------------------------------------------------------------------
// initGL
// ------------------------------------------------------------------
bool ISFEffectPlugin::initGL()
{
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc); wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ISFEffWGL";
    RegisterClassExA(&wc);

    mHWND = CreateWindowExA(0, "ISFEffWGL", "", WS_POPUP,
                            0, 0, 1, 1, NULL, NULL,
                            GetModuleHandleA(NULL), NULL);
    if (!mHWND) return false;
    mHDC = GetDC(mHWND);
    if (!mHDC) return false;

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int fmt = ChoosePixelFormat(mHDC, &pfd);
    if (!fmt || !SetPixelFormat(mHDC, fmt, &pfd)) return false;

    mHGLRC = wglCreateContext(mHDC);
    if (!mHGLRC || !wglMakeCurrent(mHDC, mHGLRC)) return false;

    LOADEXT(PFNGLCREATESHADERPROC,           glCreateShader);
    LOADEXT(PFNGLSHADERSOURCEPROC,           glShaderSource);
    LOADEXT(PFNGLCOMPILESHADERPROC,          glCompileShader);
    LOADEXT(PFNGLGETSHADERIVPROC,            glGetShaderiv);
    LOADEXT(PFNGLGETSHADERINFOLOGPROC,       glGetShaderInfoLog);
    LOADEXT(PFNGLCREATEPROGRAMPROC,          glCreateProgram);
    LOADEXT(PFNGLATTACHSHADERPROC,           glAttachShader);
    LOADEXT(PFNGLLINKPROGRAMPROC,            glLinkProgram);
    LOADEXT(PFNGLGETPROGRAMIVPROC,           glGetProgramiv);
    LOADEXT(PFNGLGETPROGRAMINFOLOGPROC,      glGetProgramInfoLog);
    LOADEXT(PFNGLUSEPROGRAMPROC,             glUseProgram);
    LOADEXT(PFNGLGETUNIFORMLOCATIONPROC,     glGetUniformLocation);
    LOADEXT(PFNGLUNIFORM1FPROC,              glUniform1f);
    LOADEXT(PFNGLUNIFORM1IPROC,              glUniform1i);
    LOADEXT(PFNGLUNIFORM2FPROC,              glUniform2f);
    LOADEXT(PFNGLDELETESHADERPROC,           glDeleteShader);
    LOADEXT(PFNGLDELETEPROGRAMPROC,          glDeleteProgram);
    LOADEXT(PFNGLGENFRAMEBUFFERSEXTPROC,     glGenFramebuffersEXT);
    LOADEXT(PFNGLBINDFRAMEBUFFEREXTPROC,     glBindFramebufferEXT);
    LOADEXT(PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, glFramebufferTexture2DEXT);
    LOADEXT(PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, glCheckFramebufferStatusEXT);
    LOADEXT(PFNGLDELETEFRAMEBUFFERSEXTPROC,  glDeleteFramebuffersEXT);
    LOADEXT(PFNGLACTIVETEXTUREPROC,          glActiveTexture);

    wglMakeCurrent(NULL, NULL);
    return true;
}

// ------------------------------------------------------------------
// Shader
// ------------------------------------------------------------------
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader_(type);
    glShaderSource_(s, 1, &src, NULL);
    glCompileShader_(s);
    GLint ok = 0;
    glGetShaderiv_(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[4096]; GLsizei len;
        glGetShaderInfoLog_(s, sizeof(buf), &len, buf);
        FILE* f = fopen(ERROR_LOG, "a");
        if (f) { fwrite(buf, 1, len, f); fclose(f); }
        glDeleteShader_(s);
        return 0;
    }
    return s;
}

void ISFEffectPlugin::unloadShader()
{
    if (mProg) { glDeleteProgram_(mProg); mProg = 0; }
    if (mFS)   { glDeleteShader_(mFS);   mFS = 0; }
    if (mVS)   { glDeleteShader_(mVS);   mVS = 0; }
    releaseTargets();
    mPassCount = 0;
    mTargetCount = 0;
    mInputCount = 0;
    mExtraCount = 0;
    mLocTIME = mLocRENDERSIZE = mLocTIMEDELTA = mLocFRAMEINDEX = mLocPASSINDEX = -1;
    for (int i = 0; i < MAX_ALL_INPUTS; ++i) mLocP[i] = -1;
    strcpy(mShaderLabel, "none");
}

bool ISFEffectPlugin::loadShader(int idx)
{
    unloadShader();
    if (idx < 0 || idx >= (int)mFsFiles.size()) return false;

    const std::string& path = mFsFiles[idx];
    size_t sep = path.rfind('\\');
    size_t dot = path.rfind('.');
    std::string label = (sep != std::string::npos)
                        ? path.substr(sep + 1, dot - sep - 1)
                        : path.substr(0, dot);
    strncpy(mShaderLabel, label.c_str(), sizeof(mShaderLabel) - 1);
    mShaderLabel[sizeof(mShaderLabel) - 1] = '\0';

    remove(ERROR_LOG);   // fresh log per load; compile errors are appended
    mFrameIdx = 0;       // shaders seed their feedback buffers on FRAMEINDEX < 2

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    std::string raw(sz, '\0');
    fread(&raw[0], 1, sz, f);
    fclose(f);

    const char* bodyPtr = strstr(raw.c_str(), "*/");
    if (bodyPtr) {
        std::string header(raw.c_str(), (size_t)(bodyPtr - raw.c_str()));
        parseISFInputs(header.c_str(), mInputs, &mInputCount, MAX_ALL_INPUTS);
        parseISFExtras(header.c_str(), mExtra, &mExtraCount, MAX_EXTRA_UNIFORMS);
        parsePasses(header.c_str());
        bodyPtr += 2;
        if (*bodyPtr == '\n') bodyPtr++;
    } else {
        bodyPtr = raw.c_str();
    }
    std::string cleanBody = stripPrecisionLines(bodyPtr);

    // Initialise mAllParams to ISF defaults
    for (int i = 0; i < mInputCount; ++i) {
        float range = mInputs[i].maxVal - mInputs[i].minVal;
        float norm  = (range > 0.0f)
                    ? (mInputs[i].defaultVal - mInputs[i].minVal) / range
                    : 0.5f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        mAllParams[i] = norm;
    }
    // Reset slot bindings to the default "slot i <- input i" mapping on every
    // (re)load — any custom assignment from a previous shader would otherwise
    // point at the wrong input, or one that no longer exists.
    for (int i = 0; i < NUM_FF_PARAMS; ++i)
        mSlotToInput[i] = (i < mInputCount) ? i : -1;
    for (int s = 0; s < NUM_FF_PARAMS; ++s)
        if (mSlotToInput[s] >= 0) mParam[s] = mAllParams[mSlotToInput[s]];

    // Uniform declarations shared by both GLSL dialects below.
    std::string decls =
        "uniform float TIME;\n"
        "uniform float TIMEDELTA;\n"
        "uniform int   FRAMEINDEX;\n"
        "uniform int   PASSINDEX;\n"
        "uniform vec2  RENDERSIZE;\n"
        "uniform float uInstanceSeed;\n";
    for (int i = 0; i < mInputCount; ++i) {
        decls += "uniform float ";
        decls += mInputs[i].name;
        decls += ";\n";
    }
    // Declare unsupported types so the shader compiles; uniforms default to 0.
    // "image" inputs land here too - every sampler of that kind gets the
    // layer's video (texture unit 0) in ProcessFrame.
    for (int i = 0; i < mExtraCount; ++i) {
        switch (mExtra[i].type) {
        case ISF_EXTRA_COLOR:   decls += "uniform vec4 ";        break;
        case ISF_EXTRA_POINT2D: decls += "uniform vec2 ";        break;
        case ISF_EXTRA_EVENT:   decls += "uniform bool ";        break;
        case ISF_EXTRA_SAMPLER: decls += "uniform sampler2D ";   break;
        }
        decls += mExtra[i].name;
        decls += ";\n";
    }
    for (int t = 0; t < mTargetCount; ++t) {
        decls += "uniform sampler2D ";
        decls += mTargets[t].name;
        decls += ";\n";
    }

    // Modern GLSL first (the ffgl ISF Effect compiles as 4.10 core; a legacy
    // wglCreateContext context is a compatibility profile, which still allows
    // gl_FragColor/gl_Vertex). Falls back to GLSL 1.20 with shims for what our
    // shaders use from 1.30+ (texture(), tanh, textureSize) on old drivers.
    std::string modern =
        "#version 330 compatibility\n"
        "in vec2 isf_FragNormCoord;\n"
        "#define texture2D texture\n"
        "#define IMG_NORM_PIXEL(img, uv)   texture(img, uv)\n"
        "#define IMG_PIXEL(img, px)        texture(img, (px) / RENDERSIZE)\n"
        "#define IMG_THIS_NORM_PIXEL(img)  texture(img, isf_FragNormCoord)\n"
        "#define IMG_THIS_PIXEL(img)       texture(img, isf_FragNormCoord)\n"
        "#define IMG_SIZE(img)             vec2(textureSize(img, 0))\n";
    std::string legacy =
        "#version 120\n"
        "varying vec2  isf_FragNormCoord;\n"
        "#define texture texture2D\n"
        "#define IMG_NORM_PIXEL(img, uv)   texture2D(img, uv)\n"
        "#define IMG_PIXEL(img, px)        texture2D(img, (px) / RENDERSIZE)\n"
        "#define IMG_THIS_NORM_PIXEL(img)  texture2D(img, isf_FragNormCoord)\n"
        "#define IMG_THIS_PIXEL(img)       texture2D(img, isf_FragNormCoord)\n"
        "#define IMG_SIZE(img)             RENDERSIZE\n"
        "float tanh(float x) { float e = exp(2.0*clamp(x, -10.0, 10.0)); return (e - 1.0)/(e + 1.0); }\n"
        "vec2  tanh(vec2  x) { vec2  e = exp(2.0*clamp(x, -10.0, 10.0)); return (e - 1.0)/(e + 1.0); }\n"
        "vec3  tanh(vec3  x) { vec3  e = exp(2.0*clamp(x, -10.0, 10.0)); return (e - 1.0)/(e + 1.0); }\n"
        "vec4  tanh(vec4  x) { vec4  e = exp(2.0*clamp(x, -10.0, 10.0)); return (e - 1.0)/(e + 1.0); }\n";

    if (!compileProgram(modern + decls + cleanBody, true) &&
        !compileProgram(legacy + decls + cleanBody, false)) {
        unloadShader();
        return false;
    }

    mLocTIME       = glGetUniformLocation_(mProg, "TIME");
    mLocTIMEDELTA  = glGetUniformLocation_(mProg, "TIMEDELTA");
    mLocFRAMEINDEX = glGetUniformLocation_(mProg, "FRAMEINDEX");
    mLocRENDERSIZE = glGetUniformLocation_(mProg, "RENDERSIZE");
    mLocInstanceSeed = glGetUniformLocation_(mProg, "uInstanceSeed");
    mLocPASSINDEX  = glGetUniformLocation_(mProg, "PASSINDEX");
    for (int t = 0; t < mTargetCount; ++t)
        mTargets[t].loc = glGetUniformLocation_(mProg, mTargets[t].name);
    for (int i = 0; i < mInputCount; ++i)
        mLocP[i] = glGetUniformLocation_(mProg, mInputs[i].name);
    for (int i = 0; i < mExtraCount; ++i)
        mExtra[i].loc = glGetUniformLocation_(mProg, mExtra[i].name);

    return true;
}

// Compiles + links one dialect. Leaves mVS/mFS/mProg set on success, all 0 on failure.
bool ISFEffectPlugin::compileProgram(const std::string& fsSrc, bool modernGLSL)
{
    const char* vsModern =
        "#version 330 compatibility\n"
        "out vec2 isf_FragNormCoord;\n"
        "void main() {\n"
        "    gl_Position       = vec4(gl_Vertex.xy, 0.0, 1.0);\n"
        "    isf_FragNormCoord = gl_Vertex.xy * 0.5 + 0.5;\n"
        "}\n";
    const char* vsLegacy =
        "#version 120\n"
        "varying vec2 isf_FragNormCoord;\n"
        "void main() {\n"
        "    gl_Position       = vec4(gl_Vertex.xy, 0.0, 1.0);\n"
        "    isf_FragNormCoord = gl_Vertex.xy * 0.5 + 0.5;\n"
        "}\n";

    FILE* ef = fopen(ERROR_LOG, "a");
    if (ef) { fprintf(ef, "--- %s GLSL ---\n", modernGLSL ? "330 compatibility" : "1.20"); fclose(ef); }

    mVS = compileShader(GL_VERTEX_SHADER,   modernGLSL ? vsModern : vsLegacy);
    mFS = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str());
    if (mVS && mFS) {
        mProg = glCreateProgram_();
        glAttachShader_(mProg, mVS);
        glAttachShader_(mProg, mFS);
        glLinkProgram_(mProg);
        GLint ok = 0;
        glGetProgramiv_(mProg, GL_LINK_STATUS, &ok);
        if (ok) return true;
        char buf[4096]; GLsizei len;
        glGetProgramInfoLog_(mProg, sizeof(buf), &len, buf);
        ef = fopen(ERROR_LOG, "a");
        if (ef) { fwrite(buf, 1, len, ef); fclose(ef); }
    }
    if (mProg) { glDeleteProgram_(mProg); mProg = 0; }
    if (mFS)   { glDeleteShader_(mFS);   mFS = 0; }
    if (mVS)   { glDeleteShader_(mVS);   mVS = 0; }
    return false;
}

// ------------------------------------------------------------------
// Pass targets
// ------------------------------------------------------------------
void ISFEffectPlugin::releaseTargets()
{
    for (int t = 0; t < mTargetCount; ++t) {
        ISFTarget& T = mTargets[t];
        for (int b = 0; b < 2; ++b) {
            if (T.fbo[b]) { glDeleteFramebuffersEXT_(1, &T.fbo[b]); T.fbo[b] = 0; }
            if (T.tex[b]) { glDeleteTextures(1, &T.tex[b]); T.tex[b] = 0; }
        }
        T.cur = 0;
    }
    mTargetW = mTargetH = 0;
}

// (Re)creates every pass target at W x H when the size changed or after a
// shader load; new buffers start cleared to black.
bool ISFEffectPlugin::buildTargets(int W, int H)
{
    if (mTargetCount == 0 || (W == mTargetW && H == mTargetH)) return true;
    releaseTargets();
    for (int t = 0; t < mTargetCount; ++t) {
        ISFTarget& T = mTargets[t];
        for (int b = 0; b < 2; ++b) {
            glGenTextures(1, &T.tex[b]);
            glBindTexture(GL_TEXTURE_2D, T.tex[b]);
            glTexImage2D(GL_TEXTURE_2D, 0, T.isFloat ? GL_RGBA16F : GL_RGBA8, W, H, 0,
                         GL_RGBA, T.isFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
            GLint filter = T.nearest ? GL_NEAREST : GL_LINEAR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenFramebuffersEXT_(1, &T.fbo[b]);
            glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, T.fbo[b]);
            glFramebufferTexture2DEXT_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                       GL_TEXTURE_2D, T.tex[b], 0);
            GLenum st = glCheckFramebufferStatusEXT_(GL_FRAMEBUFFER_EXT);
            if (st == GL_FRAMEBUFFER_COMPLETE_EXT) {
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
            if (st != GL_FRAMEBUFFER_COMPLETE_EXT) { releaseTargets(); return false; }
        }
    }
    mTargetW = W; mTargetH = H;
    return true;
}

// ------------------------------------------------------------------
// FBO
// ------------------------------------------------------------------
bool ISFEffectPlugin::buildFBO(int W, int H)
{
    if (mColorTex) { glDeleteTextures(1, &mColorTex); mColorTex = 0; }
    if (mFBO)      { glDeleteFramebuffersEXT_(1, &mFBO); mFBO = 0; }

    glGenTextures(1, &mColorTex);
    glBindTexture(GL_TEXTURE_2D, mColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffersEXT_(1, &mFBO);
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, mFBO);
    glFramebufferTexture2DEXT_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                               GL_TEXTURE_2D, mColorTex, 0);
    GLenum st = glCheckFramebufferStatusEXT_(GL_FRAMEBUFFER_EXT);
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
    if (st != GL_FRAMEBUFFER_COMPLETE_EXT) return false;

    // The layer's video goes into mInTex every frame (shader texture unit 0).
    if (mInTex) { glDeleteTextures(1, &mInTex); mInTex = 0; }
    glGenTextures(1, &mInTex);
    glBindTexture(GL_TEXTURE_2D, mInTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    mFBOW = W; mFBOH = H;
    delete[] mRowBuf;
    mRowBuf = new BYTE[W * 3];
    delete[] mUpBuf;
    mUpBuf = new BYTE[W * H * 3];
    return true;
}

void ISFEffectPlugin::teardownGL()
{
    if (!mHGLRC) return;
    wglMakeCurrent(mHDC, mHGLRC);
    unloadShader();
    if (mFBO)      { glDeleteFramebuffersEXT_(1, &mFBO); mFBO = 0; }
    if (mColorTex) { glDeleteTextures(1, &mColorTex); mColorTex = 0; }
    if (mInTex)    { glDeleteTextures(1, &mInTex); mInTex = 0; }
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(mHGLRC); mHGLRC = NULL;
    if (mHDC && mHWND) { ReleaseDC(mHWND, mHDC); mHDC = NULL; }
    if (mHWND)         { DestroyWindow(mHWND); mHWND = NULL; }
}

// ------------------------------------------------------------------
// updateSharedState — called each frame, read by dialog timer
// ------------------------------------------------------------------
void ISFEffectPlugin::updateSharedState()
{
    mSharedCurrentIdx = mCurrentIdx;
    strncpy(mSharedShaderName, mShaderLabel, sizeof(mSharedShaderName) - 1);
    mSharedShaderName[sizeof(mSharedShaderName) - 1] = '\0';

    for (int s = 0; s < NUM_FF_PARAMS; ++s) {
        int inputIdx = mSlotToInput[s];
        if (inputIdx >= 0 && inputIdx < mInputCount) {
            float t   = mAllParams[inputIdx];
            float val = mInputs[inputIdx].minVal
                      + t * (mInputs[inputIdx].maxVal - mInputs[inputIdx].minVal);
            snprintf(mSharedParamLabels[s], sizeof(mSharedParamLabels[s]),
                     "%s=%.2f", mInputs[inputIdx].name, val);
        } else {
            snprintf(mSharedParamLabels[s], sizeof(mSharedParamLabels[s]),
                     "P%d — unassigned", s+1);
        }
    }
}

// ------------------------------------------------------------------
// ProcessFrame
// ------------------------------------------------------------------
DWORD ISFEffectPlugin::ProcessFrame(void* pFrame)
{
    mLastFrameTick = GetTickCount();
    if (!pFrame) return FF_FAIL;
    int W = (int)GetFrameWidth();
    int H = (int)GetFrameHeight();
    if (W <= 0 || H <= 0) return FF_FAIL;

    if (!mGLReady) {
        if (!initGL()) return FF_FAIL;
        mGLReady = true;
    }

    // Always reap an exited panel thread, even when we're not going to respawn
    // (e.g. after an intentional WM_CLOSE where mPanelSpawned stays true).
    if (mPanelThread &&
        WaitForSingleObject(mPanelThread, 0) == WAIT_OBJECT_0) {
        CloseHandle(mPanelThread);
        mPanelThread = NULL;
    }

    // panelGone is true on first activation (!mPanelSpawned) or if the window
    // disappeared without going through our WM_CLOSE handler (IsWindow check).
    // An intentional WM_CLOSE close leaves mPanelSpawned=true and mPanelHWND=NULL,
    // which evaluates to panelGone=false — so no spurious respawn after removal.
    bool panelGone = !mPanelSpawned ||
                     ((HWND)mPanelHWND != NULL && !IsWindow((HWND)mPanelHWND));
    if (panelGone && !mPanelThread) {
        mPanelLastIC = -1;
        mPanelSeenIdx = -1;
        spawnPanel();
        mPanelSpawned = true;
    }

    wglMakeCurrent(mHDC, mHGLRC);

    // Pick up shader selection from panel (the only way to switch shaders
    // now — PARAM_SHADER is a plain assignable slot, not a shader index).
    // Atomic take: a click landing between read and reset must not be lost
    int req = (int)InterlockedExchange((volatile LONG*)&mRequestedIdx, -1);
    if (req >= 0 && req != mCurrentIdx) {
        mCurrentIdx       = req;
        mSharedCurrentIdx = req;  // early update — prevents WM_TIMER from syncing listbox back
        mNeedReload = true;
    }

    if (mNeedReload) {
        loadShader(mCurrentIdx);
        mNeedReload = false;
    }

    if (W != mFBOW || H != mFBOH) {
        if (!buildFBO(W, H)) { wglMakeCurrent(NULL, NULL); return FF_FAIL; }
    }

    updateSharedState();

    // No usable shader (none found, or it failed to compile - see
    // C:\\isf_effect_error.txt): pass the layer's video through untouched.
    if (!mProg || !mFBO || !buildTargets(W, H)) {
        wglMakeCurrent(NULL, NULL);
        return FF_SUCCESS;
    }

    // Real time, so generation-rate shaders (TIME vs TIME - TIMEDELTA) run at
    // the same speed whatever frame rate Resolume delivers.
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double t = (double)(now.QuadPart - mQpcStart.QuadPart) / (double)mQpcFreq.QuadPart;
    float delta = (float)(t - mLastTime);
    mLastTime = t;
    mTime = (float)t;

    // Upload the frame (top row first) as the input texture (bottom row first).
    BYTE* fb = (BYTE*)pFrame;
    const int rowBytes = W * 3;
    for (int y = 0; y < H; ++y)
        memcpy(mUpBuf + (H - 1 - y) * rowBytes, fb + y * rowBytes, rowBytes);
    glActiveTexture_(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mInTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, mUpBuf);

    glUseProgram_(mProg);
    if (mLocTIME       >= 0) glUniform1f_(mLocTIME,      mTime);
    if (mLocTIMEDELTA  >= 0) glUniform1f_(mLocTIMEDELTA, delta);
    if (mLocFRAMEINDEX >= 0) glUniform1i_(mLocFRAMEINDEX, (GLint)mFrameIdx);
    if (mLocRENDERSIZE >= 0) glUniform2f_(mLocRENDERSIZE, (float)W, (float)H);
    if (mLocInstanceSeed >= 0) glUniform1f_(mLocInstanceSeed, mInstanceSeed);
    for (int i = 0; i < mInputCount; ++i) {
        if (mLocP[i] < 0) continue;
        float val = mInputs[i].minVal
                  + mAllParams[i] * (mInputs[i].maxVal - mInputs[i].minVal);
        glUniform1f_(mLocP[i], val);
    }
    for (int i = 0; i < mExtraCount; ++i)
        if (mExtra[i].type == ISF_EXTRA_SAMPLER && mExtra[i].loc >= 0)
            glUniform1i_(mExtra[i].loc, 0);

    // Passes: a single-pass shader is one pass into the output FBO. Unit 1+t
    // holds target t's readable buffer, rebound before each pass because the
    // previous pass may just have swapped it.
    int passes = (mPassCount > 0) ? mPassCount : 1;
    GLuint readFbo = mFBO;
    for (int p = 0; p < passes; ++p) {
        if (mLocPASSINDEX >= 0) glUniform1i_(mLocPASSINDEX, p);
        for (int t2 = 0; t2 < mTargetCount; ++t2) {
            glActiveTexture_(GL_TEXTURE0 + 1 + t2);
            glBindTexture(GL_TEXTURE_2D, mTargets[t2].tex[mTargets[t2].cur]);
            if (mTargets[t2].loc >= 0) glUniform1i_(mTargets[t2].loc, 1 + t2);
        }

        int ti = (mPassCount > 0) ? mPassTarget[p] : -1;
        GLuint dst = (ti >= 0) ? mTargets[ti].fbo[1 - mTargets[ti].cur] : mFBO;
        glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, dst);
        glViewport(0, 0, W, H);
        glBegin(GL_QUADS);
            glVertex2f(-1.0f, -1.0f); glVertex2f( 1.0f, -1.0f);
            glVertex2f( 1.0f,  1.0f); glVertex2f(-1.0f,  1.0f);
        glEnd();
        if (ti >= 0) {
            mTargets[ti].cur = 1 - mTargets[ti].cur;
            readFbo = mTargets[ti].fbo[mTargets[ti].cur];
        } else {
            readFbo = mFBO;
        }
    }

    // ISF shows the last pass, even when it rendered into a target - so read
    // back from whichever buffer the last pass wrote, flipped to top-down.
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, readFbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, mUpBuf);
    for (int y = 0; y < H; ++y)
        memcpy(fb + y * rowBytes, mUpBuf + (H - 1 - y) * rowBytes, rowBytes);

    for (int u = 0; u <= mTargetCount; ++u) {
        glActiveTexture_(GL_TEXTURE0 + u);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture_(GL_TEXTURE0);
    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
    glUseProgram_(0);
    wglMakeCurrent(NULL, NULL);

    mFrameIdx++;
    return FF_SUCCESS;
}

// ------------------------------------------------------------------
// Parameters
// ------------------------------------------------------------------
DWORD ISFEffectPlugin::GetParameter(DWORD index)
{
    if (index >= NUM_FF_PARAMS) return FF_FAIL;
    union { float f; DWORD d; } u;
    u.f = mParam[index];
    return u.d;
}

DWORD ISFEffectPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (!pParam || pParam->ParameterNumber >= NUM_FF_PARAMS) return FF_FAIL;
    union { float f; DWORD d; } u;
    u.d = pParam->NewParameterValue;
    float val = u.f;
    DWORD pn  = pParam->ParameterNumber;

    mParam[pn] = val;

    // Every slot is a plain assignable ISF-input slider now; shader
    // switching happens only via the panel's file list (see ProcessFrame).
    int inputIdx = mSlotToInput[pn];
    if (inputIdx >= 0 && inputIdx < mInputCount)
        mAllParams[inputIdx] = val;
    return FF_SUCCESS;
}

char* ISFEffectPlugin::GetParameterDisplay(DWORD index)
{
    if (index < NUM_FF_PARAMS)
        return mSharedParamLabels[index];
    return (char*)"";
}

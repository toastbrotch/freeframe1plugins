#include "ISFBrowser.h"
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

#define GL_FRAMEBUFFER_EXT              0x8D40
#define GL_COLOR_ATTACHMENT0_EXT        0x8CE0
#define GL_FRAMEBUFFER_COMPLETE_EXT     0x8CD5
#define GL_FRAGMENT_SHADER              0x8B30
#define GL_VERTEX_SHADER                0x8B31
#define GL_COMPILE_STATUS               0x8B81
#define GL_LINK_STATUS                  0x8B82

#define LOADEXT(T, name) \
    name##_ = (T)wglGetProcAddress(#name); \
    if (!name##_) return false;

// ------------------------------------------------------------------
// Panel constants
// ------------------------------------------------------------------
#define IDC_LISTBOX    101
#define IDC_LBL_NAME   110
#define IDC_LBL_P0     120   // IDC_LBL_P0 + i  (i = 0..MAX_ALL_INPUTS-1)
#define IDC_TRACK_P0   140   // IDC_TRACK_P0 + i
// One BS_AUTORADIOBUTTON|BS_PUSHLIKE per (input row, slot) pair, so the
// currently-bound slot renders visibly pressed/highlighted. Row i's 4
// buttons are IDC_BTN_SLOT0 + i*NUM_FF_PARAMS + s (s = 0..NUM_FF_PARAMS-1).
#define IDC_BTN_SLOT0  160

#define TRACK_RANGE    1000

// Counts plugin instances created this process, so simultaneously-loaded
// copies (e.g. two ISFBrowser layers) still get distinct seeds even when
// GetTickCount() ties between them.
static volatile LONG s_isfInstanceCounter = 0;

#define CLR_BG        RGB(14,  14,  14)
#define CLR_LIST_BG   RGB( 6,   6,   6)
#define CLR_TEXT      RGB(140, 210, 140)
#define CLR_DIM       RGB( 80, 130,  80)

// ------------------------------------------------------------------
// CreateInstance
// ------------------------------------------------------------------
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new ISFBrowserPlugin();
    return (*ppInstance) ? FF_SUCCESS : FF_FAIL;
}

// ------------------------------------------------------------------
// ISF JSON parsing
// ------------------------------------------------------------------
const char* ISFBrowserPlugin::findKeyVal(const char* json, const char* key)
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

float ISFBrowserPlugin::parseFloatAt(const char* p, float def)
{
    if (!p) return def;
    char* end;
    float v = strtof(p, &end);
    return (end == p) ? def : v;
}

bool ISFBrowserPlugin::parseStringAt(const char* p, char* out, int maxLen)
{
    if (!p || *p != '"') return false;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxLen - 1) out[i++] = *p++;
    out[i] = '\0';
    return i > 0;
}

bool ISFBrowserPlugin::parseISFInputs(const char* header,
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

void ISFBrowserPlugin::parseISFExtras(const char* header,
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
                             ISFBrowserPlugin* self, int i)
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

static void syncTrackFromAllParams(HWND hwnd, ISFBrowserPlugin* self, int i)
{
    HWND ht  = GetDlgItem(hwnd, IDC_TRACK_P0 + i);
    int  pos = (int)(self->mAllParams[i] * TRACK_RANGE + 0.5f);
    int  cur = (int)SendMessageA(ht, TBM_GETPOS, 0, 0);
    if (abs(cur - pos) > 2)
        SendMessageA(ht, TBM_SETPOS, TRUE, (LPARAM)pos);
}

// Which Resolume slot (0..NUM_FF_PARAMS-1), if any, is currently bound to
// ISF input i. Returns -1 if input i isn't bound to any slot.
static int slotForInput(ISFBrowserPlugin* self, int i)
{
    for (int s = 0; s < NUM_FF_PARAMS; ++s)
        if (self->mSlotToInput[s] == i) return s;
    return -1;
}

static int slotBtnId(int i, int s) { return IDC_BTN_SLOT0 + i*NUM_FF_PARAMS + s; }

// Forces a repaint of row i's 4 (owner-drawn) slot buttons so WM_DRAWITEM
// re-reads mSlotToInput and reflects the current binding.
static void syncSlotButtons(HWND hwnd, ISFBrowserPlugin* /*self*/, int i)
{
    for (int s = 0; s < NUM_FF_PARAMS; ++s)
        InvalidateRect(GetDlgItem(hwnd, slotBtnId(i, s)), NULL, FALSE);
}

// Binds ISF input i to Resolume slot `slot` (or unbinds it if slot < 0),
// evicting whichever input previously held that slot — each slot can only
// point at one input at a time. Pushes the input's current value into
// mParam right away so the Resolume UI doesn't show a stale number.
static void assignSlot(ISFBrowserPlugin* self, int i, int slot)
{
    int oldSlot = slotForInput(self, i);
    if (oldSlot >= 0) self->mSlotToInput[oldSlot] = -1;
    if (slot >= 0) {
        self->mSlotToInput[slot] = i;  // overwrite implicitly evicts any prior owner
        self->mParam[slot] = self->mAllParams[i];
    }
}

// ------------------------------------------------------------------
// Panel — window procedure
// ------------------------------------------------------------------
LRESULT CALLBACK ISFBrowserPlugin::panelWndProc(HWND hwnd, UINT msg,
                                                  WPARAM wp, LPARAM lp)
{
    static HBRUSH hBrBg    = NULL;
    static HBRUSH hBrList  = NULL;
    static HBRUSH hBrGreen = NULL;  // active slot-button fill
    static HFONT  hFont    = NULL;

    ISFBrowserPlugin* self =
        (ISFBrowserPlugin*)GetWindowLongA(hwnd, GWL_USERDATA);

    switch (msg) {

    case WM_CREATE: {
        self = (ISFBrowserPlugin*)((CREATESTRUCTA*)lp)->lpCreateParams;
        SetWindowLongA(hwnd, GWL_USERDATA, (LONG)(LONG_PTR)self);

        if (!hBrBg)    hBrBg    = CreateSolidBrush(CLR_BG);
        if (!hBrList)  hBrList  = CreateSolidBrush(CLR_LIST_BG);
        if (!hBrGreen) hBrGreen = CreateSolidBrush(CLR_TEXT);
        if (!hFont)    hFont    = CreateFontA(14, 0, 0, 0, FW_NORMAL,
                                    FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                    FIXED_PITCH | FF_MODERN, "Courier New");

        // File listbox
        HWND lb = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            8, 8, 286, 160, hwnd, (HMENU)IDC_LISTBOX, NULL, NULL);
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
            8, 176, 286, 16, hwnd, (HMENU)IDC_LBL_NAME, NULL, NULL);
        SendMessageA(ln, WM_SETFONT, (WPARAM)hFont, FALSE);

        // Pre-create MAX_ALL_INPUTS label+trackbar rows, each with 4 slot
        // radio-buttons (P1-P4), hidden initially.
        char buf[80];
        for (int i = 0; i < MAX_ALL_INPUTS; ++i) {
            buildParamLabel(buf, sizeof(buf), self, i);
            HWND lbl = CreateWindowExA(0, "STATIC", buf,
                WS_CHILD | SS_LEFT,   // not WS_VISIBLE yet
                8, 0, 176, 15,
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
                    190 + s*26, 0, 24, 18,
                    hwnd, (HMENU)(UINT_PTR)slotBtnId(i, s), NULL, NULL);
                SendMessageA(sb, WM_SETFONT, (WPARAM)hFont, FALSE);
            }

            HWND trk = CreateWindowExA(0, TRACKBAR_CLASSA, NULL,
                WS_CHILD | TBS_HORZ | TBS_NOTICKS,   // not WS_VISIBLE yet
                8, 0, 286, 22,
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
        }

        // Pick up a user listbox click: read lb.cursel here (150 ms later)
        // so the selection is fully committed before we sample it.
        if (self->mUserClicked) {
            int lbSel = (int)SendMessageA(lb, LB_GETCURSEL, 0, 0);
            if (lbSel != LB_ERR && lbSel >= 0 && lbSel != self->mSharedCurrentIdx)
                self->mRequestedIdx = lbSel;
            self->mUserClicked = false;
        }

        // Sync listbox to current shader when no user interaction is pending
        // (handles Resolume-slider-driven changes and initial selection).
        if (self->mRequestedIdx < 0) {
            int ci = self->mSharedCurrentIdx;
            if ((int)SendMessageA(lb, LB_GETCURSEL, 0, 0) != ci && ci >= 0)
                SendMessageA(lb, LB_SETCURSEL, (WPARAM)ci, 0);
        }

        SetWindowTextA(GetDlgItem(hwnd, IDC_LBL_NAME), self->mSharedShaderName);

        // Show/hide + reposition controls based on current input count.
        int ic = self->mInputCount;

        if (ic != self->mPanelLastIC) {
            // Compute positions and show/hide
            int y = 198;
            for (int i = 0; i < MAX_ALL_INPUTS; ++i) {
                HWND lbl = GetDlgItem(hwnd, IDC_LBL_P0   + i);
                HWND trk = GetDlgItem(hwnd, IDC_TRACK_P0 + i);
                if (i < ic) {
                    SetWindowPos(lbl, NULL, 8, y,      176, 15, SWP_NOZORDER);
                    for (int s = 0; s < NUM_FF_PARAMS; ++s)
                        SetWindowPos(GetDlgItem(hwnd, slotBtnId(i, s)), NULL,
                                     190 + s*26, y - 1, 24, 18, SWP_NOZORDER);
                    SetWindowPos(trk, NULL, 8, y + 17, 286, 22, SWP_NOZORDER);
                    ShowWindow(lbl, SW_SHOW);
                    for (int s = 0; s < NUM_FF_PARAMS; ++s)
                        ShowWindow(GetDlgItem(hwnd, slotBtnId(i, s)), SW_SHOW);
                    ShowWindow(trk, SW_SHOW);
                    y += 44;
                } else {
                    ShowWindow(lbl, SW_HIDE);
                    for (int s = 0; s < NUM_FF_PARAMS; ++s)
                        ShowWindow(GetDlgItem(hwnd, slotBtnId(i, s)), SW_HIDE);
                    ShowWindow(trk, SW_HIDE);
                }
            }

            // Resize window to fit
            int clientH = (ic > 0) ? (198 + ic * 44 + 10) : 210;
            RECT cr; GetClientRect(hwnd, &cr);
            RECT wr; GetWindowRect(hwnd, &wr);
            int chromaH = (wr.bottom - wr.top) - (cr.bottom - cr.top);
            int chromaW = (wr.right  - wr.left) - (cr.right  - cr.left);
            SetWindowPos(hwnd, NULL, 0, 0,
                         302 + chromaW, clientH + chromaH,
                         SWP_NOMOVE | SWP_NOZORDER);
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
            // LBN_SELCHANGE can fire *before* the listbox commits the new
            // selection internally, so LB_GETCURSEL here may return the OLD
            // index.  Set a flag; WM_TIMER reads lb.cursel once it's settled.
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

        FillRect(dis->hDC, &dis->rcItem, active ? hBrGreen : hBrList);
        DrawEdge(dis->hDC, &dis->rcItem, active ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);

        char cap[4];
        GetWindowTextA(dis->hwndItem, cap, sizeof(cap));
        SetBkMode(dis->hDC, TRANSPARENT);
        SetTextColor(dis->hDC, active ? RGB(255, 255, 255) : CLR_DIM);
        RECT r = dis->rcItem;
        DrawTextA(dis->hDC, cap, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_LIST_BG);
        return (LRESULT)hBrList;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, CLR_TEXT);
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
DWORD WINAPI ISFBrowserPlugin::panelThreadProc(LPVOID param)
{
    ISFBrowserPlugin* self = (ISFBrowserPlugin*)param;

    InitCommonControls();

    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = panelWndProc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = "ISFBrowPanel";
    wc.hCursor       = LoadCursorA(NULL, IDC_ARROW);
    RegisterClassExA(&wc);

    // Initial size: will be resized by WM_TIMER once input count is known
    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "ISFBrowPanel", "ISF Browser",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
        40, 40, 318, 250,
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
ISFBrowserPlugin::ISFBrowserPlugin()
    : mInputCount(0)
    , mCurrentIdx(-1), mNeedReload(false)
    , mHWND(NULL), mHDC(NULL), mHGLRC(NULL)
    , mVS(0), mFS(0), mProg(0)
    , mFBO(0), mColorTex(0), mFBOW(0), mFBOH(0)
    , mLocTIME(-1), mLocRENDERSIZE(-1), mLocTIMEDELTA(-1), mLocFRAMEINDEX(-1)
    , mLocInstanceSeed(-1), mInstanceSeed(0.0f)
    , mRowBuf(NULL), mTime(0.0f), mFrameIdx(0), mGLReady(false)
    , mPanelThread(NULL), mPanelHWND(NULL), mPanelSpawned(false), mPanelLastIC(-1)
    , mRequestedIdx(-1), mUserClicked(false)
    , mSharedCurrentIdx(-1), mSharedFileCount(0)
{
    SetMinInputs(0);
    SetMaxInputs(0);

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
    LONG instNum = InterlockedIncrement(&s_isfInstanceCounter);
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
ISFBrowserPlugin::~ISFBrowserPlugin()
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
}

// ------------------------------------------------------------------
// spawnPanel
// ------------------------------------------------------------------
void ISFBrowserPlugin::spawnPanel()
{
    mPanelThread = CreateThread(NULL, 0, panelThreadProc, this, 0, NULL);
}

// ------------------------------------------------------------------
// scanFiles
// ------------------------------------------------------------------
void ISFBrowserPlugin::scanFiles()
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
bool ISFBrowserPlugin::initGL()
{
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc); wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "ISFBrowWGL";
    RegisterClassExA(&wc);

    mHWND = CreateWindowExA(0, "ISFBrowWGL", "", WS_POPUP,
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
        FILE* f = fopen("C:\\isf_browser_error.txt", "w");
        if (f) { fwrite(buf, 1, len, f); fclose(f); }
        glDeleteShader_(s);
        return 0;
    }
    return s;
}

void ISFBrowserPlugin::unloadShader()
{
    if (mProg) { glDeleteProgram_(mProg); mProg = 0; }
    if (mFS)   { glDeleteShader_(mFS);   mFS = 0; }
    if (mVS)   { glDeleteShader_(mVS);   mVS = 0; }
    mInputCount = 0;
    mExtraCount = 0;
    mLocTIME = mLocRENDERSIZE = mLocTIMEDELTA = mLocFRAMEINDEX = -1;
    for (int i = 0; i < MAX_ALL_INPUTS; ++i) mLocP[i] = -1;
    strcpy(mShaderLabel, "none");
}

bool ISFBrowserPlugin::loadShader(int idx)
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

    // Build preamble with ALL input uniforms
    std::string preamble =
        "#version 120\n"
        "uniform float TIME;\n"
        "uniform float TIMEDELTA;\n"
        "uniform int   FRAMEINDEX;\n"
        "uniform vec2  RENDERSIZE;\n"
        "uniform float uInstanceSeed;\n"
        "varying vec2  isf_FragNormCoord;\n";
    for (int i = 0; i < mInputCount; ++i) {
        preamble += "uniform float ";
        preamble += mInputs[i].name;
        preamble += ";\n";
    }
    // Declare unsupported types so the shader compiles; uniforms default to 0
    for (int i = 0; i < mExtraCount; ++i) {
        switch (mExtra[i].type) {
        case ISF_EXTRA_COLOR:   preamble += "uniform vec4 ";        break;
        case ISF_EXTRA_POINT2D: preamble += "uniform vec2 ";        break;
        case ISF_EXTRA_EVENT:   preamble += "uniform bool ";        break;
        case ISF_EXTRA_SAMPLER: preamble += "uniform sampler2D ";   break;
        }
        preamble += mExtra[i].name;
        preamble += ";\n";
    }

    const char* vsSrc =
        "#version 120\n"
        "varying vec2 isf_FragNormCoord;\n"
        "void main() {\n"
        "    gl_Position       = vec4(gl_Vertex.xy, 0.0, 1.0);\n"
        "    isf_FragNormCoord = gl_Vertex.xy * 0.5 + 0.5;\n"
        "}\n";

    std::string fsSrc = preamble + cleanBody;
    mVS = compileShader(GL_VERTEX_SHADER,   vsSrc);
    mFS = compileShader(GL_FRAGMENT_SHADER, fsSrc.c_str());
    if (!mVS || !mFS) { unloadShader(); return false; }

    mProg = glCreateProgram_();
    glAttachShader_(mProg, mVS);
    glAttachShader_(mProg, mFS);
    glLinkProgram_(mProg);
    GLint ok = 0;
    glGetProgramiv_(mProg, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[4096]; GLsizei len;
        glGetProgramInfoLog_(mProg, sizeof(buf), &len, buf);
        FILE* ef = fopen("C:\\isf_browser_error.txt", "w");
        if (ef) { fwrite(buf, 1, len, ef); fclose(ef); }
        unloadShader(); return false;
    }

    mLocTIME       = glGetUniformLocation_(mProg, "TIME");
    mLocTIMEDELTA  = glGetUniformLocation_(mProg, "TIMEDELTA");
    mLocFRAMEINDEX = glGetUniformLocation_(mProg, "FRAMEINDEX");
    mLocRENDERSIZE = glGetUniformLocation_(mProg, "RENDERSIZE");
    mLocInstanceSeed = glGetUniformLocation_(mProg, "uInstanceSeed");
    for (int i = 0; i < mInputCount; ++i)
        mLocP[i] = glGetUniformLocation_(mProg, mInputs[i].name);
    for (int i = 0; i < mExtraCount; ++i)
        mExtra[i].loc = glGetUniformLocation_(mProg, mExtra[i].name);

    return true;
}

// ------------------------------------------------------------------
// FBO
// ------------------------------------------------------------------
bool ISFBrowserPlugin::buildFBO(int W, int H)
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

    mFBOW = W; mFBOH = H;
    delete[] mRowBuf;
    mRowBuf = new BYTE[W * 3];
    return true;
}

void ISFBrowserPlugin::teardownGL()
{
    if (!mHGLRC) return;
    wglMakeCurrent(mHDC, mHGLRC);
    unloadShader();
    if (mFBO)      { glDeleteFramebuffersEXT_(1, &mFBO); mFBO = 0; }
    if (mColorTex) { glDeleteTextures(1, &mColorTex); mColorTex = 0; }
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(mHGLRC); mHGLRC = NULL;
    if (mHDC && mHWND) { ReleaseDC(mHWND, mHDC); mHDC = NULL; }
    if (mHWND)         { DestroyWindow(mHWND); mHWND = NULL; }
}

// ------------------------------------------------------------------
// updateSharedState — called each frame, read by dialog timer
// ------------------------------------------------------------------
void ISFBrowserPlugin::updateSharedState()
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
DWORD ISFBrowserPlugin::ProcessFrame(void* pFrame)
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
        spawnPanel();
        mPanelSpawned = true;
    }

    wglMakeCurrent(mHDC, mHGLRC);

    // Pick up shader selection from panel (the only way to switch shaders
    // now — PARAM_SHADER is a plain assignable slot, not a shader index).
    int req = mRequestedIdx;
    if (req >= 0 && req != mCurrentIdx) {
        mCurrentIdx       = req;
        mSharedCurrentIdx = req;  // early update — prevents WM_TIMER from syncing listbox back
        mNeedReload = true;
    }
    mRequestedIdx = -1;

    if (mNeedReload) {
        loadShader(mCurrentIdx);
        mNeedReload = false;
    }

    if (W != mFBOW || H != mFBOH) {
        if (!buildFBO(W, H)) { wglMakeCurrent(NULL, NULL); return FF_FAIL; }
    }

    updateSharedState();

    if (!mProg || !mFBO) {
        memset(pFrame, 0, W * H * 3);
        wglMakeCurrent(NULL, NULL);
        return FF_SUCCESS;
    }

    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, mFBO);
    glViewport(0, 0, W, H);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram_(mProg);

    static const float kDelta = 1.0f / 60.0f;
    if (mLocTIME       >= 0) glUniform1f_(mLocTIME,      mTime);
    if (mLocTIMEDELTA  >= 0) glUniform1f_(mLocTIMEDELTA, kDelta);
    if (mLocFRAMEINDEX >= 0) glUniform1i_(mLocFRAMEINDEX, (GLint)mFrameIdx);
    if (mLocRENDERSIZE >= 0) glUniform2f_(mLocRENDERSIZE, (float)W, (float)H);
    if (mLocInstanceSeed >= 0) glUniform1f_(mLocInstanceSeed, mInstanceSeed);

    // Set ALL input uniforms from mAllParams
    for (int i = 0; i < mInputCount; ++i) {
        if (mLocP[i] < 0) continue;
        float val = mInputs[i].minVal
                  + mAllParams[i] * (mInputs[i].maxVal - mInputs[i].minVal);
        glUniform1f_(mLocP[i], val);
    }

    glBegin(GL_QUADS);
        glVertex2f(-1.0f, -1.0f); glVertex2f( 1.0f, -1.0f);
        glVertex2f( 1.0f,  1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    BYTE* fb = (BYTE*)pFrame;
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, fb);
    for (int top = 0, bot = H - 1; top < bot; ++top, --bot) {
        BYTE* rT = fb + top * W * 3;
        BYTE* rB = fb + bot * W * 3;
        memcpy(mRowBuf, rT,       W * 3);
        memcpy(rT,      rB,       W * 3);
        memcpy(rB,      mRowBuf,  W * 3);
    }

    glBindFramebufferEXT_(GL_FRAMEBUFFER_EXT, 0);
    glUseProgram_(0);
    wglMakeCurrent(NULL, NULL);

    mTime += kDelta;
    mFrameIdx++;
    return FF_SUCCESS;
}

// ------------------------------------------------------------------
// Parameters
// ------------------------------------------------------------------
DWORD ISFBrowserPlugin::GetParameter(DWORD index)
{
    if (index >= NUM_FF_PARAMS) return FF_FAIL;
    union { float f; DWORD d; } u;
    u.f = mParam[index];
    return u.d;
}

DWORD ISFBrowserPlugin::SetParameter(const SetParameterStruct* pParam)
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

char* ISFBrowserPlugin::GetParameterDisplay(DWORD index)
{
    if (index < NUM_FF_PARAMS)
        return mSharedParamLabels[index];
    return (char*)"";
}

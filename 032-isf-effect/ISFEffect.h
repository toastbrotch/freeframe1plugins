#ifndef ISFEFFECT_H
#define ISFEFFECT_H

#include "FFPluginSDK.h"
#include <windows.h>
#include <GL/gl.h>
#include <vector>
#include <string>

#define PARAM_SHADER      0
#define PARAM_P1          1
#define PARAM_P2          2
#define PARAM_P3          3
#define NUM_FF_PARAMS     4   // fixed FreeFrame interface

#define MAX_ALL_INPUTS    32  // max ISF inputs we parse + expose in panel — parseISFInputs
                               // bounds-checks against this silently (no error), so a shader
                               // with more INPUTS than this just has its tail dropped: any
                               // uniform past the cap is unset in the preamble, which fails
                               // to compile if the shader body still references it. Keep
                               // headroom above what any of our .fs files actually declare.
#define MAX_FS_FILES      128
#define MAX_EXTRA_UNIFORMS 8  // unsupported types declared but not slider-controlled
#define MAX_PASSES        8   // ISF "PASSES" entries
#define MAX_TARGETS       4   // distinct named pass targets

// A named ISF pass target. Double-buffered: a pass writing it renders into
// tex/fbo[1 - cur] while the shader samples tex[cur] (for a PERSISTENT target
// that's the previous frame, i.e. feedback); then the two swap. Same scheme as
// the ffgl ISF Effect plugin.
struct ISFTarget {
    char   name[64];
    bool   isFloat;   // "FLOAT": true -> RGBA16F, so slow feedback doesn't stall at 8 bits
    bool   nearest;   // "FILTER": "NEAREST"
    GLuint tex[2];
    GLuint fbo[2];
    int    cur;
    GLint  loc;
};

// ISF input types we can't put in the panel but must still declare for the shader to compile
enum ISFExtraType { ISF_EXTRA_COLOR, ISF_EXTRA_POINT2D, ISF_EXTRA_EVENT, ISF_EXTRA_SAMPLER };
struct ISFExtraUniform {
    char         name[64];
    ISFExtraType type;
    GLint        loc;
};

struct ISFInput {
    char  name[64];
    float minVal;
    float maxVal;
    float defaultVal;
};

class ISFEffectPlugin : public CFreeFramePlugin
{
    // ---- public: accessible from free-function panel helpers ----
public:
    // All 4 FreeFrame slots (formerly "Shader + P1/P2/P3") are now uniform,
    // freely-assignable ISF-input sliders; shader switching moved entirely
    // into the panel's file list. mSlotToInput[slot] holds the ISF input
    // index bound to that Resolume slider, or -1 if the slot is unassigned.
    float    mParam[NUM_FF_PARAMS];
    int      mSlotToInput[NUM_FF_PARAMS];
    float    mAllParams[MAX_ALL_INPUTS];     // all ISF input values, 0-1 normalised
    ISFInput mInputs[MAX_ALL_INPUTS];
    int      mInputCount;

public:
     ISFEffectPlugin();
    ~ISFEffectPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    // ---- file list ----
    std::vector<std::string> mFsFiles;
    int      mCurrentIdx;
    bool     mNeedReload;

    // ---- shader label ----
    char     mShaderLabel[64];

    // ---- GL ----
    HWND     mHWND;
    HDC      mHDC;
    HGLRC    mHGLRC;
    GLuint   mVS, mFS, mProg;
    GLuint   mFBO, mColorTex;
    int      mFBOW, mFBOH;
    GLint    mLocTIME, mLocRENDERSIZE, mLocTIMEDELTA, mLocFRAMEINDEX;
    GLint    mLocInstanceSeed;
    GLint    mLocPASSINDEX;
    GLuint   mInTex;      // the layer's video, uploaded every frame (texture unit 0)
    BYTE*    mUpBuf;      // row-flipped copy of the frame for the upload
    int      mPassCount;  // 0 = plain single-pass shader
    int      mPassTarget[MAX_PASSES];   // target index per pass, -1 = the output FBO
    ISFTarget mTargets[MAX_TARGETS];
    int      mTargetCount;
    int      mTargetW, mTargetH;
    LARGE_INTEGER mQpcFreq, mQpcStart;
    double   mLastTime;
    float    mInstanceSeed;  // unique per plugin instance; see constructor
    GLint    mLocP[MAX_ALL_INPUTS];
    int             mExtraCount;
    ISFExtraUniform mExtra[MAX_EXTRA_UNIFORMS];
    BYTE*    mRowBuf;
    float    mTime;
    DWORD    mFrameIdx;
    bool     mGLReady;
    volatile DWORD mLastFrameTick;  // updated each ProcessFrame; panel uses it for heartbeat

    // ---- panel ----
    HANDLE          mPanelThread;
    volatile HWND   mPanelHWND;
    bool            mPanelSpawned;
    int             mPanelLastIC;   // tracks last shown input count per instance
    int             mPanelSeenIdx;  // last mSharedCurrentIdx the listbox followed

    // Dialog → plugin
    volatile int    mRequestedIdx;
    volatile bool   mUserClicked;   // set by LBN_SELCHANGE; timer re-reads lb.cursel as a safety net

    // Plugin → dialog
    volatile int    mSharedCurrentIdx;
    volatile int    mSharedFileCount;
    char            mSharedFileLabels[MAX_FS_FILES][64];
    char            mSharedParamLabels[NUM_FF_PARAMS][80];  // for Resolume GetParameterDisplay
    char            mSharedShaderName[64];

    char     mDisplayBuf[80];

    void     scanFiles();
    bool     initGL();
    bool     loadShader(int idx);
    void     unloadShader();
    bool     buildFBO(int W, int H);
    bool     buildTargets(int W, int H);
    void     releaseTargets();
    bool     compileProgram(const std::string& fsSrc, bool modernGLSL);
    void     teardownGL();
    void     spawnPanel();
    void     updateSharedState();

    static DWORD WINAPI     panelThreadProc(LPVOID param);
    static LRESULT CALLBACK panelWndProc(HWND, UINT, WPARAM, LPARAM);

    static bool        parseISFInputs(const char* hdr, ISFInput* out,
                                      int* count, int maxCount);
    static void        parseISFExtras(const char* hdr, ISFExtraUniform* out,
                                      int* count, int maxCount);
    void               parsePasses(const char* hdr);
    static const char* findKeyVal(const char* json, const char* key);
    static float       parseFloatAt(const char* p, float def);
    static bool        parseStringAt(const char* p, char* out, int maxLen);
};

#endif

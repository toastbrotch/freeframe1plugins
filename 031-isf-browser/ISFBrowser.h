#ifndef ISFBROWSER_H
#define ISFBROWSER_H

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

#define MAX_ALL_INPUTS    12  // max ISF inputs we parse + expose in panel
#define MAX_FS_FILES      128
#define MAX_EXTRA_UNIFORMS 8  // unsupported types declared but not slider-controlled

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

class ISFBrowserPlugin : public CFreeFramePlugin
{
    // ---- public: accessible from free-function panel helpers ----
public:
    float    mParam[NUM_FF_PARAMS];          // FreeFrame protocol (Shader + P1/P2/P3)
    float    mAllParams[MAX_ALL_INPUTS];     // all ISF input values, 0-1 normalised
    ISFInput mInputs[MAX_ALL_INPUTS];
    int      mInputCount;

public:
     ISFBrowserPlugin();
    ~ISFBrowserPlugin();

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

    // Dialog → plugin
    volatile int    mRequestedIdx;
    volatile bool   mUserClicked;   // set by LBN_SELCHANGE; timer reads lb.cursel once committed

    // Plugin → dialog
    volatile int    mSharedCurrentIdx;
    volatile int    mSharedFileCount;
    char            mSharedFileLabels[MAX_FS_FILES][64];
    char            mSharedParamLabels[3][80];  // for Resolume GetParameterDisplay
    char            mSharedShaderName[64];

    char     mDisplayBuf[80];

    void     scanFiles();
    bool     initGL();
    bool     loadShader(int idx);
    void     unloadShader();
    bool     buildFBO(int W, int H);
    void     teardownGL();
    void     spawnPanel();
    void     updateSharedState();

    static DWORD WINAPI     panelThreadProc(LPVOID param);
    static LRESULT CALLBACK panelWndProc(HWND, UINT, WPARAM, LPARAM);

    static bool        parseISFInputs(const char* hdr, ISFInput* out,
                                      int* count, int maxCount);
    static void        parseISFExtras(const char* hdr, ISFExtraUniform* out,
                                      int* count, int maxCount);
    static const char* findKeyVal(const char* json, const char* key);
    static float       parseFloatAt(const char* p, float def);
    static bool        parseStringAt(const char* p, char* out, int maxLen);
};

#endif

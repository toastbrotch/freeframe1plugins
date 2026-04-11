// ============================================================
//  FrameDiff.cpp
//  FreeFrame 1.0 plugin — frame differencing effect
//
//  Modelled exactly on the official FFSDKTutorial pattern.
//  Uses in-place ProcessFrame(), not ProcessFrameCopy().
// ============================================================

#include "FrameDiff.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>

// ============================================================
//  Constructor
// ============================================================
FrameDiffPlugin::FrameDiffPlugin()
: CFreeFramePlugin()
{
    // Plugin capabilities — match tutorial pattern exactly
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);

    // Input properties
    SetMinInputs(1);
    SetMaxInputs(1);

    // Parameters
    SetParamInfo(PARAM_FRAMEDELAY,
                 "Frame Delay",
                 FF_TYPE_STANDARD,
                 DEFAULT_DELAY_NORM);

    // Init state
    mDelayNorm  = DEFAULT_DELAY_NORM;
    mWriteHead  = 0;
    mFrameBytes = 0;

    for (int i = 0; i < MAX_DELAY_FRAMES; i++)
        mRing[i] = NULL;

    mDisplayBuf[0] = '\0';
}

// ============================================================
//  Destructor
// ============================================================
FrameDiffPlugin::~FrameDiffPlugin()
{
    for (int i = 0; i < MAX_DELAY_FRAMES; i++) {
        if (mRing[i]) { free(mRing[i]); mRing[i] = NULL; }
    }
}

// ============================================================
//  ProcessFrame  — in-place, called by FF_PROCESSFRAME
//  pFrame points to the raw pixel buffer (modified in place)
// ============================================================
DWORD FrameDiffPlugin::ProcessFrame(void* pFrame)
{
    // Use SDK helpers — same as tutorial
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    DWORD width  = (DWORD)GetFrameWidth();
    DWORD height = (DWORD)GetFrameHeight();
    DWORD bytes  = width * height * 3;

    // Allocate ring buffer on first call (size now known)
    if (mRing[0] == NULL) {
        mFrameBytes = bytes;
        for (int i = 0; i < MAX_DELAY_FRAMES; i++) {
            mRing[i] = (BYTE*)malloc(bytes);
            if (!mRing[i]) return FF_FAIL;
            memset(mRing[i], 0, bytes);
        }
        mWriteHead = 0;
    }

    BYTE* current = (BYTE*)pFrame;

    // Which past frame to compare against
    int delay    = normToFrames(mDelayNorm);
    int readHead = (mWriteHead - delay + MAX_DELAY_FRAMES) % MAX_DELAY_FRAMES;
    BYTE* past   = mRing[readHead];

    // Save current frame into ring buffer BEFORE we overwrite pFrame
    BYTE* saved = mRing[mWriteHead];
    memcpy(saved, current, bytes);
    mWriteHead = (mWriteHead + 1) % MAX_DELAY_FRAMES;

    // Write diff result back into pFrame in-place
    DWORD pixels = width * height;
    BYTE* src    = saved;   // the frame we just saved = current
    BYTE* ref    = past;
    BYTE* dst    = current; // write back into the same buffer

    for (DWORD p = 0; p < pixels; p++) {
        int d0 = abs((int)src[0] - (int)ref[0]);
        int d1 = abs((int)src[1] - (int)ref[1]);
        int d2 = abs((int)src[2] - (int)ref[2]);

        // Rec.601 luminance
        int luma = (d0 * 299 + d1 * 587 + d2 * 114) / 1000;
        if (luma > 255) luma = 255;

        BYTE v  = (BYTE)luma;
        dst[0]  = v;
        dst[1]  = v;
        dst[2]  = v;

        src += 3;
        ref += 3;
        dst += 3;
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter
// ============================================================
DWORD FrameDiffPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    switch (index) {
    case PARAM_FRAMEDELAY:
        memcpy(&dwRet, &mDelayNorm, 4);
        return dwRet;
    default:
        return FF_FAIL;
    }
}

// ============================================================
//  SetParameter
// ============================================================
DWORD FrameDiffPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;

    switch (pParam->ParameterNumber) {
    case PARAM_FRAMEDELAY:
        memcpy(&mDelayNorm, &pParam->NewParameterValue, 4);
        if (mDelayNorm < 0.0f) mDelayNorm = 0.0f;
        if (mDelayNorm > 1.0f) mDelayNorm = 1.0f;
        return FF_SUCCESS;
    default:
        return FF_FAIL;
    }
}

// ============================================================
//  GetParameterDisplay
// ============================================================
char* FrameDiffPlugin::GetParameterDisplay(DWORD index)
{
    if (index != PARAM_FRAMEDELAY) return NULL;
    int frames = normToFrames(mDelayNorm);
    if (frames == 1)
        sprintf(mDisplayBuf, "1 frame");
    else
        sprintf(mDisplayBuf, "%d frames", frames);
    return mDisplayBuf;
}

// ============================================================
//  Factory method — matches tutorial pattern exactly
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new FrameDiffPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

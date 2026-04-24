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

    SetParamInfo(PARAM_HARDNESS,
                 "Hardness",
                 FF_TYPE_STANDARD,
                 DEFAULT_HARD_NORM);

    // Init state
    mDelayNorm  = DEFAULT_DELAY_NORM;
    mHardNorm   = DEFAULT_HARD_NORM;
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

        // Hardness: scale luma up so the range collapses toward pure black/white.
        // scale = 1 at hardness=0 (soft, full greyscale) and 256 at hardness=1
        // (hard: any nonzero luma → 255, zero stays 0).
        float scale = 1.0f / (1.0f - mHardNorm + 1.0f / 256.0f);
        int v255 = (int)((float)luma * scale + 0.5f);
        if (v255 > 255) v255 = 255;

        BYTE v  = (BYTE)v255;
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
    float val;
    switch (index) {
    case PARAM_FRAMEDELAY: val = mDelayNorm; break;
    case PARAM_HARDNESS:   val = mHardNorm;  break;
    default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

// ============================================================
//  SetParameter
// ============================================================
DWORD FrameDiffPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;

    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
    case PARAM_FRAMEDELAY: mDelayNorm = val; return FF_SUCCESS;
    case PARAM_HARDNESS:   mHardNorm  = val; return FF_SUCCESS;
    default: return FF_FAIL;
    }
}

// ============================================================
//  GetParameterDisplay
// ============================================================
char* FrameDiffPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
    case PARAM_FRAMEDELAY: {
        int frames = normToFrames(mDelayNorm);
        if (frames == 1)
            sprintf(mDisplayBuf, "1 frame");
        else
            sprintf(mDisplayBuf, "%d frames", frames);
        return mDisplayBuf;
    }
    case PARAM_HARDNESS:
        sprintf(mDisplayBuf, "%d%%", (int)(mHardNorm * 100.0f + 0.5f));
        return mDisplayBuf;
    default:
        return NULL;
    }
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

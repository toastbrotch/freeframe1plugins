#ifndef FRAMEDIFF_H
#define FRAMEDIFF_H

// ============================================================
//  FrameDiff.h — modelled on FFSDKTutorial pattern
// ============================================================

#include "FFPluginSDK.h"

#define MIN_DELAY_FRAMES    1
#define MAX_DELAY_FRAMES    30
#define PARAM_FRAMEDELAY    0
#define PARAM_HARDNESS      1
#define DEFAULT_DELAY_NORM  0.0f
#define DEFAULT_HARD_NORM   0.0f

inline int normToFrames(float norm) {
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    int f = (int)(norm * (float)(MAX_DELAY_FRAMES - MIN_DELAY_FRAMES) + 0.5f)
            + MIN_DELAY_FRAMES;
    if (f < MIN_DELAY_FRAMES) f = MIN_DELAY_FRAMES;
    if (f > MAX_DELAY_FRAMES) f = MAX_DELAY_FRAMES;
    return f;
}

class FrameDiffPlugin : public CFreeFramePlugin
{
public:
     FrameDiffPlugin();
    ~FrameDiffPlugin();

    // In-place processing — matches tutorial, called by FF_PROCESSFRAME
    DWORD ProcessFrame(void* pFrame);

    // Parameter access — note const SetParameterStruct* matches SDK signature
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    BYTE*   mRing[MAX_DELAY_FRAMES];
    int     mWriteHead;
    DWORD   mFrameBytes;
    float   mDelayNorm;
    float   mHardNorm;
    char    mDisplayBuf[32];
};

#endif // FRAMEDIFF_H

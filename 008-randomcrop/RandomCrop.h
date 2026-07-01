#ifndef RANDOMCROP_H
#define RANDOMCROP_H

#include "FFPluginSDK.h"

#define PARAM_SPEED    0
#define PARAM_MAXCROP  1
#define PARAM_HOLD     2
#define PARAM_SOFTNESS 3

#define DEFAULT_SPEED    0.35f
#define DEFAULT_MAXCROP  0.65f
#define DEFAULT_HOLD     0.3f
#define DEFAULT_SOFTNESS 0.25f

// Speed:    slider 0→1 maps build rate 0.005→0.15 per frame (200→7 frames to full)
// MaxCrop:  0→1 → 0–100% of the frame diagonal blacked out at peak
// Hold:     0→1 → 0–150 frames at maximum before reversing
// Softness: 0→1 → 0–25% of crop range as a gradient fade zone at the edge

#define MAX_HOLD_FRAMES  150
#define TILT_DEG         30.0f   // ±max random tilt off horizontal sweep

enum CropPhase { PH_IDLE, PH_BUILD, PH_HOLD, PH_RELEASE };

class RandomCropPlugin : public CFreeFramePlugin
{
public:
     RandomCropPlugin();
    ~RandomCropPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float     mSpeedNorm;
    float     mMaxCropNorm;
    float     mHoldNorm;
    float     mSoftnessNorm;

    CropPhase mPhase;
    float     mCropPos;    // 0.0 (no crop) → 1.0 (full crop)
    float     mAngle;      // sweep direction in radians (0=left, π=right)
    int       mHoldTimer;
    int       mIdleTimer;
    unsigned  mRandState;
    char      mDisplayBuf[32];

    unsigned lcg();
    float    randFloat();
    void     beginCycle();
};

#endif // RANDOMCROP_H

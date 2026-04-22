#ifndef LINEEMITTER_H
#define LINEEMITTER_H

#include "FFPluginSDK.h"
#include <cmath>

#define PARAM_NUM_LINES   0
#define PARAM_BAND_WIDTH  1
#define PARAM_SCALE       2
#define PARAM_SPEED       3

// Defaults
#define DEFAULT_NUM_LINES_NORM  0.4898f // 25 lines
#define DEFAULT_BAND_WIDTH_NORM 0.0709f // 10 px
#define DEFAULT_SCALE_NORM      0.5f    // 1.0x (no perspective)
#define DEFAULT_SPEED_NORM      0.0f    // stopped

#define MIN_LINES  1
#define MAX_LINES 50

#define MIN_PX   1
#define MAX_PX 128

// Max speed in u-space units per frame (at slider = 1.0)
#define MAX_SPEED_UPF 15.0f

inline int normToLines(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    int v = (int)(n * (float)(MAX_LINES - MIN_LINES) + 0.5f) + MIN_LINES;
    if (v < MIN_LINES) v = MIN_LINES;
    if (v > MAX_LINES) v = MAX_LINES;
    return v;
}

inline int normToPx(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    int v = (int)(n * (float)(MAX_PX - MIN_PX) + 0.5f) + MIN_PX;
    if (v < MIN_PX) v = MIN_PX;
    if (v > MAX_PX) v = MAX_PX;
    return v;
}

// 0.0 → 0.1x scale at top (lines shrink/converge toward top)
// 0.5 → 1.0x scale at top (uniform, no perspective)
// 1.0 → 10.0x scale at top (lines grow/diverge toward top)
inline float normToScale(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return powf(5.0f, 2.0f * n - 1.0f);
}

// 0.0 → 0 u-units/frame  (stopped)
// 1.0 → MAX_SPEED_UPF u-units/frame
inline float normToSpeed(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n * MAX_SPEED_UPF;
}

class LineEmitterPlugin : public CFreeFramePlugin
{
public:
     LineEmitterPlugin();
    ~LineEmitterPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float  mNumLinesNorm;
    float  mBandWidthNorm;
    float  mScaleNorm;
    float  mSpeedNorm;
    float  mOffset;        // accumulated scroll offset in u-space
    char   mDisplayBuf[32];
};

#endif // LINEEMITTER_H
#ifndef RINGS_H
#define RINGS_H

#include "FFPluginSDK.h"

#define PARAM_NUM_RINGS   0
#define PARAM_RING_WIDTH  1
#define PARAM_GAP_WIDTH   2

// Normalized defaults
#define DEFAULT_NUM_RINGS_NORM  0.1f   // ~5 rings out of 1..50
#define DEFAULT_RING_WIDTH_NORM 0.05f  // ~13px out of 1..255
#define DEFAULT_GAP_WIDTH_NORM  0.05f  // ~13px out of 1..255

#define MIN_RINGS  1
#define MAX_RINGS 50

#define MIN_PX   1
#define MAX_PX 255

inline int normToRings(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    int v = (int)(n * (float)(MAX_RINGS - MIN_RINGS) + 0.5f) + MIN_RINGS;
    if (v < MIN_RINGS) v = MIN_RINGS;
    if (v > MAX_RINGS) v = MAX_RINGS;
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

class RingsPlugin : public CFreeFramePlugin
{
public:
     RingsPlugin();
    ~RingsPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float  mNumRingsNorm;
    float  mRingWidthNorm;
    float  mGapWidthNorm;
    char   mDisplayBuf[32];
};

#endif // RINGS_H

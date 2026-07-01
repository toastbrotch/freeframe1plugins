#ifndef VHSGLITCH_H
#define VHSGLITCH_H

#include "FFPluginSDK.h"

#define PARAM_SHIFT     0
#define PARAM_CHROMA    1
#define PARAM_NOISE     2
#define PARAM_TRACKING  3

#define DEFAULT_SHIFT    0.25f
#define DEFAULT_CHROMA   0.25f
#define DEFAULT_NOISE    0.15f
#define DEFAULT_TRACKING 0.15f

// Shift:    slider 1.0  → ±60 px max horizontal row displacement
// Chroma:   slider 1.0  → ±20 px per-channel RGB separation
// Noise:    slider 1.0  → up to 45% of rows affected, full noise blend
// Tracking: slider 1.0  → 4 displaced blocks per frame

#define MAX_SHIFT_PX        60
#define MAX_CHROMA_PX       20
#define MAX_TRACKING_BLOCKS  4
#define ROW_BUF_WIDTH     4096   // supports up to 4K frame width

struct TrackingBlock { int y0, y1, dx; };

class VHSGlitchPlugin : public CFreeFramePlugin
{
public:
     VHSGlitchPlugin();
    ~VHSGlitchPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float    mShiftNorm;
    float    mChromaNorm;
    float    mNoiseNorm;
    float    mTrackingNorm;

    unsigned mFrameCount;
    BYTE     mRowBuf[ROW_BUF_WIDTH * 3];
    char     mDisplayBuf[32];

    static unsigned hashPair(unsigned a, unsigned b);
};

#endif // VHSGLITCH_H

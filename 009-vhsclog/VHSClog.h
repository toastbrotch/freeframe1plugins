#ifndef VHSCLOG_H
#define VHSCLOG_H

#include "FFPluginSDK.h"

#define PARAM_CLOG        0
#define PARAM_THICKNESS   1
#define PARAM_SMEAR       2
#define PARAM_FLUTTER     3

#define DEFAULT_CLOG       0.30f
#define DEFAULT_THICKNESS  0.40f
#define DEFAULT_SMEAR      0.40f
#define DEFAULT_FLUTTER    0.30f

// Clog:      0-1  → 0-5 horizontal signal-loss bands
// Thickness: 0-1  → 1-16 px vertical height of each smear stripe
// Smear:     0-1  → hold probability 0-98.5% (~66 px avg run at max)
// Flutter:   0-1  → band positions change from every ~10000 frames down to every frame

#define MAX_BANDS       5
#define MIN_BAND_H      5
#define MAX_BAND_H      48
#define MAX_THICK_PX    16
#define ROW_BUF_WIDTH   4096

struct ClogBand { int y0, y1; unsigned seed; };

class VHSClogPlugin : public CFreeFramePlugin
{
public:
     VHSClogPlugin();
    ~VHSClogPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float    mClogNorm;
    float    mThicknessNorm;
    float    mSmearNorm;
    float    mFlutterNorm;

    unsigned mFrameCount;
    BYTE     mRowBuf[ROW_BUF_WIDTH * 3];
    char     mDisplayBuf[32];

    static unsigned hash2(unsigned a, unsigned b);
};

#endif // VHSCLOG_H

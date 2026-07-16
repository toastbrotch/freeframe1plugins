#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "FFPluginSDK.h"

#define PARAM_SWITCH    0
#define PARAM_DENSITY   1

#define DEFAULT_SWITCH  0.30f
#define DEFAULT_DENSITY 0.50f

#define MAX_ARTEFACTS   8

enum AType {
    AT_TEXTBLOCK = 0,
    AT_WAVEFORM,
    AT_BARGRAPH,
    AT_LINECHART,
    AT_HEXDUMP,
    AT_STATUS,
    AT_SCOPE,
    AT_COUNT
};

struct Artefact {
    int      type;
    int      x, y, w, h;
    unsigned seed;
};

class TelemetryPlugin : public CFreeFramePlugin
{
public:
     TelemetryPlugin();
    ~TelemetryPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float    mSwitchNorm;
    float    mDensityNorm;

    unsigned mFrameCount;
    unsigned mLastEpoch;
    unsigned mSetSeed;

    Artefact mArtefacts[MAX_ARTEFACTS];

    char     mDisplayBuf[32];

    void     rebuildSet(int W, int H);
    void     drawArtefact(BYTE* fb, int W, int H, const Artefact& a);
    void     drawTextBlock(BYTE* fb, int W, int H, const Artefact& a);
    void     drawWaveform(BYTE* fb, int W, int H, const Artefact& a);
    void     drawBarGraph(BYTE* fb, int W, int H, const Artefact& a);
    void     drawLineChart(BYTE* fb, int W, int H, const Artefact& a);
    void     drawHexDump(BYTE* fb, int W, int H, const Artefact& a);
    void     drawStatus(BYTE* fb, int W, int H, const Artefact& a);
    void     drawScope(BYTE* fb, int W, int H, const Artefact& a);

    void     putPixel(BYTE* fb, int W, int H, int x, int y);
    void     hLine(BYTE* fb, int W, int H, int x, int y, int len);
    void     vLine(BYTE* fb, int W, int H, int x, int y, int len);
    void     rect(BYTE* fb, int W, int H, int x, int y, int w, int h);
    void     drawChar(BYTE* fb, int W, int H, int x, int y, char c);
    int      drawText(BYTE* fb, int W, int H, int x, int y, const char* s);

    static unsigned h2(unsigned a, unsigned b);
};

#endif // TELEMETRY_H

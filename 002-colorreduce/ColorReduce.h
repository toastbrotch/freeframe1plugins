#ifndef COLORREDUCE_H
#define COLORREDUCE_H

#include "FFPluginSDK.h"

#define PARAM_COLORS        0
#define DEFAULT_COLORS_NORM 0.1f   // default near 2 so effect is obvious

#define MIN_COLORS   2
#define MAX_COLORS  64             // beyond 64 the effect is invisible

inline int normToColors(float norm) {
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    int c = (int)(norm * (float)(MAX_COLORS - MIN_COLORS) + 0.5f) + MIN_COLORS;
    if (c < MIN_COLORS) c = MIN_COLORS;
    if (c > MAX_COLORS) c = MAX_COLORS;
    return c;
}

class ColorReducePlugin : public CFreeFramePlugin
{
public:
     ColorReducePlugin();
    ~ColorReducePlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float   mColorsNorm;
    char    mDisplayBuf[32];
};

#endif // COLORREDUCE_H

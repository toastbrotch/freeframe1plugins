#ifndef LINES_H
#define LINES_H

#include "FFPluginSDK.h"
#include <cmath>

#define PARAM_NUM_LINES   0
#define PARAM_LINE_WIDTH  1
#define PARAM_GAP_WIDTH   2
#define PARAM_ANGLE       3

#define DEFAULT_NUM_LINES_NORM  0.04f  // ~5 lines out of 1..50
#define DEFAULT_LINE_WIDTH_NORM 0.04f  // ~11px out of 1..255
#define DEFAULT_GAP_WIDTH_NORM  0.04f  // ~11px out of 1..255
#define DEFAULT_ANGLE_NORM      0.0f   // 0 degrees (horizontal lines)

#define MIN_LINES  1
#define MAX_LINES 50

#define MIN_PX   1
#define MAX_PX 255

// 0.0–1.0 maps to 0–179 degrees (180 covers all unique orientations)
#define MAX_ANGLE_DEG 180.0f

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

inline float normToAngleDeg(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return n * MAX_ANGLE_DEG;
}

class LinesPlugin : public CFreeFramePlugin
{
public:
     LinesPlugin();
    ~LinesPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float  mNumLinesNorm;
    float  mLineWidthNorm;
    float  mGapWidthNorm;
    float  mAngleNorm;
    char   mDisplayBuf[32];
};

#endif // LINES_H
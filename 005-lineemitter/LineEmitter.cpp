// ============================================================
//  LineEmitter.cpp
//  FreeFrame 1.0 plugin — animated perspective lines from bottom
//
//  Draws N full-width horizontal lines emitting from the bottom
//  of the frame and travelling upward. Pixels inside a line are
//  colour-inverted; pixels in the gaps are left unchanged.
//
//  Line width equals gap width (both controlled by BandWidth).
//  The Speed slider advances the lines each frame. The Scale
//  slider applies a perspective warp so lines grow or shrink
//  as they travel away from the bottom.
//
//  Algorithm:
//    1. For each row compute the perspective-corrected coordinate
//       u(d) where d = distance from bottom:
//
//         s(d) = 1 + (k-1) * d/(H-1)         (local scale)
//         u(d) = ln(1+a·d)/a   (a=(k-1)/(H-1); equals d when k=1)
//
//    2. Each frame mOffset advances by speed (u-units/frame).
//       The pattern is phase-shifted by mOffset within the active
//       zone [0, numLines*period), wrapping seamlessly so new
//       lines continuously emerge from the bottom.
//
//    3. Within the active zone: band the shifted coordinate.
//       posInBand = (u - phase) mod period.
//       Invert if posInBand < bandWidth (line half of the period).
//
//  Parameters:
//    NumLines  : 0.0–1.0  →  1–50 simultaneous lines
//    BandWidth : 0.0–1.0  →  1–255 px (line width = gap width)
//    Scale     : 0.0–1.0  →  0.1x–10.0x perspective at top
//                0.5 = flat, <0.5 = converge, >0.5 = diverge
//    Speed     : 0.0–1.0  →  0–30 u-units/frame
// ============================================================

#include "LineEmitter.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ============================================================
//  Constructor
// ============================================================
LineEmitterPlugin::LineEmitterPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_NUM_LINES,
                 "NumLines",
                 FF_TYPE_STANDARD,
                 DEFAULT_NUM_LINES_NORM);

    SetParamInfo(PARAM_BAND_WIDTH,
                 "BandWidth",
                 FF_TYPE_STANDARD,
                 DEFAULT_BAND_WIDTH_NORM);

    SetParamInfo(PARAM_SCALE,
                 "Scale",
                 FF_TYPE_STANDARD,
                 DEFAULT_SCALE_NORM);

    SetParamInfo(PARAM_SPEED,
                 "Speed",
                 FF_TYPE_STANDARD,
                 DEFAULT_SPEED_NORM);

    mNumLinesNorm  = DEFAULT_NUM_LINES_NORM;
    mBandWidthNorm = DEFAULT_BAND_WIDTH_NORM;
    mScaleNorm     = DEFAULT_SCALE_NORM;
    mSpeedNorm     = DEFAULT_SPEED_NORM;
    mOffset        = 0.0f;
    mDisplayBuf[0] = '\0';
}

LineEmitterPlugin::~LineEmitterPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD LineEmitterPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int   numLines  = normToLines(mNumLinesNorm);
    int   bandWidth = normToPx(mBandWidthNorm);  // line width = gap width
    float k         = normToScale(mScaleNorm);
    float speed     = normToSpeed(mSpeedNorm);

    int W = (int)GetFrameWidth();
    int H = (int)GetFrameHeight();

    // period = bandWidth (line) + bandWidth (gap)
    int   period    = bandWidth * 2;
    int   totalSpan = numLines * period;

    // Advance scroll offset and wrap to prevent float drift
    mOffset += speed;
    if (mOffset >= (float)totalSpan) mOffset -= (float)totalSpan;

    // Phase within one period (used to shift the band pattern)
    float phase = fmodf(mOffset, (float)period);

    // Perspective warp coefficient
    float a       = (H > 1) ? (k - 1.0f) / (float)(H - 1) : 0.0f;
    bool  uniform = (fabsf(a) < 1e-5f);

    BYTE* p = (BYTE*)pFrame;

    for (int y = 0; y < H; y++) {
        // Distance from bottom (0 = bottom row)
        float d = (float)(H - 1 - y);

        // Perspective-corrected coordinate u (0 at bottom, increases upward)
        float u;
        if (uniform) {
            u = d;
        } else {
            float arg = 1.0f + a * d;
            if (arg <= 0.0f) arg = 1e-6f;
            u = logf(arg) / a;
        }

        // Only the active zone [0, totalSpan) gets lines
        bool invert = false;
        if (u >= 0.0f && u < (float)totalSpan) {
            // Shift by phase, wrap into [0, period)
            float u_shifted = u - phase;
            if (u_shifted < 0.0f) u_shifted += (float)period;
            int posInBand = (int)u_shifted % period;
            if (posInBand < bandWidth) {
                invert = true;
            }
        }

        if (invert) {
            for (int x = 0; x < W; x++) {
                p[0] = (BYTE)(255 - p[0]);
                p[1] = (BYTE)(255 - p[1]);
                p[2] = (BYTE)(255 - p[2]);
                p += 3;
            }
        } else {
            p += W * 3;
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD LineEmitterPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_NUM_LINES:  val = mNumLinesNorm;  break;
        case PARAM_BAND_WIDTH: val = mBandWidthNorm; break;
        case PARAM_SCALE:      val = mScaleNorm;     break;
        case PARAM_SPEED:      val = mSpeedNorm;     break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD LineEmitterPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_NUM_LINES:  mNumLinesNorm  = val; return FF_SUCCESS;
        case PARAM_BAND_WIDTH: mBandWidthNorm = val; return FF_SUCCESS;
        case PARAM_SCALE:      mScaleNorm     = val; return FF_SUCCESS;
        case PARAM_SPEED:      mSpeedNorm     = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* LineEmitterPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_NUM_LINES:
            sprintf(mDisplayBuf, "%d lines", normToLines(mNumLinesNorm));
            return mDisplayBuf;
        case PARAM_BAND_WIDTH:
            sprintf(mDisplayBuf, "%d px", normToPx(mBandWidthNorm));
            return mDisplayBuf;
        case PARAM_SCALE:
            sprintf(mDisplayBuf, "%.2fx", normToScale(mScaleNorm));
            return mDisplayBuf;
        case PARAM_SPEED:
            sprintf(mDisplayBuf, "%.1f u/fr", normToSpeed(mSpeedNorm));
            return mDisplayBuf;
        default:
            return NULL;
    }
}

// ============================================================
//  Factory method
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new LineEmitterPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}
// ============================================================
//  Lines.cpp
//  FreeFrame 1.0 plugin — rotating inverting lines
//
//  Draws N full-width lines across the frame at a given angle.
//  Pixels that fall inside a line are colour-inverted (255 - ch);
//  pixels in the gaps between lines are left unchanged.
//
//  The angle is applied by projecting each pixel onto the axis
//  perpendicular to the lines. Lines span the full image width
//  at every angle.
//
//  Parameters:
//    NumLines  : 0.0–1.0  →  1–50 lines
//    LineWidth : 0.0–1.0  →  1–255 pixels
//    GapWidth  : 0.0–1.0  →  1–255 pixels
//    Angle     : 0.0–1.0  →  0–180 degrees
// ============================================================

#include "Lines.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ============================================================
//  Constructor
// ============================================================
LinesPlugin::LinesPlugin()
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

    SetParamInfo(PARAM_LINE_WIDTH,
                 "LineWidth",
                 FF_TYPE_STANDARD,
                 DEFAULT_LINE_WIDTH_NORM);

    SetParamInfo(PARAM_GAP_WIDTH,
                 "GapWidth",
                 FF_TYPE_STANDARD,
                 DEFAULT_GAP_WIDTH_NORM);

    SetParamInfo(PARAM_ANGLE,
                 "Angle",
                 FF_TYPE_STANDARD,
                 DEFAULT_ANGLE_NORM);

    mNumLinesNorm  = DEFAULT_NUM_LINES_NORM;
    mLineWidthNorm = DEFAULT_LINE_WIDTH_NORM;
    mGapWidthNorm  = DEFAULT_GAP_WIDTH_NORM;
    mAngleNorm     = DEFAULT_ANGLE_NORM;
    mDisplayBuf[0] = '\0';
}

LinesPlugin::~LinesPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD LinesPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int numLines  = normToLines(mNumLinesNorm);
    int lineWidth = normToPx(mLineWidthNorm);
    int gapWidth  = normToPx(mGapWidthNorm);
    float angleDeg = normToAngleDeg(mAngleNorm);

    int W = (int)GetFrameWidth();
    int H = (int)GetFrameHeight();

    // Period = one line + one gap
    int period = lineWidth + gapWidth;

    // Convert angle to radians. 0° = horizontal lines (project onto Y axis).
    // The perpendicular axis to the lines is the direction the lines "stack" in.
    float angleRad = (float)(angleDeg * M_PI / 180.0);

    // Unit vector along the stacking direction (perpendicular to line orientation)
    float nx = (float)sinf(angleRad);   // component along X
    float ny = (float)cosf(angleRad);   // component along Y

    // Centre of image
    float cx = (float)(W - 1) * 0.5f;
    float cy = (float)(H - 1) * 0.5f;

    // Total coverage: numLines * period pixels along the stacking axis,
    // centred in the image.
    float halfSpan = (float)(numLines * period) * 0.5f;

    BYTE* p = (BYTE*)pFrame;

    for (int y = 0; y < H; y++) {
        float dy = (float)y - cy;
        for (int x = 0; x < W; x++) {
            float dx = (float)x - cx;

            // Signed projection onto stacking axis
            float proj = dx * nx + dy * ny;

            // Shift so that 0 is the start of the first line band
            float shifted = proj + halfSpan;

            // Only invert pixels within the banded region
            if (shifted >= 0.0f && shifted < (float)(numLines * period)) {
                int pos        = (int)shifted;
                int posInBand  = pos % period;
                if (posInBand < lineWidth) {
                    p[0] = (BYTE)(255 - p[0]);
                    p[1] = (BYTE)(255 - p[1]);
                    p[2] = (BYTE)(255 - p[2]);
                }
            }

            p += 3;
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD LinesPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_NUM_LINES:  val = mNumLinesNorm;  break;
        case PARAM_LINE_WIDTH: val = mLineWidthNorm; break;
        case PARAM_GAP_WIDTH:  val = mGapWidthNorm;  break;
        case PARAM_ANGLE:      val = mAngleNorm;     break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD LinesPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_NUM_LINES:  mNumLinesNorm  = val; return FF_SUCCESS;
        case PARAM_LINE_WIDTH: mLineWidthNorm = val; return FF_SUCCESS;
        case PARAM_GAP_WIDTH:  mGapWidthNorm  = val; return FF_SUCCESS;
        case PARAM_ANGLE:      mAngleNorm     = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* LinesPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_NUM_LINES:
            sprintf(mDisplayBuf, "%d lines", normToLines(mNumLinesNorm));
            return mDisplayBuf;
        case PARAM_LINE_WIDTH:
            sprintf(mDisplayBuf, "%d px", normToPx(mLineWidthNorm));
            return mDisplayBuf;
        case PARAM_GAP_WIDTH:
            sprintf(mDisplayBuf, "%d px", normToPx(mGapWidthNorm));
            return mDisplayBuf;
        case PARAM_ANGLE:
            sprintf(mDisplayBuf, "%.0f deg", normToAngleDeg(mAngleNorm));
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
    *ppInstance = new LinesPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}
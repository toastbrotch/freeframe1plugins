// ============================================================
//  Rings.cpp
//  FreeFrame 1.0 plugin — concentric inverting rings
//
//  Draws N concentric rings centred in the frame. Pixels that
//  fall inside a ring are colour-inverted (255 - channel);
//  pixels in the gaps between rings are left unchanged.
//
//  Parameters:
//    NumRings  : 0.0–1.0  →  1–50 rings
//    RingWidth : 0.0–1.0  →  1–255 pixels
//    GapWidth  : 0.0–1.0  →  1–255 pixels
// ============================================================

#include "Rings.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ============================================================
//  Constructor
// ============================================================
RingsPlugin::RingsPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_NUM_RINGS,
                 "NumRings",
                 FF_TYPE_STANDARD,
                 DEFAULT_NUM_RINGS_NORM);

    SetParamInfo(PARAM_RING_WIDTH,
                 "RingWidth",
                 FF_TYPE_STANDARD,
                 DEFAULT_RING_WIDTH_NORM);

    SetParamInfo(PARAM_GAP_WIDTH,
                 "GapWidth",
                 FF_TYPE_STANDARD,
                 DEFAULT_GAP_WIDTH_NORM);

    mNumRingsNorm  = DEFAULT_NUM_RINGS_NORM;
    mRingWidthNorm = DEFAULT_RING_WIDTH_NORM;
    mGapWidthNorm  = DEFAULT_GAP_WIDTH_NORM;
    mDisplayBuf[0] = '\0';
}

RingsPlugin::~RingsPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD RingsPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int numRings  = normToRings(mNumRingsNorm);
    int ringWidth = normToPx(mRingWidthNorm);
    int gapWidth  = normToPx(mGapWidthNorm);

    int W = (int)GetFrameWidth();
    int H = (int)GetFrameHeight();

    // Centre of the image (floating point for accuracy)
    float cx = (float)(W - 1) * 0.5f;
    float cy = (float)(H - 1) * 0.5f;

    // Period = one ring + one gap
    int period = ringWidth + gapWidth;

    BYTE* p = (BYTE*)pFrame;

    for (int y = 0; y < H; y++) {
        float dy = (float)y - cy;
        for (int x = 0; x < W; x++) {
            float dx = (float)x - cx;

            // Euclidean distance from centre (integer radius)
            int r = (int)sqrtf(dx * dx + dy * dy);

            // Which ring band does this radius fall into?
            // Band index starts at 0 for the innermost.
            int band      = r / period;          // which period we're in
            int posInBand = r % period;           // position within that period

            // Invert if inside a ring (not the gap) and within numRings
            if (band < numRings && posInBand < ringWidth) {
                p[0] = (BYTE)(255 - p[0]);
                p[1] = (BYTE)(255 - p[1]);
                p[2] = (BYTE)(255 - p[2]);
            }

            p += 3;
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD RingsPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_NUM_RINGS:  val = mNumRingsNorm;  break;
        case PARAM_RING_WIDTH: val = mRingWidthNorm; break;
        case PARAM_GAP_WIDTH:  val = mGapWidthNorm;  break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD RingsPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_NUM_RINGS:  mNumRingsNorm  = val; return FF_SUCCESS;
        case PARAM_RING_WIDTH: mRingWidthNorm = val; return FF_SUCCESS;
        case PARAM_GAP_WIDTH:  mGapWidthNorm  = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* RingsPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_NUM_RINGS:
            sprintf(mDisplayBuf, "%d rings", normToRings(mNumRingsNorm));
            return mDisplayBuf;
        case PARAM_RING_WIDTH:
            sprintf(mDisplayBuf, "%d px", normToPx(mRingWidthNorm));
            return mDisplayBuf;
        case PARAM_GAP_WIDTH:
            sprintf(mDisplayBuf, "%d px", normToPx(mGapWidthNorm));
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
    *ppInstance = new RingsPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

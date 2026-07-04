// ============================================================
//  VHSClog.cpp
//  FreeFrame 1.0 plugin — VHS tape-head clogging simulation
//
//  Four stacking artefacts that mimic a partially clogged VHS
//  read head:
//
//  Clog      — number of horizontal signal-loss bands (0-5).
//              Rows outside all bands pass through untouched.
//
//  Thickness — vertical height of each smear stripe. Rows
//              within a clog band are grouped into slabs of
//              1-16 lines; all rows in a slab share the same
//              hold/fresh decision sequence, so smear streaks
//              become vertically fat. The band itself is
//              unaffected.
//
//  Smear     — within each clog band, a left-to-right pass
//              randomly decides per pixel whether to "read
//              fresh" from the source or to hold the last
//              pixel read. The probability tops out at 98.5%
//              (≈66 px average run), roughly twice the old
//              max, so high Smear fills whole rows with one
//              dragged colour.
//
//  Flutter — controls how quickly band positions change
//            between frames. At 0 bands are essentially
//            fixed; at 1 they reposition every frame,
//            giving a chaotic jittering appearance.
//
//  All randomness is deterministic per (frame, band, row)
//  so the effect is reproducible at any parameter position.
// ============================================================

#include "VHSClog.h"
#include <cstdio>
#include <cstring>

// ============================================================
//  Deterministic integer hash
// ============================================================
unsigned VHSClogPlugin::hash2(unsigned a, unsigned b)
{
    unsigned h = a ^ (b * 2654435761u);
    h ^= h >> 16;
    h *= 0x45d9f3bu;
    h ^= h >> 16;
    return h;
}

// ============================================================
//  Constructor
// ============================================================
VHSClogPlugin::VHSClogPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_CLOG,      "Clog",      FF_TYPE_STANDARD, DEFAULT_CLOG);
    SetParamInfo(PARAM_THICKNESS, "Thickness", FF_TYPE_STANDARD, DEFAULT_THICKNESS);
    SetParamInfo(PARAM_SMEAR,     "Smear",     FF_TYPE_STANDARD, DEFAULT_SMEAR);
    SetParamInfo(PARAM_FLUTTER,   "Flutter",   FF_TYPE_STANDARD, DEFAULT_FLUTTER);

    mClogNorm       = DEFAULT_CLOG;
    mThicknessNorm  = DEFAULT_THICKNESS;
    mSmearNorm      = DEFAULT_SMEAR;
    mFlutterNorm    = DEFAULT_FLUTTER;
    mFrameCount  = 0;
    mDisplayBuf[0] = '\0';
    memset(mRowBuf, 0, sizeof(mRowBuf));
}

VHSClogPlugin::~VHSClogPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD VHSClogPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int   W  = (int)GetFrameWidth();
    int   H  = (int)GetFrameHeight();
    BYTE* fb = (BYTE*)pFrame;

    if (W > ROW_BUF_WIDTH) return FF_SUCCESS;

    const int rowBytes = W * 3;

    const unsigned fc = mFrameCount++;

    // --- Derived parameters ---
    int numBands = (mClogNorm < 0.01f) ? 0
                 : (int)(mClogNorm * (float)MAX_BANDS + 0.5f);
    if (numBands > MAX_BANDS) numBands = MAX_BANDS;

    if (numBands == 0) return FF_SUCCESS;

    // Flutter: frames per band-position change
    // flutter=0 → period ≈ 10000 frames (stable)
    // flutter=1 → period = 1 frame (chaotic)
    unsigned period;
    if (mFlutterNorm < 0.005f) {
        period = 10000u;
    } else {
        float f = mFlutterNorm;
        period = (unsigned)(1.0f / (f * f));
        if (period < 1u) period = 1u;
    }
    const unsigned slowFrame = fc / period;

    // Thickness: rows per smear slab (all rows in a slab share the same smear pattern)
    // thickness=0 → 1 px (each row independent), thickness=1 → MAX_THICK_PX rows per slab
    const int thickPx = 1 + (int)(mThicknessNorm * (float)(MAX_THICK_PX - 1) + 0.5f);

    // --- Build clog bands (deterministic per slowFrame) ---
    ClogBand bands[MAX_BANDS];
    for (int b = 0; b < numBands; b++) {
        unsigned bSeed = hash2(slowFrame + (unsigned)(b) * 2654435761u, 0xCAFEBABEu);
        int bandH = MIN_BAND_H + (int)(hash2(bSeed, 1u) % (unsigned)(MAX_BAND_H - MIN_BAND_H + 1));
        int y0    = (int)(hash2(bSeed, 2u) % (unsigned)H);
        if (y0 + bandH >= H) y0 = H - 1 - bandH;
        if (y0 < 0) y0 = 0;
        bands[b].y0   = y0;
        bands[b].y1   = y0 + bandH;
        bands[b].seed = bSeed;
    }

    // Smear threshold: LCG value below this → hold last pixel.
    // Slope is 2× the old value; capped at 0.985 so avg run at max ≈ 66 px
    // (double the old ~33 px max).
    float smearProb = mSmearNorm * 2.0f;
    if (smearProb > 0.985f) smearProb = 0.985f;
    const unsigned smearThresh = (unsigned)(smearProb * 4294967295.0f);

    // --- Row loop ---
    for (int y = 0; y < H; y++) {
        BYTE* row = fb + y * rowBytes;

        // Find matching clog band (first match wins)
        const ClogBand* band = NULL;
        for (int b = 0; b < numBands; b++) {
            if (y >= bands[b].y0 && y <= bands[b].y1) {
                band = &bands[b];
                break;
            }
        }
        if (!band) continue;

        // Stash original row — we read from here, write to row
        memcpy(mRowBuf, row, rowBytes);

        // All rows in the same thickness slab share the same LCG seed,
        // so their hold/fresh decisions are identical → vertically fat smear stripes.
        int slabY = (y / thickPx) * thickPx;
        unsigned lcg = hash2(band->seed ^ ((unsigned)slabY << 3), 0xFEEDF00Du);

        // Smear state: start from the first pixel in the row
        int smR = mRowBuf[0], smG = mRowBuf[1], smB = mRowBuf[2];

        for (int x = 0; x < W; x++) {
            lcg = lcg * 1664525u + 1013904223u;

            int sR, sG, sB;
            if (lcg <= smearThresh) {
                // hold: repeat last-read pixel (horizontal smear)
                sR = smR; sG = smG; sB = smB;
            } else {
                // fresh read from source
                sR = mRowBuf[x * 3 + 0];
                sG = mRowBuf[x * 3 + 1];
                sB = mRowBuf[x * 3 + 2];
                smR = sR; smG = sG; smB = sB;
            }

            row[x * 3 + 0] = (BYTE)sR;
            row[x * 3 + 1] = (BYTE)sG;
            row[x * 3 + 2] = (BYTE)sB;
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD VHSClogPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_CLOG:      val = mClogNorm;      break;
        case PARAM_THICKNESS: val = mThicknessNorm; break;
        case PARAM_SMEAR:     val = mSmearNorm;     break;
        case PARAM_FLUTTER:   val = mFlutterNorm;   break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD VHSClogPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_CLOG:      mClogNorm      = val; return FF_SUCCESS;
        case PARAM_THICKNESS: mThicknessNorm = val; return FF_SUCCESS;
        case PARAM_SMEAR:     mSmearNorm     = val; return FF_SUCCESS;
        case PARAM_FLUTTER:   mFlutterNorm   = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* VHSClogPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_CLOG:
            sprintf(mDisplayBuf, "%d band(s)",
                (mClogNorm < 0.01f) ? 0 : (int)(mClogNorm * MAX_BANDS + 0.5f));
            return mDisplayBuf;
        case PARAM_THICKNESS: {
            int tp = 1 + (int)(mThicknessNorm * (float)(MAX_THICK_PX - 1) + 0.5f);
            sprintf(mDisplayBuf, "%d px", tp);
            return mDisplayBuf;
        }
        case PARAM_SMEAR:
            sprintf(mDisplayBuf, "%d%%", (int)(mSmearNorm * 100.0f + 0.5f));
            return mDisplayBuf;
        case PARAM_FLUTTER: {
            unsigned period;
            if (mFlutterNorm < 0.005f) {
                period = 10000u;
            } else {
                float f = mFlutterNorm;
                period = (unsigned)(1.0f / (f * f));
                if (period < 1u) period = 1u;
            }
            if (period >= 10000u)
                sprintf(mDisplayBuf, "static");
            else
                sprintf(mDisplayBuf, "%uf/pos", period);
            return mDisplayBuf;
        }
        default:
            return NULL;
    }
}

// ============================================================
//  Factory method
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new VHSClogPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

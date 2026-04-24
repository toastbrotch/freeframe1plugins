// ============================================================
//  ColorReduce.cpp
//  FreeFrame 1.0 plugin — dominant N-colour reduction
//
//  Finds the N most dominant colours in the frame and maps
//  every pixel to its nearest dominant colour.
//
//  Algorithm:
//    1. Quantise frame to a coarse palette (6x6x6 = 216 bins)
//    2. Count pixels per bin
//    3. Pick top N bins by pixel count
//    4. Remap each pixel to nearest of those N colours
//
//  Parameter "Colors": 0.0–1.0 → 2–64 dominant colours
//  (capped at 64 — beyond that the effect is invisible)
// ============================================================

#include "ColorReduce.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// Coarse quantisation for histogram: 6 levels per channel = 216 bins
#define HIST_LEVELS 6
#define HIST_BINS   (HIST_LEVELS * HIST_LEVELS * HIST_LEVELS)

// ============================================================
//  Constructor
// ============================================================
ColorReducePlugin::ColorReducePlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_COLORS,
                 "Colors",
                 FF_TYPE_STANDARD,
                 DEFAULT_COLORS_NORM);

    SetParamInfo(PARAM_PALROT,
                 "Pal.Rotate",
                 FF_TYPE_STANDARD,
                 DEFAULT_PALROT_NORM);

    mColorsNorm    = DEFAULT_COLORS_NORM;
    mPalRotNorm    = DEFAULT_PALROT_NORM;
    mDisplayBuf[0] = '\0';
}

ColorReducePlugin::~ColorReducePlugin() {}

// ============================================================
//  Helpers
// ============================================================

// Quantise a channel value 0-255 to a histogram level 0-(HIST_LEVELS-1)
static inline int quantCh(int v) {
    int q = v * HIST_LEVELS / 256;
    if (q >= HIST_LEVELS) q = HIST_LEVELS - 1;
    return q;
}

// Centre value of a histogram level (0-255)
static inline BYTE levelCenter(int q) {
    // centre of the bin in 0-255 space
    int c = (q * 256 + 128) / HIST_LEVELS;
    if (c > 255) c = 255;
    return (BYTE)c;
}

// Squared colour distance (no sqrt needed for comparisons)
static inline int colorDistSq(int r1, int g1, int b1,
                                int r2, int g2, int b2) {
    int dr = r1 - r2, dg = g1 - g2, db = b1 - b2;
    return dr*dr + dg*dg + db*db;
}

// Simple insertion sort on count array (descending) — N is small
static void sortTopN(int* counts, BYTE* rs, BYTE* gs, BYTE* bs,
                     int total, int N)
{
    // Partial selection sort: find top N entries
    for (int i = 0; i < N && i < total; i++) {
        int best = i;
        for (int j = i + 1; j < total; j++)
            if (counts[j] > counts[best]) best = j;
        // swap i and best
        int tc = counts[i]; counts[i] = counts[best]; counts[best] = tc;
        BYTE tr;
        tr = rs[i]; rs[i] = rs[best]; rs[best] = tr;
        tr = gs[i]; gs[i] = gs[best]; gs[best] = tr;
        tr = bs[i]; bs[i] = bs[best]; bs[best] = tr;
    }
}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD ColorReducePlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int N = normToColors(mColorsNorm);
    if (N >= MAX_COLORS) return FF_SUCCESS;  // passthrough

    DWORD pixels = (DWORD)GetFrameWidth() * (DWORD)GetFrameHeight();
    BYTE* p      = (BYTE*)pFrame;

    // ----------------------------------------------------------
    //  Step 1: Build histogram over coarse 6x6x6 RGB grid
    // ----------------------------------------------------------
    int counts[HIST_BINS];
    memset(counts, 0, sizeof(counts));

    BYTE* src = p;
    for (DWORD i = 0; i < pixels; i++) {
        int qr = quantCh(src[0]);
        int qg = quantCh(src[1]);
        int qb = quantCh(src[2]);
        counts[qr * HIST_LEVELS * HIST_LEVELS + qg * HIST_LEVELS + qb]++;
        src += 3;
    }

    // ----------------------------------------------------------
    //  Step 2: Build palette arrays from non-empty bins
    // ----------------------------------------------------------
    // At most HIST_BINS non-empty bins
    int   palCounts[HIST_BINS];
    BYTE  palR[HIST_BINS], palG[HIST_BINS], palB[HIST_BINS];
    int   palSize = 0;

    for (int qr = 0; qr < HIST_LEVELS; qr++) {
        for (int qg = 0; qg < HIST_LEVELS; qg++) {
            for (int qb = 0; qb < HIST_LEVELS; qb++) {
                int idx = qr * HIST_LEVELS * HIST_LEVELS + qg * HIST_LEVELS + qb;
                if (counts[idx] > 0) {
                    palCounts[palSize] = counts[idx];
                    palR[palSize]      = levelCenter(qr);
                    palG[palSize]      = levelCenter(qg);
                    palB[palSize]      = levelCenter(qb);
                    palSize++;
                }
            }
        }
    }

    // ----------------------------------------------------------
    //  Step 3: Sort to get top N dominant colours
    // ----------------------------------------------------------
    int useN = N < palSize ? N : palSize;
    sortTopN(palCounts, palR, palG, palB, palSize, useN);

    // ----------------------------------------------------------
    //  Step 4: Remap every pixel to nearest dominant colour,
    //          then apply palette rotation (circular index shift).
    //          shift=0 → no change; shift=k → pixel that would
    //          get palette[j] gets palette[(j+k) % useN] instead.
    // ----------------------------------------------------------
    int shift = (int)(mPalRotNorm * (float)useN) % useN;

    src = p;
    for (DWORD i = 0; i < pixels; i++) {
        int r = src[0], g = src[1], b = src[2];

        int bestDist = 0x7fffffff;
        int bestIdx  = 0;
        for (int j = 0; j < useN; j++) {
            int d = colorDistSq(r, g, b, palR[j], palG[j], palB[j]);
            if (d < bestDist) { bestDist = d; bestIdx = j; }
        }

        int rotIdx = (bestIdx + shift) % useN;
        src[0] = palR[rotIdx];
        src[1] = palG[rotIdx];
        src[2] = palB[rotIdx];
        src += 3;
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD ColorReducePlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
    case PARAM_COLORS: val = mColorsNorm; break;
    case PARAM_PALROT: val = mPalRotNorm; break;
    default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD ColorReducePlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
    case PARAM_COLORS: mColorsNorm = val; return FF_SUCCESS;
    case PARAM_PALROT: mPalRotNorm = val; return FF_SUCCESS;
    default: return FF_FAIL;
    }
}

char* ColorReducePlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
    case PARAM_COLORS: {
        int N = normToColors(mColorsNorm);
        if (N >= MAX_COLORS)
            sprintf(mDisplayBuf, "full");
        else
            sprintf(mDisplayBuf, "%d colors", N);
        return mDisplayBuf;
    }
    case PARAM_PALROT: {
        int N = normToColors(mColorsNorm);
        int shift = (int)(mPalRotNorm * (float)N) % N;
        sprintf(mDisplayBuf, "+%d/%d", shift, N);
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
    *ppInstance = new ColorReducePlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

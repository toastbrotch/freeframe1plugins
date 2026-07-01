// ============================================================
//  VHSGlitch.cpp
//  FreeFrame 1.0 plugin — VHS tape artefact simulation
//
//  Four stacking artefacts, all computed in one pass per row:
//
//  Shift    — horizontal scan-line jitter. Most rows drift by a
//             small amount; 1 in 16 rows snaps to a random large
//             offset for a "tape snap" look.
//
//  Chroma   — RGB channel separation. R is read from x−chromaPx,
//             B from x+chromaPx; G stays at x. Mimics VHS chroma
//             delay and the colour fringing it leaves on edges.
//
//  Noise    — static bands. A random fraction of rows (up to 45%)
//             are blended with LCG noise. Intensity and coverage
//             both scale with the slider.
//
//  Tracking — tracking-error blocks. 1–4 horizontal slabs are
//             each displaced sideways by a large random amount
//             every frame, simulating a slipping VHS head.
//
//  Shift and Tracking displacements use modulo wrapping so the
//  image tiles cleanly at the edges. Chroma uses the same wrap.
//  All randomness is deterministic per frame+row so there is no
//  external RNG state — the effect is reproducible at any slider.
// ============================================================

#include "VHSGlitch.h"
#include <cstdio>
#include <cstring>

// ============================================================
//  Deterministic integer hash — stable per (frameSeed, row)
// ============================================================
unsigned VHSGlitchPlugin::hashPair(unsigned a, unsigned b)
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
VHSGlitchPlugin::VHSGlitchPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_SHIFT,    "Shift",    FF_TYPE_STANDARD, DEFAULT_SHIFT);
    SetParamInfo(PARAM_CHROMA,   "Chroma",   FF_TYPE_STANDARD, DEFAULT_CHROMA);
    SetParamInfo(PARAM_NOISE,    "Noise",    FF_TYPE_STANDARD, DEFAULT_NOISE);
    SetParamInfo(PARAM_TRACKING, "Tracking", FF_TYPE_STANDARD, DEFAULT_TRACKING);

    mShiftNorm    = DEFAULT_SHIFT;
    mChromaNorm   = DEFAULT_CHROMA;
    mNoiseNorm    = DEFAULT_NOISE;
    mTrackingNorm = DEFAULT_TRACKING;
    mFrameCount   = 0;
    mDisplayBuf[0] = '\0';
    memset(mRowBuf, 0, sizeof(mRowBuf));
}

VHSGlitchPlugin::~VHSGlitchPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD VHSGlitchPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int   W  = (int)GetFrameWidth();
    int   H  = (int)GetFrameHeight();
    BYTE* fb = (BYTE*)pFrame;

    if (W > ROW_BUF_WIDTH) return FF_SUCCESS; // frame too wide: pass through

    const int rowBytes = W * 3;

    // --- Derived parameters ---
    const int   maxShiftPx = (int)(mShiftNorm    * (float)MAX_SHIFT_PX  + 0.5f);
    const int   chromaPx   = (int)(mChromaNorm   * (float)MAX_CHROMA_PX + 0.5f);
    const float noiseProb  = mNoiseNorm * 0.45f;
    const float noiseAmt   = mNoiseNorm;
    int         numBlocks  = (mTrackingNorm < 0.01f) ? 0 :
                             (int)(mTrackingNorm * (float)MAX_TRACKING_BLOCKS + 0.5f);
    if (numBlocks > MAX_TRACKING_BLOCKS) numBlocks = MAX_TRACKING_BLOCKS;

    // Per-frame seed — changes every frame, stable within a frame
    const unsigned fSeed = mFrameCount * 2654435761u;
    mFrameCount++;

    // --- Pre-generate tracking error blocks ---
    TrackingBlock blocks[MAX_TRACKING_BLOCKS];
    for (int b = 0; b < numBlocks; b++) {
        unsigned r0 = hashPair(fSeed, (unsigned)(b * 4 + 0));
        unsigned r1 = hashPair(fSeed, (unsigned)(b * 4 + 1));
        unsigned r2 = hashPair(fSeed, (unsigned)(b * 4 + 2));
        unsigned r3 = hashPair(fSeed, (unsigned)(b * 4 + 3));
        int blockH  = 8  + (int)(r0 % 60u);
        int y0      = (int)(r1 % (unsigned)H);
        int dxMag   = 20 + (int)(r2 % (unsigned)(W / 2));
        blocks[b].y0 = y0;
        blocks[b].y1 = y0 + blockH;
        if (blocks[b].y1 >= H) blocks[b].y1 = H - 1;
        blocks[b].dx = (r3 & 1u) ? dxMag : -dxMag;
    }

    // --- Row loop ---
    for (int y = 0; y < H; y++) {
        BYTE* srcRow = fb + y * rowBytes;

        // Copy original row — we read from here, write back to srcRow
        memcpy(mRowBuf, srcRow, rowBytes);

        // 1. Scan-line jitter
        int rowShift = 0;
        if (maxShiftPx > 0) {
            unsigned rh = hashPair(fSeed, (unsigned)y + 0x8000u);
            float jitter = ((float)(rh & 0xFFFFu) / 65535.0f) * 2.0f - 1.0f;
            if ((rh >> 28) > 0u) jitter *= 0.12f;  // 15/16 rows: subtle drift only
            rowShift = (int)(jitter * (float)maxShiftPx);
        }

        // 2. Tracking block displacement (first matching block wins)
        for (int b = 0; b < numBlocks; b++) {
            if (y >= blocks[b].y0 && y <= blocks[b].y1) {
                rowShift += blocks[b].dx;
                break;
            }
        }

        // 3. Noise — decide per row whether it's a static band
        bool     noisy    = false;
        float    rowNoise = 0.0f;
        unsigned pixRand  = 0u;
        if (noiseProb > 0.0f) {
            unsigned rn = hashPair(fSeed + 1u, (unsigned)y);
            if ((float)(rn & 0xFFFFu) / 65535.0f < noiseProb) {
                noisy    = true;
                rowNoise = noiseAmt;
                pixRand  = hashPair(fSeed + 2u, (unsigned)y);
            }
        }

        // 4. Pixel loop — shift + chroma separation + noise, all in one pass
        for (int x = 0; x < W; x++) {
            // Each channel reads from a different horizontal offset (modulo wrap)
            int xR = ((x + rowShift - chromaPx) % W + W) % W;
            int xG = ((x + rowShift           ) % W + W) % W;
            int xB = ((x + rowShift + chromaPx) % W + W) % W;

            int R = mRowBuf[xR * 3 + 0];
            int G = mRowBuf[xG * 3 + 1];
            int B = mRowBuf[xB * 3 + 2];

            if (noisy) {
                pixRand = pixRand * 1664525u + 1013904223u;
                int nR = (int)(pixRand        & 0xFFu);
                int nG = (int)((pixRand >>  8) & 0xFFu);
                int nB = (int)((pixRand >> 16) & 0xFFu);
                R = R + (int)(rowNoise * (float)(nR - R));
                G = G + (int)(rowNoise * (float)(nG - G));
                B = B + (int)(rowNoise * (float)(nB - B));
            }

            srcRow[x * 3 + 0] = (BYTE)R;
            srcRow[x * 3 + 1] = (BYTE)G;
            srcRow[x * 3 + 2] = (BYTE)B;
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD VHSGlitchPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_SHIFT:    val = mShiftNorm;    break;
        case PARAM_CHROMA:   val = mChromaNorm;   break;
        case PARAM_NOISE:    val = mNoiseNorm;    break;
        case PARAM_TRACKING: val = mTrackingNorm; break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD VHSGlitchPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_SHIFT:    mShiftNorm    = val; return FF_SUCCESS;
        case PARAM_CHROMA:   mChromaNorm   = val; return FF_SUCCESS;
        case PARAM_NOISE:    mNoiseNorm    = val; return FF_SUCCESS;
        case PARAM_TRACKING: mTrackingNorm = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* VHSGlitchPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_SHIFT:
            sprintf(mDisplayBuf, "+/-%d px", (int)(mShiftNorm * MAX_SHIFT_PX + 0.5f));
            return mDisplayBuf;
        case PARAM_CHROMA:
            sprintf(mDisplayBuf, "+/-%d px", (int)(mChromaNorm * MAX_CHROMA_PX + 0.5f));
            return mDisplayBuf;
        case PARAM_NOISE:
            sprintf(mDisplayBuf, "%d%%", (int)(mNoiseNorm * 100.0f + 0.5f));
            return mDisplayBuf;
        case PARAM_TRACKING:
            sprintf(mDisplayBuf, "%d block(s)", (mTrackingNorm < 0.01f) ? 0 :
                    (int)(mTrackingNorm * MAX_TRACKING_BLOCKS + 0.5f));
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
    *ppInstance = new VHSGlitchPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

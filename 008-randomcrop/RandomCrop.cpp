// ============================================================
//  RandomCrop.cpp
//  FreeFrame 1.0 plugin — animated angled black crop wipe
//
//  A black region sweeps across the frame from one side, covers
//  up to MaxCrop of the image, holds, then retreats. Each cycle
//  picks a random direction (left→right or right→left) and a
//  random tilt of ±30° so the crop boundary is never the same
//  diagonal twice. All four effects stack in one pixel pass.
//
//  Geometry:
//    A pixel at (x, y) is blacked out when its projection onto
//    the sweep direction vector (cosA, sinA) falls within the
//    current crop window. The window grows from the "near" side
//    corner of the frame inward. Softness creates a linear
//    fade zone at the advancing edge.
//
//  Parameters:
//    Speed    : 0→1 → build rate 0.005–0.15 /frame (and idle freq)
//    MaxCrop  : 0→1 → 0–100% of the diagonal at peak
//    Hold     : 0→1 → 0–150 frames at maximum before reversing
//    Softness : 0→1 → hard pixel edge → 25%-of-range gradient blade
// ============================================================

#include "RandomCrop.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ============================================================
//  LCG random
// ============================================================
unsigned RandomCropPlugin::lcg() {
    mRandState = mRandState * 1664525u + 1013904223u;
    return mRandState;
}

float RandomCropPlugin::randFloat() {
    return (float)(lcg() & 0x7FFFFFFFu) * (1.0f / 2147483648.0f);
}

// ============================================================
//  Begin a new crop cycle: randomise direction and tilt
// ============================================================
void RandomCropPlugin::beginCycle() {
    mCropPos = 0.0f;
    float tilt      = (randFloat() - 0.5f) * 2.0f * TILT_DEG * (3.14159265f / 180.0f);
    float baseAngle = (lcg() & 1u) ? 3.14159265f : 0.0f;  // left or right sweep
    mAngle = baseAngle + tilt;
}

// ============================================================
//  Constructor
// ============================================================
RandomCropPlugin::RandomCropPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_SPEED,    "Speed",    FF_TYPE_STANDARD, DEFAULT_SPEED);
    SetParamInfo(PARAM_MAXCROP,  "MaxCrop",  FF_TYPE_STANDARD, DEFAULT_MAXCROP);
    SetParamInfo(PARAM_HOLD,     "Hold",     FF_TYPE_STANDARD, DEFAULT_HOLD);
    SetParamInfo(PARAM_SOFTNESS, "Softness", FF_TYPE_STANDARD, DEFAULT_SOFTNESS);

    mSpeedNorm    = DEFAULT_SPEED;
    mMaxCropNorm  = DEFAULT_MAXCROP;
    mHoldNorm     = DEFAULT_HOLD;
    mSoftnessNorm = DEFAULT_SOFTNESS;

    mPhase     = PH_IDLE;
    mCropPos   = 0.0f;
    mAngle     = 0.0f;
    mHoldTimer = 0;
    mIdleTimer = 20;    // short initial delay before first crop
    mRandState = 0xBEEFCAFEu;
    mDisplayBuf[0] = '\0';
}

RandomCropPlugin::~RandomCropPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD RandomCropPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int   W  = (int)GetFrameWidth();
    int   H  = (int)GetFrameHeight();
    BYTE* fb = (BYTE*)pFrame;

    const float buildRate = 0.005f + mSpeedNorm * 0.145f;

    // --- State machine ---
    switch (mPhase) {
    case PH_IDLE:
        if (--mIdleTimer <= 0) {
            beginCycle();
            mPhase = PH_BUILD;
        }
        break;
    case PH_BUILD:
        mCropPos += buildRate;
        if (mCropPos >= 1.0f) {
            mCropPos = 1.0f;
            mHoldTimer = (int)(mHoldNorm * (float)MAX_HOLD_FRAMES);
            mPhase = PH_HOLD;
        }
        break;
    case PH_HOLD:
        if (--mHoldTimer <= 0) mPhase = PH_RELEASE;
        break;
    case PH_RELEASE:
        mCropPos -= buildRate;
        if (mCropPos <= 0.0f) {
            mCropPos = 0.0f;
            // Idle longer at low speed (infrequent), shorter at high speed (frequent)
            mIdleTimer = 10 + (int)((1.0f - mSpeedNorm) * 90.0f)
                            + (int)(randFloat() * 50.0f);
            mPhase = PH_IDLE;
        }
        break;
    }

    if (mPhase == PH_IDLE || mCropPos <= 0.0f) return FF_SUCCESS;

    // --- Crop geometry ---
    // Project each pixel onto the sweep direction (cosA, sinA).
    // The four corners give the full projection range.
    const float cosA = cosf(mAngle);
    const float sinA = sinf(mAngle);

    float p00 = 0.0f;
    float p10 = cosA * (float)(W - 1);
    float p01 = sinA * (float)(H - 1);
    float p11 = p10 + p01;

    float projMin = p00, projMax = p00;
    if (p10 < projMin) projMin = p10;
    if (p10 > projMax) projMax = p10;
    if (p01 < projMin) projMin = p01;
    if (p01 > projMax) projMax = p01;
    if (p11 < projMin) projMin = p11;
    if (p11 > projMax) projMax = p11;

    const float cropRange = projMax - projMin;
    if (cropRange < 1.0f) return FF_SUCCESS;

    // threshold: projection value at the crop boundary
    const float threshold     = projMin + mCropPos * mMaxCropNorm * cropRange;
    const float softZoneWidth = mSoftnessNorm * 0.25f * cropRange;
    const bool  doSoft        = softZoneWidth > 0.5f;
    const float hardThreshold = doSoft ? threshold - softZoneWidth : threshold;

    // --- Pixel loop ---
    BYTE* p = fb;
    for (int y = 0; y < H; y++) {
        const float yBase = sinA * (float)y;
        for (int x = 0; x < W; x++, p += 3) {
            const float proj = cosA * (float)x + yBase;

            if (proj <= hardThreshold) {
                p[0] = p[1] = p[2] = 0;
            } else if (doSoft && proj < threshold) {
                // Linear fade: fully black at hardThreshold, original at threshold
                const float keep = (proj - hardThreshold) / softZoneWidth;
                p[0] = (BYTE)((float)p[0] * keep);
                p[1] = (BYTE)((float)p[1] * keep);
                p[2] = (BYTE)((float)p[2] * keep);
            }
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD RandomCropPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_SPEED:    val = mSpeedNorm;    break;
        case PARAM_MAXCROP:  val = mMaxCropNorm;  break;
        case PARAM_HOLD:     val = mHoldNorm;     break;
        case PARAM_SOFTNESS: val = mSoftnessNorm; break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD RandomCropPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_SPEED:    mSpeedNorm    = val; return FF_SUCCESS;
        case PARAM_MAXCROP:  mMaxCropNorm  = val; return FF_SUCCESS;
        case PARAM_HOLD:     mHoldNorm     = val; return FF_SUCCESS;
        case PARAM_SOFTNESS: mSoftnessNorm = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* RandomCropPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_SPEED:
            sprintf(mDisplayBuf, "%.3f /fr", 0.005f + mSpeedNorm * 0.145f);
            return mDisplayBuf;
        case PARAM_MAXCROP:
            sprintf(mDisplayBuf, "%d%%", (int)(mMaxCropNorm * 100.0f + 0.5f));
            return mDisplayBuf;
        case PARAM_HOLD:
            sprintf(mDisplayBuf, "%d fr", (int)(mHoldNorm * (float)MAX_HOLD_FRAMES));
            return mDisplayBuf;
        case PARAM_SOFTNESS:
            sprintf(mDisplayBuf, "%d%%", (int)(mSoftnessNorm * 100.0f + 0.5f));
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
    *ppInstance = new RandomCropPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

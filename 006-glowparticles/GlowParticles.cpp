// ============================================================
//  GlowParticles.cpp
//  FreeFrame 1.0 plugin — particle emitter from bright pixels
//
//  Bright pixels in the input frame act as particle emitters.
//  Each particle gets a random 2-D direction and constant velocity.
//  Particles are rendered as flat filled circles that fade linearly
//  over their lifetime, additively blended onto the frame.
//
//  Parameters:
//    Threshold : 0.0–1.0  → luminance threshold for emitter pixels
//    Speed     : 0.0–1.0  → 0.5–10.0 px/frame
//    MaxAge    : 0.0–1.0  → 10–150 frames particle lifetime
//    Hue       : 0.0–1.0  → full colour wheel (full saturation)
// ============================================================

#include "GlowParticles.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ============================================================
//  LCG random number generator
// ============================================================
unsigned GlowParticlesPlugin::lcg() {
    mRandState = mRandState * 1664525u + 1013904223u;
    return mRandState;
}

float GlowParticlesPlugin::randFloat() {
    return (float)(lcg() & 0x7FFFFFFFu) * (1.0f / 2147483648.0f);
}

// ============================================================
//  HSV → RGB (all values 0–1; outputs clamped byte values)
// ============================================================
static void hsvToRgb(float h, float s, float v, int& r, int& g, int& b)
{
    float R, G, B;
    if (s <= 0.0f) {
        R = G = B = v;
    } else {
        float H = h * 6.0f;
        if (H >= 6.0f) H = 0.0f;
        int   i = (int)H;
        float f = H - (float)i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        switch (i) {
            case 0: R = v; G = t; B = p; break;
            case 1: R = q; G = v; B = p; break;
            case 2: R = p; G = v; B = t; break;
            case 3: R = p; G = q; B = v; break;
            case 4: R = t; G = p; B = v; break;
            default: R = v; G = p; B = q; break;
        }
    }
    r = (int)(R * 255.0f + 0.5f); if (r > 255) r = 255;
    g = (int)(G * 255.0f + 0.5f); if (g > 255) g = 255;
    b = (int)(B * 255.0f + 0.5f); if (b > 255) b = 255;
}

// ============================================================
//  Constructor
// ============================================================
GlowParticlesPlugin::GlowParticlesPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_THRESHOLD, "Threshold", FF_TYPE_STANDARD, DEFAULT_THRESHOLD);
    SetParamInfo(PARAM_SPEED,     "Speed",     FF_TYPE_STANDARD, DEFAULT_SPEED);
    SetParamInfo(PARAM_MAXAGE,    "MaxAge",    FF_TYPE_STANDARD, DEFAULT_MAXAGE);
    SetParamInfo(PARAM_HUE,       "Hue",       FF_TYPE_STANDARD, DEFAULT_HUE);

    mThreshNorm    = DEFAULT_THRESHOLD;
    mSpeedNorm     = DEFAULT_SPEED;
    mMaxAgeNorm    = DEFAULT_MAXAGE;
    mHueNorm       = DEFAULT_HUE;
    mRandState     = 0xDEADBEEFu;
    mDisplayBuf[0] = '\0';

    for (int i = 0; i < MAX_PARTICLES; i++)
        mParticles[i].alive = false;
}

GlowParticlesPlugin::~GlowParticlesPlugin() {}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD GlowParticlesPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int   W  = (int)GetFrameWidth();
    int   H  = (int)GetFrameHeight();
    BYTE* fb = (BYTE*)pFrame;

    int   threshByte = (int)(mThreshNorm * 255.0f + 0.5f);
    float speed      = normToSpeed(mSpeedNorm);
    int   maxAge     = normToMaxAge(mMaxAgeNorm);

    // --- 1. Sample random pixels; spawn particles from bright ones ---
    int spawned = 0;
    for (int s = 0; s < SAMPLE_COUNT && spawned < SPAWN_PER_FRAME; s++) {
        int px = (int)(lcg() % (unsigned)W);
        int py = (int)(lcg() % (unsigned)H);
        BYTE* src = fb + (py * W + px) * 3;
        int lum = ((int)src[0] + (int)src[1] + (int)src[2]) / 3;
        if (lum < threshByte) continue;

        // Find a free slot
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!mParticles[i].alive) {
                float angle          = randFloat() * 6.28318530f;
                mParticles[i].x      = (float)px;
                mParticles[i].y      = (float)py;
                mParticles[i].vx     = cosf(angle) * speed;
                mParticles[i].vy     = sinf(angle) * speed;
                mParticles[i].age    = 0;
                mParticles[i].maxAge = maxAge;
                mParticles[i].alive  = true;
                spawned++;
                break;
            }
        }
    }

    // --- 2. Update positions; render glow; kill expired particles ---
    int cr, cg, cb;
    hsvToRgb(mHueNorm, 1.0f, 1.0f, cr, cg, cb);

    const float R    = (float)GLOW_RADIUS;
    const float R2   = R * R;
    const float fcr  = (float)cr;
    const float fcg  = (float)cg;
    const float fcb  = (float)cb;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!mParticles[i].alive) continue;

        Particle& p = mParticles[i];
        p.x += p.vx;
        p.y += p.vy;
        p.age++;

        if (p.age >= p.maxAge ||
            p.x < 0.0f || p.x >= (float)W ||
            p.y < 0.0f || p.y >= (float)H)
        {
            p.alive = false;
            continue;
        }

        // Brightness fades linearly from 1.0 at birth to 0.0 at death
        float brightness = 1.0f - (float)p.age / (float)p.maxAge;

        int cx = (int)p.x;
        int cy = (int)p.y;
        int Ri = GLOW_RADIUS;
        int y0 = cy - Ri; if (y0 < 0)  y0 = 0;
        int y1 = cy + Ri; if (y1 >= H) y1 = H - 1;
        int x0 = cx - Ri; if (x0 < 0)  x0 = 0;
        int x1 = cx + Ri; if (x1 >= W) x1 = W - 1;

        for (int gy = y0; gy <= y1; gy++) {
            float dy  = (float)(gy - cy);
            float dy2 = dy * dy;
            BYTE* row = fb + gy * W * 3;
            for (int gx = x0; gx <= x1; gx++) {
                float dx = (float)(gx - cx);
                float d2 = dx * dx + dy2;
                if (d2 > R2) continue;

                // Flat fill × age fade → additive blend
                float alpha = brightness;
                BYTE* pix   = row + gx * 3;
                int   nr    = (int)pix[0] + (int)(alpha * fcr);
                int   ng    = (int)pix[1] + (int)(alpha * fcg);
                int   nb    = (int)pix[2] + (int)(alpha * fcb);
                pix[0] = (BYTE)(nr > 255 ? 255 : nr);
                pix[1] = (BYTE)(ng > 255 ? 255 : ng);
                pix[2] = (BYTE)(nb > 255 ? 255 : nb);
            }
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD GlowParticlesPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_THRESHOLD: val = mThreshNorm; break;
        case PARAM_SPEED:     val = mSpeedNorm;  break;
        case PARAM_MAXAGE:    val = mMaxAgeNorm; break;
        case PARAM_HUE:       val = mHueNorm;    break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD GlowParticlesPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_THRESHOLD: mThreshNorm = val; return FF_SUCCESS;
        case PARAM_SPEED:     mSpeedNorm  = val; return FF_SUCCESS;
        case PARAM_MAXAGE:    mMaxAgeNorm = val; return FF_SUCCESS;
        case PARAM_HUE:       mHueNorm    = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* GlowParticlesPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_THRESHOLD:
            sprintf(mDisplayBuf, "%d%%", (int)(mThreshNorm * 100.0f + 0.5f));
            return mDisplayBuf;
        case PARAM_SPEED:
            sprintf(mDisplayBuf, "%.1f px/fr", normToSpeed(mSpeedNorm));
            return mDisplayBuf;
        case PARAM_MAXAGE:
            sprintf(mDisplayBuf, "%d fr", normToMaxAge(mMaxAgeNorm));
            return mDisplayBuf;
        case PARAM_HUE:
            sprintf(mDisplayBuf, "%.0f deg", mHueNorm * 360.0f);
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
    *ppInstance = new GlowParticlesPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

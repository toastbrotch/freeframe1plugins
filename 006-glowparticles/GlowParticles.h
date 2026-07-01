#ifndef GLOWPARTICLES_H
#define GLOWPARTICLES_H

#include "FFPluginSDK.h"
#include <cmath>

#define PARAM_THRESHOLD  0
#define PARAM_SPEED      1
#define PARAM_MAXAGE     2
#define PARAM_HUE        3

#define DEFAULT_THRESHOLD  0.5f
#define DEFAULT_SPEED      0.25f
#define DEFAULT_MAXAGE     0.4f
#define DEFAULT_HUE        0.6f

#define MAX_PARTICLES    4000
#define SAMPLE_COUNT      512
#define SPAWN_PER_FRAME    30
#define GLOW_RADIUS         8

// Speed: slider 0.0 → 0.5 px/fr, slider 1.0 → 10.0 px/fr
#define MIN_SPEED_PX    0.5f
#define MAX_SPEED_PX   10.0f

// MaxAge: slider 0.0 → 10 fr, slider 1.0 → 150 fr
#define MIN_AGE_FRAMES  10
#define MAX_AGE_FRAMES 150

struct Particle {
    float x, y;
    float vx, vy;
    int   age;
    int   maxAge;
    bool  alive;
};

inline float normToSpeed(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return MIN_SPEED_PX + n * (MAX_SPEED_PX - MIN_SPEED_PX);
}

inline int normToMaxAge(float n) {
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return MIN_AGE_FRAMES + (int)(n * (float)(MAX_AGE_FRAMES - MIN_AGE_FRAMES) + 0.5f);
}

class GlowParticlesPlugin : public CFreeFramePlugin
{
public:
     GlowParticlesPlugin();
    ~GlowParticlesPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float    mThreshNorm;
    float    mSpeedNorm;
    float    mMaxAgeNorm;
    float    mHueNorm;

    Particle mParticles[MAX_PARTICLES];
    unsigned mRandState;
    char     mDisplayBuf[32];

    unsigned lcg();
    float    randFloat();
};

#endif // GLOWPARTICLES_H

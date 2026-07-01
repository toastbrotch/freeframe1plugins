// ============================================================
//  FFPluginInfoData.cpp — GlowParticles plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "GLWP",                 // 4-char unique ID
    "GlowParticles",        // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Bright pixels act as emitters spawning coloured glow particles "
    "that fly outward in random directions and fade over their lifetime.",
    "GlowParticles v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

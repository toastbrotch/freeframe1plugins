// ============================================================
//  FFPluginInfoData.cpp — VHSGlitch plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "VHSG",                 // 4-char unique ID
    "VHSGlitch",            // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Stacks four VHS tape artefacts: scan-line jitter, RGB chroma "
    "separation, noise static bands, and tracking-error block displacement.",
    "VHSGlitch v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

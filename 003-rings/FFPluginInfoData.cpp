// ============================================================
//  FFPluginInfoData.cpp — Rings plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "RNGX",                 // 4-char unique ID
    "Rings",                // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Draws concentric inverting rings centred in the frame. "
    "Pixels inside a ring are colour-inverted; gaps are unchanged.",
    "Rings v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

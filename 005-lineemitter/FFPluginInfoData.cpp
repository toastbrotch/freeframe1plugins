// ============================================================
//  FFPluginInfoData.cpp — LineEmitter plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "LMTR",                 // 4-char unique ID
    "LineEmitter",          // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Full-width inverting lines emitting from the bottom with "
    "optional perspective scaling — lines shrink or grow toward the top.",
    "LineEmitter v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);
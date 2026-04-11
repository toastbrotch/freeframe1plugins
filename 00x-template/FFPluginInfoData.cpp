// ============================================================
//  FFPluginInfoData.cpp — ColorReduce plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "CRDC",                 // 4-char unique ID
    "ColorReduce",          // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Reduces the number of colours in the frame by quantising "
    "each RGB channel to N evenly spaced levels (posterisation).",
    "ColorReduce v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

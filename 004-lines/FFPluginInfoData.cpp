// ============================================================
//  FFPluginInfoData.cpp — Lines plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "LNSX",                 // 4-char unique ID
    "Lines",                // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Draws full-width inverting lines across the frame at a given angle. "
    "Pixels inside a line are colour-inverted; gaps are unchanged.",
    "Lines v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);
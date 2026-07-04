// ============================================================
//  FFPluginInfoData.cpp — VHSClog plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "VCOG",                 // 4-char unique ID
    "VHSClog",              // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Simulates VHS tape-head clogging: horizontal signal-loss bands with "
    "luma dropout, pixel smear, and temporal flutter.",
    "VHSClog v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

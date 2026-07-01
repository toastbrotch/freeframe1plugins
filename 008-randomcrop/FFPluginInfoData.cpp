// ============================================================
//  FFPluginInfoData.cpp — RandomCrop plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "RCFX",                 // 4-char unique ID
    "RandomCrop",           // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Sweeps an angled black band across the frame from a random side, "
    "holds at maximum crop, then retreats. Angle and direction randomise each cycle.",
    "RandomCrop v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

// ============================================================
//  FFPluginInfoData.cpp — NDIOut plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "NDIO",                 // 4-char unique ID
    "NDIOut",               // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Sends the current clip as an NDI source named 'Resolume FreeFrame'. "
    "Requires the NDI 6 runtime to be installed. Clip passes through unchanged.",
    "NDIOut v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

// ============================================================
//  FFPluginInfoData.cpp — NDIIn plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "NDII",                 // 4-char unique ID
    "NDIIn",                // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_SOURCE,
    "Receives an NDI stream and outputs it as a Resolume video source. "
    "Use the Source slider to select among discovered NDI sources on the network. "
    "Requires the NDI 6 runtime to be installed.",
    "NDIIn v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

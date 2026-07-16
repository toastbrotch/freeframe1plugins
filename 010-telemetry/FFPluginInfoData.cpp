// ============================================================
//  FFPluginInfoData.cpp — Telemetry plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "TELM",                 // 4-char unique ID
    "Telemetry",            // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_SOURCE,
    "Procedural cyberpunk telemetry: scrolling hex dumps, waveforms, "
    "bar graphs, line charts, status panels and scope traces. White on black.",
    "Telemetry v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

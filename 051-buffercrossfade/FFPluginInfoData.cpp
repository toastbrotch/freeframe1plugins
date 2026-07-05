// ============================================================
//  FFPluginInfoData.cpp — BufferCrossfade plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "BFCX",                 // 4-char unique ID
    "BufferCrossfade",      // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Crossfades between two of three shared buffers (A/B/C) "
    "written by BufferCapture (010). Empty buffers output black.",
    "BufferCrossfade v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);
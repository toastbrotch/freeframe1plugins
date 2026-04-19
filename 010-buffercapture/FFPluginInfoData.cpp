// ============================================================
//  FFPluginInfoData.cpp — BufferCapture plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "BFCP",                 // 4-char unique ID
    "BufferCapture",        // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,
    "Captures the input frame into shared buffer A, B or C. "
    "Returns black. Use with BufferCrossfade (011).",
    "BufferCapture v1.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);
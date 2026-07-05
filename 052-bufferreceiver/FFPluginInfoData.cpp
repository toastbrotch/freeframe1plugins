// ============================================================
//  FFPluginInfoData.cpp — BufferReceiver plugin identity
// ============================================================

#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "BFRX",                 // 4-char unique ID
    "BufferReceiver",       // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_SOURCE,
    "Outputs one of three shared buffers (A/B/C) written by BufferCapture (010) "
    "as a Resolume video source. Use the Source slider to select A, B, or C.",
    "BufferReceiver v2.0 - FreeFrame 1.0 plugin",
    0,
    NULL
);

// ============================================================
//  FFPluginInfoData.cpp
//  Plugin identity — the only file you customise per plugin.
//
//  The SDK's FreeFrame.cpp and FFPluginInfo.cpp both extern
//  "CFFPluginInfo* g_CurrPluginInfo". FFPluginInfo.cpp sets
//  this pointer to 'this' inside the CFFPluginInfo constructor.
//  So we define the pointer here (starts NULL) and constructing
//  g_PluginInfo below automatically fills it in.
// ============================================================

#include "FFPluginInfo.h"

// Forward declaration of the factory function in FrameDiff.cpp
DWORD __stdcall CreateInstance(void** ppInstance);

// ------------------------------------------------------------
//  The pointer the SDK looks for — set automatically by the
//  CFFPluginInfo constructor via "g_CurrPluginInfo = this"
// ------------------------------------------------------------
CFFPluginInfo* g_CurrPluginInfo = NULL;

// ------------------------------------------------------------
//  Constructing this global fills in g_CurrPluginInfo above.
// ------------------------------------------------------------
CFFPluginInfo g_PluginInfo(
    CreateInstance,         // factory function
    "FRDI",                 // 4-char unique ID  (NOT null-terminated)
    "FrameDiff",            // plugin name, up to 16 chars
    1,                      // FF API major version
    0,                      // FF API minor version
    1,                      // plugin major version
    0,                      // plugin minor version
    FF_EFFECT,              // FF_EFFECT or FF_SOURCE
    "Outputs the per-pixel absolute difference between "
    "the current frame and a frame N steps in the past. "
    "Black = no change. White = maximum change.",
    "FrameDiff v1.0 - FreeFrame 1.0 plugin",
    0,                      // extended data size (0 = not used)
    NULL                    // extended data block (NULL = not used)
);

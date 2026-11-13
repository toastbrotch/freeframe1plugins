#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "ISFX",
    "ISF Effect",
    1, 0,
    1, 0,
    FF_EFFECT,
    "Live ISF effect shaders: drop .fs files alongside the DLL; the panel "
    "picks one, the layer's video is the shader's image input, and P1-P4 "
    "can be bound to any of its inputs. Multi-pass/feedback shaders work.",
    "ISF Effect v1.0 - FreeFrame 1.0 effect plugin",
    0,
    NULL
);

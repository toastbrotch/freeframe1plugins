#include "FFPluginInfo.h"

DWORD __stdcall CreateInstance(void** ppInstance);

CFFPluginInfo* g_CurrPluginInfo = NULL;

CFFPluginInfo g_PluginInfo(
    CreateInstance,
    "ISFB",
    "ISF Browser",
    1, 0,
    1, 0,
    FF_SOURCE,
    "Live ISF shader browser: drop .fs files alongside the DLL, "
    "Shader slider picks which one, P1-P3 control the first 3 inputs.",
    "ISF Browser v1.0 - FreeFrame 1.0 source plugin",
    0,
    NULL
);

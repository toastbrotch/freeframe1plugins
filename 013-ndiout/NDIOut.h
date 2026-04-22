#ifndef NDIOUT_H
#define NDIOUT_H

#define NDILIB_CPP_DEFAULT_CONSTRUCTORS 0
#include "../NDI_6_SDK/Include/Processing.NDI.Lib.h"

#include "FFPluginSDK.h"

#define PARAM_ACTIVE  0
#define DEFAULT_ACTIVE_NORM  1.0f   // on by default

class NDIOutPlugin : public CFreeFramePlugin
{
public:
     NDIOutPlugin();
    ~NDIOutPlugin();

    DWORD ProcessFrame(void* pFrame);
    DWORD GetParameter(DWORD index);
    DWORD SetParameter(const SetParameterStruct* pParam);
    char* GetParameterDisplay(DWORD index);

private:
    char mDisplayBuf[16];

    static void convertBGRtoBGRA(const BYTE* src, BYTE* dst, DWORD numPixels);
};

#endif // NDIOUT_H
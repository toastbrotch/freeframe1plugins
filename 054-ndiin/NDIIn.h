#ifndef NDIIN_H
#define NDIIN_H

#define NDILIB_CPP_DEFAULT_CONSTRUCTORS 0
#include "../NDI_6_SDK/Include/Processing.NDI.Lib.h"

#include "FFPluginSDK.h"

#define PARAM_SOURCE  0
#define DEFAULT_SOURCE_NORM  0.0f

class NDIInPlugin : public CFreeFramePlugin
{
public:
     NDIInPlugin();
    ~NDIInPlugin();

    DWORD ProcessFrame(void* pFrame);
    DWORD GetParameter(DWORD index);
    DWORD SetParameter(const SetParameterStruct* pParam);
    char* GetParameterDisplay(DWORD index);

private:
    char mDisplayBuf[32];
};

#endif // NDIIN_H

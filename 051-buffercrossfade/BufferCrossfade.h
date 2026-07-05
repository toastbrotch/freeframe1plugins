#ifndef BUFFERCROSSFADE_H
#define BUFFERCROSSFADE_H

#include "FFPluginSDK.h"
#include "FFSharedBuf.h"

#define PARAM_SOURCE1  0
#define PARAM_SOURCE2  1
#define PARAM_MIX      2

#define DEFAULT_SOURCE1_NORM  0.0f   // slot A
#define DEFAULT_SOURCE2_NORM  0.5f   // slot B
#define DEFAULT_MIX_NORM      0.5f   // 50/50

class BufferCrossfadePlugin : public CFreeFramePlugin
{
public:
     BufferCrossfadePlugin();
    ~BufferCrossfadePlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float  mSource1Norm;
    float  mSource2Norm;
    float  mMixNorm;

    // Shared memory handles and mapped pointers for all 3 buffers
    HANDLE mHnd[3];
    void*  mPtr[3];

    char   mDisplayBuf[16];
};

#endif // BUFFERCROSSFADE_H
#ifndef BUFFERRECEIVER_H
#define BUFFERRECEIVER_H

#include "FFPluginSDK.h"
#include "FFSharedBuf.h"

#define PARAM_SOURCE  0

#define DEFAULT_SOURCE_NORM  0.0f   // slot A

class BufferReceiverPlugin : public CFreeFramePlugin
{
public:
     BufferReceiverPlugin();
    ~BufferReceiverPlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float  mSourceNorm;

    // Shared memory handles and mapped pointers for all 3 buffers
    HANDLE mHnd[3];
    void*  mPtr[3];

    char   mDisplayBuf[16];
};

#endif // BUFFERRECEIVER_H

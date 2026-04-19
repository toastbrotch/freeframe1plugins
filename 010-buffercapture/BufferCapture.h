#ifndef BUFFERCAPTURE_H
#define BUFFERCAPTURE_H

#include "FFPluginSDK.h"
#include "FFSharedBuf.h"

#define PARAM_TARGET   0
#define PARAM_PASSTHRU 1

#define DEFAULT_TARGET_NORM   0.0f   // slot A
#define DEFAULT_PASSTHRU_NORM 0.0f   // return black

class BufferCapturePlugin : public CFreeFramePlugin
{
public:
     BufferCapturePlugin();
    ~BufferCapturePlugin();

    DWORD  ProcessFrame(void* pFrame);
    DWORD  GetParameter(DWORD index);
    DWORD  SetParameter(const SetParameterStruct* pParam);
    char*  GetParameterDisplay(DWORD index);

private:
    float   mTargetNorm;
    float   mPassthruNorm;

    // Shared memory handles and mapped pointers for all 3 buffers
    HANDLE  mHnd[3];
    void*   mPtr[3];

    char    mDisplayBuf[16];
};

#endif // BUFFERCAPTURE_H
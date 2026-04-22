// ============================================================
//  BufferReceiver.cpp
//  FreeFrame 1.0 plugin — crossfades the current clip with one
//  of three named shared-memory buffers (A, B or C) written by
//  BufferCapture (010).
//
//  Parameters:
//    Source : 0.0–1.0  →  A (0.0–0.33) | B (0.33–0.67) | C (0.67–1.0)
//    Mix    : 0.0–1.0  →  0.0 = full current clip, 1.0 = full buffer
//
//  If the selected buffer is unavailable or has wrong dimensions
//  the output is the unmodified current clip.
//
//  See FFSharedBuf.h for the shared memory contract.
// ============================================================

#include "BufferReceiver.h"
#include <cstdio>
#include <cstring>

// ============================================================
//  Constructor / Destructor
// ============================================================
BufferReceiverPlugin::BufferReceiverPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_SOURCE,
                 "Source",
                 FF_TYPE_STANDARD,
                 DEFAULT_SOURCE_NORM);

    SetParamInfo(PARAM_MIX,
                 "Mix",
                 FF_TYPE_STANDARD,
                 DEFAULT_MIX_NORM);

    mSourceNorm    = DEFAULT_SOURCE_NORM;
    mMixNorm       = DEFAULT_MIX_NORM;
    mDisplayBuf[0] = '\0';

    for (int i = 0; i < 3; i++) { mHnd[i] = NULL; mPtr[i] = NULL; }

    bool ok = ffBufMapAll(mHnd[0], mHnd[1], mHnd[2],
                          mPtr[0], mPtr[1], mPtr[2],
                          FILE_MAP_READ);

    if (!ok) {
        for (int i = 0; i < 3; i++) {
            if (!mPtr[i] && mHnd[i]) { CloseHandle(mHnd[i]); mHnd[i] = NULL; }
        }
    }
}

BufferReceiverPlugin::~BufferReceiverPlugin()
{
    ffBufUnmapAll(mHnd[0], mHnd[1], mHnd[2],
                  mPtr[0], mPtr[1], mPtr[2]);
}

// ============================================================
//  ProcessFrame
// ============================================================

static const BYTE* validPixels(void* ptr, DWORD expectW, DWORD expectH)
{
    if (!ptr) return NULL;
    const FFBufHeader* hdr = (const FFBufHeader*)ptr;
    if (!hdr->valid)            return NULL;
    if (hdr->width  != expectW) return NULL;
    if (hdr->height != expectH) return NULL;
    return (const BYTE*)ptr + FF_BUF_PIXEL_OFFSET;
}

DWORD BufferReceiverPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    DWORD W        = GetFrameWidth();
    DWORD H        = GetFrameHeight();
    DWORD numBytes = W * H * 3;

    int slot = normToSlot(mSourceNorm);
    const BYTE* buf = validPixels(mPtr[slot], W, H);

    // If buffer unavailable, pass clip through unmodified
    if (!buf) return FF_SUCCESS;

    int mix255 = (int)(mMixNorm * 255.0f + 0.5f);
    if (mix255 < 0)   mix255 = 0;
    if (mix255 > 255) mix255 = 255;

    BYTE* clip = (BYTE*)pFrame;

    if (mix255 == 0) {
        // Full clip — nothing to do
    } else if (mix255 == 255) {
        // Full buffer
        memcpy(clip, buf, numBytes);
    } else {
        // Blend: clip*(255-mix) + buf*mix
        int inv = 255 - mix255;
        for (DWORD i = 0; i < numBytes; i++) {
            clip[i] = (BYTE)(((int)clip[i] * inv + (int)buf[i] * mix255) >> 8);
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD BufferReceiverPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_SOURCE: val = mSourceNorm; break;
        case PARAM_MIX:    val = mMixNorm;    break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD BufferReceiverPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_SOURCE: mSourceNorm = val; return FF_SUCCESS;
        case PARAM_MIX:    mMixNorm    = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* BufferReceiverPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_SOURCE:
            sprintf(mDisplayBuf, "Buf %s", slotName(normToSlot(mSourceNorm)));
            return mDisplayBuf;
        case PARAM_MIX:
            sprintf(mDisplayBuf, "%d%%", (int)(mMixNorm * 100.0f + 0.5f));
            return mDisplayBuf;
        default:
            return NULL;
    }
}

// ============================================================
//  Factory method
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new BufferReceiverPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}
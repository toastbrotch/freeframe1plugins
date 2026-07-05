// ============================================================
//  BufferReceiver.cpp
//  FreeFrame 1.0 source plugin — outputs one of three named
//  shared-memory buffers (A, B or C) written by BufferCapture
//  (010) directly as a Resolume video source.
//
//  Parameters:
//    Source : 0.0–1.0  →  A (0.0–0.33) | B (0.33–0.67) | C (0.67–1.0)
//
//  If the selected buffer is unavailable the output is black.
//  If the buffer dimensions differ from the canvas a nearest-
//  neighbour scale is applied automatically.
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
    SetSupportedOptimizations(FF_OPT_INPLACE);
    SetMinInputs(0);
    SetMaxInputs(0);

    SetParamInfo(PARAM_SOURCE,
                 "Source",
                 FF_TYPE_STANDARD,
                 DEFAULT_SOURCE_NORM);

    mSourceNorm    = DEFAULT_SOURCE_NORM;
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
//  ProcessFrame — pFrame is the OUTPUT buffer (source mode)
// ============================================================
DWORD BufferReceiverPlugin::ProcessFrame(void* pFrame)
{
    DWORD W        = GetFrameWidth();
    DWORD H        = GetFrameHeight();
    DWORD outBytes = W * H * 3;
    BYTE* out      = (BYTE*)pFrame;

    int   slot = normToSlot(mSourceNorm);
    void* ptr  = mPtr[slot];

    if (!ptr) {
        memset(out, 0, outBytes);
        return FF_SUCCESS;
    }

    const FFBufHeader* hdr = (const FFBufHeader*)ptr;
    if (!hdr->valid || hdr->width == 0 || hdr->height == 0) {
        memset(out, 0, outBytes);
        return FF_SUCCESS;
    }

    DWORD srcW = hdr->width;
    DWORD srcH = hdr->height;
    const BYTE* src = (const BYTE*)ptr + FF_BUF_PIXEL_OFFSET;

    if (srcW == W && srcH == H) {
        // Dimensions match — direct copy
        memcpy(out, src, outBytes);
    } else {
        // Nearest-neighbour scale to canvas size
        BYTE* dstRow = out;
        for (DWORD y = 0; y < H; y++) {
            DWORD srcY = (DWORD)((float)y * (float)srcH / (float)H);
            if (srcY >= srcH) srcY = srcH - 1;
            const BYTE* srcRow = src + (size_t)srcY * srcW * 3;
            BYTE* dst = dstRow;
            for (DWORD x = 0; x < W; x++) {
                DWORD srcX = (DWORD)((float)x * (float)srcW / (float)W);
                if (srcX >= srcW) srcX = srcW - 1;
                const BYTE* px = srcRow + srcX * 3;
                dst[0] = px[0];
                dst[1] = px[1];
                dst[2] = px[2];
                dst += 3;
            }
            dstRow += W * 3;
        }
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD BufferReceiverPlugin::GetParameter(DWORD index)
{
    if (index != PARAM_SOURCE) return FF_FAIL;
    DWORD dwRet;
    memcpy(&dwRet, &mSourceNorm, 4);
    return dwRet;
}

DWORD BufferReceiverPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (!pParam || pParam->ParameterNumber != PARAM_SOURCE) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    mSourceNorm = val;
    return FF_SUCCESS;
}

char* BufferReceiverPlugin::GetParameterDisplay(DWORD index)
{
    if (index != PARAM_SOURCE) return NULL;
    sprintf(mDisplayBuf, "Buf %s", slotName(normToSlot(mSourceNorm)));
    return mDisplayBuf;
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

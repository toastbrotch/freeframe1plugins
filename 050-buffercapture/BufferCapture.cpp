// ============================================================
//  BufferCapture.cpp
//  FreeFrame 1.0 plugin — captures the input frame into one of
//  three named shared-memory buffers (A, B or C).
//
//  Use together with BufferCrossfade (011) which reads the
//  buffers and crossfades between any two of them.
//
//  Parameters:
//    Target   : 0.0–1.0  →  A (0.0–0.33) | B (0.33–0.67) | C (0.67–1.0)
//    Passthru : 0.0–1.0  →  0.0 = return black, 1.0 = return original frame
//                           values in between blend black with the original
//
//  Shared memory:
//    Three named pagefile-backed mappings are created (or opened if
//    they already exist) when the plugin is instantiated. They persist
//    until all handles in the process are closed. Layout per buffer:
//      [ FFBufHeader | RGB24 pixel data ]
//    See FFSharedBuf.h for the full contract.
// ============================================================

#include "BufferCapture.h"
#include <cstdio>
#include <cstring>

// ============================================================
//  Constructor / Destructor
// ============================================================
BufferCapturePlugin::BufferCapturePlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_TARGET,
                 "Target",
                 FF_TYPE_STANDARD,
                 DEFAULT_TARGET_NORM);

    SetParamInfo(PARAM_PASSTHRU,
                 "Passthru",
                 FF_TYPE_STANDARD,
                 DEFAULT_PASSTHRU_NORM);

    mTargetNorm    = DEFAULT_TARGET_NORM;
    mPassthruNorm  = DEFAULT_PASSTHRU_NORM;
    mDisplayBuf[0] = '\0';

    // Must be NULL before ffBufMapAll — if it returns early on a failed
    // CreateFileMapping, any unassigned pointer stays NULL and ProcessFrame
    // skips the write safely instead of writing to a garbage address.
    for (int i = 0; i < 3; i++) { mHnd[i] = NULL; mPtr[i] = NULL; }

    // Open-or-create all 3 shared buffers
    bool ok = ffBufMapAll(mHnd[0], mHnd[1], mHnd[2],
                          mPtr[0], mPtr[1], mPtr[2],
                          FILE_MAP_WRITE);

    if (!ok) {
        // Partial failure: zero out anything that didn't map so the
        // destructor can safely skip them
        for (int i = 0; i < 3; i++) {
            if (!mPtr[i] && mHnd[i]) { CloseHandle(mHnd[i]); mHnd[i] = NULL; }
        }
    }
}

BufferCapturePlugin::~BufferCapturePlugin()
{
    ffBufUnmapAll(mHnd[0], mHnd[1], mHnd[2],
                  mPtr[0], mPtr[1], mPtr[2]);
}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD BufferCapturePlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    int   slot      = normToSlot(mTargetNorm);
    void* pShared   = mPtr[slot];
    DWORD W         = GetFrameWidth();
    DWORD H         = GetFrameHeight();
    DWORD pixelBytes = W * H * 3;

    if (pShared && pixelBytes <= FF_BUF_PIXEL_MAX) {
        // Write header
        FFBufHeader* hdr = (FFBufHeader*)pShared;
        hdr->width    = W;
        hdr->height   = H;
        hdr->valid    = 1;
        hdr->reserved = 0;

        // Copy pixel data after header
        BYTE* dst = (BYTE*)pShared + FF_BUF_PIXEL_OFFSET;
        memcpy(dst, pFrame, pixelBytes);
    }

    // Return black, original, or a blend depending on Passthru slider
    int pt = (int)(mPassthruNorm * 255.0f + 0.5f);
    if (pt <= 0) {
        // Full black
        memset(pFrame, 0, pixelBytes);
    } else if (pt >= 255) {
        // Full passthrough — frame already contains the original, nothing to do
    } else {
        // Blend: out = original * pt/255  (fade original toward black)
        BYTE* px = (BYTE*)pFrame;
        for (DWORD i = 0; i < pixelBytes; i++) {
            px[i] = (BYTE)(((int)px[i] * pt) >> 8);
        }
    }
    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD BufferCapturePlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_TARGET:   val = mTargetNorm;   break;
        case PARAM_PASSTHRU: val = mPassthruNorm; break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD BufferCapturePlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_TARGET:   mTargetNorm   = val; return FF_SUCCESS;
        case PARAM_PASSTHRU: mPassthruNorm = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* BufferCapturePlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_TARGET:
            sprintf(mDisplayBuf, "Buf %s", slotName(normToSlot(mTargetNorm)));
            return mDisplayBuf;
        case PARAM_PASSTHRU:
            if (mPassthruNorm <= 0.0f)      sprintf(mDisplayBuf, "black");
            else if (mPassthruNorm >= 1.0f)  sprintf(mDisplayBuf, "passthru");
            else                             sprintf(mDisplayBuf, "%d%%", (int)(mPassthruNorm * 100.0f + 0.5f));
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
    *ppInstance = new BufferCapturePlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}
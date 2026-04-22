// ============================================================
//  BufferCrossfade.cpp
//  FreeFrame 1.0 plugin — crossfade between shared buffers
//
//  Reads two of the three named shared-memory buffers written
//  by BufferCapture (010) and outputs a linear blend of them.
//  The input frame from Resolume is ignored; output is written
//  entirely from the shared buffers.
//
//  Robustness rules:
//    - Buffer never written (valid == 0)  → treated as black
//    - Source1 == Source2                 → that buffer at full, mix irrelevant
//    - Buffer dimensions mismatch frame   → that buffer treated as black
//    - Shared mapping unavailable         → black
//
//  Parameters:
//    Source1 : 0.0–1.0  →  A | B | C  (first blend input)
//    Source2 : 0.0–1.0  →  A | B | C  (second blend input)
//    Mix     : 0.0–1.0  →  0.0 = full Source1, 1.0 = full Source2
//
//  See FFSharedBuf.h for the shared memory contract.
// ============================================================

#include "BufferCrossfade.h"
#include <cstdio>
#include <cstring>

// ============================================================
//  Constructor / Destructor
// ============================================================
BufferCrossfadePlugin::BufferCrossfadePlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(1);
    SetMaxInputs(1);

    SetParamInfo(PARAM_SOURCE1,
                 "Source1",
                 FF_TYPE_STANDARD,
                 DEFAULT_SOURCE1_NORM);

    SetParamInfo(PARAM_SOURCE2,
                 "Source2",
                 FF_TYPE_STANDARD,
                 DEFAULT_SOURCE2_NORM);

    SetParamInfo(PARAM_MIX,
                 "Mix",
                 FF_TYPE_STANDARD,
                 DEFAULT_MIX_NORM);

    mSource1Norm   = DEFAULT_SOURCE1_NORM;
    mSource2Norm   = DEFAULT_SOURCE2_NORM;
    mMixNorm       = DEFAULT_MIX_NORM;
    mDisplayBuf[0] = '\0';

    // Open-or-create all 3 shared buffers (read access is sufficient
    // for this plugin, but CreateFileMapping requires PAGE_READWRITE;
    // MapViewOfFile then uses FILE_MAP_READ for our view)
    bool ok = ffBufMapAll(mHnd[0], mHnd[1], mHnd[2],
                          mPtr[0], mPtr[1], mPtr[2],
                          FILE_MAP_READ);

    if (!ok) {
        for (int i = 0; i < 3; i++) {
            if (!mPtr[i] && mHnd[i]) { CloseHandle(mHnd[i]); mHnd[i] = NULL; }
        }
    }
}

BufferCrossfadePlugin::~BufferCrossfadePlugin()
{
    ffBufUnmapAll(mHnd[0], mHnd[1], mHnd[2],
                  mPtr[0], mPtr[1], mPtr[2]);
}

// ============================================================
//  ProcessFrame
// ============================================================

// Returns a validated pixel pointer for the given slot, or NULL if
// the buffer is unavailable, invalid, or has wrong dimensions.
static const BYTE* validPixels(void* ptr, DWORD expectW, DWORD expectH)
{
    if (!ptr) return NULL;
    const FFBufHeader* hdr = (const FFBufHeader*)ptr;
    if (!hdr->valid)             return NULL;
    if (hdr->width  != expectW)  return NULL;
    if (hdr->height != expectH)  return NULL;
    return (const BYTE*)ptr + FF_BUF_PIXEL_OFFSET;
}

DWORD BufferCrossfadePlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;

    DWORD W         = GetFrameWidth();
    DWORD H         = GetFrameHeight();
    DWORD numPixels = W * H;
    DWORD numBytes  = numPixels * 3;

    int s1 = normToSlot(mSource1Norm);
    int s2 = normToSlot(mSource2Norm);

    const BYTE* p1 = validPixels(mPtr[s1], W, H);
    const BYTE* p2 = validPixels(mPtr[s2], W, H);

    BYTE* out = (BYTE*)pFrame;

    // Fast paths
    if (s1 == s2) {
        // Both sources identical: copy that buffer (or black if invalid)
        if (p1) memcpy(out, p1, numBytes);
        else    memset(out, 0,  numBytes);
        return FF_SUCCESS;
    }

    if (!p1 && !p2) {
        memset(out, 0, numBytes);
        return FF_SUCCESS;
    }

    // mix255: 0 = full p1, 255 = full p2
    int mix255 = (int)(mMixNorm * 255.0f + 0.5f);
    if (mix255 < 0)   mix255 = 0;
    if (mix255 > 255) mix255 = 255;

    if (!p1 || mix255 == 255) {
        // Source1 invalid or mix fully at Source2
        if (p2) memcpy(out, p2, numBytes);
        else    memset(out, 0,  numBytes);
        return FF_SUCCESS;
    }

    if (!p2 || mix255 == 0) {
        // Source2 invalid or mix fully at Source1
        memcpy(out, p1, numBytes);
        return FF_SUCCESS;
    }

    // General blend: out = p1*(255-mix) + p2*mix, all in integer
    int inv = 255 - mix255;
    for (DWORD i = 0; i < numBytes; i++) {
        out[i] = (BYTE)(((int)p1[i] * inv + (int)p2[i] * mix255) >> 8);
    }

    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD BufferCrossfadePlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_SOURCE1: val = mSource1Norm; break;
        case PARAM_SOURCE2: val = mSource2Norm; break;
        case PARAM_MIX:     val = mMixNorm;     break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD BufferCrossfadePlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (pParam == NULL) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_SOURCE1: mSource1Norm = val; return FF_SUCCESS;
        case PARAM_SOURCE2: mSource2Norm = val; return FF_SUCCESS;
        case PARAM_MIX:     mMixNorm     = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* BufferCrossfadePlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_SOURCE1:
            sprintf(mDisplayBuf, "Buf %s", slotName(normToSlot(mSource1Norm)));
            return mDisplayBuf;
        case PARAM_SOURCE2:
            sprintf(mDisplayBuf, "Buf %s", slotName(normToSlot(mSource2Norm)));
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
    *ppInstance = new BufferCrossfadePlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}
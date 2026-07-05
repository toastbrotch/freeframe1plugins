#ifndef FF_SHARED_BUF_H
#define FF_SHARED_BUF_H

// ============================================================
//  FFSharedBuf.h — shared memory contract between
//  BufferCapture (010) and BufferCrossfade (011)
//
//  KEEP THIS FILE IDENTICAL IN BOTH PLUGIN FOLDERS.
//  The struct layout and buffer names must match exactly.
// ============================================================

#include <windows.h>

// Named kernel objects — one per capture slot
#define FF_BUF_NAME_A  "FF_Cap_BufA"
#define FF_BUF_NAME_B  "FF_Cap_BufB"
#define FF_BUF_NAME_C  "FF_Cap_BufC"

// Pre-allocated size per buffer: covers up to 1920×1080 RGB24 + header
// Header: 4 × DWORD = 16 bytes
#define FF_BUF_PIXEL_MAX  (1920 * 1080 * 3)
#define FF_BUF_TOTAL_SIZE (FF_BUF_PIXEL_MAX + 16)

// Layout at the start of each shared mapping
#pragma pack(push, 1)
struct FFBufHeader {
    DWORD width;      // frame width written by capture plugin
    DWORD height;     // frame height written by capture plugin
    DWORD valid;      // 0 = never written (treat as black), 1 = valid
    DWORD reserved;
};
#pragma pack(pop)

// Pixel data immediately follows the header
#define FF_BUF_PIXEL_OFFSET  sizeof(FFBufHeader)

// Helper: open-or-create a single named mapping.
// Returns NULL on failure. Both create and open call this.
inline HANDLE ffBufOpenOrCreate(const char* name) {
    HANDLE h = CreateFileMapping(
        INVALID_HANDLE_VALUE,   // pagefile-backed
        NULL,                   // default security
        PAGE_READWRITE,
        0,
        FF_BUF_TOTAL_SIZE,
        name
    );
    // h is valid whether we created it or opened an existing one
    // (ERROR_ALREADY_EXISTS is not an error here)
    return h;
}

// Helper: map all 3 buffers. Returns false if any mapping fails.
// hA/hB/hC receive the handles; pA/pB/pC receive the mapped pointers.
inline bool ffBufMapAll(HANDLE& hA, HANDLE& hB, HANDLE& hC,
                         void*&  pA, void*&  pB, void*&  pC,
                         DWORD   access)
{
    hA = ffBufOpenOrCreate(FF_BUF_NAME_A);
    hB = ffBufOpenOrCreate(FF_BUF_NAME_B);
    hC = ffBufOpenOrCreate(FF_BUF_NAME_C);

    if (!hA || !hB || !hC) return false;

    pA = MapViewOfFile(hA, access, 0, 0, FF_BUF_TOTAL_SIZE);
    pB = MapViewOfFile(hB, access, 0, 0, FF_BUF_TOTAL_SIZE);
    pC = MapViewOfFile(hC, access, 0, 0, FF_BUF_TOTAL_SIZE);

    return (pA && pB && pC);
}

// Helper: unmap and close all 3 buffers
inline void ffBufUnmapAll(HANDLE hA, HANDLE hB, HANDLE hC,
                           void*  pA, void*  pB, void*  pC)
{
    if (pA) UnmapViewOfFile(pA);
    if (pB) UnmapViewOfFile(pB);
    if (pC) UnmapViewOfFile(pC);
    if (hA) CloseHandle(hA);
    if (hB) CloseHandle(hB);
    if (hC) CloseHandle(hC);
}

// Slot index helpers
inline int normToSlot(float n) {
    if (n < 0.333f) return 0;  // A
    if (n < 0.667f) return 1;  // B
    return 2;                   // C
}

inline const char* slotName(int slot) {
    if (slot == 0) return "A";
    if (slot == 1) return "B";
    return "C";
}

#endif // FF_SHARED_BUF_H
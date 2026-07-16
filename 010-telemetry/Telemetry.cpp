// ============================================================
//  Telemetry.cpp
//  FreeFrame 1.0 source plugin — procedural cyberpunk telemetry
//
//  Renders white-on-black artefacts: scrolling text blocks,
//  oscilloscope waveforms, bar graphs, line charts, hex dumps,
//  status panels, and cardiac-style scope traces.
//
//  Parameters:
//    Switch  : 0→1 → set changes every ~3000→30 frames
//    Density : 0→1 → 1–8 artefacts visible simultaneously
// ============================================================

#include "Telemetry.h"
#include <cstring>
#include <cstdio>
#include <cmath>

// ============================================================
//  5×7 bitmap font, column-major, LSB=top row, ASCII 32–127
// ============================================================
static const unsigned char kFont[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 0x20 ' '
    {0x00,0x00,0x5F,0x00,0x00}, // 0x21 '!'
    {0x00,0x07,0x00,0x07,0x00}, // 0x22 '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // 0x23 '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // 0x24 '$'
    {0x23,0x13,0x08,0x64,0x62}, // 0x25 '%'
    {0x36,0x49,0x55,0x22,0x50}, // 0x26 '&'
    {0x00,0x05,0x03,0x00,0x00}, // 0x27 '\''
    {0x00,0x1C,0x22,0x41,0x00}, // 0x28 '('
    {0x00,0x41,0x22,0x1C,0x00}, // 0x29 ')'
    {0x14,0x08,0x3E,0x08,0x14}, // 0x2A '*'
    {0x08,0x08,0x3E,0x08,0x08}, // 0x2B '+'
    {0x00,0x50,0x30,0x00,0x00}, // 0x2C ','
    {0x08,0x08,0x08,0x08,0x08}, // 0x2D '-'
    {0x00,0x60,0x60,0x00,0x00}, // 0x2E '.'
    {0x20,0x10,0x08,0x04,0x02}, // 0x2F '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // 0x30 '0'
    {0x00,0x42,0x7F,0x40,0x00}, // 0x31 '1'
    {0x42,0x61,0x51,0x49,0x46}, // 0x32 '2'
    {0x21,0x41,0x45,0x4B,0x31}, // 0x33 '3'
    {0x18,0x14,0x12,0x7F,0x10}, // 0x34 '4'
    {0x27,0x45,0x45,0x45,0x39}, // 0x35 '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // 0x36 '6'
    {0x01,0x71,0x09,0x05,0x03}, // 0x37 '7'
    {0x36,0x49,0x49,0x49,0x36}, // 0x38 '8'
    {0x06,0x49,0x49,0x29,0x1E}, // 0x39 '9'
    {0x00,0x36,0x36,0x00,0x00}, // 0x3A ':'
    {0x00,0x56,0x36,0x00,0x00}, // 0x3B ';'
    {0x08,0x14,0x22,0x41,0x00}, // 0x3C '<'
    {0x14,0x14,0x14,0x14,0x14}, // 0x3D '='
    {0x00,0x41,0x22,0x14,0x08}, // 0x3E '>'
    {0x02,0x01,0x51,0x09,0x06}, // 0x3F '?'
    {0x32,0x49,0x79,0x41,0x3E}, // 0x40 '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 0x41 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 0x42 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 0x43 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 0x44 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 0x45 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 0x46 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 0x47 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 0x48 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 0x49 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 0x4A 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 0x4B 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 0x4C 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 0x4D 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 0x4E 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 0x4F 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 0x50 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 0x51 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 0x52 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 0x53 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 0x54 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 0x55 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 0x56 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 0x57 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 0x58 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 0x59 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 0x5A 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // 0x5B '['
    {0x02,0x04,0x08,0x10,0x20}, // 0x5C '\'
    {0x00,0x41,0x41,0x7F,0x00}, // 0x5D ']'
    {0x04,0x02,0x01,0x02,0x04}, // 0x5E '^'
    {0x40,0x40,0x40,0x40,0x40}, // 0x5F '_'
    {0x00,0x01,0x02,0x04,0x00}, // 0x60 '`'
    {0x20,0x54,0x54,0x54,0x78}, // 0x61 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 0x62 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 0x63 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 0x64 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 0x65 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 0x66 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 0x67 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 0x68 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 0x69 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 0x6A 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 0x6B 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 0x6C 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 0x6D 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 0x6E 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 0x6F 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 0x70 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 0x71 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 0x72 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 0x73 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 0x74 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 0x75 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 0x76 'v'
    {0x3C,0x40,0x20,0x40,0x3C}, // 0x77 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 0x78 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 0x79 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 0x7A 'z'
    {0x00,0x08,0x36,0x41,0x00}, // 0x7B '{'
    {0x00,0x00,0x7F,0x00,0x00}, // 0x7C '|'
    {0x00,0x41,0x36,0x08,0x00}, // 0x7D '}'
    {0x10,0x08,0x08,0x10,0x08}, // 0x7E '~'
    {0x00,0x00,0x00,0x00,0x00}, // 0x7F DEL
};

// ============================================================
//  Helpers
// ============================================================
unsigned TelemetryPlugin::h2(unsigned a, unsigned b) {
    a ^= b;
    a = (a ^ (a >> 16)) * 0x45d9f3bu;
    a = (a ^ (a >> 16)) * 0x45d9f3bu;
    return a ^ (a >> 16);
}

void TelemetryPlugin::putPixel(BYTE* fb, int W, int H, int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    BYTE* p = fb + (y * W + x) * 3;
    p[0] = p[1] = p[2] = 255;
}

void TelemetryPlugin::hLine(BYTE* fb, int W, int H, int x, int y, int len) {
    for (int i = 0; i < len; i++) putPixel(fb, W, H, x + i, y);
}

void TelemetryPlugin::vLine(BYTE* fb, int W, int H, int x, int y, int len) {
    for (int i = 0; i < len; i++) putPixel(fb, W, H, x, y + i);
}

void TelemetryPlugin::rect(BYTE* fb, int W, int H, int x, int y, int w, int h) {
    hLine(fb, W, H, x,         y,         w);
    hLine(fb, W, H, x,         y + h - 1, w);
    vLine(fb, W, H, x,         y,         h);
    vLine(fb, W, H, x + w - 1, y,         h);
}

void TelemetryPlugin::drawChar(BYTE* fb, int W, int H, int x, int y, char c) {
    if (c < 32 || c > 127) c = '?';
    const unsigned char* col = kFont[(unsigned char)c - 32];
    for (int cx = 0; cx < 5; cx++) {
        unsigned char bits = col[cx];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row))
                putPixel(fb, W, H, x + cx, y + row);
        }
    }
}

int TelemetryPlugin::drawText(BYTE* fb, int W, int H, int x, int y, const char* s) {
    int cx = x;
    while (*s) {
        drawChar(fb, W, H, cx, y, *s++);
        cx += 6;
    }
    return cx;
}

// ============================================================
//  Constructor
// ============================================================
TelemetryPlugin::TelemetryPlugin()
: CFreeFramePlugin()
{
    SetProcessFrameCopySupported(false);
    SetSupportedFormats(FF_RGB_24);
    SetSupportedOptimizations(FF_OPT_NONE);
    SetMinInputs(0);
    SetMaxInputs(0);

    SetParamInfo(PARAM_SWITCH,  "Switch",  FF_TYPE_STANDARD, DEFAULT_SWITCH);
    SetParamInfo(PARAM_DENSITY, "Density", FF_TYPE_STANDARD, DEFAULT_DENSITY);

    mSwitchNorm  = DEFAULT_SWITCH;
    mDensityNorm = DEFAULT_DENSITY;
    mFrameCount  = 0;
    mLastEpoch   = 0xFFFFFFFFu;
    mSetSeed     = 0xDEADF00Du;
    mDisplayBuf[0] = '\0';

    for (int i = 0; i < MAX_ARTEFACTS; i++) {
        mArtefacts[i].type = AT_TEXTBLOCK;
        mArtefacts[i].x = mArtefacts[i].y = 0;
        mArtefacts[i].w = mArtefacts[i].h = 64;
        mArtefacts[i].seed = (unsigned)i * 0x9e3779b9u;
    }
}

TelemetryPlugin::~TelemetryPlugin() {}

// ============================================================
//  Rebuild artefact layout for a new epoch
// ============================================================
void TelemetryPlugin::rebuildSet(int W, int H) {
    unsigned s = mSetSeed;
    for (int i = 0; i < MAX_ARTEFACTS; i++) {
        unsigned si = h2(s, (unsigned)i * 0xABCDu);
        mArtefacts[i].type = (int)(h2(si, 1u) % (unsigned)AT_COUNT);
        mArtefacts[i].seed = h2(si, 2u);

        // Minimum sizes per type
        int minW = 60, minH = 40;
        switch (mArtefacts[i].type) {
            case AT_TEXTBLOCK: minW = 110; minH = 50; break;
            case AT_WAVEFORM:  minW =  80; minH = 30; break;
            case AT_BARGRAPH:  minW =  70; minH = 40; break;
            case AT_LINECHART: minW =  80; minH = 40; break;
            case AT_HEXDUMP:   minW = 130; minH = 50; break;
            case AT_STATUS:    minW = 100; minH = 50; break;
            case AT_SCOPE:     minW =  90; minH = 28; break;
        }
        int maxW = W * 2 / 3;
        int maxH = H * 2 / 3;
        if (maxW < minW) maxW = minW;
        if (maxH < minH) maxH = minH;

        mArtefacts[i].w = minW + (int)(h2(si, 3u) % (unsigned)(maxW - minW + 1));
        mArtefacts[i].h = minH + (int)(h2(si, 4u) % (unsigned)(maxH - minH + 1));

        int maxX = W - mArtefacts[i].w;  if (maxX < 0) maxX = 0;
        int maxY = H - mArtefacts[i].h;  if (maxY < 0) maxY = 0;
        mArtefacts[i].x = (int)(h2(si, 5u) % (unsigned)(maxX + 1));
        mArtefacts[i].y = (int)(h2(si, 6u) % (unsigned)(maxY + 1));
    }
}

// ============================================================
//  Artefact renderers
// ============================================================

// ---- Scrolling text block ----
static const char* kLabels[] = {
    "SYS","MEM","NET","CPU","DSP","BUS","IO ","GPU",
    "PKT","SYNC","LINK","NODE","CORE","TEMP","VOLT","CLK"
};
static const char* kStatus[] = {
    "OK","NOMINAL","ACTIVE","LOCKED","IDLE","WARN","ERR","---"
};

void TelemetryPlugin::drawTextBlock(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    const int lineH = 9;
    const int innerW = a.w - 4;
    const int nLines = (a.h - 4) / lineH + 2;
    const int scrollOff = (int)(mFrameCount / 2) % (nLines * lineH);

    // clip region to box interior
    int by0 = a.y + 2, by1 = a.y + a.h - 2;

    for (int i = 0; i < nLines + 1; i++) {
        int py = a.y + 2 + i * lineH - scrollOff;
        if (py + 7 < by0 || py > by1) continue;

        unsigned ls = h2(a.seed, (unsigned)i * 0x3F7u);
        unsigned lv = h2(ls, mFrameCount / 60); // value flickers slowly

        const char* lbl = kLabels[h2(ls, 0u) % 16];
        unsigned val1 = h2(lv, 1u) & 0xFFFF;
        unsigned val2 = h2(lv, 2u) & 0xFF;
        const char* sta = kStatus[h2(ls, 3u) % 8];

        char line[40];
        int fmt = h2(ls, 7u) % 4;
        if (fmt == 0)
            sprintf(line, "%s: 0x%04X  %s", lbl, val1, sta);
        else if (fmt == 1)
            sprintf(line, "%s: %3d.%02d  [%s]", lbl, (int)(val1 % 999), (int)(val2 % 100), sta);
        else if (fmt == 2)
            sprintf(line, ">> %s %3d%%  %s", lbl, (int)(val1 % 101), sta);
        else
            sprintf(line, "%s %04X:%02X:%02X  %s", lbl, val1, val2, (unsigned)(h2(lv,3u)&0xFF), sta);

        // clip char-by-char isn't needed; putPixel clips per-pixel
        // but we do skip fully-out-of-range rows
        const char* src = line;
        int cx = a.x + 2;
        while (*src && cx < a.x + innerW) {
            drawChar(fb, W, H, cx, py, *src++);
            cx += 6;
        }
    }
}

// ---- Oscilloscope waveform ----
void TelemetryPlugin::drawWaveform(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    int mx = a.x + 2, my = a.y + 2;
    int iw = a.w - 4, ih = a.h - 4;
    float cy = (float)(my + ih / 2);
    float amp = (float)(ih / 2 - 2);
    float freq1 = 0.03f + (float)(h2(a.seed, 10u) % 40) * 0.002f;
    float freq2 = freq1 * (1.5f + (float)(h2(a.seed, 11u) % 30) * 0.05f);
    float phase = (float)mFrameCount * 0.05f;

    int prevY = (int)(cy + amp * sinf(freq1 * 0.0f + phase));
    for (int px = 1; px < iw; px++) {
        float t = (float)px;
        float noise = 0.15f * amp * sinf(freq2 * t * 3.3f + phase * 1.7f);
        float val = sinf(freq1 * t + phase) * amp * 0.7f + noise;
        int curY = (int)(cy + val);
        // draw vertical segment connecting prev to cur
        int y0 = prevY < curY ? prevY : curY;
        int y1 = prevY < curY ? curY : prevY;
        for (int yy = y0; yy <= y1; yy++)
            putPixel(fb, W, H, mx + px, yy);
        prevY = curY;
    }
    // horizontal axis label
    char buf[16];
    sprintf(buf, "CH%02d", (int)(h2(a.seed, 99u) % 16));
    drawText(fb, W, H, a.x + 3, a.y + a.h - 10, buf);
}

// ---- Bar graph ----
void TelemetryPlugin::drawBarGraph(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    int nBars = 4 + (int)(h2(a.seed, 20u) % 5);
    int iw = a.w - 4, ih = a.h - 12;
    int barW = (iw - 2) / nBars;
    if (barW < 3) barW = 3;

    for (int b = 0; b < nBars; b++) {
        unsigned bseed = h2(a.seed, (unsigned)b * 0x71u);
        // Value oscillates slowly
        float twave = (float)mFrameCount * 0.02f + (float)b * 1.1f;
        float base = (float)(h2(bseed, 1u) % 80 + 10) * 0.01f;
        float delta = 0.15f * sinf(twave + (float)(bseed & 0xFF) * 0.04f);
        float frac = base + delta;
        if (frac < 0.03f) frac = 0.03f;
        if (frac > 1.0f)  frac = 1.0f;

        int barH = (int)(frac * (float)ih);
        int bx = a.x + 2 + b * barW + 1;
        int by = a.y + 2 + ih - barH;

        // filled bar
        for (int row = by; row < a.y + 2 + ih; row++)
            for (int col = bx; col < bx + barW - 1; col++)
                putPixel(fb, W, H, col, row);

        // percentage label
        char pct[5];
        sprintf(pct, "%2d", (int)(frac * 100.0f + 0.5f));
        drawText(fb, W, H, bx, a.y + a.h - 9, pct);
    }
}

// ---- Scrolling line chart ----
void TelemetryPlugin::drawLineChart(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    int iw = a.w - 4, ih = a.h - 12;
    float oy = (float)(a.y + 2 + ih);

    // x-axis
    hLine(fb, W, H, a.x + 2, a.y + 2 + ih, iw);

    int prevY = -1;
    for (int px = 0; px < iw; px++) {
        // sample index scrolls with frame
        unsigned sampleIdx = (unsigned)(mFrameCount + (unsigned)(iw - px));
        unsigned sv = h2(h2(a.seed, sampleIdx), sampleIdx * 3u);
        float base = (float)(sv & 0xFF) / 255.0f;
        // smooth with neighbours
        unsigned sv2 = h2(h2(a.seed, sampleIdx + 1), (sampleIdx + 1) * 3u);
        float s2 = (float)(sv2 & 0xFF) / 255.0f;
        float v = (base + s2) * 0.5f;

        int curY = (int)(oy - v * (float)ih);
        if (prevY >= 0) {
            int y0 = prevY < curY ? prevY : curY;
            int y1 = prevY < curY ? curY : prevY;
            for (int yy = y0; yy <= y1; yy++)
                putPixel(fb, W, H, a.x + 2 + px - 1, yy);
        }
        putPixel(fb, W, H, a.x + 2 + px, curY);
        prevY = curY;
    }

    // label
    char buf[20];
    sprintf(buf, "SIG%02d", (int)(h2(a.seed, 88u) % 32));
    drawText(fb, W, H, a.x + 3, a.y + a.h - 9, buf);
}

// ---- Hex dump ----
void TelemetryPlugin::drawHexDump(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    const int lineH = 9;
    int nLines = (a.h - 4) / lineH;
    int scrollOff = (int)(mFrameCount / 3) % (nLines * lineH + 1);

    for (int i = 0; i < nLines + 1; i++) {
        int py = a.y + 2 + i * lineH - scrollOff;
        if (py + 7 < a.y + 2 || py > a.y + a.h - 2) continue;

        unsigned ls = h2(a.seed, (unsigned)i * 0x1FDu);
        unsigned addr = h2(ls, 0u) & 0xFFFFFF00u;
        char line[48];
        sprintf(line, "%08X: %02X %02X %02X %02X %02X %02X %02X %02X",
            addr + (unsigned)(i * 8),
            h2(ls,1u)&0xFF, h2(ls,2u)&0xFF, h2(ls,3u)&0xFF, h2(ls,4u)&0xFF,
            h2(ls,5u)&0xFF, h2(ls,6u)&0xFF, h2(ls,7u)&0xFF, h2(ls,8u)&0xFF);

        int cx = a.x + 2;
        const char* s = line;
        while (*s && cx + 5 < a.x + a.w) {
            drawChar(fb, W, H, cx, py, *s++);
            cx += 6;
        }
    }
}

// ---- Status panel ----
static const char* kSysLines[] = {
    "UPLINK: NOMINAL",
    "CLOCK: SYNCED",
    "TEMP: %dC",
    "VOLT: %d.%02dV",
    "ERRORS: %04X",
    "LOAD: %d%%",
    "PKT RX: %08X",
    "SECTOR: %04d",
    "DEPTH: %04.1fm",
    "FREQ: %d.%02dMHz",
};
static const int kNumSysLines = 10;

void TelemetryPlugin::drawStatus(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    // Header bar
    hLine(fb, W, H, a.x + 1, a.y + 10, a.w - 2);
    char hdr[24];
    sprintf(hdr, "NODE %02X STATUS", (int)(h2(a.seed, 0u) % 256));
    drawText(fb, W, H, a.x + 4, a.y + 2, hdr);

    const int lineH = 9;
    int nLines = (a.h - 16) / lineH;
    for (int i = 0; i < nLines && i < kNumSysLines; i++) {
        unsigned ls = h2(a.seed, (unsigned)i * 0x4Du + 1u);
        unsigned lv = h2(ls, mFrameCount / 90);
        char line[40];
        const char* tmpl = kSysLines[(i + (int)(h2(a.seed, 99u) % kNumSysLines)) % kNumSysLines];
        sprintf(line, tmpl,
            (int)(lv & 0x7F) + 20,
            (int)(lv >> 4 & 0xF) + 3,
            (int)(lv >> 8 & 0x3F),
            lv & 0xFFFF,
            (int)((lv & 0x7F) + 1),
            lv & 0xFFFFFFFF,
            (int)(lv & 0xFFF),
            (float)(lv & 0x3FFF) * 0.1f,
            (int)(lv & 0x3FF) + 100,
            (int)(lv & 0x3F));
        int py = a.y + 13 + i * lineH;
        const char* s = line;
        int cx = a.x + 4;
        while (*s && cx + 5 < a.x + a.w) {
            drawChar(fb, W, H, cx, py, *s++);
            cx += 6;
        }
    }
}

// ---- Cardiac scope ----
void TelemetryPlugin::drawScope(BYTE* fb, int W, int H, const Artefact& a) {
    rect(fb, W, H, a.x, a.y, a.w, a.h);
    int iw = a.w - 4, ih = a.h - 4;
    int cy = a.y + 2 + ih / 2;
    int period = 40 + (int)(h2(a.seed, 50u) % 40);
    // phase offset so each scope starts at a different point
    int phaseOff = (int)(h2(a.seed, 51u) % (unsigned)period);

    int prevY = cy;
    for (int px = 0; px < iw; px++) {
        int t = ((int)mFrameCount + (iw - px) + phaseOff) % period;
        int curY;
        if (t == 0)
            curY = cy - ih * 2 / 5;        // sharp upstroke
        else if (t == 1)
            curY = cy + ih / 5;            // overshoot
        else if (t == 2)
            curY = cy;                     // return to baseline
        else
            curY = cy + (int)((float)(ih / 8) * sinf((float)t * 0.3f));  // gentle noise

        int y0 = prevY < curY ? prevY : curY;
        int y1 = prevY < curY ? curY : prevY;
        for (int yy = y0; yy <= y1; yy++)
            putPixel(fb, W, H, a.x + 2 + px, yy);
        prevY = curY;
    }

    char buf[16];
    unsigned bpm = 60 + h2(a.seed, 52u) % 80;
    sprintf(buf, "HR:%d", (int)bpm);
    drawText(fb, W, H, a.x + 3, a.y + 3, buf);
}

// ============================================================
//  Dispatch
// ============================================================
void TelemetryPlugin::drawArtefact(BYTE* fb, int W, int H, const Artefact& a) {
    switch (a.type) {
        case AT_TEXTBLOCK: drawTextBlock(fb, W, H, a); break;
        case AT_WAVEFORM:  drawWaveform(fb, W, H, a);  break;
        case AT_BARGRAPH:  drawBarGraph(fb, W, H, a);  break;
        case AT_LINECHART: drawLineChart(fb, W, H, a); break;
        case AT_HEXDUMP:   drawHexDump(fb, W, H, a);  break;
        case AT_STATUS:    drawStatus(fb, W, H, a);    break;
        case AT_SCOPE:     drawScope(fb, W, H, a);     break;
    }
}

// ============================================================
//  ProcessFrame
// ============================================================
DWORD TelemetryPlugin::ProcessFrame(void* pFrame)
{
    if (GetFrameDepth() != FF_DEPTH_24) return FF_FAIL;
    int   W  = (int)GetFrameWidth();
    int   H  = (int)GetFrameHeight();
    BYTE* fb = (BYTE*)pFrame;

    // Clear to black
    memset(fb, 0, (size_t)(W * H * 3));

    // Determine switch period: slider 0→1 maps 3000→30 frames (quadratic)
    float sw = 1.0f - mSwitchNorm;
    unsigned period = (unsigned)(30.0f + sw * sw * 2970.0f);
    unsigned epoch  = mFrameCount / period;

    if (epoch != mLastEpoch) {
        mSetSeed   = h2(epoch, 0xFACEB00Cu);
        mLastEpoch = epoch;
        rebuildSet(W, H);
    }

    // Number of active artefacts from Density
    int nActive = 1 + (int)(mDensityNorm * (float)(MAX_ARTEFACTS - 1) + 0.5f);
    if (nActive > MAX_ARTEFACTS) nActive = MAX_ARTEFACTS;

    for (int i = 0; i < nActive; i++)
        drawArtefact(fb, W, H, mArtefacts[i]);

    mFrameCount++;
    return FF_SUCCESS;
}

// ============================================================
//  GetParameter / SetParameter / GetParameterDisplay
// ============================================================
DWORD TelemetryPlugin::GetParameter(DWORD index)
{
    DWORD dwRet;
    float val;
    switch (index) {
        case PARAM_SWITCH:  val = mSwitchNorm;  break;
        case PARAM_DENSITY: val = mDensityNorm; break;
        default: return FF_FAIL;
    }
    memcpy(&dwRet, &val, 4);
    return dwRet;
}

DWORD TelemetryPlugin::SetParameter(const SetParameterStruct* pParam)
{
    if (!pParam) return FF_FAIL;
    float val;
    memcpy(&val, &pParam->NewParameterValue, 4);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    switch (pParam->ParameterNumber) {
        case PARAM_SWITCH:  mSwitchNorm  = val; return FF_SUCCESS;
        case PARAM_DENSITY: mDensityNorm = val; return FF_SUCCESS;
        default: return FF_FAIL;
    }
}

char* TelemetryPlugin::GetParameterDisplay(DWORD index)
{
    switch (index) {
        case PARAM_SWITCH: {
            float sw = 1.0f - mSwitchNorm;
            unsigned p = (unsigned)(30.0f + sw * sw * 2970.0f);
            sprintf(mDisplayBuf, "%u fr", p);
            return mDisplayBuf;
        }
        case PARAM_DENSITY: {
            int n = 1 + (int)(mDensityNorm * (float)(MAX_ARTEFACTS - 1) + 0.5f);
            sprintf(mDisplayBuf, "%d panels", n);
            return mDisplayBuf;
        }
        default: return NULL;
    }
}

// ============================================================
//  Factory
// ============================================================
DWORD __stdcall CreateInstance(void** ppInstance)
{
    *ppInstance = new TelemetryPlugin();
    if (*ppInstance != NULL) return FF_SUCCESS;
    return FF_FAIL;
}

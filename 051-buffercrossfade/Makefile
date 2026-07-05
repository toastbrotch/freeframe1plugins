# ==============================================================
#  Makefile — BufferCrossfade FreeFrame 1.0 plugin
#  Cross-compile to 32-bit Windows DLL on Linux
#
#  Usage:
#    make          — build BufferCrossfade.dll
#    make clean    — remove build artefacts
#    make verify   — check plugMain export
# ==============================================================

CC      = i686-w64-mingw32-g++-win32

CFLAGS  = -O2 \
          -Wall \
          -Wno-unused-result \
          -Wno-attributes

LDFLAGS = -shared \
          -static-libgcc \
          -static-libstdc++ \
          -Wl,--kill-at \
          -lkernel32

TARGET  = BufferCrossfade.dll

SRCS    = BufferCrossfade.cpp \
          FFPluginInfoData.cpp \
          FFPluginSDK.cpp \
          FFPluginInfo.cpp \
          FFPluginManager.cpp \
          FreeFrame.cpp

OBJS    = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -Wl,--out-implib,lib$(TARGET:.dll=).a
	@echo ""
	@echo "  Built: $(TARGET)"
	@echo "  Copy to your Resolume FreeFrame folder."
	@echo ""

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

verify: $(TARGET)
	@echo "--- Exported symbols ---"
	i686-w64-mingw32-objdump -p $(TARGET) | grep -i plugmain || \
	    echo "WARNING: plugMain not found!"
	@echo "You should see: plugMain  (no @12 decoration)"

clean:
	rm -f $(OBJS) $(TARGET) libBufferCrossfade.a

.PHONY: all clean verify

# ==============================================================
#  Makefile — Rings FreeFrame 1.0 plugin
#  Cross-compile to 32-bit Windows DLL on Linux
#
#  Requirements (Debian/Ubuntu):
#    sudo apt install gcc-mingw-w64-i686 g++-mingw-w64-i686
#
#  SDK files to copy from Source/MSVC7/ into this folder:
#    FreeFrame.h / FreeFrame.cpp
#    FFPluginSDK.h / FFPluginSDK.cpp
#    FFPluginInfo.h / FFPluginInfo.cpp
#    FFPluginManager.h / FFPluginManager.cpp / FFPluginManager_inl.h
#
#  Also copy your patched FreeFrame.cpp and FreeFrame.h
#  (with the extern "C" fix applied) from the FrameDiff folder.
#
#  Usage:
#    make          — build Rings.dll
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
          -Wl,--kill-at

TARGET  = Rings.dll

SRCS    = Rings.cpp \
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
	rm -f $(OBJS) $(TARGET) libRings.a

.PHONY: all clean verify

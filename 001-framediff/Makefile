# ==============================================================
#  Makefile — cross-compile FrameDiff.dll on Linux
#  Target: 32-bit Windows DLL for Resolume 2.x (FreeFrame 1.0)
#
#  Requirements (Debian/Ubuntu):
#    sudo apt install gcc-mingw-w64-i686 g++-mingw-w64-i686
#
#  Usage:
#    make          — build FrameDiff.dll
#    make clean    — remove build artefacts
#    make verify   — check that plugMain is exported correctly
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

TARGET  = FrameDiff.dll

# ------------------------------------------------------------
#  Source files
#
#  YOUR plugin files:
#    FrameDiff.cpp
#    FFPluginInfoData.cpp     (your customised copy)
#
#  SDK files copied from Source/MSVC7/:
#    FFPluginSDK.cpp          (contains plugMain dispatcher)
#    FFPluginInfo.cpp
#    FFPluginManager.cpp
#    FreeFrame.cpp
# ------------------------------------------------------------
SRCS    = FrameDiff.cpp \
          FFPluginInfoData.cpp \
          FFPluginSDK.cpp \
          FFPluginInfo.cpp \
          FFPluginManager.cpp \
          FreeFrame.cpp

OBJS    = $(SRCS:.cpp=.o)

# ------------------------------------------------------------
#  Default target
# ------------------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ -Wl,--out-implib,lib$(TARGET:.dll=).a
	@echo ""
	@echo "  Built: $(TARGET)"
	@echo "  Copy it to your Resolume FreeFrame folder."
	@echo "  Rename to .frf if Resolume requires that extension."
	@echo ""

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------------------------
#  Verify the export is correct
# ------------------------------------------------------------
verify: $(TARGET)
	@echo "--- Exported symbols ---"
	i686-w64-mingw32-objdump -p $(TARGET) | grep -i plugmain || \
	    echo "WARNING: plugMain not found in exports!"
	@echo ""
	@echo "You should see:  plugMain   (without @12 decoration)"

# ------------------------------------------------------------
#  Clean
# ------------------------------------------------------------
clean:
	rm -f $(OBJS) $(TARGET) libframediff.a

.PHONY: all clean verify

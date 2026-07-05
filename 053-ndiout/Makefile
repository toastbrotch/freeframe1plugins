# ==============================================================
#  Makefile — NDIOut FreeFrame 1.0 plugin
#  Cross-compile to 32-bit Windows DLL on Linux
#
#  The NDI SDK headers are referenced from ../NDI_6_SDK/Include.
#  The NDI runtime DLL is loaded dynamically at runtime — no
#  NDI .lib is linked here.
#
#  Usage:
#    make          — build NDIOut.dll
#    make clean    — remove build artefacts
#    make verify   — check plugMain export
# ==============================================================

CC      = i686-w64-mingw32-g++-win32

NDI_INC = ../NDI_6_SDK/Include

CFLAGS  = -O2 \
          -Wall \
          -Wno-unused-result \
          -Wno-attributes \
          -Wno-unknown-pragmas \
          -I$(NDI_INC)

LDFLAGS = -shared \
          -static-libgcc \
          -static-libstdc++ \
          -Wl,--kill-at \
          -lkernel32

TARGET  = NDIOut.dll

SRCS    = NDIOut.cpp \
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
	@echo "  Requires NDI 6 runtime installed on the Windows machine."
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
	rm -f $(OBJS) $(TARGET) libNDIOut.a

.PHONY: all clean verify

# ==============================================================
#  Makefile — NDIIn FreeFrame 1.0 source plugin
#  Cross-compile to 32-bit Windows DLL on Linux
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

TARGET  = NDIIn.dll

SRCS    = NDIIn.cpp \
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

clean:
	rm -f $(OBJS) $(TARGET) libNDIIn.a

.PHONY: all clean

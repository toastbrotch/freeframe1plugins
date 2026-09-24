CXX      = i686-w64-mingw32-g++-win32
CXXFLAGS = -O2 -Wall -m32 \
           -DWIN32 -D_WINDOWS \
           -Wno-unused-variable \
           -Wno-write-strings

TARGET   = ISFBrowser.dll

SRCS = ISFBrowser.cpp \
       FFPluginInfoData.cpp \
       FFPluginSDK.cpp \
       FFPluginInfo.cpp \
       FFPluginManager.cpp \
       FreeFrame.cpp

OBJS = $(SRCS:.cpp=.o)

LDFLAGS  = -shared -static-libgcc -static-libstdc++ \
           -Wl,--kill-at \
           -lopengl32 -lgdi32 -lcomctl32

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

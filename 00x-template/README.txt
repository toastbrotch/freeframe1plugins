FrameDiff — FreeFrame 1.0 plugin
================================
Outputs the per-pixel absolute difference between the current
frame and a frame N steps in the past.

  Unchanged pixels  →  black
  Changed pixels    →  grey / white  (brighter = more change)

Parameter "Frame Delay":  knob 0.0 – 1.0  →  1 – 30 frames


YOUR FILES (in this folder)
---------------------------
FrameDiff.h             Plugin class declaration
FrameDiff.cpp           Plugin implementation
FFPluginInfoData.cpp    Plugin identity (name, ID, description)
FrameDiff.def           Linker exports  (MSVC only)
Makefile                Linux cross-compile build


SDK FILES TO COPY IN
--------------------
From the FreeFrame 1.0 SDK folder  Source/MSVC7/  copy these
files INTO THIS SAME FOLDER:

    FreeFrame.h
    FreeFrame.cpp
    FFPluginSDK.h
    FFPluginSDK.cpp         ← contains plugMain(), don't edit
    FFPluginInfo.h
    FFPluginInfo.cpp
    FFPluginManager.h
    FFPluginManager.cpp
    FFPluginManager_inl.h

Do NOT copy FFPluginInfoData.cpp from the SDK — you already
have a customised version of that file here.


BUILD ON LINUX  (cross-compile to Windows .dll)
------------------------------------------------
1. Install MinGW-w64:

       sudo apt install mingw-w64        # Debian / Ubuntu
       sudo pacman -S mingw-w64-gcc      # Arch
       sudo dnf install mingw32-gcc-c++  # Fedora

2. Copy the SDK source files listed above into this folder.

3. Run:

       make

   Output: FrameDiff.dll

4. Verify the export:

       make verify

   You should see "plugMain" without any @12 decoration.


BUILD ON WINDOWS  (Visual Studio)
----------------------------------
1. Create a new DLL project in Visual Studio (any version ≥ 2019).

2. Add ALL .cpp and .h files from this folder to the project,
   plus all the SDK .cpp/.h files listed above.

3. Project Settings:
     Configuration Type:  Dynamic Library (.dll)
     Platform:            Win32  (32-bit — required for Resolume 2.x)
     Runtime Library:     Multi-threaded (/MT)  — static, no DLL deps
     Module Definition:   FrameDiff.def

4. Build → FrameDiff.dll


INSTALL
-------
Copy FrameDiff.dll into Resolume's FreeFrame folder, e.g.:
    C:\Program Files\Resolume Avenue\FreeFrame\

Some Resolume 2.x versions expect .frf extension:
    rename FrameDiff.dll FrameDiff.frf   (Windows CMD)
    cp FrameDiff.dll FrameDiff.frf       (just copy, same bytes)

Restart Resolume. The effect appears in the effects list
as "FrameDiff".


NOTES
-----
- Only 24-bit RGB input is supported.
- The ring buffer holds 30 frames.  RAM usage at 24-bit RGB:
    320×240  (SD)   ≈  7 MB
    640×480         ≈  27 MB
    720×576  (PAL)  ≈  36 MB
- Processing is on the CPU; performance is resolution-dependent.
  SD runs easily on any hardware from that era.
- plugMain is NOT written by you — it lives in FFPluginSDK.cpp
  from the official SDK. That file dispatches all function codes
  to your class methods automatically.

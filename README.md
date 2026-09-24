# FreeFrame 1 Effects for Resolume 2.41

After several years of not playing any/much VJ-gigs [I](https://kompiuterzeugs.com/) recently re-started and went with [Resolume 2.41](https://resolume.com/download/file?file=resolume-2-41-installer.exe) from 2007, which I very much loved back in the days. I tried to find all the effects I used but started to miss some, so I built these [FreeFrame 1.0](https://freeframe.sourceforge.net/) effects for Resolume 2.41 together with [Claude](https://claude.ai). Cross-compiled to 32-bit Windows DLLs from Linux using MinGW.


[check here all Effects ready to download with previews](https://toastbrotch.github.io/freeframe1plugins/)


## Build environment setup

All plugins cross-compile to 32-bit Windows DLLs via MinGW-w64. Each Makefile calls the compiler by its exact name, `i686-w64-mingw32-g++-win32` (the win32-threads build, not posix), so install that specific variant:

**Debian / Ubuntu / Pop!_OS** (verified on Pop!_OS 24.04 / Ubuntu noble):
```` sudo apt install g++-mingw-w64-i686 mingw-w64-i686-dev ````

This installs both a `-win32` and a `-posix` threading variant and registers them with `update-alternatives`. Make sure `-win32` is the active one (it should be by default — higher priority):
```` update-alternatives --display i686-w64-mingw32-g++ ````
If it points at `-posix` instead, switch it with `update-alternatives --config i686-w64-mingw32-g++`.

Verify the toolchain is reachable:
```` i686-w64-mingw32-g++-win32 --version ````

**Arch**: `sudo pacman -S mingw-w64-gcc`
**Fedora**: `sudo dnf install mingw32-gcc-c++` (you may need to adjust the Makefile's `CXX` to match the binary name these distros ship)

Once the toolchain is installed, `cd` into any plugin folder and run `make` — see that folder's README for plugin-specific notes.


Notes:
* Find the build instructions in each plugin-folder/README.md
* Find the original FreeFrameSDK Version 1 in the FreeFrameSDK Folder
* howto convert Resolume 2.41 recordings to mp4 <br />
  ```` ffmpeg -i 2026-04-06-20-33-49.avi -c:v libx264 -crf 23 -c:a aac -pix_fmt yuv420p bla.mp4 ````


<br />

---

*FreeFrame 1.0 — built with MinGW i686 on Linux — tested in Resolume 2.41 on Windows 11 — made with Claude*

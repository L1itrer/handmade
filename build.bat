@echo off
mkdir build
pushd build
cl -MT -nologo -W4 -std:c++20 -Zi -D_CRT_SECURE_NO_WARNINGS ..\src\handmade.cpp /link -subsystem:windows,5.2 User32.lib Gdi32.lib Dsound.lib Xinput.lib
popd

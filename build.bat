@echo off
mkdir build
pushd build
cl -W4 -std:c++20 -Zi ..\src\handmade.cpp User32.lib Gdi32.lib Dsound.lib Xinput.lib
popd

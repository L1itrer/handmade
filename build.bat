@echo off
mkdir build
pushd build
cl -std:c++20 -Zi ..\src\handmade.cpp User32.lib Gdi32.lib
popd

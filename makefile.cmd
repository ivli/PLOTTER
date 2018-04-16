echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"

echo on
cl -Od /EHsc plotter.cpp parser.cpp
del plotter.obj parser.obj
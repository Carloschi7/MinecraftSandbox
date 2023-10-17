@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

git submodule init
git submodule update
cd dependencies\C7Engine
call "build.bat"
cd ..\..

mkdir build && cd build
cmake -DENGINE_PATH=%cd%\dependencies\C7Engine -DENGINE_BUILD_PATH=dependencies\C7Engine\build ..
msbuild MinecraftClone.sln
cd ..


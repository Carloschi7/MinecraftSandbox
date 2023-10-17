@echo off

git submodule init
git submodule update
cd dependencies\C7Engine
call "build.bat"
cd ..\..

mkdir build && cd build
cmake -DENGINE_PATH=%cd%\dependencies\C7Engine -DENGINE_BUILD_PATH=dependencies\C7Engine\build ..
msbuild MinecraftClone.sln
cd ..


@echo off

git submodule update --init
cd dependencies\C7Engine
git checkout master
call "build.bat" NO_ASSIMP
cd ..\..

set main_dir=%cd%
mkdir build && cd build
cmake -DENGINE_PATH=%main_dir%\dependencies\C7Engine -DENGINE_BUILD_PATH=%main_dir%\dependencies\C7Engine\build -DSTANDALONE=1 -DAPP_NAME=MinecraftSandbox ..
msbuild MinecraftSandbox.sln /property:Configuration=Release 
cd ..


@echo off

git submodule update --init

set main_dir=%cd%
set engine_dir="%main_dir%\dependencies\C7Engine"

cd %engine_dir%
git checkout master
cd %main_dir%

mkdir build && cd build
if "%1" == "standalone" (
	cd %engine_dir%
	call "build.bat" NO_ASSIMP STANDALONE
	cd %main_dir%
	mkdir build
	cd build	
	cmake -DENGINE_PATH=%main_dir%\dependencies\C7Engine -DENGINE_BUILD_PATH=%main_dir%\dependencies\C7Engine\build -DSTANDALONE=1 -DAPP_NAME=MinecraftSandbox ..
) else (
	cd %engine_dir%
	call "build.bat" NO_ASSIMP
	cd %main_dir%
	mkdir build
	cd build
	cmake -DENGINE_PATH=%main_dir%\dependencies\C7Engine -DENGINE_BUILD_PATH=%main_dir%\dependencies\C7Engine\build -DAPP_NAME=MinecraftSandbox ..
)
msbuild MinecraftSandbox.sln /property:Configuration=Release 
cd %main_dir%

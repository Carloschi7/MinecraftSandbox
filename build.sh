#/bin/bash

git submodule update --init
cd dependencies/C7Engine
git checkout master
git pull
sudo chmod +x build.sh
./build.sh
cd ../..

current_path=$(pwd)
mkdir debug && cd debug
cmake -DENGINE_PATH=$current_path/dependencies/C7Engine -DENGINE_BUILD_PATH=$current_path/dependencies/C7Engine/debug -DAPP_NAME=MinecraftSandbox ..
make
cd ..

mkdir release && cd release
cmake -DENGINE_PATH=$current_path/dependencies/C7Engine -DENGINE_BUILD_PATH=$current_path/dependencies/C7Engine/release -DCMAKE_BUILD_TYPE=Release -DAPP_NAME=MinecraftSandbox ..
make
cd ..

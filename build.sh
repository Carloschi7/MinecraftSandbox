#/bin/bash

git submodule init
git submodule update
cd dependencies/C7Engine
sudo chmod +x build.sh
./build.sh
cd ../..

current_path=$(pwd)
mkdir debug && cd debug
cmake -DENGINE_PATH=$current_path/dependencies/C7Engine -DENGINE_BUILD_PATH=$current_path/dependencies/C7Engine/debug ..
make
cd ..

mkdir release && cd release
cmake -DENGINE_PATH=$current_path/dependencies/C7Engine -DENGINE_BUILD_PATH=$current_path/dependencies/C7Engine/release ..
make
cd ..

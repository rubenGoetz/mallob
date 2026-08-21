#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="1b7b107cf7cd1ae06a3f32d4b11a08c71e7b9b74" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DDRUP_TO_LRUP_CONVERSION=1
make -j
cd ..
echo "[$dirname] Build complete"

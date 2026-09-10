#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="48bc4d4275769fdd7200f1af1cd2c8e428f26c94" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
cd ..
echo "[$dirname] Build complete"

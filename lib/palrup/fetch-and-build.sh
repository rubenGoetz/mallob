#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="0b13f87236b3b48e229617dfe95d483665f545e7" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
cd ..
echo "[$dirname] Build complete"

#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="0b62502607784909ff4474154c48d6ddcda46388" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
cd ..
echo "[$dirname] Build complete"

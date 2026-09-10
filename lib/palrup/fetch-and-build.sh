#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="ecec228f8196e0aa536d498a81683e0b3f24020a" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
cd ..
echo "[$dirname] Build complete"

#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="9ba2641d61beca1b20279a0103c47d70ced56b3d" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
cd ..
echo "[$dirname] Build complete"

#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="8415f4e22ca311e9b321ae482a5722a570dfb6a0" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
chmod +x pal_launcher.sh
cd ..
echo "[$dirname] Build complete"

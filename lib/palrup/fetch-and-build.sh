#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="d91690c3558ae29dee06d7f6cfa617eb795287c4" # updated 2026-07-08
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

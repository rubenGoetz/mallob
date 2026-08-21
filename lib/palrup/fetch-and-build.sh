#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="da4f074444bb4ab6157f34d1b54689e61d8df74e" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DDRUP_TO_LRUP_CONVERSION=1
make -j
cd ..
echo "[$dirname] Build complete"

#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="ced0b4c32a79fb8c5dd1da95ff2a70badc6117f2" # updated 2026-07-08
fetch_and_extract $dirname CMakeLists.txt https://github.com/rubenGoetz/PalRUP-Check/archive/${branchorcommit}.zip

sed -i 's/-Werror//g' CMakeLists.txt

echo "[$dirname] Building ..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make -j
chmod +x pal_launcher.sh
chmod +x pal.sh
cd ..
echo "[$dirname] Build complete"

if ! [ -z "$1" ]; then
    echo "[$dirname] cp compress-proof.sh $1/"
    cp compress-proof.sh "$1/"
fi

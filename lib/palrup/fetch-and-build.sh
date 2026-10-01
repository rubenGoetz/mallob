#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="99abec6d765a8da38109756fa586b30f949ef687" # updated 2026-07-08
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

#!/bin/bash

source ../base-build-functions.sh
dirname="PalRUP-Check"

branchorcommit="56ad2d8b1aa8700fe3ce1c7091a0ec4381ac241f" # updated 2026-07-08
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

#!/bin/bash

source ../base-build-functions.sh
dirname="kissat"

branchorcommit="cc0dc45e2a2e1e5d7c97fd7855eb3d2e8d0d4392" # updated 2026-07-11
fetch_and_extract $dirname configure https://github.com/rubenGoetz/kissat/archive/${branchorcommit}.zip

echo "[kissat] Building ..."
./configure -O3
make -j
echo "[kissat] Build complete"

#!/bin/bash

source ../base-build-functions.sh
dirname="kissat"

branchorcommit="5cb25f62e25fede21ac579b898958e58eaf89a82" # updated 2026-07-11
fetch_and_extract $dirname configure https://github.com/rubenGoetz/kissat/archive/${branchorcommit}.zip

echo "[kissat] Building ..."
./configure -O3
make -j
echo "[kissat] Build complete"

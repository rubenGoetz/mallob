#!/bin/bash

source ../base-build-functions.sh
dirname="kissat"

branchorcommit="bc8833b423d40f7c153d1a0672f665d60cf21eba" # updated 2026-07-11
fetch_and_extract $dirname configure https://github.com/rubenGoetz/kissat/archive/${branchorcommit}.zip

echo "[kissat] Building ..."
./configure -O3
make -j
echo "[kissat] Build complete"

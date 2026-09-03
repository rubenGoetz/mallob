#!/bin/bash

source ../base-build-functions.sh
dirname="kissat"

branchorcommit="f769a0a5bb91b50b6160d1cb8791cbb0ba1e6312" # updated 2026-07-11
fetch_and_extract $dirname configure https://github.com/rubenGoetz/kissat/archive/${branchorcommit}.zip

echo "[kissat] Building ..."
./configure -O3
make -j
echo "[kissat] Build complete"

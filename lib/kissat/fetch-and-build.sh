#!/bin/bash

source ../base-build-functions.sh
dirname="kissat"

branchorcommit="5cb25f62e25fede21ac579b898958e58eaf89a82" # updated 2026-09-02
fetch_and_extract $dirname configure https://github.com/rubenGoetz/kissat/archive/${branchorcommit}.zip
# Niccos Sweep Kissat
# branchorcommit="d4e76a387c93b28bbe84a2db043008e5bc70b185" #FMCAD26 Artifact Commit
# fetch_and_extract $dirname configure https://github.com/nrilu/kissat/archive/${branchorcommit}.zip

echo "[kissat] Building ..."
./configure -O3
make -j
echo "[kissat] Build complete"

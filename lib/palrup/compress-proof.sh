#!/bin/bash

mode="$1" # XZ or VASKIN_GOETZ
input="$2" # path to pipe (already set up)
output="$3" # path to plain file
instance="$4" # path to formula cnf
nb_threads="$5" # number of solver threads
vg_exe="$6" # path to the xVASKIN_GOETZ executable

if [ "x$mode" == "xXZ" ]; then
    xz -k -z -c -T 1 "$input" > "$output"
elif [ "x$mode" == "xVASKIN_GOETZ" ]; then
    if [[ ! -f $vg_exe ]]; then
        echo "--ERROR-- valid compression executable needed - exiting"
        exit 1
    fi
    $vg_exe encode --threads "$nb_threads" "$input" "$instance" "$output"
else
    echo "--WARNING-- Unknown proof compression mode \"$mode\" - defaulting to no compression" 
    cat "$input" > "$output"
fi

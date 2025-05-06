#!/bin/bash

benchmark_dir=$1
log_dir=$2
trace_dir=$3

RDMAV_FORK_SAFE=1
NPROCS=2
THREADS=4
TIMEOUT=300s

echo "** begin benchmarks for instances in $benchmark_dir"

for file in $benchmark_dir/*; do 
    if [ -f "$file" ]; then 

        file_name=$(basename $file)
        echo "* run benchmark for $file_name"

        if [ ! -d "$log_dir/$file_name" ]; then
            mkdir $log_dir/$file_name
        fi

        # rather multiple jobs with own timeout?
        { time -p timeout $TIMEOUT mpirun -np $NPROCS --bind-to core --map-by ppr:${NPROCS}:node:pe=4 build/mallob -mono=$file -satsolver='s' -trace-dir=$trace_dir/$file_name -log=$log_dir/$file_name -t=$THREADS > $log_dir/$file_name/out_file; } 2>> $log_dir/$file_name/out_file

    fi 
done

echo "** finished benchmarks for instances in $benchmark_dir"

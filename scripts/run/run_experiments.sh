#!/bin/bash

benchmark_dir=~/masterarbeit/mallob/instances/sat-instances-for-small-tests/
log_dir=logs
trace_dir=logs
solver_str=k_

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
        { time -p timeout $TIMEOUT mpirun -np $NPROCS --bind-to core --map-by ppr:${NPROCS}:node:pe=$THREADS build/mallob -mono=$file -satsolver=$solver_str -pb=0 -pjp=999999 -pef=1 -mono-app=SATWITHPRE -rlbd=3 -ilbd=0 -trace-dir=$trace_dir/$file_name -log=$log_dir/$file_name -t=$THREADS > $log_dir/$file_name/out_file; } 2>> $log_dir/$file_name/out_file

    fi 
done

echo "** finished benchmarks for instances in $benchmark_dir"

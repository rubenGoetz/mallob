#!/bin/bash
#SBATCH --nodes=$DS_NODES
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=48
#SBATCH --ntasks-per-core=2 # enables hyperthreading
#SBATCH -t $DS_RUNTIME_SLURMSTR
#SBATCH -p $DS_PARTITION # general or micro
#SBATCH --account=$DS_PROJECTNAME
#SBATCH -J $DS_JOBNAME
#SBATCH --ear=off # Needed for profiling / benchmarking
#SBATCH --switches=1 # Force a single island

# SBATCH --ear-mpi-dist=openmpi # For OpenMPI

# SuperMUC has TWO processors with 24 physical cores each, totalling 48 physical cores (96 hwthreads)
# See: https://doku.lrz.de/download/attachments/43321076/SuperMUC-NG_computenode.png

module load slurm_setup; module unload devEnv/Intel/2019 intel-mpi; module load gcc/14.3.0 intel-mpi/default cmake/3.31.11 gdb/15.2 mpifileutils

username="$DS_USERNAME"
projname="$DS_PROJECTNAME"
starttime=$(date +%s)

# For debugging
hostname
module list
which mpirun
echo "#ranks: $SLURM_NTASKS"

build="build" # TODO your build directory for Mallob

# Environment variables
export PATH="$build/:$PATH"
export RDMAV_FORK_SAFE=1
export MALLOC_CONF="thp:always"

# HOME
globallogdir_base=logs/$DS_JOBNAME-$SLURM_JOB_ID
# WORK
if [ -d "$WORK_$DS_PROJECTNAME" ]; then globallogdir_base="$WORK_$DS_PROJECTNAME/$globallogdir_base"; fi
# SCRATCH
#if [ -d "$SCRATCH" ]; then globallogdir_base="$SCRATCH/$globallogdir_base"; fi

# Directories for writing and for storing logs
localtmpdir_base=${SCRATCH}/tmp/$DS_JOBNAME-$SLURM_JOB_ID # fast (?) disk
mkdir -p $localtmpdir_base $globallogdir_base

# Benchmark instances, one per line
#benchmarkfile="/hppfs/work/$projname/$username/instances/2023+2024-unique.txt" # TODO benchmark file
#benchmarkfile="/dss/dsshome1/06/di97xah/small_unsat_benchmarks.txt"
benchmarkfile="/hppfs/work/pn72pu/di97xah/main_2025/unsat_solved.txt"
if [ ! -f $benchmarkfile ]; then
    echo "Benchmark file not found!"
    exit 1
fi

# Failsafe: Exit if an unreasonable number of jobs of this kind have launched
echo "I" >> sbatch/generated/$DS_JOBNAME/.ticks
if [ $(cat sbatch/generated/$DS_JOBNAME/.ticks|wc -l) -gt $(($DS_LASTJOBIDX - $DS_FIRSTJOBIDX + 1)) ]; then exit; fi

change=false # track if this task makes any progress and should hence start another task
lockfile=sbatch/generated/$DS_JOBNAME/.lock
idfile=sbatch/generated/$DS_JOBNAME/.remaining_ids

while [ $(( $(date +%s) - $starttime + $DS_SECONDSPERJOB + 30 )) -lt $DS_RUNTIME ]; do

    # obtain lock
    while ! mkdir $lockfile 2> /dev/null ; do
        if [ $(( $(date +%s) - $starttime + $DS_SECONDSPERJOB + 30 )) -gt $DS_RUNTIME ]; then
            break # timeout
        fi
        sleep 0.1
    done
    # lock held

    # exit if you may be unable to finish another job in time
    if [ $(( $(date +%s) - $starttime + $DS_SECONDSPERJOB + 30 )) -gt $DS_RUNTIME ]; then
        rmdir $lockfile # release lock
        break
    fi

    # extract next job from file of remaining instance IDs (should be constant-time)
    i=$(tail -n 1 $idfile)
    truncate -s -$((${#i} + 1)) $idfile # +1 for the newline

    rmdir $lockfile # release lock

    if [ -z "$i" ]; then
        # ID file is empty - all jobs should be done
        break
    fi

    change=true

    # Benchmark file
    f=$(cat $benchmarkfile|sed $i'q;d')

    localtmpdir="${localtmpdir_base}/tmp/$i" # make sure that proof and disk data are written on FAST, local disk
    globallogdir="${localtmpdir_base}/$i"
    outputlogdir="${globallogdir_base}"
    proof_palrup="${SCRATCH}/$DS_JOBNAME-$SLURM_JOB_ID"
    proof_working="/hppfs/work/$projname/$username/palrup/$DS_JOBNAME-$SLURM_JOB_ID"

    echo "logdir: $globallogdir , localtmp: $localtmpdir , output: $outputlogdir"

    # TODO Configure Mallob
    internal_jobs=9	# solving, drup, conversion, lrup
    timeout=$(($DS_SECONDSPERJOB/$internal_jobs))
    cmd="$build/mallob -mono-app=SAT -pb=1 -pjp=999999 -pef=1 -mono=$f -mjwl=$timeout -jwl=$((timeout*2)) -T=$((($timeout*$internal_jobs)+30)) -wam=60``000 -pre-cleanup=1 \
    -q=1 -log=$globallogdir -tmp=$localtmpdir -comment-outputlogdir=$outputlogdir -sro=${globallogdir}/processed-jobs.out -trace-dir=${globallogdir}/ -os=1 -v=4 -iff=0 -s2f=${globallogdir}/model -cm=0 \
    -rpa=1 -pph=${SLURM_NTASKS_PER_NODE} -mlpt=20``000``000 -t=$((${SLURM_CPUS_PER_TASK} / 2)) \
    -satsolver=k_ -isp=0 -div-phases=1 -div-noise=0 -div-seeds=1 -div-elim=0 -div-native=0 -scsd=0 \
    -scll=60 -slbdl=60 -qcll=60 -qlbdl=60 -csm=3 -cfm=3 -cfci=30 -mscf=5 -bem=1 -aim=1 -rlbd=0 -ilbd=1 -randlbd=0 -scramble-lbds=0 -seed=0 \
    -palrup=1 -proof-dir=$proof_palrup -palrup-binary=1 -palrup-drup=1 \
    -palrup-sequence=dclD -palrup-check-dir=$proof_working -palrup-clean=0"

    # Pre-create network-disk output directories to avoid many concurrent filesystem manips
    mkdir -p $(for rank in $(seq 0 $(($SLURM_NTASKS-1))); do echo $outputlogdir/$i/$rank; done)

    export MALLOB_GLOBALLOGDIR=$globallogdir
    export MALLOB_LOCALTMPDIR=$localtmpdir
    export MALLOB_OUTPUTLOGDIR=$outputlogdir
    export MALLOB_BUILDDIR=$build
    export MALLOB_NUMNODES=$DS_NODES
    export PROOF_PALRUP=$proof_palrup
    export PROOF_WORKING=$proof_working

    # Assemble MPI command options
    mpicall="mpiexec -n $SLURM_NTASKS --bind-to core --map-by numa -genvall"
    mpicleanup="mpiexec -n $((SLURM_NTASKS*SLURM_CPUS_PER_TASK)) drm --traceless --lite --aggressive"
    mpicopy="mpiexec -n $((SLURM_NTASKS*SLURM_CPUS_PER_TASK)) dcp"

    # Drop hint which file we're solving
    echo "$f -> $globallogdir"

    # Launch
    echo "$(date) JOB $i LAUNCHING"

    echo $mpicall $cmd
    $mpicall bash -c "scripts/slurm/prolog.sh ; $cmd"

    sleep 3 # avoid "nodes are still busy" issue?
    echo "EPILOG"
    echo "$mpicopy $globallogdir $outputlogdir"
    $mpicopy $globallogdir $outputlogdir
    mkdir "$outputlogdir/$i/.alldone"
    echo "$mpicleanup $globallogdir"
    $mpicleanup $globallogdir

    echo "CLEANUP"
    echo "$mpicleanup /hppfs/work/$projname/$username/palrup/$DS_JOBNAME-$SLURM_JOB_ID"
    $mpicleanup /hppfs/work/$projname/$username/palrup/$DS_JOBNAME-$SLURM_JOB_ID
    echo "$mpicleanup $SCRATCH/$DS_JOBNAME-$SLURM_JOB_ID"
    $mpicleanup $SCRATCH/$DS_JOBNAME-$SLURM_JOB_ID
    

    echo "$(date) JOB $i FINISHED"

    sleep 3 # maybe a means to avoid "nodes are still busy" srun error?

done # END OF MAIN LOOP

# No progress made? => The chain is (about to be) done, so don't submit another task
if ! $change; then exit; fi

# Submit next instance, to begin after this job is done (do nothing if it fails)
sbatch --begin=now`#+$(($DS_RUNTIME+30))` sbatch/generated/$DS_JOBNAME/sbatch.sh

# clean up lod dir skeletons
$mpicleanup $localtmpdir_base


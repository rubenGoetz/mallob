#!/bin/bash

build=$MALLOB_BUILDDIR
globallogdir=$MALLOB_GLOBALLOGDIR
localtmpdir=$MALLOB_LOCALTMPDIR
outputlogdir=$MALLOB_OUTPUTLOGDIR
numnodes=$MALLOB_NUMNODES
proof_palrup=$PROOF_PALRUP
proof_working=$PROOF_WORKING

if [ -z "$globallogdir" ]; then exit; fi
if [ -z "$localtmpdir" ]; then exit; fi

# Only a single process on each node gets to run this cleanup script
if ! mkdir /tmp/.epilog.lock 2>/dev/null ; then exit ; fi

# Assemble destination directory path, exit if everything's already done
dest="$outputlogdir/$(basename $globallogdir)"
if [ -f "$dest/.alldone" ]; then exit ; fi

>&2 echo "$(date) EPILOG $(hostname): $build $globallogdir $outputlogdir"

# move global log dir to output log dir
prevdir=$(pwd)
cd "$globallogdir"
for x in * ; do
    # enable merging of pal hierarchy
    if [[ $x == "pals" ]]; then
        for pal in pals/*/* ; do
            mkdir -p "$dest/$pal"
            mv $pal/* "$dest/$pal" &
        done
        continue
    fi
    if [ -d $x ]; then mkdir -p "$dest/$x";  mv $x/* "$dest/$x" & : ; fi
    if [ -f $x ]; then mv $x "$dest/" & : ; fi
done
wait
cd $prevdir

# Clean up local proof data
rm -r "${proof_palrup}" 2>/dev/null

# Clean up global checking data
rm -r "${proof_working}" 2>/dev/null

# Barrier across hosts (note that we clean up the lock directory afterwards)
touch "$dest/.done.$(hostname)"
while [ $(ls $dest/.done.* | wc -l) -lt $numnodes ]; do sleep 1; done
touch "$dest/.alldone"

# Clean up orphaned processes and tmp directory
killall -9 $build/mallob $build/mallob_sat_process $build/mallob_process_dispatcher 2>/dev/null || :
rm -rf ${localtmpdir} /dev/shm/edu.kit.iti.mallob.* /tmp/.epilog.lock 2>/dev/null || :

>&2 echo "$(date) END EPILOG $(hostname)"

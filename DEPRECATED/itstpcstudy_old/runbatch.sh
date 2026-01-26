#!/bin/bash
#script for multiple sim runs
echo "Bash version ${BASH_VERSION}..."
nprocesses=`cat NumberOfProcesses`
echo "Number of processes to keep alive: $nprocesses"
for i in {00000..02100}
do
  echo "Batch run: $i"
  ./micro.sh $i %> log_${i}.txt &

  while [ $(ls subJobLock_* | wc -l) -gt $nprocesses ]; do
    sleep 2s;
    nprocesses=`cat NumberOfProcesses`
  done
done

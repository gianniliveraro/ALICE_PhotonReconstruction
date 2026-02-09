#!/bin/bash

SEED=$1 # seed will be based on the batch number 
CPU_LIMIT=$2
MEM_LIMIT=$3
WORKFLOWFILE=$4
Subdirectory=$5

touch subJobLock_${1}
mkdir ${1}
cp runSimulation.sh ${1}/.
cp configs.sh ${1}/.
cp configParticleGun.ini ${1}/.
cp configCustomParticleGun.cfg ${1}/.
cp generator_pythia8_gun.C ${1}/.
cp ALICEStandard_Run3.cmnd ${1}/.
cd ${1}

echo "We are at batch `pwd`"

startdate=$(date)
source runSimulation.sh $CPU_LIMIT $MEM_LIMIT $WORKFLOWFILE $Subdirectory $SEED

COUNTER=0
until [ -f tf1/AO2D.root ]
do
    currentdate=$(date)
    echo "Processing started: ${startdate}, currently ${currentdate}..."
    sleep 5
    let COUNTER=COUNTER+5
    if [ "$COUNTER" -gt "4000" ]; then
      echo "This is taking too long and should be finished by now, breaking"
      break
      rm -rf tf*
      cd ../..
      rm subJobLock_${1}
    fi
done
echo "File tf1/AO2D.root found, proceeding"
rm -rf tf*/*_Hits???.root
cd ..
rm subJobLock_${1}

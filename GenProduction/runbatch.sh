#!/bin/bash

# Loading variables from config.sh
source "./configs.sh"

OutputDir=$1
Subdirectory=$2

[ ! "${O2DPG_ROOT}" ] && echo "Error: This needs O2DPG loaded" && exit 1
[ ! "${O2_ROOT}" ] && echo "Error: This needs O2 loaded" && exit 1

# ----------- LOAD UTILITY FUNCTIONS --------------------------
. ${O2_ROOT}/share/scripts/jobutils.sh

# -----------  CREATING OUTPUT DIRECTORIES  --------------------------
mkdir -p "../OutputData/${OutputDir}"
mkdir -p "../GenProduction/${OutputDir}"

# Copy key scripts/files to /GenProduction/OutputDir/
cp runSimulation.sh ../GenProduction/${OutputDir}/.
cp runbatch.sh ../GenProduction/${OutputDir}/.
cp configParticleGun.ini ../GenProduction/${OutputDir}/.
cp configCustomParticleGun.cfg ../GenProduction/${OutputDir}/.
cp ALICEStandard_Run3.cmnd ../GenProduction/${OutputDir}/.
cp generator_pythia8_gun.C ../GenProduction/${OutputDir}/.  
cp FilesBeforeITSTPCMatchingToDelete.txt ../GenProduction/${OutputDir}/.
cp FilesAfterITSTPCMatchingToDelete.txt ../GenProduction/${OutputDir}/.
cp micro.sh ../GenProduction/${OutputDir}/.
cp NumberOfProcesses ../GenProduction/${OutputDir}/.
cp stopall.sh ../GenProduction/${OutputDir}/.
cp configs.sh ../GenProduction/${OutputDir}/.

# Go to working directory
cd ${OutputDir}/

# -----------  RUNNING SIMULATION BATCHES --------------------------

nprocesses=`cat NumberOfProcesses`
echo "Number of processes to keep alive: $nprocesses"
echo "Number of batches: $NBATCHES"

for ((i=0; i<NBATCHES; i++));do
  echo "Batch run: $i"
  ./micro.sh $i $CPU_LIMIT $MEM_LIMIT $WORKFLOWFILE %> log_${i}.txt &

  while [ $(ls subJobLock_* | wc -l) -gt $nprocesses ]; do
    sleep 2s;
    nprocesses=`cat NumberOfProcesses`
  done
done


# -----------  POST-PROCESSING --------------------------
# Create output directory
mkdir -p "../../OutputData/${OutputDir}/${Subdirectory}"

# Move workflow.json
i=1
find */ -maxdepth 1 -type f -name "workflow.json" -print0 |
while IFS= read -r -d '' file; do
  cp "$file" "../../OutputData/${OutputDir}/${Subdirectory}/workflow_${i}.json"
  ((i++))
done


echo "Waiting for all simulations to finish (subJobLock files are still available!)..."
while ls subJobLock_* 1>/dev/null 2>&1; do
  sleep 10
done

echo "All simulations finished. Starting post-processing."
echo "Moving output files to OutputData and cleaning up temporary files..."

#-----------
# Move AO2Ds
i=1
find */ -maxdepth 1 -type f -name AO2D.root -print0 | while IFS= read -r -d '' file; do
  mv "$file" "../../OutputData/${OutputDir}/${Subdirectory}/AO2D_${i}.root"
  ((i++))
done

# Delete irrelevant files to save disk
while IFS= read -r pattern; do
  find */ -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null
done < FilesBeforeITSTPCMatchingToDelete.txt

while IFS= read -r pattern; do
  find */ -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null
done < FilesAfterITSTPCMatchingToDelete.txt

# Kill all zombie processes
bash stopall.sh
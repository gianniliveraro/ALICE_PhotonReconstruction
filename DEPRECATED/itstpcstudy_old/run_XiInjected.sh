#!/bin/bash

#
# A example workflow MC->RECO->AOD for a simple pp min bias
# production, targetting test beam conditions.

# make sure O2DPG + O2 is loaded
export O2DPG_ROOT=/storage1/daviddc/alice/O2DPG
export CURRENTDIR=`pwd`

[ ! "${O2DPG_ROOT}" ] && echo "Error: This needs O2DPG loaded" && exit 1
[ ! "${O2_ROOT}" ] && echo "Error: This needs O2 loaded" && exit 1

# ----------- LOAD UTILITY FUNCTIONS --------------------------
. ${O2_ROOT}/share/scripts/jobutils.sh

# ----------- START ACTUAL JOB  -----------------------------

NWORKERS=${NWORKERS:-1}
MODULES="--skipModules ZDC"
SIMENGINE=${SIMENGINE:-TGeant3}
NSIGEVENTS=${NSIGEVENTS:-200}
NTIMEFRAMES=${NTIMEFRAMES:-1}
INTRATE=${INTRATE:-500000}
SYSTEM=${SYSTEM:-pp}
ENERGY=${ENERGY:-13500}
[[ ${SPLITID} != "" ]] && SEED="-seed ${SPLITID}" || SEED=""

echo "Create workflow"
echo "Current directory: ${CURRENTDIR}"
# create workflow
${O2DPG_ROOT}/MC/bin/o2dpg_sim_workflow.py -eCM ${ENERGY} -col ${SYSTEM} -gen external -j ${NWORKERS} -ns ${NSIGEVENTS} -tf ${NTIMEFRAMES} -confKey "Diamond.width[2]=6." -e ${SIMENGINE} ${SEED} -mod "--skipModules ZDC" -ini /storage1/daviddc/xigun/configPythia.ini -field -5

# run workflow
echo "Apply cuts"
python3 apply_cuts_to_json.py
echo "Actually run"
${O2DPG_ROOT}/MC/bin/o2_dpg_workflow_runner.py -f workflow_mod.json -tt aod --cpu-limit 6 --mem-limit 200000

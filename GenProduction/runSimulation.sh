#!/bin/bash
source "./configs.sh"

[ ! "${O2DPG_ROOT}" ] && echo "Error: This needs O2DPG loaded" && exit 1
[ ! "${O2_ROOT}" ] && echo "Error: This needs O2 loaded" && exit 1

# ----------- LOAD UTILITY FUNCTIONS --------------------------
. ${O2_ROOT}/share/scripts/jobutils.sh

CPU_LIMIT=$1
MEM_LIMIT=$2
WORKFLOWFILE=$3
  
# create workflow
${O2DPG_ROOT}/MC/bin/o2dpg_sim_workflow.py -eCM ${ENERGY} -col ${SYSTEM} -gen external -j ${NWORKERS} -ns ${NSIGEVENTS} -tf ${NTIMEFRAMES} -confKey "Diamond.width[2]=6." -e ${SIMENGINE} -seed "42"  --skipModules "ZDC" -ini ${CONFIGfILE} -field -5 -interactionRate ${INTRATE}  

echo "Running simulation at `pwd`. Fasten your seatbelts!"
echo "Current directory: ${PWD}"
${O2DPG_ROOT}/MC/bin/o2_dpg_workflow_runner.py -f $WORKFLOWFILE -tt aod --cpu-limit $CPU_LIMIT --mem-limit $MEM_LIMIT 
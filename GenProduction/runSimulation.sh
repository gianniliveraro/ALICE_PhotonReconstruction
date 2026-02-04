#!/bin/bash
source "./configs.sh"

[ ! "${O2DPG_ROOT}" ] && echo "Error: This needs O2DPG loaded" && exit 1
[ ! "${O2_ROOT}" ] && echo "Error: This needs O2 loaded" && exit 1

# ----------- LOAD UTILITY FUNCTIONS --------------------------
. ${O2_ROOT}/share/scripts/jobutils.sh

CPU_LIMIT=$1
MEM_LIMIT=$2
WORKFLOWFILE=$3
CURRENTSIMDIR=`pwd`
Subdirectory=$4

# Only create workflow if it is the reference run
if [[ $(basename "$Subdirectory") == Reference ]]; then
    # create workflow (using .ini file)
    #${O2DPG_ROOT}/MC/bin/o2dpg_sim_workflow.py -eCM ${ENERGY} -col ${SYSTEM} -gen external -j ${NWORKERS} -ns ${NSIGEVENTS} -tf ${NTIMEFRAMES} -confKey "Diamond.width[2]=6." -e ${SIMENGINE} -seed "42"  --skipModules "ZDC" -ini ${CONFIGfILE} -field -5 -interactionRate ${INTRATE}  

    # create workflow (using pythia .cmnd file)
    ${O2DPG_ROOT}/MC/bin/o2dpg_sim_workflow.py -eCM ${ENERGY} -col ${SYSTEM} -gen pythia8 -j ${NWORKERS} -ns ${NSIGEVENTS} -tf ${NTIMEFRAMES} -confKey "Diamond.width[2]=6.;GeneratorPythia8.config=${CURRENTSIMDIR}/../ALICEStandard_Run3.cmnd" -e ${SIMENGINE} -seed "42"  --skipModules "ZDC" -field -5 -interactionRate ${INTRATE}  

fi

echo "Running simulation at `pwd`. Fasten your seatbelts!"
echo "Current directory: ${PWD}"
${O2DPG_ROOT}/MC/bin/o2_dpg_workflow_runner.py -f $WORKFLOWFILE -tt aod --cpu-limit $CPU_LIMIT --mem-limit $MEM_LIMIT 
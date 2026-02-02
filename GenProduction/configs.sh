# This is the place to set all the configuration variables for the simulation!
export O2DPG_ROOT=/storage1/liveraro/alice/O2DPG
export CURRENTDIR=`pwd`

WORKFLOWFILE="workflow.json" 

CPU_LIMIT=64
MEM_LIMIT=200000000
NBATCHES=10

NWORKERS=${NWORKERS:-32}
MODULES="--skipModules ZDC"
CONFIGfILE="../configParticleGun.ini"
SIMENGINE=${SIMENGINE:-TGeant3}
NSIGEVENTS=${NSIGEVENTS:-500}
NTIMEFRAMES=${NTIMEFRAMES:-5}
INTRATE=${INTRATE:-500000}
SYSTEM=${SYSTEM:-pp}
ENERGY=${ENERGY:-13600}
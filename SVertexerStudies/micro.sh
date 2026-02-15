#!/bin/bash
set -euo pipefail

SEED="${1:?Missing BATCHID}"
CPU_LIMIT="${2:?Missing CPU_LIMIT}"
MEM_LIMIT="${3:?Missing MEM_LIMIT}"
WORKFLOWFILE_IN="${4:?Missing WORKFLOWFILE}"
Subdirectory="${5:?Missing Subdirectory}"
SYSTEMTAG="${6:-pp}"

if [[ "${SYSTEMTAG}" == "pbpb" ]]; then
  SYSTEMTAG="PbPb"
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

LOCKFILE="subJobLock_${SEED}"
touch "${LOCKFILE}"
cleanup() { rm -f "${LOCKFILE}" 2>/dev/null || true; }
trap cleanup EXIT

mkdir -p "${SEED}"

# Copy required scripts/configs into the batch folder
cp -f "${SCRIPT_DIR}/runSimulation.sh"            "${SEED}/."
cp -f "${SCRIPT_DIR}/configs_pp.sh"               "${SEED}/." 2>/dev/null || true
cp -f "${SCRIPT_DIR}/configs_PbPb.sh"             "${SEED}/." 2>/dev/null || true

# Create configs.sh in the batch folder
if [[ "${SYSTEMTAG}" == "pp" ]]; then
  cp -f "${SCRIPT_DIR}/configs_pp.sh" "${SEED}/configs.sh"
elif [[ "${SYSTEMTAG}" == "PbPb" ]]; then
  cp -f "${SCRIPT_DIR}/configs_PbPb.sh" "${SEED}/configs.sh"
else
  echo "FATAL: unknown SYSTEMTAG '${SYSTEMTAG}'. Use 'pp' or 'PbPb'."
  exit 1
fi

# Resolve workflow abs path before cd (may be missing for Reference)
WORKFLOWFILE_ABS=""
if [[ -f "${WORKFLOWFILE_IN}" ]]; then
  WORKFLOWFILE_ABS="$(readlink -f "${WORKFLOWFILE_IN}")"
fi

# Ensure batch-local workflow.json for Tests
if [[ "${Subdirectory}" != "Reference" ]]; then
  if [[ -z "${WORKFLOWFILE_ABS}" || ! -f "${WORKFLOWFILE_ABS}" ]]; then
    echo "FATAL: workflow file missing for Test '${Subdirectory}': ${WORKFLOWFILE_IN}"
    exit 1
  fi
  cp -f "${WORKFLOWFILE_ABS}" "${SEED}/workflow.json"
else
  # Reference: workflow may not exist yet; runSimulation.sh will generate it
  if [[ -n "${WORKFLOWFILE_ABS}" && -f "${WORKFLOWFILE_ABS}" ]]; then
    cp -f "${WORKFLOWFILE_ABS}" "${SEED}/workflow.json"
  fi
fi

cd "${SEED}" || exit 1
echo "We are at batch $(pwd)"
echo "System: ${SYSTEMTAG}"
echo "Subdirectory: ${Subdirectory}"
if [[ -f workflow.json ]]; then
  echo "Workflow in batch: $(ls -l workflow.json)"
else
  echo "Workflow in batch: <none yet>"
fi

startdate="$(date)"

# Run (Reference may generate workflow.json inside this dir)
bash ./runSimulation.sh "${CPU_LIMIT}" "${MEM_LIMIT}" "workflow.json" "${Subdirectory}" "${SEED}" "${SYSTEMTAG}"

# Wait until any AO2D.root appears (any TF dir)
COUNTER=0
TIMEOUT_SEC="${MICRO_TIMEOUT_SEC:-4000}"

until find . -maxdepth 4 -type f -name AO2D.root | grep -q AO2D.root; do
  echo "Processing started: ${startdate}, currently $(date)..."
  sleep 5
  COUNTER=$((COUNTER+5))
  if [[ "${COUNTER}" -gt "${TIMEOUT_SEC}" ]]; then
    echo "Timeout after ${TIMEOUT_SEC}s: AO2D.root not found; cleaning tf* and exiting batch"
    rm -rf tf* 2>/dev/null || true
    exit 0
  fi
done

echo "AO2D.root found, proceeding"
rm -rf tf*/*_Hits???.root 2>/dev/null || true

cd ..

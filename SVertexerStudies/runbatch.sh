#!/bin/bash
set -euo pipefail

OutputDir="${1:?Missing OutputDir}"
Subdirectory="${2:?Missing Subdirectory}"
SYSTEMTAG="${3:-pp}"

# Normalize
if [[ "${SYSTEMTAG}" == "pbpb" ]]; then
  SYSTEMTAG="PbPb"
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# ------------------ LOAD CONFIG ------------------
if [[ "${SYSTEMTAG}" == "pp" ]]; then
  source "${SCRIPT_DIR}/configs_pp.sh"
elif [[ "${SYSTEMTAG}" == "PbPb" ]]; then
  source "${SCRIPT_DIR}/configs_PbPb.sh"
else
  echo "FATAL: unknown system tag '${SYSTEMTAG}'. Use 'pp' or 'PbPb'."
  exit 1
fi

[[ -z "${O2DPG_ROOT:-}" ]] && echo "Error: This needs O2DPG loaded" && exit 1
[[ -z "${O2_ROOT:-}"   ]] && echo "Error: This needs O2 loaded" && exit 1
. "${O2_ROOT}/share/scripts/jobutils.sh"

# ------------------ OUTPUT DIRECTORIES ------------------
mkdir -p "${SCRIPT_DIR}/../OutputData/${OutputDir}"
mkdir -p "${SCRIPT_DIR}/../GenProduction/${OutputDir}"
mkdir -p "${SCRIPT_DIR}/${OutputDir}"

# ------------------ COPY KEY FILES INTO GenProduction/<OutputDir>/ ------------------
for f in runSimulation.sh runbatch.sh micro.sh NumberOfProcesses stopall.sh \
         configs_pp.sh configs_PbPb.sh \
         FilesBeforeSVertexing.txt FilesAfterSVertexing.txt deletefiles.sh; do
  cp -f "${SCRIPT_DIR}/${f}" "${SCRIPT_DIR}/../GenProduction/${OutputDir}/." 2>/dev/null || true
done

# ------------------ ENTER RUN DIRECTORY ------------------
cd "${SCRIPT_DIR}/${OutputDir}" || { echo "FATAL: cannot cd into ${SCRIPT_DIR}/${OutputDir}"; exit 1; }

echo "Running system: ${SYSTEMTAG}"
echo "OutputDir: ${OutputDir}"
echo "Subdirectory: ${Subdirectory}"
echo "PWD: $(pwd)"

# Ensure required files exist in run dir (micro.sh runs from here)
for f in micro.sh NumberOfProcesses runSimulation.sh stopall.sh \
         configs_pp.sh configs_PbPb.sh \
         FilesBeforeSVertexing.txt FilesAfterSVertexing.txt; do
  cp -f "${SCRIPT_DIR}/${f}" . 2>/dev/null || true
done

# Create configs.sh in run dir for compatibility
if [[ "${SYSTEMTAG}" == "pp" ]]; then
  cp -f "${SCRIPT_DIR}/configs_pp.sh" ./configs.sh
else
  cp -f "${SCRIPT_DIR}/configs_PbPb.sh" ./configs.sh
fi

# ------------------ WORKFLOW FILE PATH ------------------
# In this run dir, main.py will create ./workflow.json for Tests.
WORKFLOWFILE_LOCAL="${WORKFLOWFILE:-workflow.json}"
WORKFLOWFILE_ABS="$(readlink -f "./${WORKFLOWFILE_LOCAL}" 2>/dev/null || true)"

echo "WORKFLOWFILE=${WORKFLOWFILE_LOCAL} (abs-if-exists: ${WORKFLOWFILE_ABS:-<missing>})"

# Reference may start without workflow.json; Tests must have it.
if [[ "${Subdirectory}" != "Reference" ]]; then
  if [[ ! -f "./${WORKFLOWFILE_LOCAL}" ]]; then
    echo "FATAL: Missing ./${WORKFLOWFILE_LOCAL} for Test '${Subdirectory}'."
    echo "       main.py should have created it by patching reference_workflow.json."
    exit 1
  fi
else
  if [[ ! -f "./${WORKFLOWFILE_LOCAL}" ]]; then
    echo "INFO: Reference run and ./${WORKFLOWFILE_LOCAL} is missing -> will be generated inside batches by runSimulation.sh"
  fi
fi

# ------------------ CLEANUP STRATEGY ------------------
rm -f subJobLock_* 2>/dev/null || true

# Keep old behavior: Reference cleans batch dirs; Tests keep them
if [[ "${Subdirectory}" == "Reference" ]]; then
  echo "Reference run: full cleanup of batch directories"
  for ((i=0; i<NBATCHES; i++)); do
    if [[ -d "$i" ]]; then
      echo "Removing stale batch directory: $i/"
      rm -rf "$i"
    fi
  done
else
  echo "Test run (${Subdirectory}): keeping batch dirs; deleting only svertexer-dependent outputs"
  if [[ -f "${SCRIPT_DIR}/FilesAfterSVertexing.txt" ]]; then
    for ((i=0; i<NBATCHES; i++)); do
      [[ -d "$i" ]] || continue
      while IFS= read -r pattern; do
        [[ -z "$pattern" ]] && continue
        find "$i" -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null || true
      done < "${SCRIPT_DIR}/FilesAfterSVertexing.txt"
    done
  else
    echo "WARNING: ${SCRIPT_DIR}/FilesAfterSVertexing.txt not found; not deleting anything for tests."
  fi
fi

# ------------------ RUNNING BATCHES ------------------
nprocesses="$(cat NumberOfProcesses)"
echo "Number of processes to keep alive: $nprocesses"
echo "Number of batches: ${NBATCHES}"
echo "CPU_LIMIT=${CPU_LIMIT} MEM_LIMIT=${MEM_LIMIT}"

for ((i=0; i<NBATCHES; i++)); do
  echo "Batch run: $i"
  ./micro.sh "$i" "$CPU_LIMIT" "$MEM_LIMIT" "./${WORKFLOWFILE_LOCAL}" "$Subdirectory" "$SYSTEMTAG" \
    > "log_${i}.txt" 2>&1 &

  while [ "$(ls subJobLock_* 2>/dev/null | wc -l)" -ge "$nprocesses" ]; do
    sleep 2
    nprocesses="$(cat NumberOfProcesses)"
  done
done

# ------------------ POST-PROCESSING ------------------
mkdir -p "../../OutputData/${OutputDir}/${Subdirectory}"

echo "Waiting for all simulations to finish..."
while ls subJobLock_* 1>/dev/null 2>&1; do
  sleep 10
done

echo "All simulations finished. Starting post-processing."

# Move AO2Ds
i=1
find */ -maxdepth 1 -type f -name AO2D.root -print0 | while IFS= read -r -d '' file; do
  mv "$file" "../../OutputData/${OutputDir}/${Subdirectory}/AO2D_${i}.root"
  ((i++))
done

# Save a reference_workflow.json after Reference finishes (template for main.py)
if [[ "${Subdirectory}" == "Reference" ]]; then
  ref_wf="$(find */ -maxdepth 1 -type f -name 'workflow.json' -print -quit 2>/dev/null || true)"
  if [[ -n "${ref_wf}" && -f "${ref_wf}" ]]; then
    cp -f "${ref_wf}" ./reference_workflow.json
    echo "Saved reference workflow -> $(pwd)/reference_workflow.json"
  else
    echo "WARNING: could not find workflow.json inside batches to save reference_workflow.json"
  fi
fi

bash "${SCRIPT_DIR}/stopall.sh" || true
echo "Done."

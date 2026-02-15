#!/bin/bash
set -euo pipefail


CPU_LIMIT="${1:?Missing CPU limit}"
MEM_LIMIT="${2:?Missing MEM limit}"
WORKFLOWFILE_IN="${3:?Missing workflow file name/path (e.g. workflow.json)}"
Subdirectory="${4:?Missing subdirectory (Reference/Test_0/...)}"
SEED="${5:?Missing seed}"
SYSTEMTAG="${6:-""}"

CURRENTSIMDIR="$(pwd)"

# ----------------------------
# Pick config file by system tag
# ----------------------------
if [[ "${SYSTEMTAG}" == "pp" ]]; then
  source "./configs_pp.sh"
elif [[ "${SYSTEMTAG}" == "PbPb" || "${SYSTEMTAG}" == "pbpb" ]]; then
  source "./configs_PbPb.sh"
else
  source "./configs.sh"
fi

# ----------------------------
# Basic environment checks
# ----------------------------
[[ -z "${O2DPG_ROOT:-}" ]] && echo "Error: This needs O2DPG loaded" && exit 1
[[ -z "${O2_ROOT:-}"   ]] && echo "Error: This needs O2 loaded" && exit 1
. "${O2_ROOT}/share/scripts/jobutils.sh"

# ----------------------------
# Generator selection
# ----------------------------
GEN="pythia8"
CONFKEY="Diamond.width[2]=6."

if [[ "${SYSTEMTAG}" == "PbPb" || "${SYSTEMTAG}" == "pbpb" ]]; then
  GEN="pythia8hi"
  CONFKEY="Diamond.width[2]=6."
fi

IS_REFERENCE=0
if [[ "$(basename "${Subdirectory}")" == "Reference" ]]; then
  IS_REFERENCE=1
fi

echo "Running simulation at ${PWD}"
echo "SystemTag:  ${SYSTEMTAG:-<default>}"
echo "Generator:  ${GEN}"
echo "Subdir:     ${Subdirectory}"
echo "Seed:       ${SEED}"
echo "WorkflowIn: ${WORKFLOWFILE_IN}"
echo "Reference?: ${IS_REFERENCE}"
echo "NTF:        ${NTIMEFRAMES:-<unset>}"
echo "NSIG:       ${NSIGEVENTS:-<unset>}"
echo "NWORKERS:   ${NWORKERS:-<unset>}"
echo "INTRATE:    ${INTRATE:-<unset>}"
echo "SYSTEM:     ${SYSTEM:-<unset>}"
echo "ENERGY:     ${ENERGY:-<unset>}"

# ----------------------------------------------------
# 1) Reference: generate workflow locally if it doesn't exist
# ----------------------------------------------------

if [[ "${IS_REFERENCE}" -eq 1 ]]; then
  if [[ ! -f "./workflow.json" && ! -f "${WORKFLOWFILE_IN}" ]]; then
    echo "[INFO] Reference run -> generating workflow now (local to ${PWD})"
    "${O2DPG_ROOT}/MC/bin/o2dpg_sim_workflow.py" \
      -eCM "${ENERGY}" -col "${SYSTEM}" -gen "${GEN}" -j "${NWORKERS}" \
      -ns "${NSIGEVENTS}" -tf "${NTIMEFRAMES}" \
      -confKey "${CONFKEY}" \
      -e "${SIMENGINE}" -seed "${SEED}" \
      --skipModules "ZDC" \
      -field -5 \
      -interactionRate "${INTRATE}" \
      > o2dpg_generate_workflow.log 2>&1
  fi
fi

if [[ -f "${WORKFLOWFILE_IN}" ]]; then
  src_abs="$(readlink -f "${WORKFLOWFILE_IN}")"
  dst_abs="$(readlink -f ./workflow.json 2>/dev/null || echo "")"

  if [[ "${src_abs}" != "${dst_abs}" ]]; then
    cp -vf "${WORKFLOWFILE_IN}" ./workflow.json
  fi
fi

if [[ ! -f "./workflow.json" ]]; then
  alt="$(ls -1 2>/dev/null | grep -E '^workflow.*\.json$' | head -n 1 || true)"
  if [[ -n "${alt}" && -f "${alt}" ]]; then
    cp -vf "${alt}" ./workflow.json
  fi
fi

if [[ ! -f "./workflow.json" ]]; then
  echo "ERROR: No workflow file found to run (./workflow.json missing)."
  echo "PWD: ${PWD}"
  echo "Contents:"
  ls -lah
  echo "JSON files:"
  ls -lah *.json 2>/dev/null || true
  exit 1
fi

WORKFLOWFILE="./workflow.json"

# ----------------------------------------------------
# Run workflow 
# ----------------------------------------------------
echo "[INFO] Running workflow runner with ${WORKFLOWFILE}"
"${O2DPG_ROOT}/MC/bin/o2_dpg_workflow_runner.py" \
  -f "${WORKFLOWFILE}" \
  -tt aod \
  --cpu-limit "${CPU_LIMIT}" \
  --mem-limit "${MEM_LIMIT}"

echo "[INFO] Workflow runner finished."

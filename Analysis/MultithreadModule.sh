#!/bin/bash

# original concept/script by Cicero D. Muncinelli
# Modified by Gianni S. S. Liveraro

# ==============================================================================
# ALICE O2 MULTI-THREAD MODULE 
# ==============================================================================
# Usage: ./MultithreadModule.sh <input_list.txt> <config.json> [num_jobs] [files_per_chunk] "<o2_command_template>" <output_dir>
# ==============================================================================

# ------------------------------------------------------------------------
# --- 0. INPUT ARGS ---
INPUT_LIST="$1"
JSON_CONFIG_PATH="$2"
MAX_JOBS="${3:-10}" # Number of CPU threads to use. If unset/empty, 16 is used.
FILES_PER_CHUNK="$4"
O2_COMMAND_TEMPLATE="$5"
OUTPUT_DIR="$6"

echo "this is the command received!"
echo $O2_COMMAND_TEMPLATE

# ------------------------------------------------------------------------
# Safeguards: 
if [[ -z "$INPUT_LIST" ]] || [[ -z "$JSON_CONFIG_PATH" ]]; then
    echo "Usage: $0 <input_list.txt> <config.json> [num_jobs]"
    exit 1
fi

if [[ -z "$O2_COMMAND_TEMPLATE" ]]; then
    echo "Missing O2 command template"
    exit 1
fi

INPUT_LIST=$(realpath "$INPUT_LIST")                        # Input list with AO2Ds
JSON_CONFIG_PATH=$(realpath "$JSON_CONFIG_PATH")            # Json config file
WORK_DIR=$(pwd)                                             # Working Directory
TEMP_BASE="${WORK_DIR}/temp_staging_area"                   # Temporary Staging Area (intermediate/temporary AnalysisResults.root files are saved here!)
RESULTS_DIR="${WORK_DIR}/${OUTPUT_DIR}"                           # Final (merged) AnalysisResults.root file is saved here!
LOGS_DIR="${RESULTS_DIR}/logs"                              # Log Directory (per job)

# ------------------------------------------------------------------------
# --- 1. SAFETY TRAP ---
# This ensures the temp folder is ALWAYS deleted, even if you Ctrl+C or the script crashes.
cleanup() {
    # Does not delete logs though! Be aware of that!
    if [ -d "$TEMP_BASE" ]; then
        echo ""
        echo "  [Cleanup] Removing temp staging area..."
        rm -rf "$TEMP_BASE"
    fi
}
# Trigger cleanup on Exit (0/1), Interrupt (Ctrl+C), or Terminate signals
trap cleanup EXIT INT TERM # When the shell receives one of these signals, run cleanup()

echo "========================================================"
echo "   Starting O2 Staged Analysis"
echo "   Threads: $MAX_JOBS"
echo "   Batch Size: $FILES_PER_CHUNK files"
echo "   Logs: $LOGS_DIR"
echo "   Json path: $JSON_CONFIG_PATH"
echo "========================================================"

# ------------------------------------------------------------------------
# --- 2. CLEANUP & SETUP ---
if [ -d "$TEMP_BASE" ]; then
    rm -rf "$TEMP_BASE"
fi

# Clean old logs to prevent appending to previous runs
if [ -d "$LOGS_DIR" ]; then
    echo "   Cleaning old logs..."
    rm -rf "$LOGS_DIR"
fi

# Create necessary directories
mkdir -p "$TEMP_BASE/lists"
mkdir -p "$TEMP_BASE/outputs"
mkdir -p "$RESULTS_DIR"
mkdir -p "$LOGS_DIR" # Ensure log dir exists

# ------------------------------------------------------------------------
# --- 3. SPLIT INPUT ---
echo "   Splitting input list..."
grep -vE '^\s*$' "$INPUT_LIST" | split -l "$FILES_PER_CHUNK" - "$TEMP_BASE/lists/batch_" # This removes blank lines from the input list
NUM_BATCHES=$(ls "$TEMP_BASE/lists/batch_"* | wc -l)
echo "   Created $NUM_BATCHES batches."

# ------------------------------------------------------------------------
# --- 4. THE WORKER ---
run_staged_job() {
    BATCH_LIST="$1"
    JOB_ID="$2"
    BASE_TMP="$3"
    LOG_DIR="$4" # Pass the log dir
    command="$5" 

    # Define Log File in the PERMANENT directory
    LOG_FILE="${LOG_DIR}/job_${JOB_ID}.log"

    # Define Isolated SSD Workspace
    JOB_DIR="${BASE_TMP}/worker_${JOB_ID}"
    DATA_DIR="${JOB_DIR}/data"
    mkdir -p "$DATA_DIR"
    
    # ------------------------------------------------------------------------
    # Create local Input List for this Job
    LOCAL_LIST_FILE="${JOB_DIR}/local_input.txt"
    mkdir -p "$(dirname "$LOCAL_LIST_FILE")"
    > "$LOCAL_LIST_FILE"

    while IFS= read -r RAW_LINE; do
        HDD_PATH="${RAW_LINE#"file:"}"
        ABS_PATH=$(realpath "$HDD_PATH")  # convert to absolute path
        if [ -f "$ABS_PATH" ]; then
            echo "file:${ABS_PATH}" >> "$LOCAL_LIST_FILE"
        else
            echo "Warning: Source file not found: $ABS_PATH" >> "$LOG_FILE"
        fi
    done < "$BATCH_LIST"

    # ------------------------------------------------------------------------
    # O2 Command
    cd "$JOB_DIR" || return 1
    
    # We redirect output to the persistent log file
    command+=" >> "$LOG_FILE" 2>&1"

    # Execution
    eval $command # Using eval to execute the passed O2-command template
    
    RC=$? # exit status of the o2 command - failure/success?

    # ------------------------------------------------------------------------
    # STAGE-OUT
    if [ "$RC" -eq 0 ] && [ -f "AnalysisResults.root" ]; then

        mv "AnalysisResults.root" "${BASE_TMP}/outputs/AnalysisResults_${JOB_ID}.root"

        # Check if we are dealing with derived data production
        if [ -f "AO2D.root" ] && [[ "$command" == *"--aod-writer-json"* ]]; then 
            mv "AO2D.root" "${BASE_TMP}/outputs/AO2D_${JOB_ID}.root"
        fi

        STATUS="OK"
    else
        STATUS="Error!!!"
        echo "   !!! JOB FAILED. See log: $LOG_FILE" # Print to main screen
    fi

    # CLEANUP DATA ONLY
    rm -rf "$JOB_DIR"
    echo "   [Batch $JOB_ID] $STATUS"
}

# Exports the function to subshells - required because GNU parallel runs jobs in separate shells
export -f run_staged_job

# ------------------------------------------------------------------------
# --- 5. EXECUTE ---
echo "  Processing queue... (Logs are in $LOGS_DIR)"
find "$TEMP_BASE/lists" -name "batch_*" | parallel --progress --eta -j "$MAX_JOBS" \
    run_staged_job {} {#} "$TEMP_BASE" "$LOGS_DIR" "'$O2_COMMAND_TEMPLATE'"

# ------------------------------------------------------------------------
# --- 6. MERGE ---
echo "   Merging Results..."
TARGET_FILE="${RESULTS_DIR}/AnalysisResultsMerged.root"
TARGETAO2D_FILE="${RESULTS_DIR}/AO2DMerged.root"

if ls "$TEMP_BASE/outputs"/AnalysisResults_*.root 1> /dev/null 2>&1; then # try ls, don’t print anything (/dev/null is a black hole), just check return code
    hadd -f -k -j "$MAX_JOBS" "$TARGET_FILE" "$TEMP_BASE/outputs"/AnalysisResults_*.root
    # Also copy the JSON with a matching name for records:
    cp "$JSON_CONFIG_PATH" "${RESULTS_DIR}/dpl-config.json" 2>/dev/null
    echo "  Done. Final file: $TARGET_FILE"
else
    echo "!!! No results found to merge."
    echo "   Check the log files in $LOGS_DIR for details."
    exit 1
fi

# If we are dealing with derived data production:
if [[ "$O2_COMMAND_TEMPLATE" == *"--aod-writer-json"* ]]; then
    echo "Detected --aod-writer-json → merging AO2Ds"

    AO2D_INPUT_LIST="${TEMP_BASE}/AO2D_paths.txt"

    if ls "$TEMP_BASE/outputs"/AO2D_*.root > /dev/null 2>&1; then
        find "$TEMP_BASE/outputs" -name 'AO2D_*.root' \
            | sed 's|^|file:|' > "$AO2D_INPUT_LIST"

        o2-aod-merger --input "$AO2D_INPUT_LIST" --output "$TARGETAO2D_FILE"
        echo "  Done. Final file: $TARGETAO2D_FILE"
    else
        echo "!!! No AO2D found to merge."
        exit 1
    fi
else
    echo "No --aod-writer-json detected → skipping AO2D merge"
fi




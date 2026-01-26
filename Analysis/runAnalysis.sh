#!/bin/bash

# 
INPUT_FILE="LocalTest_pp_paths.txt"                       # Input list with AO2Ds
JSON="DerivedData-MC-config-OverRaw.json"                    # Json config file. Set "aod-file-private": "@local_input.txt". 
OUTPUTDIRECTOR="OutputDirector.json"                         # Output director json file
OUTPUT_DIR="TestDirResults"                                  # Output directory 

# Parallelization settings
MAX_JOBS="2"                # Number of CPU threads to use.
FILES_PER_CHUNK=1            # N. AO2Ds per batch. Watch out for too large files! Memory = NJobs * FilesPerChunk * AvgFileSize

# O2-specific (per job settings)
AOD_MEMORY_RATE_LIMIT=500000000 
AOD_MAXIO=3000
SHARED_MEMORY=100000000000
TIMEFRAMES=4
READERS=1 

# O2 Command Template
OPTION="-b --configuration json://$(realpath "$JSON")"
OUTPUTDIRECTOR="$(realpath "$OUTPUTDIRECTOR")"
O2_CMD="o2-analysis-lf-sigma0builder ${OPTION} | o2-analysis-lf-v0mlscoresconverter ${OPTION} | o2-analysis-lf-strangederivedbuilder ${OPTION} | o2-analysis-propagationservice ${OPTION}  | o2-analysis-trackselection ${OPTION} | o2-analysis-pid-tpc-service ${OPTION} | o2-analysis-pid-tof-base ${OPTION} | o2-analysis-pid-tof-beta ${OPTION} | o2-analysis-pid-tof-full ${OPTION} | o2-analysis-multcenttable ${OPTION} | o2-analysis-ft0-corrected-table ${OPTION} | o2-analysis-event-selection-service ${OPTION} | o2-analysis-mm-track-propagation ${OPTION} | o2-analysis-fwdtrack-to-collision-associator ${OPTION} |  o2-analysis-mccollisionextra ${OPTION} --pipeline pid-tpc-service:2, propagationservice:4 --timeframes-rate-limit ${TIMEFRAMES} --aod-max-io-rate ${AOD_MAXIO} --shm-segment-size ${SHARED_MEMORY} --readers ${READERS}"
#O2_CMD="o2-analysis-lf-strangederivedbuilder ${OPTION} | o2-analysis-propagationservice ${OPTION}  | o2-analysis-trackselection ${OPTION} | o2-analysis-pid-tpc-service ${OPTION} | o2-analysis-pid-tof-base ${OPTION} | o2-analysis-pid-tof-beta ${OPTION} | o2-analysis-pid-tof-full ${OPTION} | o2-analysis-multcenttable ${OPTION} | o2-analysis-tracks-extra-v002-converter ${OPTION} | o2-analysis-ft0-corrected-table ${OPTION} | o2-analysis-event-selection-service ${OPTION} | o2-analysis-mm-track-propagation ${OPTION} | o2-analysis-fwdtrack-to-collision-associator ${OPTION} | o2-analysis-mccollision-converter ${OPTION} |  o2-analysis-mccollisionextra ${OPTION} --pipeline pid-tpc-service:2, propagationservice:4 --timeframes-rate-limit ${TIMEFRAMES} --aod-max-io-rate ${AOD_MAXIO} --shm-segment-size ${SHARED_MEMORY} --readers ${READERS} --aod-writer-json ${OUTPUTDIRECTOR}"
#O2_CMD="o2-analysis-lf-sigma0builder ${OPTION} | o2-analysis-lf-v0mlscoresconverter ${OPTION} --aod-memory-rate-limit ${AOD_MEMORY_RATE_LIMIT} --pipeline sigma0builder:4 --timeframes-rate-limit ${TIMEFRAMES} --aod-max-io-rate ${AOD_MAXIO} --shm-segment-size ${SHARED_MEMORY} --readers ${READERS} "

# Execution:
./MultithreadModule.sh "$INPUT_FILE" "$JSON" "$MAX_JOBS" "$FILES_PER_CHUNK" "$O2_CMD" "$OUTPUT_DIR"

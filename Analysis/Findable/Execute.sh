#!/bin/bash
JSON_DATA="jsonDATA.json"
JSON_MC="config.json"

find DataFiles -type f -name "*.txt" -print0 | sort -zV | \
while IFS= read -r -d '' FILE; do
    FILE=${FILE#./}
    echo "Processing file: $FILE"
    
    # Debug: check if source exists
    ls -la "$FILE"
    
    cp "$FILE" ./AO2D_list.txt
    
    # Debug: check if copy succeeded
    ls -la ./AO2D_list.txt
    
    JSON="$JSON_MC"
    ./run.sh "./AO2D_list.txt" "$JSON" < /dev/null
    until [ -f AnalysisResults.root ]; do
        sleep 5
    done
    OUTPUT_NAME="$(basename "${FILE%.txt}_AnalysisResults.root")"
    mv AnalysisResults.root "$OUTPUT_NAME"
done
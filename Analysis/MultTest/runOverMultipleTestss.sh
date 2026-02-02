#!/bin/bash

JSON_DATA="jsonDATA.json"   # default JSON
JSON_MC="DerivedData-MC-config-OverRaw.json"       # MC JSON

find . -type f -name "*.txt" -print0 |
while IFS= read -r -d '' FILE; do
  FILE=${FILE#./}   # remove leading ./ safely
  echo "Processing file list: $FILE"

  # Choose JSON based on file name
  if [[ $(basename "$FILE") == LHC24f4d* ]]; then
    JSON="$JSON_MC"
  else
    JSON="$JSON_MC"
  fi

  ./runAnalysis.sh "$FILE" "$JSON"

  until [ -f RunOutput/AnalysisResultsMerged.root ]; do
    sleep 5
  done

  OUTPUT_NAME="$(basename "${FILE%.txt}_AnalysisResultsMerged.root")"

  mv RunOutput/AnalysisResultsMerged.root "RunOutput/$OUTPUT_NAME"

done




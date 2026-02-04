#!/bin/bash

# Dataset directory
datasetName="Localpp_MinBias"

# Base directory where AO2Ds are stored
base_dir="../../OutputData/${datasetName}"

# Loop over each subdirectory in datasetName/
for subdir in "$base_dir"/*/; do
    # Skip if no subdirectories exist
    [ -d "$subdir" ] || continue

    # Get subdirectory name
    subdir_name=$(basename "$subdir")

    # Output file for this subdirectory
    output_file="${subdir_name}_paths.txt"

    # Remove old file if it exists
    rm -f "$output_file"

    # Find AO2D files inside this subdirectory
    find "$subdir" -type f -name "AO2D_*.root" | sort > "$output_file"

    echo "Saved AO2D paths for ${subdir_name} in ${output_file}"
done

echo "Done. One output file created per subdirectory in ${datasetName}/"

#!/bin/bash

# Make sure you are in the O2Physics environment

# GRID base destination 
DEST_BASE="/alice/cern.ch/user/g/gsetouel/ITSTPCMatching"

> GRID_FileListToUpload.txt

find */ -maxdepth 3 -type f | sort | while read -r f; do
  # Absolute local source
  src="file:$(realpath "$f")"

  # Path relative to current directory (remove leading ./)
  rel_path="${f#./}"

  # Final GRID destination (alien: ONLY ONCE)
  dest="alien:${DEST_BASE}/${rel_path}"
  
  # Remove any /./ in the path
  dest="${dest//\/.\//\/}"

  # Write to file list
  echo "$src $dest" >> GRID_FileListToUpload.txt
done

# Save list of AO2D files for analysis
> GRID_FileList.txt

find . -maxdepth 3 -type f -name "AO2D_*.root" | sort | while read -r f; do
  # Path relative to current directory (remove leading ./)
  rel_path="${f#./}"

  # Final GRID destination (alien: ONLY ONCE)
  dest="alien:${DEST_BASE}/${rel_path}"
  
  # Remove any /./ in the path
  dest="${dest//\/.\//\/}"

  # Write to file list
  echo "$dest" >> GRID_FileList.txt
done

# Upload files!
#alien.py cp -T 10 -input GRID_FileListToUpload.txt

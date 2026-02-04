#!/bin/bash

# Delete irrelevant files to save disk
while IFS= read -r pattern; do
    find */ -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null
done < FilesBeforeITSTPCMatchingToDelete.txt

while IFS= read -r pattern; do
    find */ -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null
done < FilesAfterITSTPCMatchingToDelete.txt
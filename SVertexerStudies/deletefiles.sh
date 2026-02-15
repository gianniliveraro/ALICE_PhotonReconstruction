#!/bin/bash

# Delete irrelevant files in order to investigate different parameter setting for the S-Vertexer
while IFS= read -r pattern; do
    find */ -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null
done < FilesBeforeSVertexing.txt

while IFS= read -r pattern; do
    find */ -maxdepth 2 -type f -name "$pattern" -delete 2>/dev/null
done < FilesAfterSVertexing.txt
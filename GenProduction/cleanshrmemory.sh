#!/bin/bash

# Removes shared memory segments created by O2 processes. Sometimes, old large files may cause issues with new runs (bus error)
rm -rf /dev/shm/fmq* /dev/shm/o2*

echo "Shared memory segments removed. You can now run your O2 analysis without old shared memory conflicts."
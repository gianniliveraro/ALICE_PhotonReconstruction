# Copyright 2019-2020 CERN and copyright holders of ALICE O2.
# See https:#alice-o2.web.cern.ch/copyright for details of the copyright holders.
# All rights not expressly granted are reserved.
#
# This software is distributed under the terms of the GNU General Public
# License v3 (GPL Version 3), copied verbatim in the file "COPYING".
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.
#
# 00_ProcessData
# ================
#
# This code reads an AO2D a parquet file
#
#    Comments, questions, complaints, suggestions?
#    Please write to:
#    gianni.shigeru.setoue.liveraro@cern.ch

#____________________________
# Imports
import numpy as np
import pandas as pd
import uproot
import os
from sklearn.model_selection import train_test_split
import time
t0 = time.time() # Initial time

#---------------------------  MAIN CONFIGURATIONS ----------------------------
fSetMaximumDFs = True # if true, sets the maximum number of DFs to be used
NMaxDfs = 1

##--------------------------------- DATASET ----------------------------------
DatasetName = 'AO2D_1' # root flat TTree 
Target = "AO2D_v0AssoQAML"
Class_name = "fIsCorrectlyAssoc"

#--------------------------------- LOADING DATA ------------------------------
rfile = uproot.open("{}.root".format(DatasetName))

# Get the list of directories (TDirectory) in the ROOT file
keys = rfile.keys()
directory_names = [x.split(';')[0] for x in keys if "/" not in x ]
group_names = [x.split(';')[0] for x in keys if "/" in x ]
Subgroups = np.unique(np.array([x.split('/')[1] for x in group_names]))

print("\n_________________________________________")
print('[INFO]: The input dataset has {} directories'.format(len(directory_names)))
print('[INFO]: The input dataset has {} subgroups'.format(len(Subgroups)))
print('[INFO]: The input dataset has {} groups'.format(len(group_names)))
print('[INFO]: The input dataset has {} keys'.format(len(keys)))
# Creating Pandas dataframes from TTrees 
iteraction = 0
for dir in group_names:
  if fSetMaximumDFs and iteraction >= NMaxDfs:
    print(f"[INFO]: Maximum number of DFs ({NMaxDfs}) reached. Stopping loading.")
    break

  if "O2track_iu" not in dir:
    continue

  print("\n[INFO]: Loading directory: {}".format(dir))
  tree = rfile[dir]

  if iteraction==0:
    dataframeFinal = tree.arrays(library='pd')

  else:
    dataframe = tree.arrays(library='pd')
    dataframeFinal = pd.concat([dataframeFinal, dataframe],axis=0)

  iteraction = iteraction + 1

#--------------------------------- PROCESSING ---------------------------------

print("\n-------------------------------------------")
print('[INFO]: The input dataset has a total of {} tracks'.format(len(dataframeFinal)))

dataframeFinal.to_parquet("{}.parquet".format(DatasetName))
dataframeFinal.to_csv("{}.csv".format(DatasetName))

t1 = time.time() - t0
print("\n_________________________________________")
print("Total time elapsed (min): ", t1/60)
print("_________________________________________")
#!/bin/sh

import json
import os
from pathlib import Path

# Reconstruction selections
sv_cuts = ''
#sv_cuts = 'svertexer.minCosPAXYMeanVertex=-1;svertexer.minDCAToPV=0.001;svertexer.minCosPA=-1;svertexer.maxChi2=5;svertexer.maxDCAXYToMeanVertexV0Casc=10.;svertexer.maxDCAXYCasc=0.9;svertexer.maxDCAZCasc=0.9;svertexer.minRDiffV0Casc=0.1;svertexer.checkV0Hypothesis=false;svertexer.checkCascadeHypothesis=false;svertexer.maxPVContributors=3;svertexer.minCosPAXYMeanVertexCascV0=-1'
# ab_cuts = 'tpcitsMatch.requireToReachLayerAB=6;tpcitsMatch.minContributingLayersAB = 1; tpcitsMatch.maxABLinksOnLayer=100;tpcitsMatch.maxABFinalHyp=100;tpcitsMatch.cutABTrack2ClChi2=100;tpcitsMatch.nABSigmaY=10;tpcitsMatch.nABSigmaZ=10'

# get path 
testnumber=0
print("Directory Path:", Path().absolute())
fullpath = str(Path().absolute())
path_list = fullpath.split('/')
for path_ind,path in enumerate(path_list):
  #print('dir comp '+str(path))
  if path.startswith('itstpcstudy_new'):
      testnumber=int(int(path_list[path_ind+1])/100)

# total number of variables to try: 11
# full combinations: 6x11 = 66, 0..65 in index

# [4126468:itstpc-track-matcher]: tpcitsMatch.crudeAbsDiffCut[0] : 2		[ CODE ]
# [4126468:itstpc-track-matcher]: tpcitsMatch.crudeAbsDiffCut[1] : 2		[ CODE ]
# [4126468:itstpc-track-matcher]: tpcitsMatch.crudeAbsDiffCut[2] : 0.2		[ CODE ]
# [4126468:itstpc-track-matcher]: tpcitsMatch.crudeAbsDiffCut[3] : 0.2		[ CODE ]
# [4126468:itstpc-track-matcher]: tpcitsMatch.crudeAbsDiffCut[4] : 4		[ CODE ]
# [290511:itstpc-track-matcher]: tpcitsMatch.crudeNSigma2Cut[0] : 49              [ CODE ]
# [290511:itstpc-track-matcher]: tpcitsMatch.crudeNSigma2Cut[1] : 49              [ CODE ]
# [290511:itstpc-track-matcher]: tpcitsMatch.crudeNSigma2Cut[2] : 49              [ CODE ]
# [290511:itstpc-track-matcher]: tpcitsMatch.crudeNSigma2Cut[3] : 49              [ CODE ]
# [290511:itstpc-track-matcher]: tpcitsMatch.crudeNSigma2Cut[4] : 49              [ CODE ]

mat_vars = []
mat_vars.append([30, 30, 30, 30, 30, 30,  30 , 100, 100, 100, 100, 100, 100, 100, 1000, 1000, 1000, 1000,1000,1000,1000]) # chi2
#mat_vars.append([10, 15, 25, 35, 50, 100, 150, 10,  15,  25,  35,  50,  100, 150, 10,   15,   25,   35,  50,  100, 150  ]) # max row for TPC start of track
mat_vars.append([15, 25, 35, 50, 100, 150, 10,  15,  25,  35,  50,  100, 150, 10,   15,   25,   35,  50,  100, 150  ]) # max row for TPC start of track

# ITS TPC matcher cuts
mat_cuts0 = 'tpcitsMatch.cutMatchingChi2='+str(mat_vars[0][testnumber])+'; '
mat_cuts1 = 'tpcitsMatch.askMinTPCRow='+str(mat_vars[1][testnumber])+'; '

#open json 
json_dict = json.load(open('workflow.json'))
json_items = json_dict['stages']
for item in json_items:
    if item['name'].startswith('svfinder'):
        cmd_list = item['cmd'].split(' ')
        for cmd_ind, cmd in enumerate(cmd_list):
            if cmd.startswith('--configKeyValues'):
                cfg = cmd_list[cmd_ind + 1].strip('"')
                kv_pairs = [x.strip() for x in cfg.split(';') if x.strip()]

                if sv_cuts:
                    kv_pairs.extend([x.strip() for x in sv_cuts.split(';') if x.strip()])

                new_cfg = ';'.join(kv_pairs)
                cmd_list[cmd_ind + 1] = f"\"{new_cfg}\""
                item['cmd'] = ' '.join(cmd_list)

    
    if item['name'].startswith('itstpcMatch'):
        cmd_list = item['cmd'].split(' ')
        for cmd_ind, cmd in enumerate(cmd_list):
            if cmd.startswith('--configKeyValues'):
                # Remove ""
                cfg = cmd_list[cmd_ind + 1].strip('"')

                #Add selection, no ';' in the end
                kv_pairs = [x.strip() for x in cfg.split(';') if x.strip()]
                kv_pairs.append(f"tpcitsMatch.cutMatchingChi2={mat_vars[0][testnumber]}")
                
                min_row = mat_vars[1][testnumber]

                for i in range(36):
                    kv_pairs.append(f"tpcitsMatch.askMinTPCRow[{i}]={min_row}")

                new_cfg = ';'.join(kv_pairs)

                cmd_list[cmd_ind + 1] = f"\"{new_cfg}\""
                item['cmd'] = ' '.join(cmd_list)


json.dump(json_dict, open('workflow_mod.json','w'), indent=4)
                            

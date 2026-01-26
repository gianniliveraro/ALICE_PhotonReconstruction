# ALICE_PhotonReconstruction
Repository to document developments on photon reconstruction with ALICE during Run 3

## Requirements
O2Physics and O2DPG packages - currently using the 'daily-20260106-0000-1' tag. More info:
https://github.com/sawenzel/MCReleasePrototype/blob/main/releases/release-notes/release-notes-O2PDPSuite%3A%3AMC-prod-2026-v1.md

## Structure/organization:

~~~
├── GenProduction/
│   ├── Main.py                            <- Run this to execute the simulation + set and run all tests with reco parameters
│   ├── configs.sh                         <- Place where you set the properties of the simulation (system, energy, etc)
│   ├── generator_pythia8_gun.C            <- Pythia script to generate/enrich collisions.  
│   ├── configParticleGun.ini              <- config file to set the generator/enrichment scheme
│   ├── runbatch.sh                        <- produce batches of simulations / Save merged AO2Ds + configs / cleanup unused files 
│   ├── NumberOfProcesses                  <- number of simultaneous processes running
│   ├── micro.sh                           <- manages the processing of each batch
│   ├── stopall.sh                         <- script to kill zombie process (remnant of the simulation)
│   └── runSimulation                      <- executes o2dpg_sim_workflow.py and o2_dpg_workflow_runner.py
│
├── OutputData/                            <- directory that saves AO2Ds / workflow.json / config files
│   ├── uploadAO2Ds.sh                     <- Lists/uploads files to alien
│
├── Analysis/                              <- Basic scripts to run analysis over AO2Ds
│
└── DEPRECATED/                            <- Old scripts / backup

~~~

## Extras:

- List of ITS-TPC Matching parameters: https://github.com/AliceO2Group/AliceO2/blob/9634a2ee3fd8481f9f222c27eee03d1c459a1b13/Detectors/GlobalTracking/include/GlobalTracking/MatchTPCITSParams.h

- O2 sim documentation: https://aliceo2group.github.io/simulation/

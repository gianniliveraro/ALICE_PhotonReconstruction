# Define input and output names
JSONPATH="config"

export OPTIONS="-b --configuration json://${JSONPATH}.json --shm-segment-size 10000000000"

# Latest
o2-analysis-lf-findable-study ${OPTIONS} | o2-analysis-lf-str-derived-genqa ${OPTIONS} | o2-analysis-lf-v0mlscoresconverter ${OPTIONS} | o2-analysis-lf-strangederivedbuilder ${OPTIONS} | o2-analysis-propagationservice ${OPTIONS}  | o2-analysis-trackselection ${OPTIONS} | o2-analysis-pid-tpc-service ${OPTIONS} | o2-analysis-lf-strangenesstofpid ${OPTIONS} | o2-analysis-multcenttable ${OPTIONS} | o2-analysis-ft0-corrected-table ${OPTIONS} | o2-analysis-event-selection-service ${OPTIONS} | o2-analysis-mm-track-propagation ${OPTIONS} | o2-analysis-fwdtrack-to-collision-associator ${OPTIONS} | o2-analysis-mccollisionextra ${OPTIONS} | o2-analysis-pid-tof-base ${OPTIONS} | o2-analysis-pid-tof-beta ${OPTIONS} | o2-analysis-pid-tof-full ${OPTIONS} > log_analysis.txt

# mv dpl-config.json ${JSONPATH}.json
# echo "Moving analysis results..."
# mv AnalysisResults.root AnalysisResults_LF_LHC24f4d.root
# mv AO2D.root LF_LHC24f4d_DerivedAO2D.root

# o2-analysis-mccollision-converter ${OPTIONS} | 
# o2-analysis-tracks-extra-v002-converter

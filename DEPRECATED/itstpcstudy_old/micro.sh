#!/bin/bash
touch subJobLock_${1}
mkdir ${1}
#cp -r ccdb ${1}/ccdb
cp run_XiInjected.sh ${1}/.
cp apply_cuts_to_json.py ${1}/.
cd ${1}
startdate=$(date)
source run_XiInjected.sh 
# echo "DUMMY COMMAND WAIT ISSUED"
# sleep 120
# echo "DUMMY WAIT OVER"
COUNTER=0
until [ -f tf1/AO2D.root ]
do
    currentdate=$(date)
    echo "Processing started: ${startdate}, currently ${currentdate}..."
    sleep 5
    let COUNTER=COUNTER+5
    if [ "$COUNTER" -gt "18000" ]; then
      echo "This is taking too long and should be finished by now, breaking"
      break
      rm -rf tf*
      cd ../..
      rm subJobLock_${1}
    fi
done
echo "File tf1/AO2D.root found, proceeding"
rm -rf tf*/*_Hits???.root
cd ..
rm subJobLock_${1}

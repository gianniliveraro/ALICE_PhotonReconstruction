# script to merge 
#root.exe -q -b runMatcherStudy01.C+\(\"..\"\,\"test.root\"\,1\)

for i in {000..065}
do
  hadd merged_${i}.root output_slot${i}_tf*.root
done
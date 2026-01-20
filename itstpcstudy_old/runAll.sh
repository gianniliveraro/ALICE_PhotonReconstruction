# script to run multiple itstpc matcher tests

#root.exe -q -b runMatcherStudy01.C+\(\"..\"\,\"test.root\"\,1\)

for i in {000..011}
do
  for j in {1..5}
  do
    echo "Preparing command tf ${i}, file ${j}"
    root.exe -q -b runMatcherStudy01.C+\(\"\/storage1\/daviddc\/k0gun\/${i}\/tf${j}\"\,\"output_slot${i}_tf${j}.root\"\,${j}\) &> log${i}_${j}.txt &
    sleep 2
  done
  sleep 20 
done
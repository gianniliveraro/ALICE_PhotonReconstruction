# This is the main code to run simulations with different TPC-ITS matching cuts
# Based on scripts from David Chinellato and Romain Schotter

# Modified by: Gianni Liveraro, Oussama Benchikhi, Stefanie Mrozinski
# Date: Jan 2025
# email: gianni.shigeru.setoue.liveraro@cern.ch
#        oussama.benchikhi@cern.ch
#        stefanie.mrozinski@cern.ch

#____________________________
# Imports
from itertools import product
import json
from pathlib import Path
import subprocess
import time 

# Change this to your desired output directory
OutputDir = "Localpp_PhotonEnriched"

# ------------------ PARAMETER SPACE ------------------
Test_params = {
    "cutMatchingChi2": [30, 100, 1000],
    "askMinTPCRow": [15, 25, 35, 50, 100],
}

# Test_params = {
#     "cutMatchingChi2": [30],
#     "askMinTPCRow": [15, 25],
# }

# ------------------ JSON MODIFICATION ------------------
# TODO: Generalize to include other stages/parameters if needed (e.g, svertexer, etc)
def apply_cuts_to_json(json_path: Path, combination: dict):
    """
    Modify workflow.json in-place using values from `combination`
    """
    with open(json_path) as f:
        json_dict = json.load(f)

    for stage in json_dict.get("stages", []):
        if stage.get("name", "").startswith("itstpcMatch"):
            cmd_list = stage["cmd"].split()

            for i, token in enumerate(cmd_list):
                if token == "--configKeyValues":
                    cfg = cmd_list[i + 1].strip('"')

                    kv_pairs = [x.strip() for x in cfg.split(";") if x.strip()]

                    # Remove old values if they exist
                    kv_pairs = [
                        kv for kv in kv_pairs
                        if not kv.startswith("tpcitsMatch.cutMatchingChi2")
                        and not kv.startswith("tpcitsMatch.askMinTPCRow")
                    ]

                    # Add new cutMatchingChi2
                    kv_pairs.append(
                        f"tpcitsMatch.cutMatchingChi2={combination['cutMatchingChi2']}"
                    )

                    # Add askMinTPCRow per layer
                    min_row = combination["askMinTPCRow"]
                    for layer in range(36):
                        kv_pairs.append(
                            f"tpcitsMatch.askMinTPCRow[{layer}]={min_row}"
                        )

                    new_cfg = ";".join(kv_pairs)
                    cmd_list[i + 1] = f"\"{new_cfg}\""
                    stage["cmd"] = " ".join(cmd_list)

    with open(json_path, "w") as f:
        json.dump(json_dict, f, indent=4)

# ------------------ REFERENCE RUN ------------------
print("Running reference simulation...")
t0 = time.time()
subprocess.run(["./runbatch.sh", OutputDir, "Reference"])

# ------------------ TESTING LOOP ------------------
print("DONE! {} tests will be run.".format(len(list(product(*Test_params.values())))))
keys = list(Test_params.keys())
values = list(Test_params.values())

counter = 0
for combo in product(*values):
    combination = dict(zip(keys, combo))
    test_name = f"Test_{counter}"

    print(f"\n=== Running {test_name}: {combination}")

    # Locate workflow.json files
    workflow_files = Path(OutputDir).glob("*/workflow.json")

    for wf in workflow_files:
        print("wf:", wf)
        apply_cuts_to_json(wf, combination)

    # Run simulation
    subprocess.run(["./runbatch.sh", OutputDir, test_name])

    # Save configuration
    outdir = Path(f"../OutputData/{OutputDir}/{test_name}")
    outdir.mkdir(parents=True, exist_ok=True)

    with open(outdir / "config.txt", "w") as f:
        for k, v in combination.items():
            f.write(f"{k}: {v}\n")

    counter += 1


print("\nAll tests completed in {:.2f} minutes.".format((time.time() - t0) / 60))
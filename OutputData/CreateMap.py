import os
import csv

root_dir = "Localpp_MinBias2"          # change this to your top-level directory if needed
output_csv = root_dir+"_configssummary.csv"

rows = []

for dirpath, dirnames, filenames in os.walk(root_dir):
    if "config.txt" in filenames:
        config_path = os.path.join(dirpath, "config.txt")
        directory_name = os.path.basename(dirpath)

        cutMatchingChi2 = None
        askMinTPCRow = None

        with open(config_path, "r") as f:
            for line in f:
                line = line.strip()
                if line.startswith("cutMatchingChi2:"):
                    cutMatchingChi2 = line.split(":", 1)[1].strip()
                elif line.startswith("askMinTPCRow:"):
                    askMinTPCRow = line.split(":", 1)[1].strip()

        rows.append([
            directory_name,
            cutMatchingChi2,
            askMinTPCRow
        ])

# Write CSV
with open(output_csv, "w", newline="") as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["Directory Name", "cutMatchingChi2", "askMinTPCRow"])
    writer.writerows(rows)

print(f"CSV file written to: {output_csv}")

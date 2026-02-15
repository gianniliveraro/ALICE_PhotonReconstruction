from itertools import product
import json
import shutil
from pathlib import Path
import subprocess
import time
import argparse

OUTPUT_DIR = {"pp": "Localpp", "PbPb": "LocalPbPb"}
RUN_REFERENCE = True

CMD_MATCHERS = ["o2-secondary-vertexing-workflow"]

# ══════════════════════════════════════════════════════════════════════
#  PHOTON-RELEVANT PARAMETERS
# ══════════════════════════════════════════════════════════════════════

PHOTON_PARAMS = {
    "pp": {
        #  minCosPA: 3D pointing angle to PV.
        "svertexer.minCosPA": {
            "default": 0.9,
            "scan": [0.80, 0.85, 0.90, 0.95],
        },
        #  minCosPAXYMeanVertex: transverse pointing angle (prompt V0).
        "svertexer.minCosPAXYMeanVertex": {
            "default": 0.95,
            "scan": [0.90, 0.95, 0.98],
        },
        #  maxDCAXYToMeanVertex: how close V0 approaches beam line in XY.
        "svertexer.maxDCAXYToMeanVertex": {
            "default": 0.2,
            "scan": [0.1, 0.2, 0.5, 1.0],
        },
        #  minRToMeanVertex: minimum conversion radius.
        "svertexer.minRToMeanVertex": {
            "default": 0.5,
            "scan": [0.2, 0.5, 1.0, 2.0],
        },

        #  maxChi2: DCA of prongs to vertex.
        "svertexer.maxChi2": {
            "default": 2.0,
            "scan": [1.0, 2.0, 3.0, 5.0],
        },
        #  causalityRTolerance: V0 R cannot exceed contributors' minR
        "svertexer.causalityRTolerance": {
            "default": 1.0,
            "scan": [0.5, 1.0, 2.0, 4.0],
        },

        #  maxV0TglAbsDiff: |tgl(e+) - tgl(e-)| upper bound; e+e- from massless photon should have very similar tgl??? TODO: -- Discuss with Fredi + David
        "svertexer.maxV0TglAbsDiff": {
            "default": 0.3,
            "scan": [0.1, 0.2, 0.3, 0.5],
        },

        #  minDCAToPV: single-track DCA to PV.

        "svertexer.minDCAToPV": {
            "default": 0.05,
            "scan": [0.01, 0.03, 0.05, 0.10],
        },

        #  TPC-only photon parameters: MaxChi2
        "svertexer.mTPCTrackMaxChi2": {
            "default": 4.0,
            "scan": [2.0, 4.0, 6.0, 8.0],
        },
        #  TPC-only photon parameters: MaxDCAXY2ToMeanVertex
        "svertexer.mTPCTrackMaxDCAXY2ToMeanVertex": {
            "default": 2.0,
            "scan": [1.0, 2.0, 4.0],
        },
    },

    "PbPb": {

        "svertexer.minCosPA": {
            "default": 0.9,
            "scan": [0.90, 0.95, 0.98],
        },
        "svertexer.minCosPAXYMeanVertex": {
            "default": 0.95,
            "scan": [0.93, 0.95, 0.98],
        },
        "svertexer.maxDCAXYToMeanVertex": {
            "default": 0.2,
            "scan": [0.1, 0.2, 0.5],
        },
        "svertexer.minRToMeanVertex": {
            "default": 0.5,
            "scan": [0.5, 1.0, 2.0],
        },
        "svertexer.maxChi2": {
            "default": 2.0,
            "scan": [1.0, 2.0, 3.0, 4.0],
        },
        "svertexer.causalityRTolerance": {
            "default": 1.0,
            "scan": [0.5, 1.0, 2.0],
        },
        "svertexer.maxV0TglAbsDiff": {
            "default": 0.3,
            "scan": [0.1, 0.2, 0.3],
        },
        "svertexer.minDCAToPV": {
            "default": 0.05,
            "scan": [0.03, 0.05, 0.10, 0.15],
        },
        "svertexer.mTPCTrackMaxChi2": {
            "default": 4.0,
            "scan": [2.0, 4.0, 6.0],
        },
        "svertexer.mTPCTrackMaxDCAXY2ToMeanVertex": {
            "default": 2.0,
            "scan": [1.0, 2.0, 4.0],
        },
    },
}


def stage_is_target(stage: dict) -> bool:
    cmd = (stage.get("cmd", "") or "").lower()
    return any(m in cmd for m in CMD_MATCHERS)


def apply_configkeys_to_json(json_path: Path, combination: dict) -> bool:
    """Patch ONLY the target stage(s) in a workflow json."""
    with open(json_path) as f:
        json_dict = json.load(f)

    keys_to_set = list(combination.keys())
    modified_any = False

    for stage in json_dict.get("stages", []):
        if not stage_is_target(stage):
            continue

        cmd = stage.get("cmd", "")
        if "--configKeyValues" not in cmd:
            continue

        cmd_list = cmd.split()

        for i, token in enumerate(cmd_list):
            if token != "--configKeyValues":
                continue
            if i + 1 >= len(cmd_list):
                continue

            cfg = cmd_list[i + 1].strip('"')
            kv_pairs = [x.strip() for x in cfg.split(";") if x.strip()]

            new_pairs = [
                kv for kv in kv_pairs
                if kv.split("=", 1)[0].strip() not in keys_to_set
            ]
            for k in keys_to_set:
                new_pairs.append(f"{k}={combination[k]}")

            cmd_list[i + 1] = f"\"{';'.join(new_pairs)}\""
            stage["cmd"] = " ".join(cmd_list)

            modified_any = True
            break

    if modified_any:
        with open(json_path, "w") as f:
            json.dump(json_dict, f, indent=4)

    return modified_any


def prepare_test_workflow(outdir: str, combination: dict) -> bool:
    base = Path(outdir)
    ref_wf = base / "reference_workflow.json"
    target_wf = base / "workflow.json"

    if not ref_wf.exists():
        print(f"ERROR: {ref_wf} not found!")
        return False

    shutil.copy2(ref_wf, target_wf)
    patched = apply_configkeys_to_json(target_wf, combination)

    if patched:
        print(f"  Patched {target_wf} with: {combination}")
    else:
        print(f"  WARNING: Could not patch {target_wf}")
    return patched


def build_oat_combinations(params: dict) -> list[tuple[str, dict]]:
    """
    One-at-a-time: for each parameter, loop over its scan values while
    keeping everything else at the default.

    Returns list of (test_name, {key: value, ...}) tuples.
    """
    defaults = {k: v["default"] for k, v in params.items()}
    combos = []

    for scan_key, spec in params.items():
        short = scan_key.split(".")[-1]          # e.g. "minCosPA"
        for val in spec["scan"]:
            combo = dict(defaults)                # start from all defaults
            combo[scan_key] = val
            label = f"OAT_{short}_{val}"
            combos.append((label, combo))

    return combos


def build_grid_combinations(params: dict, grid_keys: list[str]) -> list[tuple[str, dict]]:

    defaults = {k: v["default"] for k, v in params.items()}

    grid_values = [params[k]["scan"] for k in grid_keys]
    combos = []

    for i, vals in enumerate(product(*grid_values)):
        combo = dict(defaults)
        parts = []
        for k, v in zip(grid_keys, vals):
            combo[k] = v
            parts.append(f"{k.split('.')[-1]}={v}")
        label = f"GRID_{i}_{'_'.join(parts)}"
        combos.append((label, combo))

    return combos


# ══════════════════════════════════════════════════════════════════════
#  Main
# ══════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description="Photon conversion sensitivity scan for the S-Vertexer"
    )
    parser.add_argument("--system", choices=["pp", "PbPb"], default="pp")
    parser.add_argument("--no-reference", action="store_true",
                        help="Skip the reference run (assumes it already exists)")

    # Scan mode
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--oat", action="store_true", default=True,
                      help="One-at-a-time scan (default)")
    mode.add_argument("--grid", nargs="+", metavar="PARAM",
                      help="Full grid over selected parameter short-names, "
                           "e.g. --grid minCosPA maxV0TglAbsDiff")

    parser.add_argument("--dry-run", action="store_true",
                        help="Print planned runs without executing")
    args = parser.parse_args()

    system = args.system
    outdir = OUTPUT_DIR[system]
    params = PHOTON_PARAMS[system]

    t0 = time.time()

    # ── Reference ──────────────────────────────────────────────────
    if RUN_REFERENCE and not args.no_reference:
        print(f"Running reference simulation for {system} (outdir={outdir})...")
        if not args.dry_run:
            subprocess.run(["./runbatch.sh", outdir, "Reference", system],
                           check=True)

    ref_wf = Path(outdir) / "reference_workflow.json"
    if not args.dry_run and not ref_wf.exists():
        print(f"FATAL: {ref_wf} does not exist after Reference run.")
        return

    # ── Build combinations ─────────────────────────────────────────
    if args.grid:
        # Map short names → full keys
        short_to_full = {k.split(".")[-1]: k for k in params}
        grid_keys = []
        for s in args.grid:
            if s not in short_to_full:
                print(f"ERROR: '{s}' not in params. Available: {list(short_to_full)}")
                return
            grid_keys.append(short_to_full[s])
        combos = build_grid_combinations(params, grid_keys)
        print(f"Grid scan over {args.grid}: {len(combos)} combinations")
    else:
        combos = build_oat_combinations(params)
        print(f"OAT scan: {len(combos)} runs across {len(params)} parameters")

    # ── Dry run ────────────────────────────────────────────────────
    if args.dry_run:
        for name, combo in combos:
            # Highlight what differs from default
            defaults = {k: v["default"] for k, v in params.items()}
            diff = {k: v for k, v in combo.items() if v != defaults[k]}
            print(f"  {name:40s}  varied: {diff}")
        print(f"\nTotal: {len(combos)} runs (+ 1 Reference)")
        return

    # ── Execute ────────────────────────────────────────────────────
    for counter, (test_name, combination) in enumerate(combos):
        print(f"\n=== [{counter+1}/{len(combos)}] {test_name}")

        ok = prepare_test_workflow(outdir, combination)
        if not ok:
            print(f"  SKIPPING {test_name}")
            continue

        subprocess.run(["./runbatch.sh", outdir, test_name, system], check=True)

        # Bookkeeping
        save_dir = Path(f"../OutputData/{outdir}/{test_name}")
        save_dir.mkdir(parents=True, exist_ok=True)
        with open(save_dir / "config.json", "w") as f:
            json.dump(combination, f, indent=2)

    elapsed = (time.time() - t0) / 60
    print(f"\nAll {len(combos)} tests completed in {elapsed:.1f} minutes.")


if __name__ == "__main__":
    main()
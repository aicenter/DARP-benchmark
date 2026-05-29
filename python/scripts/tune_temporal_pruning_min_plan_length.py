import argparse
import logging
import sys
from pathlib import Path

import darpbenchmark.log
from darpbenchmark.temporal_pruning_tuning import TemporalPruningMinPlanLengthTuner


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Tune IH temporal_pruning_min_plan_length for one DARP instance."
    )
    parser.add_argument(
        "instance_config",
        type=Path,
        help="Path to the instance config.yaml.",
    )
    parser.add_argument(
        "working_dir",
        type=Path,
        help="Directory where experiment configs, outputs, logs, and CSV summary are written.",
    )
    parser.add_argument(
        "initial_length_a",
        type=int,
        help="First endpoint of the inclusive min-plan-length search interval. Use 0 to include disabled pruning.",
    )
    parser.add_argument(
        "initial_length_b",
        type=int,
        help="Second endpoint of the inclusive min-plan-length search interval.",
    )
    parser.add_argument(
        "--executable",
        type=Path,
        default=Path("DARP-benchmark"),
        help="Path to the DARP-benchmark executable. Defaults to DARP-benchmark on PATH.",
    )
    parser.add_argument(
        "--tcount",
        type=int,
        default=1,
        help="Number of benchmark trials per length.",
    )
    parser.add_argument(
        "--tmax",
        type=int,
        default=0,
        help="Maximum number of OpenMP threads. 0 keeps benchmark default.",
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)
    tuner = TemporalPruningMinPlanLengthTuner(
        args.instance_config,
        args.working_dir,
        args.initial_length_a,
        args.initial_length_b,
        args.executable,
        args.tcount,
        args.tmax,
    )
    best = tuner.tune()
    print(
        "Best temporal_pruning_min_plan_length: "
        f"{best.length} "
        f"(total_time={best.total_time}, cost={best.cost}, dropped_requests={best.dropped_requests})"
    )
    print(f"Results written to: {tuner.results_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

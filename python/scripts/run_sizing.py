import argparse
import logging
import sys
from pathlib import Path

import darpbenchmark.log
from darpbenchmark.sizing import FleetSizing


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run vehicle sizing (IH binary search on vehicles.csv) for a DARP instance."
    )
    parser.add_argument(
        "experiment_config",
        type=Path,
        help="Path to experiment config.yaml (instance, method, outdir)",
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)
    config_path = args.experiment_config.resolve()
    if not config_path.is_file():
        parser.error(f"Experiment config not found: {config_path}")

    fleet_sizing = FleetSizing(config_path)
    fleet_sizing.calculate_sizing_for_instance()
    return 0


if __name__ == "__main__":
    sys.exit(main())

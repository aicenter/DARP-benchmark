#
# MIT License
#
# Copyright (c) 2026 Czech Technical University in Prague
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.#
import argparse
import logging
import sys
from pathlib import Path

import darpbenchmark.log
from darpbenchmark.sizing import CostPerRequestSizing, FleetSizing


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run vehicle sizing (IH runs with the first k vehicles of vehicles.csv) for one or more DARP "
                    "instances sharing the vehicle pool. By default, a binary search for the smallest k with which no "
                    "instance drops a request; with --min-cost, a scan for the minimum route cost per served request."
    )
    parser.add_argument(
        "experiment_config",
        type=Path,
        nargs="+",
        help="Path to experiment config.yaml (instance, method, outdir), one per instance",
    )
    parser.add_argument(
        "--executable",
        type=Path,
        help="Path to the DARP-benchmark executable (default: DARP-benchmark on PATH)",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=1,
        help="Number of instances solved in parallel (each DARP-benchmark process loads its travel time model)",
    )
    parser.add_argument(
        "--min-cost",
        action="store_true",
        help="Size for the minimum route cost per served request instead of serving all requests",
    )
    parser.add_argument(
        "--step",
        type=int,
        default=10,
        help="Step of the coarse vehicle count scan of --min-cost",
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)
    config_paths = [path.resolve() for path in args.experiment_config]
    for config_path in config_paths:
        if not config_path.is_file():
            parser.error(f"Experiment config not found: {config_path}")

    if args.min_cost:
        fleet_sizing = CostPerRequestSizing(config_paths, args.workers, args.step)
    else:
        fleet_sizing = FleetSizing(config_paths, args.workers)
    fleet_sizing.calculate_sizing(args.executable.resolve() if args.executable else None)
    return 0


if __name__ == "__main__":
    sys.exit(main())

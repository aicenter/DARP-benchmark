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

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
import json
import sys
from pathlib import Path

import pytest
import yaml

from darpbenchmark.sizing import FleetSizing, SIZING_FILE, VEHICLES_FILE

# The fake DARP-benchmark drops (required vehicles - vehicle count) requests, where the required vehicle count is
# set in the instance config. It logs its calls as "<instance dir name>,<vehicle count>".
FAKE_EXECUTABLE = f"""#!{sys.executable}
import argparse, json, pathlib, yaml
parser = argparse.ArgumentParser()
parser.add_argument("--instance", type=pathlib.Path)
parser.add_argument("--outdir", type=pathlib.Path)
parser.add_argument("--method")
args = parser.parse_args()
config = yaml.safe_load(args.instance.read_text())
vehicle_count = len((args.instance.parent / "vehicles.csv").read_text().splitlines())
dropped = max(0, config["required_vehicles"] - vehicle_count)
solution = {{"dropped_requests": list(range(dropped))}}
(args.outdir / f"{{args.instance.name}}-solution.json").write_text(json.dumps(solution))
with open(pathlib.Path(__file__).parent / "calls.log", "a") as log:
    log.write(f"{{args.instance.parent.name}},{{vehicle_count}}\\n")
"""


@pytest.fixture
def executable(tmp_path, monkeypatch) -> Path:
    # the experiment config loading changes the working directory
    monkeypatch.chdir(tmp_path)
    path = tmp_path / "DARP-benchmark"
    path.write_text(FAKE_EXECUTABLE)
    path.chmod(0o755)
    return path


def create_experiment(root: Path, name: str, required_vehicles: int, pool_size: int) -> Path:
    instance_dir = root / "instances" / name
    instance_dir.mkdir(parents=True)
    instance_config = {"vehicles": {"vehicle_count": pool_size}, "required_vehicles": required_vehicles}
    (instance_dir / "config.yaml").write_text(yaml.dump(instance_config))
    (instance_dir / VEHICLES_FILE).write_text("".join(f"{node}\t70\n" for node in range(pool_size)))
    experiment_dir = root / "experiments" / name
    experiment_dir.mkdir(parents=True)
    experiment_config = {"instance": f"../../instances/{name}/config.yaml", "outdir": ".", "method": "ih"}
    experiment_config_file = experiment_dir / "config.yaml"
    experiment_config_file.write_text(yaml.dump(experiment_config))
    return experiment_config_file


def read_calls(root: Path) -> list[tuple[str, int]]:
    lines = (root / "calls.log").read_text().splitlines()
    return [(name, int(count)) for name, count in (line.split(",") for line in lines)]


def read_sizing(sizing_file: Path) -> list[tuple[int, int, int]]:
    lines = sizing_file.read_text().splitlines()[1:]
    return [tuple(int(num) for num in line.split(",")) for line in lines]


def test_single_instance(tmp_path, executable):
    experiment = create_experiment(tmp_path, "day_1", required_vehicles=3, pool_size=8)
    vehicle_count = FleetSizing([experiment]).calculate_sizing(executable)

    assert vehicle_count == 3
    sizing_file = tmp_path / "instances" / "day_1" / SIZING_FILE
    assert read_sizing(sizing_file) == [(8, 0, 8), (4, 0, 4), (2, 1, 2), (3, 0, 1)]


def test_joint_sizing(tmp_path, executable):
    experiments = [
        create_experiment(tmp_path, "day_1", required_vehicles=7, pool_size=10),
        create_experiment(tmp_path, "day_2", required_vehicles=2, pool_size=10),
        create_experiment(tmp_path, "day_3", required_vehicles=4, pool_size=10),
    ]
    vehicle_count = FleetSizing(experiments, workers=2).calculate_sizing(executable)

    assert vehicle_count == 7
    sizing = read_sizing(tmp_path / "instances" / SIZING_FILE)
    assert sizing[-1] == (7, 0, 1)
    for name in ["day_1", "day_2", "day_3"]:
        instance_dir = tmp_path / "instances" / name
        assert len((instance_dir / VEHICLES_FILE).read_text().splitlines()) == 7
        assert len((instance_dir / "vehicles_pre_sizing.csv").read_text().splitlines()) == 10
        assert yaml.safe_load((instance_dir / "config.yaml").read_text())["vehicles"]["vehicle_count"] == 7
        solution = json.loads((tmp_path / "experiments" / name / "config.yaml-solution.json").read_text())
        assert solution["dropped_requests"] == []


def test_fail_fast_skips_pending_instances(tmp_path, executable):
    experiments = [
        create_experiment(tmp_path, "day_1", required_vehicles=5, pool_size=8),
        create_experiment(tmp_path, "day_2", required_vehicles=3, pool_size=8),
    ]
    FleetSizing(experiments).calculate_sizing(executable)

    calls = read_calls(tmp_path)
    # 4 vehicles are insufficient for day 1, so day 2 does not run with them
    assert ("day_1", 4) in calls
    assert ("day_2", 4) not in calls
    assert read_sizing(tmp_path / "instances" / SIZING_FILE)[1] == (4, 1, 4)


def test_resume(tmp_path, executable):
    experiment = create_experiment(tmp_path, "day_1", required_vehicles=3, pool_size=8)
    sizing_file = tmp_path / "instances" / "day_1" / SIZING_FILE
    sizing_file.write_text("vehicle_count,dropped_requests,interval_size\n8,0,8\n")

    assert FleetSizing([experiment]).calculate_sizing(executable) == 3
    assert read_calls(tmp_path)[0] == ("day_1", 4)


def test_finished_sizing_runs_nothing(tmp_path, executable):
    experiment = create_experiment(tmp_path, "day_1", required_vehicles=3, pool_size=8)
    sizing_file = tmp_path / "instances" / "day_1" / SIZING_FILE
    sizing_file.write_text("vehicle_count,dropped_requests,interval_size\n8,0,8\n5,0,1\n")

    assert FleetSizing([experiment]).calculate_sizing(executable) == 5
    assert not (tmp_path / "calls.log").exists()
    assert len((tmp_path / "instances" / "day_1" / VEHICLES_FILE).read_text().splitlines()) == 5


def test_different_vehicle_pools_raise(tmp_path, executable):
    experiments = [
        create_experiment(tmp_path, "day_1", required_vehicles=3, pool_size=8),
        create_experiment(tmp_path, "day_2", required_vehicles=3, pool_size=9),
    ]
    with pytest.raises(ValueError, match="differs"):
        FleetSizing(experiments).calculate_sizing(executable)


def test_insufficient_vehicle_pool_raises(tmp_path, executable):
    experiment = create_experiment(tmp_path, "day_1", required_vehicles=9, pool_size=8)
    with pytest.raises(ValueError, match="less than requested"):
        FleetSizing([experiment]).calculate_sizing(executable)

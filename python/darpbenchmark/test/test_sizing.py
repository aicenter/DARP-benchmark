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

from darpbenchmark.sizing import (
    COST_SIZING_FILE, DROPPED_REQUESTS_FILE, DROPPING_FILE, REQUEST_POOL_FILE, REQUESTS_FILE, SIZING_FILE, VEHICLES_FILE,
    CostPerRequestSizing, DroppingSizing, FleetSizing,
)

# The fake DARP-benchmark serves the requests of requests.csv (100 if there is none). With the IH method, it drops
# (required vehicles - vehicle count) requests, where the required vehicle count is set in the instance config, or
# computed from requests_per_vehicle. With the worst_removal method, it drops the requests whose "profitable" column
# is 0, and those with 2 when at most 7 requests remain. Its cost per served request is
# (vehicle count - cost optimum)^2 + 1 (one plan, the cost is its duration). It logs its calls as
# "<instance dir name>,<vehicle count>".
FAKE_EXECUTABLE = f"""#!{sys.executable}
import argparse, json, math, pathlib, yaml
parser = argparse.ArgumentParser()
parser.add_argument("--instance", type=pathlib.Path)
parser.add_argument("--outdir", type=pathlib.Path)
parser.add_argument("--method")
parser.add_argument("--drop_threshold")
args = parser.parse_args()
if args.drop_threshold is not None:
    (pathlib.Path(__file__).parent / "drop_threshold.log").write_text(args.drop_threshold)
config = yaml.safe_load(args.instance.read_text())
vehicle_count = len((args.instance.parent / "vehicles.csv").read_text().splitlines())
requests_file = args.instance.parent / "requests.csv"
rows = requests_file.read_text().splitlines()[1:] if requests_file.is_file() else ["1"] * 100
if args.method == "worst_removal":
    profitable = [int(row.split(",")[-1]) for row in rows]
    dropped = [i for i, p in enumerate(profitable) if p == 0 or (p == 2 and len(rows) <= 7)]
else:
    required = config.get("required_vehicles")
    if required is None:
        required = math.ceil(len(rows) / config["requests_per_vehicle"])
    dropped = list(range(min(len(rows), max(0, required - vehicle_count))))
served = len(rows) - len(dropped)
route_duration = served * ((vehicle_count - config.get("cost_optimum", 0)) ** 2 + 1)
plan = {{
    "departure_time": 0,
    "arrival_time": route_duration,
    "actions": [{{"action": {{"type": "pickup"}}}}] * served + [{{"action": {{"type": "drop_off"}}}}] * served,
    "cost_components": {{"travel_time": route_duration, "distance": 0}},
}}
solution = {{"cost": route_duration, "dropped_requests": [{{"index": i}} for i in dropped], "plans": [plan]}}
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


def create_experiment(
    root: Path, name: str, required_vehicles: int, pool_size: int, cost_optimum: int | None = None
) -> Path:
    instance_dir = root / "instances" / name
    instance_dir.mkdir(parents=True)
    instance_config = {"vehicles": {"vehicle_count": pool_size}, "required_vehicles": required_vehicles}
    if cost_optimum is not None:
        instance_config["cost_optimum"] = cost_optimum
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


def read_cost_sizing(sizing_file: Path) -> dict[int, tuple[int, int, float]]:
    lines = sizing_file.read_text().splitlines()[1:]
    records = {}
    for line in lines:
        vehicle_count, served, dropped, cost, _ = line.split(",")
        records[int(vehicle_count)] = (int(served), int(dropped), float(cost))
    return records


def test_min_cost_sizing(tmp_path, executable):
    experiments = [
        create_experiment(tmp_path, "day_1", required_vehicles=0, pool_size=30, cost_optimum=13),
        create_experiment(tmp_path, "day_2", required_vehicles=0, pool_size=30, cost_optimum=13),
    ]
    vehicle_count = CostPerRequestSizing(experiments, workers=2, step=10).calculate_sizing(executable)

    assert vehicle_count == 13
    records = read_cost_sizing(tmp_path / "instances" / COST_SIZING_FILE)
    # the coarse scan, then every vehicle count closer than the step to its best (10)
    assert sorted(records) == list(range(1, 21)) + [30]
    assert records[13] == (200, 0, 200.0)
    for name in ["day_1", "day_2"]:
        instance_dir = tmp_path / "instances" / name
        assert len((instance_dir / VEHICLES_FILE).read_text().splitlines()) == 13
        assert yaml.safe_load((instance_dir / "config.yaml").read_text())["vehicles"]["vehicle_count"] == 13
    # the solutions in the experiment dirs are those of the best vehicle count
    assert read_calls(tmp_path)[-2:] in ([("day_1", 13), ("day_2", 13)], [("day_2", 13), ("day_1", 13)])


def test_min_cost_sizing_counts_only_served_requests(tmp_path, executable):
    # with fewer than 5 vehicles, requests are dropped; the cost per served request still has its minimum at 3
    experiment = create_experiment(tmp_path, "day_1", required_vehicles=5, pool_size=20, cost_optimum=3)
    assert CostPerRequestSizing([experiment], step=5).calculate_sizing(executable) == 3
    records = read_cost_sizing(tmp_path / "instances" / "day_1" / COST_SIZING_FILE)
    assert records[3] == (98, 2, 98.0)


def test_min_cost_sizing_resume(tmp_path, executable):
    experiment = create_experiment(tmp_path, "day_1", required_vehicles=0, pool_size=30, cost_optimum=13)
    sizing_file = tmp_path / "instances" / "day_1" / COST_SIZING_FILE
    sizing_file.write_text(
        "vehicle_count,served_requests,dropped_requests,route_cost,cost_per_served_request\n"
        "10,100,0,1000.0,10.0\n"
    )

    assert CostPerRequestSizing([experiment], step=10).calculate_sizing(executable) == 13
    assert ("day_1", 10) not in read_calls(tmp_path)[:-1]


def create_dropping_experiment(root: Path, name: str, profitable: list[int], pool_size: int = 8) -> tuple[Path, Path]:
    """The IH and the worst_removal experiment of an instance with the requests of the given profitability."""
    experiment = create_experiment(root, name, required_vehicles=0, pool_size=pool_size)
    instance_dir = root / "instances" / name
    instance_config = yaml.safe_load((instance_dir / "config.yaml").read_text())
    del instance_config["required_vehicles"]
    instance_config["requests_per_vehicle"] = 2
    (instance_dir / "config.yaml").write_text(yaml.dump(instance_config))
    (instance_dir / REQUESTS_FILE).write_text(
        "time,origin,destination,profitable\n" + "".join(f"0,{i},{i + 1},{p}\n" for i, p in enumerate(profitable)))
    dropping_dir = root / "experiments_drop" / name
    dropping_dir.mkdir(parents=True)
    dropping_experiment = dropping_dir / "config.yaml"
    dropping_experiment.write_text(yaml.dump(
        {"instance": f"../../instances/{name}/config.yaml", "outdir": ".", "method": "worst_removal"}))
    return experiment, dropping_experiment


def read_dropping(path: Path) -> list[list[str]]:
    return [line.split(",") for line in path.read_text().splitlines()[1:]]


def test_dropping_rounds(tmp_path, executable):
    day_1 = create_dropping_experiment(tmp_path, "day_1", [1, 1, 0, 1, 1, 0, 1, 0, 2, 1])
    day_2 = create_dropping_experiment(tmp_path, "day_2", [1, 1, 1, 1, 1, 1])
    sizing = DroppingSizing([day_1[0], day_2[0]], [day_1[1], day_2[1]], workers=2)

    vehicle_count = sizing.calculate_sizing(executable)

    # round 1: 10 requests need 5 vehicles, 3 are dropped; round 2: 7 need 4, the request at 8 is dropped;
    # round 3: 6 need 3, nothing is dropped
    assert vehicle_count == 3
    rounds = read_dropping(tmp_path / "instances" / DROPPING_FILE)
    assert [(r[0], r[1], r[3]) for r in rounds] == [("1", "5", "3"), ("2", "4", "1"), ("3", "3", "0")]
    assert rounds[0][2] == str(7 + 6)
    instance_dir = tmp_path / "instances" / "day_1"
    assert (instance_dir / DROPPED_REQUESTS_FILE).read_text().splitlines()[1:] == ["1,2", "1,5", "1,7", "2,8"]
    remaining = [line.split(",")[1] for line in (instance_dir / REQUESTS_FILE).read_text().splitlines()[1:]]
    assert remaining == ["0", "1", "3", "4", "6", "9"]
    assert len((instance_dir / REQUEST_POOL_FILE).read_text().splitlines()) == 11
    assert len((instance_dir / VEHICLES_FILE).read_text().splitlines()) == 3
    assert (tmp_path / "instances" / "sizing_round_2.csv").is_file()


def test_dropping_resume_and_finished(tmp_path, executable):
    day_1 = create_dropping_experiment(tmp_path, "day_1", [1, 1, 0, 1, 1, 0, 1, 0, 2, 1])
    sizing = DroppingSizing([day_1[0]], [day_1[1]])
    sizing.calculate_sizing(executable)
    calls_file = tmp_path / "calls.log"
    calls_file.unlink()

    # a finished sizing runs nothing
    assert DroppingSizing([day_1[0]], [day_1[1]]).calculate_sizing(executable) == 3
    assert not calls_file.exists()

    # an interrupted third round (logged drops, no round record) is repeated without duplicating them
    dropping_file = tmp_path / "instances" / "day_1" / DROPPING_FILE
    dropping_file.write_text("\n".join(dropping_file.read_text().splitlines()[:-1]) + "\n")
    dropped_file = tmp_path / "instances" / "day_1" / DROPPED_REQUESTS_FILE
    dropped_file.write_text(dropped_file.read_text() + "3,0\n")
    assert DroppingSizing([day_1[0]], [day_1[1]]).calculate_sizing(executable) == 3
    assert dropped_file.read_text().splitlines()[1:] == ["1,2", "1,5", "1,7", "2,8"]
    assert len(read_dropping(dropping_file)) == 3


def test_dropping_threshold_is_passed(tmp_path, executable):
    day_1 = create_dropping_experiment(tmp_path, "day_1", [1, 1, 1, 1])
    sizing = DroppingSizing([day_1[0]], [day_1[1]], drop_threshold=60)
    parameters = sizing.dropping.instances[0].load_experiment_parameters()
    assert "drop_threshold" not in parameters
    sizing.calculate_sizing(executable)
    assert float((tmp_path / "drop_threshold.log").read_text()) == 60

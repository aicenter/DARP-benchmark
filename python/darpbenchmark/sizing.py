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
import logging
import math
import os
import shutil
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json

import yaml

import darpbenchmark.experiments
from darpinstances.experiments import load_experiment_config
from darpinstances.utils import load_yaml

VEHICLES_FILE = "vehicles.csv"
VEHICLE_POOL_FILE = "vehicles_pre_sizing.csv"
SIZING_FILE = "sizing.csv"
SIZING_HEADER = "vehicle_count,dropped_requests,interval_size\n"

# the sizing ends when no request is dropped and the search interval is at most this size
EPSILON = 1

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class SizingInstance:
    """
    One instance of the fleet sizing: its experiment, vehicle pool and result files.
    The fleet of size k consists of the first k vehicles of the vehicle pool.
    """
    def __init__(self, experiment_config_file: Path):
        self.experiment_config_file = experiment_config_file
        self.instance_config_file = self.get_instance_config_file_for_experiment()
        self.solution_file = experiment_config_file.parent / f"{self.instance_config_file.name}-solution.json"
        self.performance_file = experiment_config_file.parent / f"{self.instance_config_file.name}-performance.json"
        self.solution_backup_file = self.solution_file.with_name(self.solution_file.name + ".bak")
        self.performance_backup_file = self.performance_file.with_name(self.performance_file.name + ".bak")
        self.vehicles_file = self.instance_config_file.parent / VEHICLES_FILE
        self.vehicle_pool_file = self.instance_config_file.parent / VEHICLE_POOL_FILE

    def get_instance_config_file_for_experiment(self) -> Path:
        cfg = load_yaml(self.experiment_config_file)
        return (self.experiment_config_file.parent / Path(cfg["instance"])).resolve()

    def backup_vehicle_pool(self):
        """The vehicles file before the sizing is the vehicle pool. It is backed up once, by the first sizing run."""
        if not self.vehicle_pool_file.is_file():
            shutil.copy(self.vehicles_file, self.vehicle_pool_file)

    def get_vehicle_pool(self) -> list[str]:
        with open(self.vehicle_pool_file, "rt", encoding="utf-8") as f:
            return f.readlines()

    def set_vehicles_file(self, vehicle_count: int):
        vehicles = self.get_vehicle_pool()
        if vehicle_count > len(vehicles):
            logger.error(
                f"{vehicle_count} vehicles requested from vehicle pool {self.vehicle_pool_file}, it only has {len(vehicles)}")
            raise ValueError("Vehicle pool had less than requested vehicle count!")

        with open(self.vehicles_file, "wt", encoding="utf-8") as f:
            f.writelines(vehicles[:vehicle_count])

    def _backup_and_remove(self, result_file: Path, backup_file: Path) -> None:
        if result_file.is_file():
            shutil.copy2(result_file, backup_file)
            result_file.unlink()

    def delete_result_files(self):
        self._backup_and_remove(self.solution_file, self.solution_backup_file)
        self._backup_and_remove(self.performance_file, self.performance_backup_file)

    def get_dropped_requests_count(self):
        with open(self.solution_file, "rt", encoding="utf-8") as json_data:
            solution = json.load(json_data)
        return len(solution["dropped_requests"])

    def get_route_cost(self) -> tuple[int, float]:
        """
        The served request count and the route cost of the solution: the instance's travel time weight applies to the
        whole duration of each plan (travel, service and waiting, as the vehicle is paid for its route), plus the other
        cost components of the plans (e.g., distance).
        """
        with open(self.solution_file, "rt", encoding="utf-8") as json_data:
            solution = json.load(json_data)
        travel_time_weight = load_yaml(self.instance_config_file).get("cost", {}).get("travel_time_weight", 1)
        served_requests = sum(
            1 for plan in solution["plans"] for action in plan["actions"] if action["action"]["type"] == "pickup")
        route_cost = 0.0
        for plan in solution["plans"]:
            route_cost += (plan["arrival_time"] - plan["departure_time"]) * travel_time_weight
            route_cost += sum(value for name, value in plan["cost_components"].items() if name != "travel_time")
        return served_requests, route_cost

    def load_experiment_parameters(self) -> dict:
        """The DARP-benchmark parameters of the experiment. Not thread-safe, the loading changes the working directory."""
        return load_experiment_config(self.experiment_config_file)

    def set_instance_config_vehicle_count(self, vehicle_count: int):
        instance_config = load_yaml(self.instance_config_file)
        instance_config["vehicles"].pop("vehicle_to_request_ratio", None)
        instance_config["vehicles"]["vehicle_count"] = vehicle_count
        with open(self.instance_config_file, "wt", encoding="utf-8") as f:
            yaml.dump(instance_config, f, sort_keys=False)


class FleetSizing:
    """
    Fleet sizing by the binary search on the vehicle count, each vehicle count is tested by IH.

    With several instances (e.g., the days of a week), all of them must have the same vehicle pool and a vehicle
    count is sufficient only if no request is dropped in any of the instances. The instances of a vehicle count run
    in parallel in `workers` processes. Once an instance drops a request, the instances not yet started are skipped.
    """
    def __init__(self, experiment_config_files: list[Path], workers: int = 1):
        assert experiment_config_files, "At least one experiment config is required"
        assert workers > 0, "The worker count must be positive"
        self.instances = [SizingInstance(file) for file in experiment_config_files]
        self.workers = workers
        instance_dirs = [instance.instance_config_file.parent for instance in self.instances]
        self.sizing_file = Path(os.path.commonpath(instance_dirs)) / SIZING_FILE

    def check_vehicle_pools(self):
        for instance in self.instances:
            instance.backup_vehicle_pool()
        vehicle_pool = self.instances[0].get_vehicle_pool()
        for instance in self.instances[1:]:
            if instance.get_vehicle_pool() != vehicle_pool:
                raise ValueError(
                    f"The vehicle pool {instance.vehicle_pool_file} differs from {self.instances[0].vehicle_pool_file}")
        return len(vehicle_pool)

    def write_sizing_info(self, vehicle_count: int, dropped_requests: int, interval_size: int):
        with open(self.sizing_file, "at", encoding="utf-8") as f:
            f.write(f"{vehicle_count},{dropped_requests},{interval_size}\n")

    def get_last_sizing_record(self) -> tuple[int, int, int] | None:
        if not self.sizing_file.is_file():
            return None
        with open(self.sizing_file, "rt", encoding="utf-8") as f:
            lines = [line.strip() for line in f.readlines() if line.strip()]
        if len(lines) <= 1:
            return None
        vehicle_count, dropped_requests, interval_size = (int(num) for num in lines[-1].split(","))
        return vehicle_count, dropped_requests, interval_size

    @staticmethod
    def is_final(dropped_requests: int, interval_size: int) -> bool:
        return dropped_requests == 0 and interval_size <= EPSILON

    @staticmethod
    def compute_next_sizing_parameters(record: tuple[int, int, int]) -> tuple[int, int]:
        previous_vehicle_count, previous_dropped_requests, previous_interval_size = record
        interval_size = math.ceil(previous_interval_size / 2)
        if previous_dropped_requests == 0:
            vehicle_count = previous_vehicle_count - interval_size
        else:
            vehicle_count = previous_vehicle_count + interval_size
        return vehicle_count, interval_size

    def run_instances(
        self,
        vehicle_count: int,
        experiment_parameters: list[dict],
        executable_path: Path | None,
        fail_fast: bool
    ) -> list[int | None]:
        """
        Runs all instances with the given vehicle count, returns the dropped requests of each instance. With `fail_fast`,
        the instances not yet started once an instance drops a request are skipped (None).
        """
        for instance in self.instances:
            instance.set_vehicles_file(vehicle_count)
            instance.delete_result_files()

        stop = threading.Event()

        def run_instance(instance: SizingInstance, parameters: dict) -> int | None:
            if stop.is_set():
                logger.debug(f"Skipping {instance.experiment_config_file}, the vehicle count is already insufficient")
                return None
            parameters = dict(parameters)
            timeout = parameters.pop("timeout", None)
            if not darpbenchmark.experiments.call_experiment_runner_plain(parameters, timeout, executable_path):
                raise RuntimeError(f"The experiment {instance.experiment_config_file} failed")
            dropped_requests_count = instance.get_dropped_requests_count()
            logger.info(f"{instance.experiment_config_file}: {vehicle_count} vehicles, "
                        f"{dropped_requests_count} dropped requests")
            if fail_fast and dropped_requests_count > 0:
                stop.set()
            return dropped_requests_count

        with ThreadPoolExecutor(max_workers=self.workers) as executor:
            return list(executor.map(run_instance, self.instances, experiment_parameters))

    def run_step(
        self,
        vehicle_count: int,
        experiment_parameters: list[dict],
        executable_path: Path | None
    ) -> int:
        """Runs all instances with the given vehicle count, returns the dropped requests summed over the instances that ran."""
        results = self.run_instances(vehicle_count, experiment_parameters, executable_path, fail_fast=True)
        return sum(result for result in results if result is not None)

    def run_vehicle_sizing_experiments(self, executable_path: Path | None = None) -> int:
        """
        Runs the binary search, resuming from the last record of the sizing file if it exists.
        The initial vehicle count and interval size are the size of the vehicle pool.
        Returns the final vehicle count, i.e., the last tested one.
        """
        pool_size = self.check_vehicle_pools()

        record = self.get_last_sizing_record()
        if record is None:
            with open(self.sizing_file, "wt", encoding="utf-8") as f:
                f.write(SIZING_HEADER)
            vehicle_count = pool_size
            interval_size = pool_size
        elif self.is_final(record[1], record[2]):
            logger.info(f"The sizing in {self.sizing_file} is already finished")
            return record[0]
        else:
            vehicle_count, interval_size = self.compute_next_sizing_parameters(record)

        # loaded in advance, the loading changes the working directory
        experiment_parameters = [instance.load_experiment_parameters() for instance in self.instances]

        logger.info(f"Starting sizing with vehicle count: {vehicle_count} and interval size: {interval_size}")
        while True:
            dropped_requests_count = self.run_step(vehicle_count, experiment_parameters, executable_path)
            self.write_sizing_info(vehicle_count, dropped_requests_count, interval_size)
            logger.info(f"Vehicle count: {vehicle_count}, dropped requests: {dropped_requests_count}, "
                        f"interval size: {interval_size}")
            if self.is_final(dropped_requests_count, interval_size):
                return vehicle_count
            vehicle_count, interval_size = self.compute_next_sizing_parameters(
                (vehicle_count, dropped_requests_count, interval_size))
            logger.info(f"Next sizing parameters: vehicle count: {vehicle_count} and interval size: {interval_size}")

    def calculate_sizing(self, executable_path: Path | None = None) -> int:
        """
        Sizes the fleet and sets the result in the instances: the vehicles file holds the first k vehicles of the pool
        and the instance config has `vehicles.vehicle_count` k. The solutions in the experiment dirs are those of k.
        """
        for instance in self.instances:
            assert instance.instance_config_file.is_file(), \
                f"Instance config file {instance.instance_config_file} does not exist"

        vehicle_count = self.run_vehicle_sizing_experiments(executable_path)

        for instance in self.instances:
            instance.set_vehicles_file(vehicle_count)
            instance.set_instance_config_vehicle_count(vehicle_count)

        logger.info(f"FINISHED: vehicle count {vehicle_count} saved in {self.sizing_file}")
        return vehicle_count


COST_SIZING_FILE = "cost_sizing.csv"
COST_SIZING_HEADER = "vehicle_count,served_requests,dropped_requests,route_cost,cost_per_served_request\n"


class CostPerRequestSizing(FleetSizing):
    """
    Fleet sizing for the minimum route cost per served request (dropped requests bring no revenue and no cost), see
    `SizingInstance.get_route_cost`. The costs and served requests are summed over all instances. The cost per served
    request need not be monotone or unimodal in the vehicle count, so the search is a scan: every `step`-th vehicle
    count of the pool, then every vehicle count closer than `step` to the best one.
    """
    def __init__(self, experiment_config_files: list[Path], workers: int = 1, step: int = 10):
        super().__init__(experiment_config_files, workers)
        assert step > 0, "The scan step must be positive"
        self.step = step
        self.sizing_file = self.sizing_file.with_name(COST_SIZING_FILE)

    def get_sizing_records(self) -> dict[int, tuple[int, int, float]]:
        """vehicle count -> (served requests, dropped requests, route cost) of the evaluated vehicle counts"""
        if not self.sizing_file.is_file():
            return {}
        with open(self.sizing_file, "rt", encoding="utf-8") as f:
            lines = [line.strip() for line in f.readlines()[1:] if line.strip()]
        records = {}
        for line in lines:
            vehicle_count, served_requests, dropped_requests, route_cost, _ = line.split(",")
            records[int(vehicle_count)] = (int(served_requests), int(dropped_requests), float(route_cost))
        return records

    @staticmethod
    def cost_per_served_request(record: tuple[int, int, float]) -> float:
        served_requests, _, route_cost = record
        return route_cost / served_requests if served_requests else math.inf

    def evaluate(
        self,
        vehicle_count: int,
        experiment_parameters: list[dict],
        executable_path: Path | None
    ) -> tuple[int, int, float]:
        dropped = self.run_instances(vehicle_count, experiment_parameters, executable_path, fail_fast=False)
        served_requests = 0
        route_cost = 0.0
        for instance in self.instances:
            instance_served_requests, instance_route_cost = instance.get_route_cost()
            served_requests += instance_served_requests
            route_cost += instance_route_cost
        record = (served_requests, sum(dropped), route_cost)
        with open(self.sizing_file, "at", encoding="utf-8") as f:
            f.write(f"{vehicle_count},{served_requests},{sum(dropped)},{route_cost:.4f},"
                    f"{self.cost_per_served_request(record):.4f}\n")
        logger.info(f"Vehicle count: {vehicle_count}, served requests: {served_requests}, dropped requests: "
                    f"{sum(dropped)}, cost per served request: {self.cost_per_served_request(record):.4f}")
        return record

    def best_vehicle_count(self, records: dict[int, tuple[int, int, float]]) -> int:
        # ties go to the smaller fleet
        return min(records, key=lambda count: (self.cost_per_served_request(records[count]), count))

    def calculate_sizing(self, executable_path: Path | None = None) -> int:
        """
        Scans the vehicle counts, resuming from the sizing file, and sets the best one in the instances: the vehicles
        file holds the first k vehicles of the pool, the instance config has `vehicles.vehicle_count` k, and the
        solutions in the experiment dirs are those of k.
        """
        for instance in self.instances:
            assert instance.instance_config_file.is_file(), \
                f"Instance config file {instance.instance_config_file} does not exist"
        pool_size = self.check_vehicle_pools()
        records = self.get_sizing_records()
        if not records:
            with open(self.sizing_file, "wt", encoding="utf-8") as f:
                f.write(COST_SIZING_HEADER)

        # loaded in advance, the loading changes the working directory
        experiment_parameters = [instance.load_experiment_parameters() for instance in self.instances]

        def scan(vehicle_counts):
            for vehicle_count in vehicle_counts:
                if vehicle_count not in records:
                    records[vehicle_count] = self.evaluate(vehicle_count, experiment_parameters, executable_path)

        scan(range(self.step, pool_size + 1, self.step))
        coarse_best = self.best_vehicle_count(records)
        scan(range(max(1, coarse_best - self.step + 1), min(pool_size, coarse_best + self.step - 1) + 1))
        vehicle_count = self.best_vehicle_count(records)

        # the solutions of the best vehicle count
        self.run_instances(vehicle_count, experiment_parameters, executable_path, fail_fast=False)
        for instance in self.instances:
            instance.set_instance_config_vehicle_count(vehicle_count)

        logger.info(f"FINISHED: vehicle count {vehicle_count} with cost per served request "
                    f"{self.cost_per_served_request(records[vehicle_count]):.4f}, records in {self.sizing_file}")
        return vehicle_count

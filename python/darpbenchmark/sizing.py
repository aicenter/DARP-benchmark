import logging
import math

import shutil

from pathlib import Path
import json

import pandas as pd
import yaml

import darpbenchmark.experiments
from darpinstances.utils import load_yaml

RESULT_PERFORMANCE = "config.yaml-performance.json"
VEHICLES_FILE = "vehicles.csv"
SIZING_FILE = "sizing.csv"

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class FleetSizing:
    def __init__(self, experiment_config_file: Path):
        self.experiment_config_file = experiment_config_file
        self.instance_config_file = self.get_instance_config_file_for_experiment()
        self.solution_file = experiment_config_file.parent / f"{self.instance_config_file.name}-solution.json"
        self.performance_file = experiment_config_file.parent / f"{self.instance_config_file.name}-performance.json"
        self.solution_backup_file = self.solution_file.with_name(self.solution_file.name + ".bak")
        self.performance_backup_file = self.performance_file.with_name(self.performance_file.name + ".bak")
        self.sizing_file = self.instance_config_file.parent / SIZING_FILE
        self.vehicles_file = self.instance_config_file.parent / VEHICLES_FILE


    def get_dropped_requests_count(self):
        with open(self.solution_file, "rt", encoding="utf-8") as json_data:
            solution = json.load(json_data)

        # logger.debug(f"Dropped requests: {solution['dropped_requests']}")

        return len(solution["dropped_requests"])


    def _backup_and_remove(self, result_file: Path, backup_file: Path) -> None:
        if result_file.is_file():
            shutil.copy2(result_file, backup_file)
            result_file.unlink()

    def delete_result_files(self):
        self._backup_and_remove(self.solution_file, self.solution_backup_file)
        self._backup_and_remove(self.performance_file, self.performance_backup_file)

    def recover_result_files(self) -> None:
        if not self.solution_file.is_file() and self.solution_backup_file.is_file():
            shutil.copy2(self.solution_backup_file, self.solution_file)
            logger.debug(f"Recovered solution from {self.solution_backup_file}")
        if not self.performance_file.is_file() and self.performance_backup_file.is_file():
            shutil.copy2(self.performance_backup_file, self.performance_file)
            logger.debug(f"Recovered performance from {self.performance_backup_file}")


    def set_vehicles_file(self, vehicle_count: int):
        with open(self.vehicles_file, "rt", encoding="utf-8") as f:
            vehicles = f.readlines()
        # handle the case where interval halving would increase vehicle count
        initial_vehicle_count = len(vehicles)
        if vehicle_count > initial_vehicle_count:
            logger.error(
                f"{vehicle_count} vehicles requested from vehicles file {initial_vehicle_count}, it only has {initial_vehicle_count} ")
            raise ValueError("Vehicles file had less than requested vehicle count!")

        with open(self.vehicles_file, "wt", encoding="utf-8") as f:
            f.writelines(vehicles[:vehicle_count])


    def write_sizing_info(self, vehicle_count: int, dropped_requests: int, interval_size: int):
        with open(self.sizing_file, "at", encoding="utf-8") as f:
            f.write(f"{vehicle_count},{dropped_requests},{interval_size}\n")


    def get_vehicle_count(self):
        with open(self.vehicles_file, "rt", encoding="utf-8") as f:
            initial_vehicle_count = len(f.readlines())
        logger.debug(f"Initial vehicle count: {initial_vehicle_count}")
        return initial_vehicle_count


    def get_previous_sizing_info(self):
        with open(self.sizing_file, "rt", encoding="utf-8") as f:
            last_line = f.readlines()[-1]
        previous_vehicle_count, previous_dropped_vehicles, interval_size = (int(num) for num in last_line.split(","))
        return previous_vehicle_count, previous_dropped_vehicles, interval_size


    def restore_orig_files_in_directory(directory: Path):
        for backup_file in directory.parent.glob("*ORIG"):
            original_filename = (backup_file.parent / backup_file.name.replace("ORIG", ""))
            try:
                original_filename.unlink()
            except FileNotFoundError:
                pass
            backup_file.rename(original_filename)


    def get_instance_config_file_for_experiment(self) -> Path:
        cfg = load_yaml(self.experiment_config_file)
        return self.experiment_config_file.parent / Path(cfg["instance"])


    # @dataclass
    # class SizingInfo:
    #     """Class for keeping track of values in the sizing experiments."""
    #     vehicle_count:int
    #     dropped_requests_count:int
    #     interval_size:int


    def restore_or_backup_initial_vehicle_file_and_get_count(self):
        vehicles_file_orig = self.vehicles_file.parent / "vehicles_pre_sizing.csv"
        if not vehicles_file_orig.is_file():
            # back up if not yet backed up
            shutil.copy(self.vehicles_file, vehicles_file_orig)
        else:
            # restore from backup if backup exists
            self.vehicles_file.unlink()
            shutil.copy(vehicles_file_orig, self.vehicles_file)


    def sizing_did_run(self) -> bool:
        """
        Checks whether sizing.csv exists and has more than the header line.
        If it does, it means that the sizing experiment failed to run.
        """
        did_run = False
        if self.sizing_file.is_file():
            with open(self.sizing_file, "rt", encoding="utf-8") as f:
                lines = f.readlines()
            if len(lines) > 1:  # if it is not just headers
                did_run = True
            else:
                self.sizing_file.unlink()
        return did_run

    def get_last_sizing_record(self) -> tuple[int, int, int] | None:
        if not self.sizing_file.is_file():
            return None
        with open(self.sizing_file, "rt", encoding="utf-8") as f:
            lines = [line.strip() for line in f.readlines() if line.strip()]
        if len(lines) <= 1:
            return None
        vehicle_count, dropped_requests, interval_size = (int(num) for num in lines[-1].split(","))
        return vehicle_count, dropped_requests, interval_size

    def last_sizing_record_matches_solution(self) -> bool:
        record = self.get_last_sizing_record()
        if record is None:
            return False
        csv_vehicle_count, csv_dropped_requests, _ = record
        return (
            csv_vehicle_count == self.get_vehicle_count()
            and csv_dropped_requests == self.get_dropped_requests_count()
        )

    def compute_next_sizing_parameters(self) -> tuple[int, int]:
        previous_vehicle_count, previous_dropped_vehicles, previous_interval_size = self.get_previous_sizing_info()
        interval_size = math.ceil(previous_interval_size / 2)
        if previous_dropped_vehicles == 0:
            vehicle_count = previous_vehicle_count - interval_size
        else:
            vehicle_count = previous_vehicle_count + interval_size
        return vehicle_count, interval_size

    def interval_size_for_unrecorded_solution(self) -> int:
        record = self.get_last_sizing_record()
        if record is None:
            return self.get_vehicle_count()
        _, _, previous_interval_size = record
        return math.ceil(previous_interval_size / 2)

    def ensure_sizing_csv_header(self):
        if not self.sizing_file.is_file():
            with open(self.sizing_file, "wt", encoding="utf-8") as f:
                f.write("vehicle_count,dropped_requests,interval_size\n")

    def run_vehicle_sizing_experiments(self):
        """
        Run vehicle sizing for a given instance and experiment config file.
        epsilon_percentage is the percentage of the vehicle count that is used for epsilon, a stopping criterion.
        By default, its 5% of the initial vehicle count. The initial vehicle count is presumed to be based on experimental
        config.yaml with `vehicle_to_request_ratio: 1`, meaning one vehicle per request.
        """
        epsilon = 1
        dropped_requests_count = 0

        self.recover_result_files()

        if self.solution_file.is_file():
            # if not self.last_sizing_record_matches_solution():
            #     vehicle_count = self.get_vehicle_count()
            #     interval_size = self.interval_size_for_unrecorded_solution()
            #     self.ensure_sizing_csv_header()
            #     dropped_requests_count = self.get_dropped_requests_count()
            #     self.write_sizing_info(vehicle_count, dropped_requests_count, interval_size)
            #     logger.debug(
            #         f"Recorded unwritten sizing result: vehicle_count={vehicle_count}, "
            #         f"dropped_requests={dropped_requests_count}, interval_size={interval_size}"
            #     )
            
            vehicle_count, interval_size = self.compute_next_sizing_parameters()
        else:
            with open(self.sizing_file, "wt", encoding="utf-8") as f:
                f.write("vehicle_count,dropped_requests,interval_size\n")
            initial_vehicle_count = self.get_vehicle_count()
            vehicle_count = initial_vehicle_count
            interval_size = initial_vehicle_count
            # epsilon = math.ceil(epsilon_percentage * initial_vehicle_count)  # 5% of vehicle count
            assert epsilon > 0, "Epsilon must be greater than 0"      

        logger.info(f"Starting sizing with vehicle count: {vehicle_count} and interval size: {interval_size}")

        while not (dropped_requests_count == 0 and interval_size <= epsilon):
            # 1. set vehicles file
            self.restore_or_backup_initial_vehicle_file_and_get_count()
            self.set_vehicles_file(vehicle_count)

            # 2. run experiment
            self.delete_result_files()
            darpbenchmark.experiments.run_experiment_using_config(str(self.experiment_config_file), executable_path=Path(r"C:\Workspaces\AIC\DARP-Benchmark\cmake-build-release/DARP-Benchmark.exe"))

            # 3. read dropped vehicle count
            dropped_requests_count = self.get_dropped_requests_count()

            # 4. write sizing info
            self.write_sizing_info(vehicle_count, dropped_requests_count, interval_size)
            logger.debug(f"Writing vehicle count: {vehicle_count}, dropped_requests: {dropped_requests_count}, "
                        f"interval_size: {interval_size}, epsilon: {epsilon}")
            
            # 5. compute next sizing parameters
            vehicle_count, interval_size = self.compute_next_sizing_parameters()
            logger.info(f"Next sizing parameters: vehicle count: {vehicle_count} and interval size: {interval_size}")

        logger.debug(
            f"Sizing for instance {self.instance_config_file} finished. Vehicle count: {vehicle_count} with interval {interval_size}")
        return vehicle_count, interval_size, epsilon

    def set_instance_config_vehicle_count(self, vehicle_count):
        instance_config = load_yaml(self.instance_config_file)
        instance_config["vehicles"].pop("vehicle_to_request_ratio")
        instance_config["vehicles"]["vehicle_count"] = vehicle_count
        with open(self.instance_config_file, "wt", encoding="utf-8") as f:
            yaml.dump(instance_config, f)


    def get_sizing_output(self, sizing_file: Path):
        sizing_output = pd.read_csv(sizing_file, header=0, sep=",")
        vehicle_count, dropped_requests, interval_size = sizing_output.iloc[-1]
        return vehicle_count, dropped_requests, interval_size


    def calculate_sizing_for_instance(self):
        assert self.instance_config_file.is_file(), f"Instance config file {self.instance_config_file} does not exist"
        assert self.experiment_config_file.is_file(), f"Experiment config file {self.experiment_config_file} does not exist"

        # Check sizing was not already performed
        instance_config = load_yaml(self.instance_config_file)
        if hasattr(instance_config, "vehicles") and "vehicle_count" in instance_config["vehicles"]:
            logger.error(f"The instance already contains sizing info! {instance_config}")
            return None


        # Run vehicle sizing
        vehicle_count, interval_size, epsilon = self.run_vehicle_sizing_experiments()

        logger.info(
            f"FINISHED: SAVED VEHICLE COUNT {vehicle_count} in sizing.csv for instance {self.instance_config_file}")

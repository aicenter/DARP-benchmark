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
RESULT_SOLUTION = "config.yaml-solution.json"
RESULT_FILES = [RESULT_PERFORMANCE, RESULT_SOLUTION]
VEHICLES_FILE = "vehicles.csv"
SIZING_FILE = "sizing.csv"

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


def get_dropped_requests_count(experiment_config_file: Path):
    with open(experiment_config_file.parent / RESULT_SOLUTION, "rt", encoding="utf-8") as json_data:
        solution = json.load(json_data)

    # logger.debug(f"Dropped requests: {solution['dropped_requests']}")

    return len(solution["dropped_requests"])


def delete_result_files(experiment_config_file: Path):
    try:
        for file in RESULT_FILES:
            (experiment_config_file.parent / file).unlink()
    except FileNotFoundError:
        logger.warning(f"No result files were present in {experiment_config_file.parent}")


def set_vehicles_file(initial_vehicles_file: Path, vehicle_count: int):
    with open(initial_vehicles_file, "rt", encoding="utf-8") as f:
        vehicles = f.readlines()
    # handle the case where interval halving would increase vehicle count
    initial_vehicle_count = len(vehicles)
    if vehicle_count > initial_vehicle_count:
        logger.error(
            f"{vehicle_count} vehicles requested from vehicles file {initial_vehicle_count}, it only has {initial_vehicle_count} ")
        raise ValueError("Vehicles file had less than requested vehicle count!")

    with open(initial_vehicles_file, "wt", encoding="utf-8") as f:
        f.writelines(vehicles[:vehicle_count])


def write_sizing_info(sizing_file: Path, vehicle_count: int, dropped_requests: int, interval_size: int):
    with open(sizing_file, "at", encoding="utf-8") as f:
        f.write(f"{vehicle_count},{dropped_requests},{interval_size}\n")


def get_vehicle_count(instance_vehicle_file: Path):
    with open(instance_vehicle_file, "rt", encoding="utf-8") as f:
        initial_vehicle_count = len(f.readlines())
    logger.debug(f"Initial vehicle count: {initial_vehicle_count}")
    return initial_vehicle_count


def get_previous_sizing_info(sizing_file: Path):
    with open(sizing_file, "rt", encoding="utf-8") as f:
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


def get_instance_config_file_for_experiment(experiment_config_file: Path) -> Path:
    cfg = load_yaml(experiment_config_file)
    return experiment_config_file.parent / Path(cfg["instance"])


# @dataclass
# class SizingInfo:
#     """Class for keeping track of values in the sizing experiments."""
#     vehicle_count:int
#     dropped_requests_count:int
#     interval_size:int


def restore_or_backup_initial_vehicle_file_and_get_count(vehicles_file):
    vehicles_file_orig = vehicles_file.parent / "vehicles_pre_sizing.csv"
    if not vehicles_file_orig.is_file():
        # back up if not yet backed up
        shutil.copy(vehicles_file, vehicles_file_orig)
    else:
        # restore from backup if backup exists
        vehicles_file.unlink()
        shutil.copy(vehicles_file_orig, vehicles_file)


def sizing_did_run(sizing_file: Path) -> bool:
    """
    Checks whether sizing.csv exists and has more than the header line.
    If it does, it means that the sizing experiment failed to run.
    """
    did_run = False
    if sizing_file.is_file():
        with open(sizing_file, "rt", encoding="utf-8") as f:
            lines = f.readlines()
        if len(lines) > 1:  # if it is not just headers
            did_run = True
        else:
            sizing_file.unlink()
    return did_run


def run_vehicle_sizing_experiments(instance_config_file: Path, experiment_config_file: Path,
                                   epsilon_percentage: float = 0.05):
    """
    Run vehicle sizing for a given instance and experiment config file.
    epsilon_percentage is the percentage of the vehicle count that is used for epsilon, a stopping criterion.
    By default, its 5% of the initial vehicle count. The initial vehicle count is presumed to be based on experimental
    config.yaml with `vehicle_to_request_ratio: 1`, meaning one vehicle per request.

    """
    sizing_file = instance_config_file.parent / SIZING_FILE
    vehicles_file = instance_config_file.parent / VEHICLES_FILE

    interval_size = math.inf  #
    epsilon = 1
    dropped_requests_count = 0

    while not (dropped_requests_count == 0 and interval_size <= epsilon):
        if not sizing_did_run(sizing_file):
            with open(sizing_file, "wt", encoding="utf-8") as f:
                f.write("vehicle_count,dropped_requests,interval_size\n")
            initial_vehicle_count = get_vehicle_count(vehicles_file)
            vehicle_count = initial_vehicle_count
            interval_size = initial_vehicle_count
            # epsilon = math.ceil(epsilon_percentage * initial_vehicle_count)  # 5% of vehicle count
            assert epsilon > 0, "Epsilon must be greater than 0"
        else:
            previous_vehicle_count, previous_dropped_vehicles, previous_interval_size = get_previous_sizing_info(
                sizing_file)
            interval_size = math.ceil(previous_interval_size / 2)
            if previous_dropped_vehicles == 0:
                vehicle_count = previous_vehicle_count - interval_size
            else:
                vehicle_count = previous_vehicle_count + interval_size

        # 1. set vehicles file
        restore_or_backup_initial_vehicle_file_and_get_count(vehicles_file)
        set_vehicles_file(vehicles_file, vehicle_count)

        # 2. run experiment
        delete_result_files(experiment_config_file)
        darpbenchmark.experiments.run_experiment_using_config(str(experiment_config_file))

        # 3. read dropped vehicle count
        dropped_requests_count = get_dropped_requests_count(experiment_config_file)

        # 4. write sizing info
        write_sizing_info(sizing_file, vehicle_count, dropped_requests_count, interval_size)
        logger.debug(f"Writing vehicle count: {vehicle_count}, dropped_requests: {dropped_requests_count}, "
                     f"interval_size: {interval_size}, epsilon: {epsilon}")

    logger.debug(
        f"Sizing for instance {instance_config_file} finished. Vehicle count: {vehicle_count} with interval {interval_size}")
    return vehicle_count, interval_size, epsilon


def set_instance_config_vehicle_count(instance_config_file, vehicle_count):
    instance_config = load_yaml(instance_config_file)
    instance_config["vehicles"].pop("vehicle_to_request_ratio")
    instance_config["vehicles"]["vehicle_count"] = vehicle_count
    with open(instance_config_file, "wt", encoding="utf-8") as f:
        yaml.dump(instance_config, f)


def get_sizing_output(sizing_file: Path):
    sizing_output = pd.read_csv(sizing_file, header=0, sep=",")
    vehicle_count, dropped_requests, interval_size = sizing_output.iloc[-1]
    return vehicle_count, dropped_requests, interval_size


def calculate_sizing_for_instance(experiment_config_file: Path):
    instance_config_file = get_instance_config_file_for_experiment(experiment_config_file)

    assert instance_config_file.is_file(), f"Instance config file {instance_config_file} does not exist"
    assert experiment_config_file.is_file(), f"Experiment config file {experiment_config_file} does not exist"

    # Check sizing was not already performed
    instance_config = load_yaml(instance_config_file)
    if "vehicle_count" in instance_config["vehicles"]:
        logger.error(f"The instance already contains sizing info! {instance_config}")
        return None

    sizing_file = instance_config_file.parent / "sizing.csv"
    if sizing_file.is_file():
        vehicle_count, dropped_requests, interval_size = get_sizing_output(sizing_file)
        if dropped_requests == 0 and interval_size == 1:
            logger.error(
                f"Instance {instance_config_file} already finished sizing. Vehicle count: {vehicle_count}, dropped "
                f"requests: {dropped_requests}, interval size: {interval_size}")
            return None
        else:
            logger.info(f"Continuing with sizing for instance {instance_config_file} from vehicle "
                        f"count {vehicle_count}, dropped requests {dropped_requests}, interval size {interval_size}")


    # Run vehicle sizing
    vehicle_count, interval_size, epsilon = run_vehicle_sizing_experiments(
        instance_config_file, experiment_config_file)

    logger.info(
        f"FINISHED: SAVED VEHICLE COUNT {vehicle_count} in sizing.csv for instance {instance_config_file}")

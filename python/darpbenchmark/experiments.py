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
from typing import Dict, Optional, List, Tuple
import logging
import os
import re
import pandas as pd
from pathlib import Path

import roadgraphtool.exec
import darpinstances.log

from darpinstances.experiments import load_experiment_config

def call_experiment_runner_plain(params: Dict[str, str], timeout: Optional[int] = None, executable_path: Optional[str] = None) -> bool:
    commands = [
        executable_path if executable_path else "DARP-benchmark"
    ]

    for param_name, param_value in params.items():

        # add - and -- sign if omitted
        if not param_name.startswith("-"):
            param_name = f"-{param_name}" if len(param_name) == 1 else f"--{param_name}"

        commands.append(param_name)
        if param_value != True:
            commands.append(param_value)

    commands = [str(arg) for arg in commands]

    return roadgraphtool.exec.call_executable(commands, timeout)


def call_experiment_runner(
        instance_path: str,
        output_path: str,
        method_params: Dict[str, str],
        dm_path: Optional[str] = None,
        node_type: str = "amodsim"
) -> bool:
    params = {
        "-i": instance_path,
        "-o": output_path,
        "-t": node_type
    }
    if dm_path:
        params['-d'] = dm_path
    # params.extend([str(param) for item in method_params.items() for param in item])

    return call_experiment_runner_plain(params | method_params)


def run_experiments(instance_paths: List[str], dm_path: str, methods: Dict, out_path_base: str, exp_count: int = 1):
    fail = False
    for instance_path in instance_paths:
        if fail:
            break
        instance_path_parts = os.path.normpath(instance_path).split(os.path.sep)
        out_path = f"{out_path_base}/{instance_path_parts[-2]}-{instance_path_parts[-1]}/"

        for method_configuration_name, method_config in methods.items():
            if not fail:
                logging.info("running experiments for method %s", method_configuration_name)
                for run in range(0, exp_count):
                    out_path_run = f"{out_path}/{method_configuration_name}-{run}"
                    result = call_experiment_runner(instance_path, out_path_run, method_config, dm_path)

                    if not result:
                        fail = True
                        break

def run_experiment_using_config(path: str, timeout: Optional[int] = None, executable_path: Optional[Path] = None) -> bool:
    config = load_experiment_config(path)
    # instance_filename = os.path.normpath(config["instance"]).split(os.sep)[-1]
    if 'timeout' in config:
        if timeout:
            timeout = min(timeout, config['timeout'])
        else:
            timeout = config['timeout']
        del config['timeout']

    solution_path = None
    for filepath in os.listdir(config['outdir']):
        if filepath.endswith('solution.json'):
            solution_path = filepath
    # solution_path = f"{config['outdir']}/{instance_filename}-solution.json"

    # if os.path.exists(solution_path):

    if solution_path is None:
        if executable_path:
            os.chdir(executable_path.parent)
        return call_experiment_runner_plain(config, timeout, executable_path)
    else:
        logging.info("The solution already exists ('%s')", solution_path)
        return True


def run_experiment_configs_in_dir(
        dir_path: str,
        sort_params: Optional[List[Tuple[str, int]]] = None,
        ignore_methods: Optional[List[str]] = None,
        timeout: Optional[int] = None
) -> bool:
    """
    Runs experiment for all configurations in dir. It is recursive, i.e., it follows subdirectories.
    Folder names starting with underscore are ignored. Also, if the solution is found for a configuration,
    the configuration is skipped.
    @param dir_path: path to the root dir
    @param sort_params: optional sort parameter to run experiments in a specific order.
    @param ignore_methods: list of methods to be ignored
    @param timeout in seconds
    @return: True in case of success, otherwise False
    """
    result = True

    experiments = search_experiments_in_dir(dir_path, ignore_methods)

    # df column labels
    columns = ["exp_path"]
    if sort_params is not None:
        for param in sort_params:
            columns.append(param[0])

    # exp df creation
    exp_processed = []
    for exp_path in experiments:
        exp = [exp_path]

        if sort_params is not None:
            for param in sort_params:
                name = param[0]
                param_name = f"{name}_(\\d+)"
                sr = re.search(param_name, exp_path)
                value = int(sr.group(1))
                exp.append(value)

        exp_processed.append(exp)

    df = pd.DataFrame(exp_processed, columns=columns)

    # sort if there are any params
    df["sort"] = 0
    if sort_params is not None:
        for param in sort_params:
            name = param[0]
            coeff = 1 if len(param) == 1 else param[1]
            df['sort'] = df['sort'] + df[name] * coeff

    df.sort_values('sort', inplace=True)

    # run the experiments
    for exp_path in df['exp_path']:
        result = run_experiment_using_config(exp_path, timeout)
        if not result:
            break

    return result


def search_experiments_in_dir(dir_path: str, ignore_methods: Optional[List[str]] = None) -> List[str]:
    experiments = []

    for file in os.listdir(dir_path):
        filename = os.fsdecode(file)
        filepath = os.path.join(dir_path, filename)

        if os.path.isdir(filepath) and not filename.startswith("_") \
                and (ignore_methods is None or filename not in ignore_methods):
            subdir_experiments = search_experiments_in_dir(filepath, ignore_methods)
            experiments.extend(subdir_experiments)
        elif filename.endswith("yaml"):
            experiments.append(filepath)

    return experiments

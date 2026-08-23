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

import os

import darpbenchmark.experiments

dm_path = r"C:/AIC Experiment Data/DARP/Plzen/dm.csv"

instance_paths = [
    r"C:/AIC Experiment Data/DARP/Plzen/experiments/instance_length_trend/30_s/instance-trips-max_delay_180-time_to_start_120-vehicle_capacity_4.di",
    r"C:/AIC Experiment Data/DARP/Plzen/experiments/instance_length_trend/40_s/instance-trips-max_delay_180-time_to_start_120-vehicle_capacity_4.di",
    r"C:/AIC Experiment Data/DARP/Plzen/experiments/instance_length_trend/50_s/instance-trips-max_delay_180-time_to_start_120-vehicle_capacity_4.di",
    r"C:/AIC Experiment Data/DARP/Plzen/experiments/instance_length_trend/60_s/instance-trips-max_delay_180-time_to_start_120-vehicle_capacity_4.di"
]
methods = [
    {'-m': "vga_chaining", "--tts": 270, "-g": 10, "-b": 16, "--mtbp": 0, "--cmg": 0.01},
    {'-m': "vga_chaining", "--tts": 270, "-g": 10, "-b": 21, "--mtbp": 0, "--cmg": 0.01},
    {'-m': "vga_chaining", "--tts": 270, "-g": 10, "-b": 26, "--mtbp": 0, "--cmg": 0.01},
    {'-m': "vga_chaining", "--tts": 270, "-g": 10, "-b": 31, "--mtbp": 0, "--cmg": 0.01}
]

for instance_path, method in zip(instance_paths, methods):
    path_parts = os.path.normpath(instance_path).split(os.path.sep)
    output_path = f"C:/AIC Experiment Data/DARP/Results/instance_length_trend/{path_parts[-2]}-{path_parts[-1]}/vga_chaining-two_batches-tts_270-0"
    darpbenchmark.experiments.call_experiment_runner(instance_path, output_path, method, dm_path)
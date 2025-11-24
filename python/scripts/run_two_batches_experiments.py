
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
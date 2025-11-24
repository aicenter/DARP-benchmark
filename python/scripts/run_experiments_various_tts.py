

import darpbenchmark.experiments


times_to_start = range(0, 301, 30)

methods = {}

for tts in times_to_start:
    method = {'-m': "vga_chaining", "--tts": tts, "-g": 10, "-b": 150, "--mtbp": 0, "--cmg": 0.01}
    methods[f"vga_chaining-tts_{tts}"] = method

out_path_base = r"C:/AIC Experiment Data/DARP/Results/tts_trend/"
dm_path = r"C:/AIC Experiment Data/DARP/Plzen/dm.csv"
instance_paths = [r'C:\AIC Experiment Data\DARP\Plzen\experiments\single_batch-exp_length_sensitivity_analysis\60_s/instance-trips-max_delay_180-time_to_start_120-vehicle_capacity_4.di']

darpbenchmark.experiments.run_experiments(instance_paths, dm_path, methods, out_path_base)
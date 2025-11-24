import logging
import sys
from pathlib import Path

import darpbenchmark.experiments

# cities = ['Manhattan', 'NYC', 'Chicago', 'DC']
# cities = ['NYC']
# cities = ['Manhattan']
if len(sys.argv) != 2:
    exit(f'There should be exactly one argument: the city name. Arguments: {sys.argv}')

city = sys.argv[1]

methods = [
    'ih',
    'vga_chaining-batch_120_s-limited',
    'vga_chaining-batch_240_s-limited',
    'vga_chaining-batch_480_s-limited',
    'halns',
    'halns-ih',
    'vga_chaining-batch_30_s',
    'vga_chaining-batch_60_s',
    'vga_chaining-batch_120_s',
    'halns-vga',
    'vga_chaining-batch_240_s',
    'vga_chaining-batch_480_s',
    'vga'
]
times = ['05', '15', '30']

base_path = Path(r"/home/fiedlda1/Experiment Data/DARP/Results/final-real_speeds")

# for city in cities:
logging.info(f"Running experiments for city {city}")
for time in times:
    res_per_instance_folder = base_path / Path(city) / Path(f"{time}_min")
    for method in methods:
        config_path = res_per_instance_folder / Path(method) / Path('config.yaml')
        darpbenchmark.experiments.run_experiment_using_config(str(config_path))



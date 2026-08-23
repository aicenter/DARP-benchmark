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


# root_path = r'C:/AIC Experiment Data/DARP/Plzen/experiments/single_batch-exp_length_sensitivity_analysis'
root_path = r'C:/AIC Experiment Data/DARP/Plzen/experiments/instance_length_trend'

methods = {
    # "ih": {},
    # "vga": {"--tts": 180, "-g": 10},
    # "vga_chaining": {"--tts": 180, "-g": 10, "-b": 150, "--mtbp": 0, "--cmg": 0.01}
    # "vga_chaining-tts_0": {"--tts": 0, "-g": 10, "-b": 150, "--mtbp": 0, "--cmg": 0.01},
    "vga_chaining-tts_270": {"-m": "vga",  "--tts": 270, "-g": 10, "-b": 150, "--mtbp": 0, "--cmg": 0.01}
}

# out_path_base = r"C:/AIC Experiment Data/DARP/Results/single_batch-exp_length_sensitivity_analysis"
out_path_base = r"C:/AIC Experiment Data/DARP/Results/instance_length_trend"
dm_path = r"C:/AIC Experiment Data/DARP/Plzen/dm.csv"

instance_paths = []

for file in os.listdir(root_path):
    filename = os.fsdecode(file)
    instance_dir_path = os.path.join(root_path, filename)
    if os.path.isdir(instance_dir_path):
        for instance_file in os.listdir(instance_dir_path):
            instance_filename = os.fsdecode(instance_file)
            instance_filename_parts = instance_filename.split(".")
            if len(instance_filename_parts) == 2 and instance_filename_parts[1] == "di":
                instance_path = os.path.join(instance_dir_path, instance_filename)
                instance_paths.append(instance_path)

darpbenchmark.experiments.run_experiments(instance_paths, dm_path, methods, out_path_base)





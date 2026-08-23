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
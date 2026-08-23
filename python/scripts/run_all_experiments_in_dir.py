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

import darpbenchmark.log

# root_path = r"D:\AIC Data/Experiment Data\DARP\Results\Prague/50_percend_demand_2_to_20_min"
# root_path = r"C:\AIC Experiment Data\DARP\Results\Manhattan/max_delay-exp_length_SA-min_vehicles"
# root_path = r"C:\AIC Experiment Data\DARP\Results\Manhattan/travel_time_SA_2"
root_path = r"C:\AIC Experiment Data\DARP\Results\Real Demand\Manhattan-max_delay_3_minutes"
# root_path = r"C:\AIC Experiment Data\DARP\Results\Real Demand\DC\24_h"
# root_path = r"D:\AIC Data/Experiment Data\DARP\Results\Real Demand\DC\24_h"
# root_path = r"C:\AIC Experiment Data\DARP\Results\Real Demand\Chicago\24_h"
# root_path = r"C:\AIC Experiment Data\DARP\Results\Real Demand\Manhattan\max_VGA_optimal"


# darpbenchmark.experiments.run_experiment_configs_in_dir(
#     root_path, aic_exp_root_path, [("exp_length",), ("max_delay", 60)],
#     ["vga_chaining-batch_length_60_s", "vga_chaining-batch_length_30_s"])

darpbenchmark.experiments.run_experiment_configs_in_dir(
    root_path,
    timeout=3600,
    ignore_methods=["vga"]
)
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

# config_path = r"/home/fiedlda1/Experiment Data/DARP/Results/final/DC/05_min/vga_chaining-batch_30_s/config.yaml"
# config_path = r"/home/fiedlda1/Experiment Data/DARP/Results/final-real_speeds/NYC/05_min/ih/config.yaml"
config_path = r"C:\Google Drive\AIC Experiment Data\DARP\ITSC_instance_paper\Results\Chicago\start_07-00\duration_16_h\max_delay_03_min\halns-vga/config.yaml"

darpbenchmark.experiments.run_experiment_using_config(config_path)

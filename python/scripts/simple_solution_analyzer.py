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
from pathlib import Path
import pandas as pd

# root = Path(r'D:\Google Drive AIC\AIC Experiment Data\DARP\final\Results')



# root = Path(r'/home/fiedlda1/AIC Experiment Data/DARP/final/Results')



# for all experiments
experiments = pd.DataFrame(columns=['city', 'method', 'result'])

for file in root.rglob('config.yaml'):
    method = file.parts[-2]
    city = file.parts[-6]
    solution = 'unsolved'

    # solutions
    solution_path = file.parent / 'config.yaml-solution.json'
    if solution_path.is_file():
        solution = 'solved'
    else:
        log_path = file.parent / 'rci_job.log'
        if log_path.is_file():
            solution = 'failed'

    experiments = experiments.append({'city': city, 'method': method, 'result': solution}, ignore_index=True)

print(f'Number of experiments: {len(experiments)}')
print(f'Number of experiments per city: {exp_counts_per_city}')
print(f'Number of experiments per method: {exp_counts_per_method}')

solutions = experiments[experiments['result'] == 'solved']
print(f'Number of solutions: {len(solutions)}')
print(f'Number of solutions per city: {solution_counts_per_city}')
print(f'Number of solutions per method: {solution_counts_per_method}')

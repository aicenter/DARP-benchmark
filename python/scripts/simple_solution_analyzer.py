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

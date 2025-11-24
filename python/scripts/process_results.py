import sys
import os
import statistics
import numpy as np
import darpbenchmark.inout

# result_dir = r"D:\AIC Data\Experiment Data\DARP\Results\Real Demand\NYC memory benchmark"
# result_dir = r"C:\AIC Experiment Data\DARP\Results\Real Demand\NYC memory benchmark"
result_dir = r"C:\AIC Experiment Data\DARP\Results\Real Demand\NYC benchmark\v5AIC"


def get_stats(results: list):
    return statistics.mean(results), statistics.stdev(results)


solutions = []
directory = os.fsencode(result_dir)

for file in os.listdir(directory):
    filename = os.fsdecode(file)
    if "solution" in filename and "chaining" not in filename and "batch" not in filename:
        filepath = os.path.join(result_dir, filename)
        solutions.append(darpbenchmark.inout.load_json(filepath))

solutions_corresponds = solutions.count(solutions[0]) == len(solutions)

if not solutions_corresponds:
    print("Solutions do not match!")
    # sys.exit(1)
else:
    print("Solutions match")

performances = []

for file in os.listdir(directory):
    filename = os.fsdecode(file)
    if "performance" in filename:
        filepath = os.path.join(result_dir, filename)
        performances.append(darpbenchmark.inout.load_json(filepath))

total_times = []
memory = []

solver_stats = {}

for_sheet = []

for perf in performances:
    total_times.append(perf["total_time"])
    if "peak_memory" in perf:
        memory.append(perf["peak_memory"])
    for stat_name, stat_value in perf["solver_stats"].items():
        if stat_name not in solver_stats:
            solver_stats[stat_name] = []
        solver_stats[stat_name].append(stat_value)

print("Mean total time: {}, the standard deviation is {}".format(
    statistics.mean(total_times), statistics.stdev(total_times)))
for_sheet.extend([*get_stats(total_times)])

if len(memory) > 0:
    print("Mean peak memory usage is: {}, the standard deviation is {}".format(
        statistics.mean(memory), statistics.stdev(memory)))
    avg, dev = get_stats(memory)
    for_sheet.extend([avg, "", dev])


if solver_stats:
    print("Solver stats:")

    for stat_name, stat_value in solver_stats.items():
        print(
            f"Mean {stat_name}: {statistics.mean(stat_value)}, the standard deviation is {statistics.stdev(stat_value)}"
        )
        for_sheet.extend([*get_stats(stat_value)])

print(', '.join((str(number) for number in for_sheet)))


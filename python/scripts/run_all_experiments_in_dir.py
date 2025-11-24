
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
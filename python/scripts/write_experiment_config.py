import darpinstances.experiments


config_path = r"C:/AIC Experiment Data/DARP/Results/2_to_20_min_length_trend/2_minutes/ih/config.yaml"

config = {
    "instance": "C:/AIC Experiment Data/DARP/Plzen/experiments/2_to_20_min_length_trend/120_s/instance-trips-max_delay_180-time_to_start_270-vehicle_capacity_4.di",
    "outdir": "C:/AIC Experiment Data/DARP/Results/2_to_20_min_length_trend/2_minutes/ih",
    "instype": "amodsim",
    "dm": "C:/AIC Experiment Data/DARP/Plzen/dm.csv",
    "method": "ih"
}

darpinstances.experiments.write_experiment_config(config_path, config)

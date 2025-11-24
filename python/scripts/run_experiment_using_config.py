import darpbenchmark.experiments

# config_path = r"/home/fiedlda1/Experiment Data/DARP/Results/final/DC/05_min/vga_chaining-batch_30_s/config.yaml"
# config_path = r"/home/fiedlda1/Experiment Data/DARP/Results/final-real_speeds/NYC/05_min/ih/config.yaml"
config_path = r"C:\Google Drive\AIC Experiment Data\DARP\ITSC_instance_paper\Results\Chicago\start_07-00\duration_16_h\max_delay_03_min\halns-vga/config.yaml"

darpbenchmark.experiments.run_experiment_using_config(config_path)

#pragma once

#include <future-config/Config_object.h>
#include <string>


struct Ih {
	int temporal_pruning_min_plan_length;

	explicit Ih(const fc::Config_object& config_object):
		temporal_pruning_min_plan_length(config_object.get<int>("temporal_pruning_min_plan_length"))
	{};
};

struct Worst_removal {
	bool stop_groups;
	double drop_threshold;
	std::string initial_solution;

	explicit Worst_removal(const fc::Config_object& config_object):
		stop_groups(config_object.get<bool>("stop_groups")),
		drop_threshold(config_object.get<double>("drop_threshold")),
		initial_solution(config_object.get<std::string>("initial_solution"))
	{};
};

struct DARP_benchmark_config {
	bool simple_csv_export;
	Ih ih;
	int tcount;
	Worst_removal worst_removal;
	std::string method;
	std::string outdir;
	int tmax;
	std::string instance;

	explicit DARP_benchmark_config(const fc::Config_object& config_object):
		simple_csv_export(config_object.get<bool>("simple_csv_export")),
		ih(config_object.get<fc::Config_object&>("ih")),
		tcount(config_object.get<int>("tcount")),
		worst_removal(config_object.get<fc::Config_object&>("worst_removal")),
		method(config_object.get<std::string>("method")),
		outdir(config_object.get<std::string>("outdir")),
		tmax(config_object.get<int>("tmax")),
		instance(config_object.get<std::string>("instance"))
	{};
};


#pragma once

#include <future-config/Config_object.h>
#include <string>


struct Ih {
	int temporal_pruning_min_plan_length;

	explicit Ih(const fc::Config_object& config_object):
		temporal_pruning_min_plan_length(config_object.get<int>("temporal_pruning_min_plan_length"))
	{};
};

struct DARP_benchmark_config {
	int tcount;
	std::string instance;
	std::string outdir;
	std::string method;
	bool simple_csv_export;
	int tmax;
	Ih ih;

	explicit DARP_benchmark_config(const fc::Config_object& config_object):
		tcount(config_object.get<int>("tcount")),
		instance(config_object.get<std::string>("instance")),
		outdir(config_object.get<std::string>("outdir")),
		method(config_object.get<std::string>("method")),
		simple_csv_export(config_object.get<bool>("simple_csv_export")),
		tmax(config_object.get<int>("tmax")),
		ih(config_object.get<fc::Config_object&>("ih"))
	{};
};


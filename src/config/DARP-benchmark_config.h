#pragma once

#include <future-config/Config_object.h>
#include <string>


struct DARP_benchmark_config {
	int tcount;
	std::string instance;
	std::string outdir;
	std::string method;
	int tmax;

	explicit DARP_benchmark_config(const fc::Config_object& config_object):
		tcount(config_object.get<int>("tcount")),
		instance(config_object.get<std::string>("instance")),
		outdir(config_object.get<std::string>("outdir")),
		method(config_object.get<std::string>("method")),
		tmax(config_object.get<int>("tmax"))
	{};
};


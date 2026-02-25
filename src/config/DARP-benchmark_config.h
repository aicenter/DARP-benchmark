#pragma once

#include <future-config/Config_object.h>
#include <string>


struct Receding_horizon_vga {
	int period;

	explicit Receding_horizon_vga(const fc::Config_object& config_object):
		period(config_object.get<int>("period"))
	{};
};

struct Vga_chaining {
	int batchl;
	int max_delay;
	int tts;
	int mtbp;
	double cmg;
	int mcc;
	int cbs;
	bool skip_vga_plan_export;

	explicit Vga_chaining(const fc::Config_object& config_object):
		batchl(config_object.get<int>("batchl")),
		max_delay(config_object.get<int>("max_delay")),
		tts(config_object.get<int>("tts")),
		mtbp(config_object.get<int>("mtbp")),
		cmg(config_object.get<double>("cmg")),
		mcc(config_object.get<int>("mcc")),
		cbs(config_object.get<int>("cbs")),
		skip_vga_plan_export(config_object.get<bool>("skip-vga-plan-export"))
	{};
};

struct Vga {
	int max_group;
	int gglimit;
	int galimit;

	explicit Vga(const fc::Config_object& config_object):
		max_group(config_object.get<int>("max-group")),
		gglimit(config_object.get<int>("gglimit")),
		galimit(config_object.get<int>("galimit"))
	{};
};

struct Halns {
	int iter;
	std::string init;
	int time_limit;

	explicit Halns(const fc::Config_object& config_object):
		iter(config_object.get<int>("iter")),
		init(config_object.get<std::string>("init")),
		time_limit(config_object.get<int>("time_limit"))
	{};
};

struct DARP_benchmark_config {
	std::string instance;
	int tcount;
	std::string outdir;
	std::string method;
	int tmax;
	Receding_horizon_vga receding_horizon_vga;
	Vga_chaining vga_chaining;
	Vga vga;
	Halns halns;

	explicit DARP_benchmark_config(const fc::Config_object& config_object):
		instance(config_object.get<std::string>("instance")),
		tcount(config_object.get<int>("tcount")),
		outdir(config_object.get<std::string>("outdir")),
		method(config_object.get<std::string>("method")),
		tmax(config_object.get<int>("tmax")),
		receding_horizon_vga(config_object.get<fc::Config_object&>("receding_horizon_vga")),
		vga_chaining(config_object.get<fc::Config_object&>("vga_chaining")),
		vga(config_object.get<fc::Config_object&>("vga")),
		halns(config_object.get<fc::Config_object&>("halns"))
	{};
};


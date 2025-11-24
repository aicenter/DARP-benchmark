#pragma once

#include <future-config/Config_object.h>
#include <string>


struct Vga_chaining {
	int max_delay;
	int batchl;
	int tts;
	int mcc;
	double cmg;
	int mtbp;
	int cbs;
	bool skip_vga_plan_export;

	explicit Vga_chaining(const fc::Config_object& config_object):
		max_delay(config_object.get<int>("max_delay")),
		batchl(config_object.get<int>("batchl")),
		tts(config_object.get<int>("tts")),
		mcc(config_object.get<int>("mcc")),
		cmg(config_object.get<double>("cmg")),
		mtbp(config_object.get<int>("mtbp")),
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
	int tcount;
	std::string instance;
	std::string outdir;
	std::string method;
	int tmax;
	Vga_chaining vga_chaining;
	Vga vga;
	Halns halns;

	explicit DARP_benchmark_config(const fc::Config_object& config_object):
		tcount(config_object.get<int>("tcount")),
		instance(config_object.get<std::string>("instance")),
		outdir(config_object.get<std::string>("outdir")),
		method(config_object.get<std::string>("method")),
		tmax(config_object.get<int>("tmax")),
		vga_chaining(config_object.get<fc::Config_object&>("vga_chaining")),
		vga(config_object.get<fc::Config_object&>("vga")),
		halns(config_object.get<fc::Config_object&>("halns"))
	{};
};


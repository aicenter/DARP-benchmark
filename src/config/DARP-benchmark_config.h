/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */
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


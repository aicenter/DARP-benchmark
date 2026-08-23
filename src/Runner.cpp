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
#include <tclap/CmdLine.h>
#include "magic_enum/magic_enum.hpp"

#include <omp.h>
#include <future-config/configuration.h>


#if(USE_MIMALLOC_ALLOCATOR)
#include <mimalloc-new-delete.h>
#endif

#include "asserts.h"
#include "DARP_benchmark.h"
#include "common.h"
#include "logging.h"
#include "inout.h"
#include "DARP_benchmark_reader.h"
#include "config/DARP-benchmark_config.h"


using namespace DARP;


int main(int argc, const char** argv) {
	try {
		// first check whether there is a local config file path argument. It must be the first argument, and it is a
		// value argument.
		std::optional<std::filesystem::path> local_config_path;
		if(argc > 1 && argv[1][0] != '-') {
			local_config_path = std::filesystem::path{argv[1]};
			--argc;
			++argv;
		}

		// std::vector<std::unique_ptr<fc::Config_definition_base>> config_definitions;
		// config_definitions.emplace_back(std::make_unique<fc::Config_definition>()); // default config
		//
		// // add registered config definitions
		// auto registered_configs = Config_registry::get().get_all_configs();
		// for(auto& config: registered_configs) {
		// 	config_definitions.push_back(std::move(config));
		// }

		fc::Load_options load_options{.command_line_arguments = std::make_pair(argc, argv)};

		// add local config if it exists
		if(local_config_path) {
			load_options.local_config_path = *local_config_path;
			// config_definitions.emplace_back(
			// 	std::make_unique<fc::Config_definition>(fc::Config_type::LOCAL, *local_config_path)
			// );
		}

		// add command line config
		// config_definitions.emplace_back(std::make_unique<fc::Command_line_config_definition>(argc, argv));

		auto config = fc::load<DARP_benchmark_config>(load_options);

		const auto instance_path = check_path(config.instance);
		const auto out_path = fs::path(config.outdir);
		std::string method_name = config.method;
		std::ranges::transform(
			method_name,
			method_name.begin(),
			[](unsigned char c) { return (char) std::toupper(c); }
		);

		const auto number_of_trials = static_cast<unsigned short>(config.tcount);
		const auto max_threads = static_cast<unsigned short>(config.tmax);

		if(max_threads > 0) {
			omp_set_num_threads(max_threads);
		}

		set_up_logger(out_path);

		if(instance_path.extension().string() == ".yaml") {
			DARP_benchmark<Amodsim_node> benchmark{std::make_unique<DARP_benchmark_reader>()};
			benchmark.run(instance_path, out_path, method_name, config, number_of_trials);
		}
		else {
			DARP_benchmark<Cordeau_node> benchmark{std::make_unique<Cordeau_reader>()};
			benchmark.run(instance_path, out_path, method_name, config, number_of_trials);
		}
	} catch(...) {
		spdlog::error(ExceptionHandeling::process_unexpected_exception());
		return -1;
	}

	return 0;
}

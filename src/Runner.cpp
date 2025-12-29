
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
		if (argc > 1 && argv[1][0] != '-') {
			local_config_path = std::filesystem::path{argv[1]};
			--argc;
			++argv;
		}

		std::vector<std::unique_ptr<fc::Config_definition_base>> config_definitions;
		config_definitions.emplace_back(std::make_unique<fc::Config_definition>()); // default config

		// add local config if it exists
		if(local_config_path) {
			config_definitions.emplace_back(
				std::make_unique<fc::Config_definition>(fc::Config_type::LOCAL, *local_config_path)
			);
		}

		// add command line config
		config_definitions.emplace_back(
			std::make_unique<fc::Command_line_config_definition>(argc, argv)
		);

		auto config = fc::load<DARP_benchmark_config>(config_definitions);

		const auto instance_path = check_path(config.instance);
		const auto out_path = check_path(config.outdir);
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

		if (instance_path.extension().string() == ".yaml") {
			DARP_benchmark<Amodsim_node> benchmark{std::make_unique<DARP_benchmark_reader>()};
			benchmark.run(instance_path, out_path, method_name, config, number_of_trials);
		}
		else {
			DARP_benchmark<Cordeau_node> benchmark{std::make_unique<Cordeau_reader>()};
			benchmark.run(instance_path, out_path, method_name, config, number_of_trials);
		}
	}
	catch (...) {
		spdlog::error(ExceptionHandeling::process_unexpected_exception());
		return -1;
	}

	return 0;
}

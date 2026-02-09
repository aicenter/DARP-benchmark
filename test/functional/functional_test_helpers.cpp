//
// Created by Fido on 12/16/2025.
//

#include "functional_test_helpers.h"
#include <cstdlib>

namespace fs = std::filesystem;

fs::path run_benchmark(std::vector<std::string>& arguments) {
	std::string command = "DARP-benchmark";

	// output path
	auto out_path = fs::temp_directory_path() / "DARP_benchmark_functional_test_output";

	// delete output directory if it exists
	if (fs::exists(out_path)) {
		fs::remove_all(out_path);
	}

	// create output directory
	fs::create_directories(out_path);

	arguments.emplace_back("--outdir");
	arguments.emplace_back(out_path.string());

	for (const auto& argument : arguments) {
		command += " " + argument;
	}

	spdlog::info("Running DARP benchmark: {}", command);
	std::system(command.c_str());

	return out_path;
}


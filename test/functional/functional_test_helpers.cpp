//
// Created by Fido on 12/16/2025.
//

#include "functional_test_helpers.h"
#include <cstdlib>

namespace fs = std::filesystem;

fs::path run_benchmark(std::vector<std::string>& arguments) {
	std::string command = "DARP-benchmark";

	// output path
	auto out_path = fs::temp_directory_path();
	arguments.emplace_back("--outdir");
	arguments.emplace_back(out_path.string());

	for (const auto& argument : arguments) {
		command += " " + argument;
	}

	std::system(command.c_str());

	return out_path;
}


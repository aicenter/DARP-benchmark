//
// Created by Fido on 12/16/2025.
//

#include "gtest_wrapper.h"

#include <filesystem>
#include "../src/Solution.h"
#include "../src/DARP_benchmark_reader.h"
#include "../src/DARP_benchmark_node.h"
#include "common.h"

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

TEST(functional_tests, ih_chyse) {
	fs::path instance_path = R"(test_resources\Chyse/config.yaml)";

	std::vector<std::string> args = {
		"--instance",
		instance_path.string(),
		"--method",
		"ih"
	};

	auto output_path = run_benchmark(args);

	// Load the DARP instance (set working directory to instance directory for relative paths in config)
	const auto original_working_dir = fs::current_path();
	const auto absolute_instance_path = fs::absolute(instance_path);
	const auto instance_dir = absolute_instance_path.parent_path();
	fs::current_path(instance_dir);
	
	DARP_benchmark_reader reader;
	DARP_instance<Amodsim_node> darp_instance = reader.read(absolute_instance_path);
	
	// Restore working directory
	fs::current_path(original_working_dir);

	// Load the computed result
	const std::string instance_filename = instance_path.filename().string();
	const fs::path computed_solution_path = output_path / (instance_filename + "-solution.json");
	Solution<Amodsim_node> computed_solution = deserialize_json<Amodsim_node>(computed_solution_path, darp_instance);

	// Load the expected results
	const fs::path expected_solution_path = absolute_instance_path.parent_path() / "expected" / "solution.json";
	Solution<Amodsim_node> expected_solution = deserialize_json<Amodsim_node>(expected_solution_path, darp_instance);

	// Compare solutions
	check_solutions_equal(computed_solution, expected_solution);
}


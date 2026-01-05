#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include "../src/Solution.h"
#include "../src/DARP_benchmark_reader.h"
#include "../src/DARP_benchmark_node.h"
#include "../common.h"

namespace fs = std::filesystem;

/**
 * @brief Runs the DARP-benchmark executable with the given arguments
 * @param arguments Command-line arguments to pass to the benchmark
 * @return Path to the output directory where results are written
 */
fs::path run_benchmark(std::vector<std::string>& arguments);

/**
 * @brief Loads a DARP instance from the given config path
 * @tparam N Node type
 * @param instance_path Path to the instance config file
 * @return The loaded DARP instance
 */
template<typename N>
DARP_instance<N> load_instance(const fs::path& instance_path) {
	// Set working directory to instance directory for relative paths in config
	const auto original_working_dir = fs::current_path();
	const auto absolute_instance_path = fs::absolute(instance_path);
	const auto instance_dir = absolute_instance_path.parent_path();
	fs::current_path(instance_dir);
	
	DARP_benchmark_reader reader;
	DARP_instance<N> darp_instance = reader.read(absolute_instance_path);
	
	// Restore working directory
	fs::current_path(original_working_dir);
	
	return darp_instance;
}

/**
 * @brief Runs a functional test: executes the benchmark and compares results
 * @tparam N Node type
 * @param instance_path Path to the instance config file
 * @param solver_args Additional solver arguments (e.g., "--method", "ih")
 * @param expected_solution_path Path to the expected solution JSON file (relative to instance directory)
 */
template<typename N>
void run_functional_test(
	const fs::path& instance_path,
	const std::vector<std::string>& solver_args,
	const fs::path& expected_solution_path = fs::path("expected") / "solution.json"
) {
	// Prepare arguments
	std::vector<std::string> args = {
		"--instance",
		instance_path.string()
	};
	args.insert(args.end(), solver_args.begin(), solver_args.end());
	
	// Run benchmark
	auto output_path = run_benchmark(args);
	
	// Load the DARP instance
	const auto absolute_instance_path = fs::absolute(instance_path);
	DARP_instance<N> darp_instance = load_instance<N>(absolute_instance_path);
	
	// Load the computed result
	const std::string instance_filename = instance_path.filename().string();
	const fs::path computed_solution_path = output_path / (instance_filename + "-solution.json");
	Solution<N> computed_solution = deserialize_json<N>(computed_solution_path, darp_instance);
	
	// Load the expected results
	const fs::path expected_solution_full_path = absolute_instance_path.parent_path() / expected_solution_path;
	Solution<N> expected_solution = deserialize_json<N>(expected_solution_full_path, darp_instance);
	
	// Compare solutions
	check_solutions_equal(computed_solution, expected_solution);
}


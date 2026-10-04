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
	DARP_benchmark_reader reader;
	return reader.read(fs::absolute(instance_path));
}

/**
 * @brief Runs a functional test: executes the benchmark and compares results
 * @tparam N Node type
 * @tparam P Plan type (default VehiclePlan<N>; use VehiclePlan<N, Vehicle_base> for fleet sizing with virtual vehicles)
 * @param instance_path Path to the instance config file
 * @param solver_args Additional solver arguments (e.g., "--method", "ih")
 * @param expected_solution_path Path to the expected solution JSON file (absolute, or relative to the instance directory)
 */
template<typename N, class P = VehiclePlan<N>>
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
	ASSERT_TRUE(fs::exists(computed_solution_path));
	Solution<N, P> computed_solution = deserialize_json<N, P>(computed_solution_path, darp_instance);
	
	// Load the expected results
	const fs::path expected_solution_full_path = absolute_instance_path.parent_path() / expected_solution_path;
	Solution<N, P> expected_solution = deserialize_json<N, P>(expected_solution_full_path, darp_instance);
	
	// Compare solutions
	check_solutions_equal<N, P>(computed_solution, expected_solution);
}


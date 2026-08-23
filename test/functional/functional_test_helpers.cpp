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


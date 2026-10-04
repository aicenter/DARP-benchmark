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
#include "common.h"
#include "../src/inout.h"
#include "src/DARP_benchmark_reader.h"
#include <test_resource_dirs.h>

std::unique_ptr<travel_time_type[]> load_dm_from_json(const rapidjson::Document& doc) {
	const auto& dm_array = doc["dm"].GetArray();
	const unsigned size = dm_array.Size();
	auto dm = std::make_unique<travel_time_type[]>(static_cast<size_t>(size) * size);
	for(unsigned i = 0; i < size; ++i) {
		const auto& dm_inner_array = dm_array[i];
		for(unsigned j = 0; j < size; ++j) {
			dm[i* size + j] = static_cast<travel_time_type>(dm_inner_array[j].GetUint());
		}
	}

	return dm;
}

fs::path get_test_resource_path(const fs::path& relative_path) {
	for (const fs::path& dir : test_resource_dirs) {
		fs::path path = dir / relative_path;
		if (fs::exists(path)) {
			return path;
		}
	}
	throw std::runtime_error("Test resource not found in any test resource directory: " + relative_path.string());
}

/**
 * Loads an Amodsim test instance using DARP_benchmark_reader::read and a YAML config.
 * Relative paths in YAML are resolved against the instance directory (parent of the config file), not the process cwd.
 */
[[nodiscard]] DARP_instance<Amodsim_node> load_test_instance_amodsim(const std::string& instance_id) {
	const fs::path config_path = get_test_resource_path("instance_" + instance_id + "/config.yaml");
	DARP_benchmark_reader reader;
	return reader.read(config_path);
}

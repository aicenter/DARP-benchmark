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
#include <stdexcept>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "Leg_cost_component.h"
#include "../travel_time_provider/CSV_reader.h"
#include "../travel_time_provider/HDF_reader.h"

Matrix_leg_cost_component::Matrix_leg_cost_component(
	std::string name, double weight, std::unique_ptr<uint32_t[]> matrix, unsigned size
):
	name_(std::move(name)),
	weight_(weight),
	matrix_(std::move(matrix)),
	size_(size) {
}

std::shared_ptr<const Matrix_leg_cost_component> Matrix_leg_cost_component::load(
	std::string name, double weight, const std::filesystem::path& filepath, unsigned node_count
) {
	spdlog::info("Loading the {} cost component matrix from {}", name, filepath.string());
	auto [matrix, size] = filepath.extension() == ".h5"
		? read_hdf_matrix<uint32_t>(filepath.string())
		: read_csv_matrix<uint32_t>(filepath.string());
	if(size != node_count) {
		throw std::runtime_error(fmt::format(
			"The {} matrix {} has {} rows and columns, but the travel time matrix has {}",
			name, filepath.string(), size, node_count));
	}
	return std::make_shared<const Matrix_leg_cost_component>(std::move(name), weight, std::move(matrix), size);
}

namespace {

/** Loader of a matrix component whose matrix path is the instance config key \p filepath_key. */
auto matrix_component_loader(const std::string& name, const std::string& weight_key, const std::string& filepath_key) {
	return [name, weight_key, filepath_key](
		double weight, const YAML::Node& config, const std::filesystem::path& instance_dir, unsigned node_count
	) -> std::shared_ptr<const Leg_cost_component> {
		if(!config[filepath_key]) {
			throw std::runtime_error(fmt::format(
				"Cost component '{}' has a non-zero weight '{}' but the instance config has no '{}'",
				name, weight_key, filepath_key));
		}
		std::filesystem::path filepath = config[filepath_key].as<std::string>();
		if(filepath.is_relative()) {
			filepath = instance_dir / filepath;
		}
		return Matrix_leg_cost_component::load(name, weight, filepath, node_count);
	};
}

} // namespace

const std::vector<Leg_cost_component_type>& leg_cost_component_types() {
	static const std::vector<Leg_cost_component_type> types{
		{"distance_weight", matrix_component_loader("distance", "distance_weight", "dist_filepath")},
	};
	return types;
}

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

#include <optional>
#include <yaml-cpp/yaml.h>
#include "Reader.h"
#include "DARP_benchmark_node.h"
#include "travel_time_provider/Grid_travel_time_provider.h"

namespace fleet_sizing {
template <>
struct grid_vertex_index_trait<Amodsim_node> {
	static unsigned get(const Amodsim_node& n) { return n.get_index(); }
};
}

namespace internal{
std::shared_ptr<DARP_instance_configuration> load_instance_configuration(const YAML::Node& config);

/**
 * Loads the `cost` section of the instance configuration with the legacy fallbacks (`demand.relative_delay_cost`
 * for the passenger delay weight, `vehicles.capital_cost` for the vehicle capital cost).
 * @throws std::runtime_error on an unknown key, on `accounting` other than `per_traveller`, and on a non-zero weight
 * of a component that the benchmark does not evaluate.
 */
Cost_weights load_cost_weights(const YAML::Node& config);

void load_vehicles_csv(std::vector<Vehicle<Amodsim_node>>& vehicles, const std::string& file_path, unsigned instance_start_time);

/**
 * Maximum delay of the requests: the `max_delay` key of the instance configuration, or one of its deprecated
 * aliases `max_travel_time_delay` and `max_prolongation`.
 */
struct Max_delay {
	/** Delay in seconds, used in the absolute mode. */
	unsigned seconds = 0;
	/** Delay as a proportion of the minimal travel time, set in the relative mode. */
	std::optional<double> relative;

	/** Maximum delay in seconds of a request with the given minimal travel time. */
	[[nodiscard]] unsigned get(unsigned min_travel_time) const;
};

/** The maximum delay is 0 if the configuration provides none of the keys. */
Max_delay load_max_delay(const YAML::Node& config);
}

class DARP_benchmark_reader : public Reader<Amodsim_node> {
public:
	DARP_instance<Amodsim_node> read(std::filesystem::path filepath) override;

	// Main dispatcher based on file extension. Relative demand.filepath is resolved against \p instance_directory.
	static std::unique_ptr<std::vector<Request<Amodsim_node>>> load_requests(
		const YAML::Node& config,
		const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider,
		const std::filesystem::path& instance_directory
	);

private:
	// Loader for .di format
	static std::unique_ptr<std::vector<Request<Amodsim_node>>> load_requests_di(
		const std::string& request_filepath_str,
		const internal::Max_delay& max_delay,
		const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
	);

	// Loader for .csv format using csv2
	static std::unique_ptr<std::vector<Request<Amodsim_node>>> load_requests_csv(
		const std::string& request_filepath_str,
		const internal::Max_delay& max_delay,
		const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
	);

	void load_vehicles(std::vector<Vehicle<Amodsim_node>>& vehicles, std::string file_path, unsigned instance_start_time) const;


};
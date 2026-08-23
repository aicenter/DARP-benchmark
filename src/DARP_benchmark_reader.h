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

void load_vehicles_csv(std::vector<Vehicle<Amodsim_node>>& vehicles, const std::string& file_path, unsigned instance_start_time);
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
		unsigned short max_prolongation,
		const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
	);

	// Loader for .csv format using csv2
	static std::unique_ptr<std::vector<Request<Amodsim_node>>> load_requests_csv(
		const std::string& request_filepath_str,
		unsigned short max_prolongation,
		const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
	);

	void load_vehicles(std::vector<Vehicle<Amodsim_node>>& vehicles, std::string file_path, unsigned instance_start_time) const;


};
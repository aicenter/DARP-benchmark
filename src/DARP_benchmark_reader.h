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

void load_vehicles_csv(std::vector<Vehicle<Amodsim_node>>& vehicles, const std::string& file_path);
}

class DARP_benchmark_reader : public Reader<Amodsim_node> {
public:
	DARP_instance<Amodsim_node> read(std::filesystem::path filepath) override;

	// Main dispatcher based on file extension
	static std::unique_ptr<std::vector<Request<Amodsim_node>>> load_requests(
		const YAML::Node& config,
		const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
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

	void load_vehicles(std::vector<Vehicle<Amodsim_node>>& vehicles, std::string file_path) const;


};
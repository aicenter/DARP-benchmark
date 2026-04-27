
#include <cmath>
#include <filesystem>
#include <unordered_map>
#include <spdlog/spdlog.h>
//#include "tqdm.hpp"
#include "../progress_bar.h"
#include "../crack_atof.cpp"


template<typename L>
Distance_matrix_node_travel_time_provider<L>::Distance_matrix_node_travel_time_provider(
	Distance_matrix_reader& reader,
	const std::string& dm_filepath
): Distance_matrix_travel_time_provider(reader, dm_filepath) {
	// fill the nodes
	for(unsigned int i = 0; i < width; ++i) {
		nodes[i] = std::make_shared<L>(i);
	}
}

template <typename L>
Distance_matrix_node_travel_time_provider<L>::Distance_matrix_node_travel_time_provider(
	const std::vector<std::shared_ptr<const L>>& nodes_par,
	const Travel_time_provider<L>& travel_time_provider
): Distance_matrix_travel_time_provider(
		static_cast<unsigned>(nodes_par.size()),
		std::make_unique<travel_time_type[]>(nodes_par.size() * nodes_par.size())
	)
{
	for(unsigned int i = 0; i < nodes_par.size(); ++i) {
		for(unsigned int j = 0; j < nodes_par.size(); ++j) {
			travel_time_type distance = 0;
			if(i != j) {
				distance = travel_time_provider.get_travel_time(*nodes_par[i], *nodes_par[j]);
			}
			this->dm[i * nodes_par.size() + j] = distance;
		}
		nodes[i] = nodes_par[i];
	}
}

template <typename L>
Distance_matrix_node_travel_time_provider<L>::Distance_matrix_node_travel_time_provider(
	const std::vector<std::shared_ptr<const L>>& nodes_par,
	std::unique_ptr<travel_time_type[]> dm
): Distance_matrix_travel_time_provider(static_cast<unsigned>(nodes_par.size()), std::move(dm)) {
	for (size_t i = 0; i < nodes_par.size(); ++i) {
		nodes[static_cast<unsigned>(i)] = nodes_par[i];
	}
}


template<typename L>
travel_time_type Distance_matrix_node_travel_time_provider<L>::get_travel_time(const L &from, const L &to) const {
	return get_travel_time(from.get_index(), to.get_index());
}

template<typename L>
std::tuple<const L&, travel_time_type> Distance_matrix_node_travel_time_provider<L>::get_vehicle_location_info(
	const L& last_action_location,
	const L& next_action_location,
	time_type time_since_last_action_departure
) const {
	auto [index, time_to_next_node] = Distance_matrix_travel_time_provider::get_vehicle_location_info(
		last_action_location.get_index(),
		next_action_location.get_index(),
		time_since_last_action_departure
	);
	return std::tuple<const L&, travel_time_type>(*nodes.at(index), time_to_next_node);
}


template <typename L>
size_t Distance_matrix_node_travel_time_provider<L>::get_nearest_vehicle(
	const L& location,
	const std::vector<const Vehicle<L>*>& vehicles
) {
	const unsigned int loc_index = location.get_index();

	if(vehicles.empty()) {
		throw std::runtime_error("No vehicles available!");
	}
	
	unsigned int min_dist = std::numeric_limits<unsigned int>::max();
	size_t best_vehicle = std::numeric_limits<size_t>::max();
	for(size_t i = 0; i < vehicles.size(); ++i) {
		const Vehicle<L>* vehicle = vehicles[i];
		const unsigned int dist = dm[vehicle->get_init_position().get_index() * width + loc_index];
		if(dist < min_dist) {
			min_dist = dist;
			best_vehicle = i;
		}
	}
	if(best_vehicle == std::numeric_limits<size_t>::max()) {
		throw std::runtime_error(fmt::format("Location {} or all {} available vehicles are outside the road network!", loc_index, vehicles.size()));
	}

	return best_vehicle;
}







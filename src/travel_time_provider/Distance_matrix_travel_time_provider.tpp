
#include <cmath>
#include <filesystem>
#include <unordered_map>
#include <spdlog/spdlog.h>
//#include "tqdm.hpp"
#include "../progress_bar.h"
#include "../crack_atof.cpp"


template <typename N>
Distance_matrix_node_travel_time_provider<N>::Distance_matrix_node_travel_time_provider(
	const std::vector<const N*>& nodes, 
	const Travel_time_provider<N>& travel_time_provider
): Distance_matrix_travel_time_provider(
		static_cast<unsigned>(nodes.size()), 
		std::make_unique<travel_time_type[]>(nodes.size() * nodes.size())
	) {
	for(unsigned int i = 0; i < nodes.size(); ++i) {
		for(unsigned int j = 0; j < nodes.size(); ++j) {
			travel_time_type distance = 0;
			if(i != j) {
				distance = travel_time_provider.get_travel_time(*nodes[i], *nodes[j]);
			}
			this->dm[i * nodes.size() + j] = distance;
		}
	}
}


template<typename N>
travel_time_type Distance_matrix_node_travel_time_provider<N>::get_travel_time(const N &from, const N &to) const {
	return get_travel_time(from.get_index(), to.get_index());
}


template <typename N>
size_t Distance_matrix_node_travel_time_provider<N>::get_nearest_vehicle(
	const N& location,
	const std::vector<const Vehicle<N>*>& vehicles
) {
	const unsigned int loc_index = location.get_index();

	if(vehicles.empty()) {
		throw std::runtime_error("No vehicles available!");
	}
	
	unsigned int min_dist = std::numeric_limits<unsigned int>::max();
	size_t best_vehicle = std::numeric_limits<size_t>::max();
	for(size_t i = 0; i < vehicles.size(); ++i) {
		const Vehicle<N>* vehicle = vehicles[i];
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







//
// Created by Fido on 2020-03-27.
//

#include <cmath>


template <typename L>
travel_time_type Euclidean_travel_time_provider<L>::get_travel_time(const L& from, const L& to) const{
	const double distance = sqrt(pow(from.getX() - to.getX(), 2) + pow(from.getY() - to.getY(),2));
	const auto travel_time = static_cast<travel_time_type>(round(distance * coordinate_resolution));
    return travel_time;
}

template <typename L>
size_t Euclidean_travel_time_provider<L>::get_nearest_vehicle(const L&,
                                                              // param name omitted to suppress the warning
                                                              const std::vector<const Vehicle<L>*>&) {
	// todo return the nearest vehicle using KDtree

	return 0;
}

template<typename L>
std::tuple<const L&, travel_time_type> Euclidean_travel_time_provider<L>::get_vehicle_location_info(
	[[maybe_unused]] const L& last_action_location,
	[[maybe_unused]] const L& next_action_location,
	[[maybe_unused]] time_type time_since_last_action_departure
) const {
	throw std::runtime_error("Not implemented");
}

template<typename L>
Euclidean_travel_time_provider<L>::Euclidean_travel_time_provider(const unsigned short coordinate_resolution)
        :coordinate_resolution(coordinate_resolution) {}

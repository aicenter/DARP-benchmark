//
// Created by Fido on 2020-03-27.
//

#include <cmath>


template <typename N>
travel_time_type Euclidean_travel_time_provider<N>::get_travel_time(const N& from, const N& to) const{
	const double distance = sqrt(pow(from.getX() - to.getX(), 2) + pow(from.getY() - to.getY(),2));
	const auto travel_time = static_cast<travel_time_type>(round(distance * coordinate_resolution));
    return travel_time;
}

template <typename N>
size_t Euclidean_travel_time_provider<N>::get_nearest_vehicle(const N&,
                                                              // param name omitted to suppress the warning
                                                              const std::vector<const Vehicle<N>*>&) {
	// todo return the nearest vehicle using KDtree

	return 0;
}

template<typename N>
Euclidean_travel_time_provider<N>::Euclidean_travel_time_provider(const unsigned short coordinate_resolution)
        :coordinate_resolution(coordinate_resolution) {}

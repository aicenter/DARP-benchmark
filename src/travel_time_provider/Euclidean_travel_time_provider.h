//
// Created by Fido on 2020-03-27.
//

#pragma once

//#include <concepts>

#include "Travel_time_provider.h"
#include "../Nearest_vehicle_provider.h"
#include "../Coordinate.h"

/*
 * Travel time provider template that computes the travel time as Euclidean distance between coordinates.
 * It can be transformed to class with using the Coordinate abstract class in the future.
 */
template <typename L>
class Euclidean_travel_time_provider final:
	public Travel_time_provider<L>,
	public Nearest_vehicle_provider<L>
{
public:
    explicit Euclidean_travel_time_provider(unsigned short coordinate_resolution);

    travel_time_type get_travel_time(const L& from, const L& to) const override;

    size_t get_nearest_vehicle(const L& location,
                               const std::vector<const Vehicle<L>*>& vehicles) override;

    [[nodiscard]] std::tuple<const L&, travel_time_type> get_vehicle_location_info(
	    const L& last_action_location,
	    const L& next_action_location,
	    time_type time_since_last_action_departure
    ) const override;

private:
    /**
     * How many seconds should it take to travel by 1 in the coordinate system
     */
    const unsigned short coordinate_resolution;

};

#include "Euclidean_travel_time_provider.tpp"

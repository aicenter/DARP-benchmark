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
template <typename N>
class Euclidean_travel_time_provider final:
	public Travel_time_provider<N>,
	public Nearest_vehicle_provider<N>
{
public:
    explicit Euclidean_travel_time_provider(unsigned short coordinate_resolution);

    travel_time_type get_travel_time(const N& from, const N& to) const override;

    size_t get_nearest_vehicle(const N& location,
                               const std::vector<const Vehicle<N>*>& vehicles) override;

private:
    /**
     * How many seconds should it take to travel by 1 in the coordinate system
     */
    const unsigned short coordinate_resolution;

};

#include "Euclidean_travel_time_provider.tpp"

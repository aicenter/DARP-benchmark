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

#include <iostream>
#include "../aliases.h"
#include "../Vehicle.h"


/**
 * @brief Computes travel time between two locations. These locations are arbitrary for this template interface, they
 * are expected to be specified in the Travel_time_provider implementations.
 * @tparam L location type 
*/
template<typename L>
class Travel_time_provider {
public:
	Travel_time_provider() = default;

	virtual ~Travel_time_provider() = default;

	/**
	 * Provides travel time in seconds between location \p from and \p to.
	 * @param from from
	 * @param to to
	 * @return Travel time between location \p from and \p to in seconds.
	 */
	[[nodiscard]] virtual travel_time_type get_travel_time(const L& from, const L& to) const = 0;

	/**
	 * @brief Provides travel time from a vehicle's starting point to a target location.
	 * For regular vehicles, this uses the vehicle's initial position.
	 * For virtual vehicles, this returns the time_to_start value.
	 * @param vehicle The vehicle (can be Vehicle<L> or Virtual_vehicle)
	 * @param target The target location
	 * @return Travel time from vehicle start to target location
	 */
	[[nodiscard]] travel_time_type get_travel_time_from_vehicle(const Vehicle_base& vehicle, const L& target) const;

	/**
	 * Method that provides location info when the vehicle is on the way from last_action_location to next_action_location.
	 * @param last_action_location
	 * @param next_action_location
	 * @param time_since_last_action_departure
	 * @return tuple of next location on the path and travel time needed to reach that location
	 */
	[[nodiscard]] virtual std::tuple<const L&, travel_time_type> get_vehicle_location_info(
		const L& last_action_location,
		const L& next_action_location,
		time_type time_since_last_action_departure
	) const = 0;

protected:
	Travel_time_provider(const Travel_time_provider& other) = default;

	Travel_time_provider(Travel_time_provider&& other) noexcept = default;

	Travel_time_provider& operator=(const Travel_time_provider& other) = default;

	Travel_time_provider& operator=(Travel_time_provider&& other) noexcept = default;

};



#include "Travel_time_provider.tpp"
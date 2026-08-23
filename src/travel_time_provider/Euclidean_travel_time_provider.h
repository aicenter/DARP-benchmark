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

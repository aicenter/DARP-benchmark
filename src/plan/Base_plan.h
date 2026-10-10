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

#include <vector>
#include "../aliases.h"

/**
 * @brief Base template class for plans. It is agnostic to the type of actions data or vehicle. It contains
 * the functionality that is common to all plans.
 * @tparam A action type
 * @tparam V vehicle type
 */
template<class A, class V>
class Base_plan {
public:

	/**
	 * @brief Construct an empty plan.
	 * @param vehicle_par vehicle
	 */
	explicit Base_plan(const V& vehicle_par);


	explicit Base_plan(
		const std::vector<A>& actions,
		const V& vehicle,
		cost_type cost,
		unsigned departure_time,
		unsigned arrival_time
	)
		: actions(actions), vehicle(vehicle), cost(cost), departure_time(departure_time), arrival_time(arrival_time) {}

//	/**
//	 * This constructor creates a plan with an inconsistent/invalid state. It should be only used by the test classes.
//	 * @param actions
//	 * @param vehicle
//	 */
//	Base_plan(const std::vector<A>& actions, const std::reference_wrapper<const V>& vehicle, unsigned cost)
//		: actions(actions), vehicle(vehicle), cost(cost) {}


	[[nodiscard]] const std::vector<A>& get_actions() const;

	[[nodiscard]] cost_type get_cost() const;

	[[nodiscard]] const V& get_vehicle() const;

	[[nodiscard]] time_type get_departure_time() const;

	[[nodiscard]] time_type get_arrival_time() const;


	const A& operator[](plan_size_type index) const;


	[[nodiscard]] typename std::vector<A>::const_iterator begin() const;

	[[nodiscard]] typename std::vector<A>::const_iterator end() const;


	[[nodiscard]] plan_size_type get_length() const;

protected:
	std::vector<A> actions;

	std::reference_wrapper<const V> vehicle;

	cost_type cost{0};

	unsigned departure_time{0};

	unsigned arrival_time{0};


};

template<class A, class V>
struct Plan_size_comparator {
	bool operator()(const Base_plan<A,V>& plan_a, const Base_plan<A,V>& plan_b);
};

#include "Base_plan.tpp"

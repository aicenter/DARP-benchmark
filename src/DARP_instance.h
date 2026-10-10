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

#include <cstdint>
#include <memory>
#include <vector>

#include "travel_time_provider/Travel_time_provider.h"
#include "Vehicle.h"
#include "Request.h"

class Node {
public:

	explicit Node(unsigned index_par)
		: index(index_par) {
	}

	//virtual bool operator==(const Node& other) const = 0;
	
	//bool operator!=(const Node& other) const{
	//	return !(*this == other);
	//}

	[[nodiscard]] unsigned int get_index() const;
private:
	const unsigned int index;
};

/**
 * Instance problem type from instance config.yaml (`problem` key), as in
 * aicenter/Ridesharing_DARP_instances: default DARP; fleet-sizing omits fixed vehicles from input.
 */
enum class problem_type : std::uint8_t {
	darp,
	fleet_sizing
};

/**
 * Weights of the generalized cost model (the `cost` section of the instance config). Only the components that the
 * benchmark evaluates are present; the loader rejects any other weight that is non-zero. The defaults reproduce the
 * legacy cost: total travel time + `demand.relative_delay_cost` * drop-off delay + `vehicles.capital_cost` per plan.
 */
struct Cost_weights {
	/** Weight of a second of vehicle travel time. */
	double travel_time_weight{1.0};
	/** Weight of a second of passenger drop-off delay (drop-off arrival minus the ideal direct-ride arrival). */
	double passenger_delay_weight{0.0};
	/** Constant charged to every non-empty plan. */
	double vehicle_capital_cost{0.0};

	[[nodiscard]] bool is_default() const {
		return travel_time_weight == 1.0 && passenger_delay_weight == 0.0 && vehicle_capital_cost == 0.0;
	}
};

class DARP_instance_configuration {
public:
	explicit DARP_instance_configuration(
		unsigned long max_route_duration = 0, 
		unsigned long max_ride_time = 0, 
		bool return_to_depot = false,
		bool virtual_vehicles = false, 
		unsigned start_time = 0,
		Cost_weights cost_weights = {},
		problem_type problem = problem_type::darp
	):
		max_route_duration(max_route_duration),
		max_ride_time(max_ride_time),
		return_to_depot(return_to_depot),
		virtual_vehicles(virtual_vehicles),
		start_time(start_time),
		cost_weights(cost_weights),
		problem(problem) {
	}

	[[nodiscard]] unsigned long get_max_route_duration() const;
	[[nodiscard]] unsigned long get_max_ride_time() const;
	[[nodiscard]] bool is_return_to_depot() const;
	[[nodiscard]] bool use_virtual_vehicles() const;
	[[nodiscard]] unsigned get_start_time() const;
	[[nodiscard]] const Cost_weights& get_cost_weights() const;
	/** Legacy accessor of `cost_weights.vehicle_capital_cost` truncated to an integer; prefer get_cost_weights(). */
	[[nodiscard]] unsigned short get_vehicle_capital_cost() const;
	/** Legacy accessor of `cost_weights.passenger_delay_weight`; prefer get_cost_weights(). */
	[[nodiscard]] double get_relative_delay_cost() const;
	[[nodiscard]] problem_type get_problem() const;

	void set_max_route_duration(unsigned long value);
	void set_max_ride_time(unsigned long value);
	void set_return_to_depot(bool value);
	void set_virtual_vehicles(bool value);
	void set_start_time(unsigned value);
	void set_cost_weights(Cost_weights value);
	/** Legacy setter of `cost_weights.vehicle_capital_cost`; prefer set_cost_weights(). */
	void set_vehicle_capital_cost(unsigned short value);
	/** Legacy setter of `cost_weights.passenger_delay_weight`; prefer set_cost_weights(). */
	void set_relative_delay_cost(double value);
	void set_problem(problem_type value);

private:
    unsigned long max_route_duration{0};

	/**
	 * Maximum ride time for a request. The ride time is computed as the interval between the departure from the pickup
	 * location and the start of the service at the drop off location.
	 */
    unsigned long max_ride_time{0};

	/**
	* @brief Specifies whether vehicles have to return to the depot after serving the last request
	*/
	bool return_to_depot{true};
	bool virtual_vehicles{false};
    unsigned start_time{0};
	Cost_weights cost_weights{};
	problem_type problem{problem_type::darp};
};

class DARP_instance_interface {
public:
	virtual ~DARP_instance_interface() = default;
};


template <typename N>
class DARP_instance: public DARP_instance_interface
{
public:
    DARP_instance(
        std::unique_ptr<std::vector<Request<N>>> requests, 
        std::unique_ptr<std::vector<Vehicle<N>>> vehicles,
        std::shared_ptr<Travel_time_provider<N>> travel_time_provider,
		std::shared_ptr<DARP_instance_configuration> darp_instance_configuration
    );

	DARP_instance(DARP_instance&& other);

    const std::vector<Vehicle<N>>& get_vehicles() const;

    const std::vector<Request<N>>& get_requests() const;

    const std::shared_ptr<Travel_time_provider<N>>& get_travelcost_provider() const;

	[[nodiscard]] const std::shared_ptr<DARP_instance_configuration>& get_darp_instance_configuration() const;

	[[nodiscard]] unsigned long get_max_route_duration() const;

    [[nodiscard]] unsigned long get_max_ride_time() const;

    [[nodiscard]] bool is_return_to_depot() const;

    [[nodiscard]] bool is_virtual_vehicles() const;

	[[nodiscard]] problem_type get_problem() const;
private:
    std::unique_ptr<std::vector<Vehicle<N>>> vehicles;
    std::unique_ptr<std::vector<Request<N>>> requests;
    const std::shared_ptr<Travel_time_provider<N>> travel_cost_provider;
	const std::shared_ptr<DARP_instance_configuration> darp_instance_configuration;
};

template <typename N>
const std::shared_ptr<DARP_instance_configuration>& DARP_instance<N>::get_darp_instance_configuration() const {
	return darp_instance_configuration;
}

template <typename N>
bool DARP_instance<N>::is_return_to_depot() const {
	return darp_instance_configuration->is_return_to_depot();
}

template <typename N>
bool DARP_instance<N>::is_virtual_vehicles() const {
	return darp_instance_configuration->use_virtual_vehicles();
}

template <typename N>
problem_type DARP_instance<N>::get_problem() const {
	return darp_instance_configuration->get_problem();
}

#include "DARP_instance.tpp"

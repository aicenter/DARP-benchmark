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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <optional>
#include <string>

#include <fmt/format.h>

#include "../aliases.h"
#include "../Action.h"
#include "../ActionData.h"
#include "../DARP_instance.h"
#include "../Request.h"
#include "../Vehicle.h"
#include "../travel_time_provider/Travel_time_provider.h"

/**
 * @brief Unweighted quantities of the cost components of a plan and the resulting weighted cost.
 */
struct Cost_breakdown {
	/** Vehicle travel time [s]. */
	double travel_time{0};
	/** Passenger drop-off delay [s]: drop-off arrival minus the ideal direct-ride arrival, summed over drop-offs. */
	double passenger_delay{0};
	/** Constant of the plan: the vehicle capital cost, charged to non-empty plans. */
	cost_type plan_constant{0};
	/** Weighted cost of the plan. */
	cost_type total{0};

	[[nodiscard]] std::string describe(const Cost_weights& weights) const {
		return fmt::format(
			"travel_time {} s * {} + passenger_delay {} s * {} + vehicle_capital_cost {}",
			travel_time, weights.travel_time_weight, passenger_delay, weights.passenger_delay_weight, plan_constant);
	}
};

/**
 * @brief Compares costs with a relative tolerance: costs are sums of weighted integer quantities, so two evaluations
 * of the same plan can differ only by floating-point rounding.
 */
[[nodiscard]] inline bool costs_equal(cost_type a, cost_type b) noexcept {
	return std::abs(a - b) <= 1e-6 * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}

// Cost components. Each one is a small value type with inline hooks, so the incremental use in the plan builders of the
// solvers costs one multiply-add per leg or drop-off. To add a component: add its weight to Cost_weights, a struct
// here, a member of Cost_evaluator with a hook, a field of Cost_breakdown, and its term in evaluate_sequence.

/** Travel time of the vehicle, accumulated per driven leg. */
struct Travel_time_component {
	double weight;

	[[nodiscard]] cost_type on_leg(time_type travel_time) const noexcept {
		return weight * static_cast<double>(travel_time);
	}
};

/** Passenger delay: drop-off arrival minus the ideal arrival of the direct ride from the desired pickup time. */
struct Passenger_delay_component {
	double weight;

	[[nodiscard]] cost_type on_dropoff(time_type arrival, time_type ideal_arrival) const noexcept {
		assert(arrival >= ideal_arrival && "drop-off before the direct ride from the desired pickup time could arrive");
		return weight * (static_cast<double>(arrival) - static_cast<double>(ideal_arrival));
	}

	/** Delay added to drop-offs already in a plan when their arrival is shifted later (time adjustments). */
	[[nodiscard]] cost_type on_delay_shift(time_type shift) const noexcept {
		return weight * static_cast<double>(shift);
	}
};

/** Constant charged to every non-empty plan. */
struct Vehicle_capital_component {
	cost_type cost;

	[[nodiscard]] cost_type on_plan() const noexcept {
		return cost;
	}
};

/**
 * @brief Solver-independent evaluation of the weighted plan cost (see Cost_weights). The solvers use the incremental
 * hooks while building plans; the framework evaluates finished plans with evaluate() when a Solution is created and
 * in the plan checks, so the reported cost never depends on the solver.
 */
class Cost_evaluator {
public:
	explicit Cost_evaluator(const Cost_weights& weights):
		weights_(weights),
		travel_time_{weights.travel_time_weight},
		passenger_delay_{weights.passenger_delay_weight},
		vehicle_capital_{weights.vehicle_capital_cost} {
	}

	[[nodiscard]] static Cost_evaluator from_configuration(const DARP_instance_configuration& configuration) {
		return Cost_evaluator(configuration.get_cost_weights());
	}

	[[nodiscard]] const Cost_weights& get_weights() const noexcept {
		return weights_;
	}

	// incremental hooks

	/** Cost of a driven leg. */
	[[nodiscard]] cost_type leg_cost(time_type travel_time) const noexcept {
		return travel_time_.on_leg(travel_time);
	}

	/** Cost of a drop-off arriving at \p arrival when the direct ride from the desired pickup time arrives at \p ideal_arrival. */
	[[nodiscard]] cost_type dropoff_cost(time_type arrival, time_type ideal_arrival) const noexcept {
		return passenger_delay_.on_dropoff(arrival, ideal_arrival);
	}

	/** Cost of shifting the arrival of drop-offs already in the plan \p shift seconds later. */
	[[nodiscard]] cost_type delay_shift_cost(time_type shift) const noexcept {
		return passenger_delay_.on_delay_shift(shift);
	}

	/** Constant part of the cost of a non-empty plan. */
	[[nodiscard]] cost_type plan_constant() const noexcept {
		return vehicle_capital_.on_plan();
	}

	/** Arrival of the direct ride started at the desired pickup time. */
	template<typename N>
	[[nodiscard]] static time_type ideal_dropoff_arrival(const Request<N>& request) noexcept {
		return request.get_pickup().get_min_time() + request.get_min_travel_time();
	}

	/** Arrival of the direct ride for the request of a drop-off action. */
	template<typename N>
	[[nodiscard]] static time_type ideal_dropoff_arrival(const Action<N>& dropoff) {
		assert(dropoff.get_action_type() == Action_type::dropoff);
		return ideal_dropoff_arrival(static_cast<const Service_action<N>&>(dropoff).get_request());
	}

	// full evaluation

	/**
	 * Evaluates a finished plan: the first leg from the vehicle start (the time to start for virtual vehicles), the
	 * legs between the actions, the return-to-depot leg when the instance requires it, and the drop-off delays.
	 * @tparam Plan plan or plan builder providing get_vehicle(), get_length(), operator[] and get_departure_time()
	 */
	template<typename N, typename Plan>
	[[nodiscard]] Cost_breakdown evaluate(
		const Plan& plan,
		const Travel_time_provider<N>& travel_time_provider,
		const DARP_instance_configuration& configuration
	) const {
		const auto first = first_service_action_index(plan);
		if(!first) {
			return evaluate_sequence(0, plan, travel_time_provider, std::nullopt);
		}
		const ActionData<N>& first_action = plan[*first];
		const Vehicle_base& vehicle = plan.get_vehicle();
		const time_type first_leg = std::max<time_type>(
			travel_time_provider.get_travel_time_from_vehicle(vehicle, first_action.get_node()),
			first_action.get_arrival_time() - plan.get_departure_time());

		std::optional<time_type> return_leg;
		if(configuration.is_return_to_depot() && configuration.get_problem() != problem_type::fleet_sizing) {
			if(const auto* regular_vehicle = dynamic_cast<const Vehicle<N>*>(&vehicle)) {
				const ActionData<N>& last_action = plan[last_service_action_index(plan)];
				return_leg = travel_time_provider.get_travel_time(last_action.get_node(), regular_vehicle->get_init_position());
			}
		}
		return evaluate_sequence(first_leg, plan, travel_time_provider, return_leg);
	}

	/**
	 * Evaluates the action sequence of a plan with an explicit first leg (e.g., the remaining time to the next node of
	 * a vehicle on its way plus the travel time from there) and an optional return leg. A leg between two actions
	 * counts the longer of the matrix travel time and the scheduled time between the departure and the arrival, so
	 * detours of re-routed vehicles count as driving.
	 */
	template<typename N, typename Plan>
	[[nodiscard]] Cost_breakdown evaluate_sequence(
		time_type first_leg_travel_time,
		const Plan& plan,
		const Travel_time_provider<N>& travel_time_provider,
		std::optional<time_type> return_leg_travel_time
	) const {
		Cost_breakdown breakdown;
		const ActionData<N>* previous = nullptr;
		for(unsigned i = 0; i < plan.get_length(); ++i) {
			const ActionData<N>& action = plan[i];
			if(action.get_action_type() == Action_type::depot) {
				continue;
			}
			time_type leg;
			if(previous == nullptr) {
				leg = first_leg_travel_time;
			}
			else {
				assert(previous->get_departure_time() >= 0);
				const time_type scheduled = action.get_arrival_time() - static_cast<time_type>(previous->get_departure_time());
				leg = std::max<time_type>(travel_time_provider.get_travel_time(previous->get_node(), action.get_node()), scheduled);
			}
			breakdown.travel_time += static_cast<double>(leg);
			if(action.get_action_type() == Action_type::dropoff) {
				breakdown.passenger_delay += static_cast<double>(action.get_arrival_time())
					- static_cast<double>(ideal_dropoff_arrival<N>(action.get_action()));
			}
			previous = &action;
		}
		if(previous != nullptr) {
			if(return_leg_travel_time) {
				breakdown.travel_time += static_cast<double>(*return_leg_travel_time);
			}
			breakdown.plan_constant = plan_constant();
		}
		breakdown.total = weights_.travel_time_weight * breakdown.travel_time
			+ weights_.passenger_delay_weight * breakdown.passenger_delay
			+ breakdown.plan_constant;
		return breakdown;
	}

private:
	Cost_weights weights_;
	Travel_time_component travel_time_;
	Passenger_delay_component passenger_delay_;
	Vehicle_capital_component vehicle_capital_;

	template<typename Plan>
	[[nodiscard]] static std::optional<unsigned> first_service_action_index(const Plan& plan) {
		for(unsigned i = 0; i < plan.get_length(); ++i) {
			if(plan[i].get_action_type() != Action_type::depot) {
				return i;
			}
		}
		return std::nullopt;
	}

	template<typename Plan>
	[[nodiscard]] static unsigned last_service_action_index(const Plan& plan) {
		for(unsigned i = plan.get_length(); i > 0; --i) {
			if(plan[i - 1].get_action_type() != Action_type::depot) {
				return i - 1;
			}
		}
		assert(false && "plan without service actions");
		return 0;
	}
};

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
#include <type_traits>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "../aliases.h"
#include "../Action.h"
#include "../ActionData.h"
#include "../DARP_instance.h"
#include "../Request.h"
#include "../Vehicle.h"
#include "../Virtual_node.h"
#include "Leg_cost_component.h"
#include "../travel_time_provider/Travel_time_provider.h"

/** Per-component breakdown of a cost: component key (as in the `cost_components` of the solution JSON) and its
 * weighted contribution. */
using Cost_components = std::vector<std::pair<std::string, cost_type>>;

/**
 * @brief Unweighted quantities of the cost components of a plan and the resulting weighted cost.
 */
struct Cost_breakdown {
	/** Vehicle travel time [s]. */
	double travel_time{0};
	/** Passenger drop-off delay [s]: drop-off arrival minus the ideal direct-ride arrival, summed over drop-offs. */
	double passenger_delay{0};
	/** Quantities of the leg cost components, in the order of Cost_weights::leg_components. */
	std::vector<double> leg_components;
	/** Constant of the plan: the vehicle capital cost, charged to non-empty plans. */
	cost_type plan_constant{0};
	/** Plan duration [s]: from the plan departure to the end of the last action or the depot return; 0 if empty. */
	double plan_duration{0};
	/** Weighted cost of the plan. */
	cost_type total{0};

	/** Weighted contributions of the components, summing to total. */
	[[nodiscard]] Cost_components components(const Cost_weights& weights) const {
		Cost_components result{
			{"travel_time", weights.travel_time_weight * travel_time},
			{"passenger_delay", weights.passenger_delay_weight * passenger_delay},
			{"vehicle_capital", plan_constant},
			{"plan_duration", weights.plan_duration_weight * plan_duration},
		};
		for(size_t i = 0; i < weights.leg_components.size(); ++i) {
			const Leg_cost_component& component = *weights.leg_components[i];
			result.emplace_back(component.name(), component.weight() * leg_components[i]);
		}
		return result;
	}

	[[nodiscard]] std::string describe(const Cost_weights& weights) const {
		std::string description = fmt::format(
			"travel_time {} s * {} + passenger_delay {} s * {} + vehicle_capital_cost {} + plan_duration {} s * {}",
			travel_time, weights.travel_time_weight, passenger_delay, weights.passenger_delay_weight, plan_constant,
			plan_duration, weights.plan_duration_weight);
		for(size_t i = 0; i < weights.leg_components.size(); ++i) {
			const Leg_cost_component& component = *weights.leg_components[i];
			description += fmt::format(" + {} {} * {}", component.name(), leg_components[i], component.weight());
		}
		return description;
	}
};

/** Writes \p components as the `cost_components` object, nothing when empty. */
inline void write_cost_components(
	rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const Cost_components& components
) {
	if(components.empty()) {
		return;
	}
	writer.Key("cost_components");
	writer.StartObject();
	for(const auto& [key, value]: components) {
		writer.Key(key.c_str());
		writer.Double(value);
	}
	writer.EndObject();
}

/** Return leg of a plan: the leg from the last action to \p target with the travel time \p travel_time. */
template<typename N>
struct Return_leg {
	const N* target;
	time_type travel_time;
};

/**
 * @brief Compares costs with a relative tolerance: costs are sums of weighted integer quantities, so two evaluations
 * of the same plan can differ only by floating-point rounding.
 */
[[nodiscard]] inline bool costs_equal(cost_type a, cost_type b) noexcept {
	return std::abs(a - b) <= 1e-6 * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}

// Cost components. The fixed ones are small value types with inline hooks, so the incremental use in the plan builders
// of the solvers costs one multiply-add per leg or drop-off. To add a fixed component: add its weight to Cost_weights,
// a struct here, a member of Cost_evaluator with a hook, a field of Cost_breakdown, and its term in evaluate_sequence.
// Components priced per leg by a model loaded at startup (e.g., distance) implement Leg_cost_component instead and are
// added to leg_cost_component_types(); leg_cost and evaluate_sequence price them generically.

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

/** Duration of a plan, charged once per non-empty plan. */
struct Plan_duration_component {
	double weight;

	[[nodiscard]] cost_type on_plan(time_type departure, time_type end) const noexcept {
		assert(end >= departure && "plan ends before its departure");
		return weight * (static_cast<double>(end) - static_cast<double>(departure));
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
		vehicle_capital_{weights.vehicle_capital_cost},
		plan_duration_{weights.plan_duration_weight} {
	}

	[[nodiscard]] static Cost_evaluator from_configuration(const DARP_instance_configuration& configuration) {
		return Cost_evaluator(configuration.get_cost_weights());
	}

	[[nodiscard]] const Cost_weights& get_weights() const noexcept {
		return weights_;
	}

	// incremental hooks

	/**
	 * Cost of a driven leg from \p from to \p to taking \p travel_time: the travel time term plus the leg cost
	 * components. The components contribute nothing on a leg from or to the virtual node (see virtual_node()), i.e.,
	 * on the first leg of a virtual vehicle.
	 */
	template<typename N>
	[[nodiscard]] cost_type leg_cost(const N& from, const N& to, time_type travel_time) const {
		cost_type cost = travel_time_.on_leg(travel_time);
		if(!is_virtual_node(from) && !is_virtual_node(to)) {
			for(const auto& component: weights_.leg_components) {
				cost += component->weight() * component->value(node_index(from), node_index(to));
			}
		}
		return cost;
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

	/** Cost of the duration of a non-empty plan departing at \p departure and ending at \p end (the end of the last
	 * action, or the depot return when the plan includes it). */
	[[nodiscard]] cost_type plan_duration_cost(time_type departure, time_type end) const noexcept {
		return plan_duration_.on_plan(departure, end);
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
	 * legs between the actions, the return-to-depot leg when the instance requires it, the drop-off delays, and the
	 * plan duration from the plan departure to the end of the last action or the depot return.
	 * @tparam Plan plan or plan builder providing get_vehicle(), get_length(), operator[] and get_departure_time()
	 */
	template<typename N, typename Plan>
	[[nodiscard]] Cost_breakdown evaluate(
		const Plan& plan,
		const Travel_time_provider<N>& travel_time_provider,
		const DARP_instance_configuration& configuration
	) const {
		const Vehicle_base& vehicle = plan.get_vehicle();
		const auto first = first_service_action_index(plan);
		if(!first) {
			return evaluate_sequence(virtual_node<N>(), time_type{0}, plan, travel_time_provider, std::nullopt);
		}
		const ActionData<N>& first_action = plan[*first];
		const time_type first_leg = std::max<time_type>(
			travel_time_provider.get_travel_time_from_vehicle(vehicle, first_action.get_node()),
			first_action.get_arrival_time() - plan.get_departure_time());

		std::optional<Return_leg<N>> return_leg;
		if(configuration.is_return_to_depot() && configuration.get_problem() != problem_type::fleet_sizing) {
			if(const auto* regular_vehicle = dynamic_cast<const Vehicle<N>*>(&vehicle)) {
				const ActionData<N>& last_action = plan[last_service_action_index(plan)];
				const N& depot = regular_vehicle->get_init_position();
				return_leg = Return_leg<N>{&depot, travel_time_provider.get_travel_time(last_action.get_node(), depot)};
			}
		}
		return evaluate_sequence(
			travel_time_provider.get_origin_node(vehicle), first_leg, plan, travel_time_provider, return_leg);
	}

	/**
	 * Evaluates the action sequence of a plan with an explicit first leg and an optional return leg. The first leg
	 * starts at \p first_leg_origin and takes \p first_leg_travel_time, e.g., for a vehicle on its way: the next node
	 * on its path and the remaining time to it plus the travel time from there (the leg cost components then count
	 * from the next node only). A leg between two actions counts the longer of the matrix travel time and the
	 * scheduled time between the departure and the arrival, so detours of re-routed vehicles count as driving; the leg
	 * cost components count the matrix values only.
	 */
	template<typename N, typename Plan>
	[[nodiscard]] Cost_breakdown evaluate_sequence(
		const N& first_leg_origin,
		time_type first_leg_travel_time,
		const Plan& plan,
		const Travel_time_provider<std::type_identity_t<N>>& travel_time_provider,
		std::optional<Return_leg<std::type_identity_t<N>>> return_leg
	) const {
		Cost_breakdown breakdown;
		breakdown.leg_components.assign(weights_.leg_components.size(), 0.0);
		const ActionData<N>* previous = nullptr;
		for(unsigned i = 0; i < plan.get_length(); ++i) {
			const ActionData<N>& action = plan[i];
			if(action.get_action_type() == Action_type::depot) {
				continue;
			}
			time_type leg;
			if(previous == nullptr) {
				leg = first_leg_travel_time;
				add_leg_components(breakdown, first_leg_origin, action.get_node());
			}
			else {
				assert(previous->get_departure_time() >= 0);
				const time_type scheduled = action.get_arrival_time() - static_cast<time_type>(previous->get_departure_time());
				leg = std::max<time_type>(travel_time_provider.get_travel_time(previous->get_node(), action.get_node()), scheduled);
				add_leg_components(breakdown, previous->get_node(), action.get_node());
			}
			breakdown.travel_time += static_cast<double>(leg);
			if(action.get_action_type() == Action_type::dropoff) {
				breakdown.passenger_delay += static_cast<double>(action.get_arrival_time())
					- static_cast<double>(ideal_dropoff_arrival<N>(action.get_action()));
			}
			previous = &action;
		}
		if(previous != nullptr) {
			assert(previous->get_departure_time() >= 0);
			time_type end = static_cast<time_type>(previous->get_departure_time());
			if(return_leg) {
				breakdown.travel_time += static_cast<double>(return_leg->travel_time);
				add_leg_components(breakdown, previous->get_node(), *return_leg->target);
				end += return_leg->travel_time;
			}
			breakdown.plan_constant = plan_constant();
			breakdown.plan_duration = static_cast<double>(end) - static_cast<double>(plan.get_departure_time());
		}
		breakdown.total = weights_.travel_time_weight * breakdown.travel_time
			+ weights_.passenger_delay_weight * breakdown.passenger_delay
			+ breakdown.plan_constant
			+ weights_.plan_duration_weight * breakdown.plan_duration;
		for(size_t i = 0; i < weights_.leg_components.size(); ++i) {
			breakdown.total += weights_.leg_components[i]->weight() * breakdown.leg_components[i];
		}
		return breakdown;
	}

private:
	Cost_weights weights_;
	Travel_time_component travel_time_;
	Passenger_delay_component passenger_delay_;
	Vehicle_capital_component vehicle_capital_;
	Plan_duration_component plan_duration_;

	template<typename N>
	void add_leg_components(Cost_breakdown& breakdown, const N& from, const N& to) const {
		if(is_virtual_node(from) || is_virtual_node(to)) {
			return;
		}
		for(size_t i = 0; i < weights_.leg_components.size(); ++i) {
			breakdown.leg_components[i] += weights_.leg_components[i]->value(node_index(from), node_index(to));
		}
	}

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

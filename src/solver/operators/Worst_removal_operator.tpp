#pragma once

#include <algorithm>
#include <cassert>

template<typename N, class P>
Worst_removal_operator<N, P>::Worst_removal_operator(const DARP_context<N>& context, bool stop_groups):
	travel_time_provider(*context.travel_time_provider()),
	configuration(*context.darp_instance_configuration()),
	evaluator(context.cost_evaluator()),
	stop_groups(stop_groups) {
}

template<typename N, class P>
void Worst_removal_operator<N, P>::init(const std::vector<P>& plans) {
	if(candidates.size() < plans.size()) {
		candidates.resize(plans.size());
		plan_costs.resize(plans.size());
	}
	cost = 0;
	served_request_count = 0;
	for(size_t plan_index = 0; plan_index < plans.size(); ++plan_index) {
		const P& plan = plans[plan_index];
		assert(std::ranges::none_of(plan.get_actions(), [](const ActionData<N>& action) {
			return action.get_action_type() == Action_type::depot;
		}) && "plans with depot actions are not supported");
		ensure_buffers(plan.get_length());
		candidates[plan_index].reserve(plan.get_length());
		plan_costs[plan_index] = evaluator.evaluate<N>(plan, travel_time_provider, configuration).total;
		cost += plan_costs[plan_index];
		for(plan_size_type i = 0; i < plan.get_length(); ++i) {
			if(plan[i].get_action_type() == Action_type::pickup) {
				++served_request_count;
			}
		}
		evaluate_plan(plans, plan_index);
	}
}

template<typename N, class P>
void Worst_removal_operator<N, P>::ensure_buffers(plan_size_type plan_length) {
	if(removed_flags.size() < plan_length) {
		removed_flags.resize(plan_length, 0);
		flagged_positions.reserve(plan_length);
		base_departures.resize(plan_length);
		base_prefix_costs.resize(plan_length);
		candidate_arrivals.resize(plan_length);
		candidate_departures.resize(plan_length);
	}
}

template<typename N, class P>
void Worst_removal_operator<N, P>::prepare_plan(const P& plan) {
	depot = nullptr;
	if(configuration.is_return_to_depot() && configuration.get_problem() != problem_type::fleet_sizing) {
		if(const auto* vehicle = dynamic_cast<const Vehicle<N>*>(&plan.get_vehicle())) {
			depot = &vehicle->get_init_position();
		}
	}
	// base simulation: the plan with all its actions
	[[maybe_unused]] const auto base_cost = simulate(plan, 0, true);
}

template<typename N, class P>
void Worst_removal_operator<N, P>::evaluate_plan(const std::vector<P>& plans, size_t plan_index) {
	const P& plan = plans[plan_index];
	std::vector<Candidate>& plan_candidates = candidates[plan_index];
	plan_candidates.clear();
	if(plan.get_length() == 0) {
		return;
	}
	prepare_plan(plan);
	for(plan_size_type i = 0; i < plan.get_length(); ++i) {
		const ActionData<N>& action = plan[i];
		const Action_type action_type = action.get_action_type();
		if(action_type == Action_type::depot) {
			continue;
		}
		if(action_type == Action_type::pickup) {
			evaluate_candidate(plan, plan_index, i, 1);
		}
		if(!stop_groups) {
			continue;
		}
		// a stop: a maximal run of actions of one type at one node
		const unsigned node = node_index(action.get_node());
		const bool run_start = i == 0 || plan[i - 1].get_action_type() != action_type
			|| node_index(plan[i - 1].get_node()) != node;
		if(!run_start) {
			continue;
		}
		plan_size_type length = 1;
		while(i + length < plan.get_length() && plan[i + length].get_action_type() == action_type
			&& node_index(plan[i + length].get_node()) == node) {
			++length;
		}
		if(length > 1) {
			evaluate_candidate(plan, plan_index, i, length);
		}
	}
}

template<typename N, class P>
plan_size_type Worst_removal_operator<N, P>::flag_candidate(const P& plan, plan_size_type first, plan_size_type length) {
	plan_size_type from = first;
	for(plan_size_type i = first; i < first + length; ++i) {
		const auto other = static_cast<plan_size_type>(plan[i].get_other_action_data_index());
		removed_flags[i] = 1;
		removed_flags[other] = 1;
		flagged_positions.push_back(i);
		flagged_positions.push_back(other);
		from = std::min(from, other);
	}
	return from;
}

template<typename N, class P>
void Worst_removal_operator<N, P>::clear_flags() {
	for(const plan_size_type position: flagged_positions) {
		removed_flags[position] = 0;
	}
	flagged_positions.clear();
}

template<typename N, class P>
void Worst_removal_operator<N, P>::evaluate_candidate(
	const P& plan, size_t plan_index, plan_size_type first, plan_size_type length
) {
	const plan_size_type from = flag_candidate(plan, first, length);
	const auto candidate_cost = simulate(plan, from, false);
	clear_flags();
	if(candidate_cost) {
		candidates[plan_index].push_back(Candidate{first, length, plan_costs[plan_index] - *candidate_cost});
	}
}

template<typename N, class P>
std::optional<cost_type> Worst_removal_operator<N, P>::simulate(const P& plan, plan_size_type from, bool base) {
	std::vector<time_type>& departures = base ? base_departures : candidate_departures;
	const Vehicle_base& vehicle = plan.get_vehicle();
	const unsigned long max_ride_time = configuration.get_max_ride_time();

	cost_type plan_cost = 0;
	const N* previous_node = nullptr;
	time_type time = 0;
	time_type departure = plan.get_departure_time();
	if(from > 0) {
		// the actions before from are the same as in the base simulation
		plan_cost = base_prefix_costs[from - 1];
		previous_node = &plan[from - 1].get_node();
		time = base_departures[from - 1];
	}

	for(plan_size_type i = from; i < plan.get_length(); ++i) {
		if(removed_flags[i]) {
			continue;
		}
		const ActionData<N>& action = plan[i];
		if(previous_node == nullptr) {
			const time_type travel_time = travel_time_provider.get_travel_time_from_vehicle(vehicle, action.get_node());
			if(i > 0) {
				// the first actions are removed: the vehicle departs to the new first action, never earlier than before
				const time_type min_time = action.get_min_time();
				departure = std::max<time_type>(departure, min_time > travel_time ? min_time - travel_time : 0);
			}
			plan_cost += evaluator.leg_cost(travel_time_provider.get_origin_node(vehicle), action.get_node(), travel_time);
			time = departure + travel_time;
		}
		else {
			const time_type travel_time = travel_time_provider.get_travel_time(*previous_node, action.get_node());
			plan_cost += evaluator.leg_cost(*previous_node, action.get_node(), travel_time);
			time += travel_time;
		}
		if(time > action.get_max_time()) {
			return std::nullopt;
		}
		if(!base) {
			candidate_arrivals[i] = time;
		}
		const bool drop_off = action.get_action_type() == Action_type::dropoff;
		if(drop_off) {
			plan_cost += evaluator.dropoff_cost(time, Cost_evaluator::ideal_dropoff_arrival<N>(action.get_action()));
		}
		const time_type service_start = std::max<time_type>(time, action.get_min_time());
		time = service_start + action.get_service_duration();
		departures[i] = time;
		if(drop_off && max_ride_time != 0) {
			const auto pickup = static_cast<plan_size_type>(action.get_other_action_data_index());
			const time_type pickup_departure = pickup >= from ? departures[pickup] : base_departures[pickup];
			if(service_start - pickup_departure > max_ride_time) {
				return std::nullopt;
			}
		}
		if(base) {
			base_prefix_costs[i] = plan_cost;
		}
		previous_node = &action.get_node();
	}

	if(previous_node == nullptr) {
		// empty plan
		simulated_departure = departure;
		simulated_end = departure;
		return cost_type{0};
	}
	time_type end = time;
	if(depot != nullptr) {
		const time_type travel_time = travel_time_provider.get_travel_time(*previous_node, *depot);
		plan_cost += evaluator.leg_cost(*previous_node, *depot, travel_time);
		end += travel_time;
	}
	const unsigned long max_route_duration = configuration.get_max_route_duration();
	if(max_route_duration != 0 && end - departure > max_route_duration) {
		return std::nullopt;
	}
	simulated_departure = departure;
	simulated_end = end;
	return plan_cost + evaluator.plan_constant() + evaluator.plan_duration_cost(departure, end);
}

template<typename N, class P>
void Worst_removal_operator<N, P>::apply(
	std::vector<P>& plans, size_t plan_index, const Candidate& candidate, std::vector<const Request<N>*>& removed
) {
	P& plan = plans[plan_index];
	prepare_plan(plan);
	const plan_size_type from = flag_candidate(plan, candidate.first, candidate.length);
	const auto candidate_cost = simulate(plan, from, false);
	assert(candidate_cost && "the candidate was feasible when evaluated");

	// new times of the remaining actions, the removed requests
	for(plan_size_type i = from; i < plan.get_length(); ++i) {
		ActionData<N>& action = plan[i];
		if(!removed_flags[i]) {
			action.set_arrival_time(candidate_arrivals[i]);
			action.set_departure_time(candidate_departures[i]);
		}
		else if(action.get_action_type() == Action_type::pickup) {
			removed.push_back(&static_cast<const Service_action<N>&>(action.get_action()).get_request());
		}
	}
	plan.set_departure_time(simulated_departure);
	plan.set_arrival_time(simulated_end);
	plan.set_cost(*candidate_cost);
	plan.remove_actions(removed_flags);
	clear_flags();

	cost += *candidate_cost - plan_costs[plan_index];
	plan_costs[plan_index] = *candidate_cost;
	served_request_count -= candidate.length;
	assert(costs_equal(
		*candidate_cost,
		plan.get_length() == 0 ? 0.0 : evaluator.evaluate<N>(plan, travel_time_provider, configuration).total));
	evaluate_plan(plans, plan_index);
}

template<typename N, class P>
unsigned Worst_removal_operator<N, P>::remove_while_improving(
	std::vector<P>& plans, std::vector<const Request<N>*>& removed, cost_type threshold
) {
	assert(candidates.size() >= plans.size() && "init() not called");
	unsigned removed_count = 0;
	while(true) {
		const Candidate* best = nullptr;
		size_t best_plan_index = 0;
		cost_type best_score = 0;
		const cost_type average = served_request_count == 0 ? 0 : cost / static_cast<cost_type>(served_request_count);
		// gains below this are rounding errors
		const cost_type epsilon = 1e-9 * std::max<cost_type>({1, average, threshold});
		for(size_t plan_index = 0; plan_index < plans.size(); ++plan_index) {
			for(const Candidate& candidate: candidates[plan_index]) {
				cost_type score;
				if(threshold > 0) {
					// gain over the threshold value of the removed requests
					score = candidate.saving - static_cast<cost_type>(candidate.length) * threshold;
				}
				else {
					if(candidate.length >= served_request_count) {
						continue;
					}
					// decrease of the average cost per served request
					score = (candidate.saving - static_cast<cost_type>(candidate.length) * average)
						/ static_cast<cost_type>(served_request_count - candidate.length);
				}
				if(score > epsilon && score > best_score) {
					best = &candidate;
					best_plan_index = plan_index;
					best_score = score;
				}
			}
		}
		if(best == nullptr) {
			return removed_count;
		}
		removed_count += best->length;
		const Candidate chosen = *best;
		apply(plans, best_plan_index, chosen, removed);
	}
}

template<typename N, class P>
unsigned Worst_removal_operator<N, P>::remove_worst(
	std::vector<P>& plans, unsigned request_count, std::vector<const Request<N>*>& removed
) {
	assert(candidates.size() >= plans.size() && "init() not called");
	unsigned removed_count = 0;
	while(removed_count < request_count) {
		const Candidate* best = nullptr;
		size_t best_plan_index = 0;
		cost_type best_saving = 0;
		for(size_t plan_index = 0; plan_index < plans.size(); ++plan_index) {
			for(const Candidate& candidate: candidates[plan_index]) {
				if(candidate.length > request_count - removed_count) {
					continue;
				}
				const cost_type saving = candidate.saving / static_cast<cost_type>(candidate.length);
				if(best == nullptr || saving > best_saving) {
					best = &candidate;
					best_plan_index = plan_index;
					best_saving = saving;
				}
			}
		}
		if(best == nullptr) {
			break;
		}
		removed_count += best->length;
		const Candidate chosen = *best;
		apply(plans, best_plan_index, chosen, removed);
	}
	return removed_count;
}

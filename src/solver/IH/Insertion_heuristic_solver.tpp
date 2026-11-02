//
// Created by Fido on 2020-03-30.
//

#include <map>
#include <vector>
#include <limits>
#include <optional>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <thread>

#include <spdlog/spdlog.h>
#include "../../ActionData.h"
#include "../../Adjustment_reason.h"
#include "../../progress_bar.h"


namespace insertion_heuristic_detail {
	inline plan_size_type validate_temporal_pruning_min_plan_length(const int temporal_pruning_min_plan_length) {
		if(temporal_pruning_min_plan_length < 0) {
			throw std::out_of_range("ih.temporal_pruning_min_plan_length must be non-negative");
		}

		if(
			static_cast<unsigned long long>(temporal_pruning_min_plan_length)
			> static_cast<unsigned long long>(std::numeric_limits<plan_size_type>::max())
		) {
			throw std::out_of_range("ih.temporal_pruning_min_plan_length does not fit plan_size_type");
		}

		return static_cast<plan_size_type>(temporal_pruning_min_plan_length);
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
Insertion_heuristic_solver<N, A, V, P>::Insertion_heuristic_solver(
	const DARP_instance<N>& instance,
	const DARP_benchmark_config& solver_config,
	[[maybe_unused]] const fs::path& out_dir_path,
	bool minimize_used_vehicles,
	std::shared_ptr<Nearest_vehicle_provider<N>> nearest_vehicle_provider
):
	DARP_benchmark_solver<N>(instance, solver_config),
	minimize_used_vehicles(minimize_used_vehicles),
	fleet_sizing(instance.get_problem() == problem_type::fleet_sizing),
	cost_evaluator(Cost_evaluator::from_configuration(*instance.get_darp_instance_configuration())),
	SVDARP_solver(DARP_context<N>(instance)),
	temporal_pruning_min_plan_length(
		insertion_heuristic_detail::validate_temporal_pruning_min_plan_length(
			solver_config.ih.temporal_pruning_min_plan_length
		)
	),
	max_parallel_vehicle_trials(solver_config.tmax > 1 ? static_cast<unsigned int>(solver_config.tmax) : 1u),
	nearest_vehicle_provider(nearest_vehicle_provider) {
	if (fleet_sizing) {
		if constexpr (!std::is_same_v<V, Vehicle<N>>) {
			throw std::runtime_error("The insertion heuristic creates vehicles in the fleet-sizing mode only with the Vehicle type");
		}
		if (instance.get_darp_instance_configuration()->get_vehicle_capacity() == 0) {
			throw std::runtime_error("The insertion heuristic in the fleet-sizing mode needs the vehicle capacity (vehicles.capacity)");
		}
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
std::span<const problem_type> Insertion_heuristic_solver<N, A, V, P>::supported_problem_types() const {
	static constexpr problem_type types[] = {problem_type::darp, problem_type::fleet_sizing};
	return types;
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::set_new_plan_penalty(std::optional<cost_type> penalty) {
	new_plan_penalty = penalty;
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::set_min_plan_revenue_repair(bool enabled) {
	min_plan_revenue_repair = enabled;
}


template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
DARP_benchmark_solver<N>::solution_impl_ret_val Insertion_heuristic_solver<N, A, V, P>::solve_impl() {
	if constexpr (std::is_same_v<A, ActionData<N>> && std::is_same_v<V, Vehicle<N>> &&
				  std::is_same_v<P, VehiclePlan<N>>) {
		return compute(this->darp_instance->get_requests(), this->darp_instance->get_vehicles());
	}
	else{
		return nullptr;
	}
}

//template <typename N>
//std::unique_ptr<Solution<N>> Insertion_heuristic_solver<N,ActionData<N>,Vehicle<N>,VehiclePlan<N>>::solve_impl() {
//	return compute(this->darp_instance->get_requests(), this->darp_instance->get_vehicles());
//}


template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
template<class R, Iterable<V> I>
std::unique_ptr<Solution<N>> Insertion_heuristic_solver<N, A, V, P>::compute(const R& requests, const I& vehicles) {
	reset();

	// we have to init unused vehicles collection, which is only used if we minimize the used vehicles
	if (minimize_used_vehicles) {
		unused_vehicles.reserve(vehicles.size());
		std::transform(
			vehicles.begin(), vehicles.end(), std::back_inserter(unused_vehicles),
			[](const Vehicle<N>& vehicle) -> const Vehicle<N>* { return &vehicle; }
		);
	}
		// we start with empty plans for all vehicles if we do not minimize the number of used vehicles
	else {
		constexpr unsigned short initial_capacity = 4;

		for (const V& vehicle: vehicles) {
			vehicle_plan_builders.emplace_back(
				vehicle, initial_capacity, vehicle.get_operation_start());
		}
	}


	constexpr unsigned int step = 100;
	const bool show_progress_bar = requests.size() > step * 10;

	std::unique_ptr<indicators::ProgressBar> bar;

	if (show_progress_bar) {
		bar = std::make_unique<indicators::ProgressBar>(
			indicators::option::BarWidth{70},
			indicators::option::PostfixText{"Processing Requests"},
			indicators::option::MaxProgress{requests.size() / step}
		);
	}

	unsigned int counter = 0;

	for (const Request<N>& request: requests) {
		process_request(request);
		assert(fleet_sizing || vehicle_plan_builders.size() <= vehicles.size());

		if (show_progress_bar && (counter % step) == 0) {
			bar->tick();
		}
		++counter;
	}

	if (min_plan_revenue_repair) {
		repair_short_plans();
	}

	// create vehicle plans from plan builders
	std::vector<VehiclePlan<N>> vehicle_plans;
	vehicle_plans.reserve(vehicle_plan_builders.size());
	for (auto& vehicle_plan_builder: vehicle_plan_builders) {

		// skip the empty plans
		if (vehicle_plan_builder.get_length() > 0) {
			SVDARP_solver.finalize_plan(vehicle_plan_builder);
			vehicle_plans.push_back(vehicle_plan_builder.to_vehicle_plan());
		}
	}

	if (fleet_sizing) {
		// The solution owns the vehicles of its plans; the plans are bound to them by the vehicle index. A vehicle was
		// created at the pickup of the request that opened its plan, but a later request may precede it, so the
		// vehicle starts at the first action of its plan when it arrives there (no first leg).
		std::vector<Vehicle<N>> plan_vehicles;
		plan_vehicles.reserve(vehicle_plans.size());
		for (auto& plan: vehicle_plans) {
			const Vehicle<N>& vehicle = plan.get_vehicle();
			plan_vehicles.emplace_back(
				vehicle.get_index(),
				plan.get_first_action().get_action().get_node_pointer(),
				vehicle.get_capacity(),
				vehicle.get_operation_start());
			plan.set_departure_time(plan.get_first_action().get_arrival_time());
		}
		return std::make_unique<Solution<N>>(
			std::move(vehicle_plans),
			this->solution_cost,
			std::move(dropped_requests),
			std::nullopt,
			std::optional<std::vector<Vehicle<N>>>{std::move(plan_vehicles)});
	}

	assert(vehicle_plans.size() <= vehicles.size());

	return std::make_unique<Solution<N>>(std::move(vehicle_plans), this->solution_cost, std::move(dropped_requests));
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
std::optional<Solution<N>> Insertion_heuristic_solver<N, A, V, P>::insert_request_in_solution(
	const Solution<N>& solution,
	const Request<N>& req
) {
	assert(solution.get_dropped_request_count() > 0);

	// reset the solver internals
	reset();

	// vehicles used in any plan
	std::unordered_set<unsigned> used_vehicles;

	// create plan builders from solution
	for (const VehiclePlan<N>& plan: solution.get_plans()) {
		vehicle_plan_builders.reserve(solution.get_plans().size());
		vehicle_plan_builders.emplace_back(plan);
		if(temporal_pruning_min_plan_length > 0) {
			vehicle_plan_builders.back().update_temporal_action_bounds(*this->travel_time_provider());
		}
		used_vehicles.insert(plan.get_vehicle().get_index());

		// also compute the cost of the existing plans
		this->solution_cost += plan.get_cost();
	}

	// init unused vehicles
	const auto& available_vehicles = this->darp_instance->get_vehicles();
	unused_vehicles.reserve(available_vehicles.size() - used_vehicles.size());
	for (const auto& vehicle: available_vehicles) {
		if (!used_vehicles.contains(vehicle.get_index())) {
			unused_vehicles.push_back(&vehicle);
		}
	}

	// insert request at best index_in_plan
	process_request(req);

	if (best_plan) {
		SVDARP_solver.finalize_plan(vehicle_plan_builders[best_vehicle_index]);

		// copy dropped requests from old solution
		dropped_requests.reserve(solution.get_dropped_request_count() - 1);
		for (const auto request: solution.get_dropped_requests()) {
			if (request != &req) {
				dropped_requests.push_back(request);
			}
		}

		// create new solution
		return export_solution();
	}

	return std::nullopt;
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
template<class R, Vehicle_plan_builder_action_with_request AR>
std::optional<P> Insertion_heuristic_solver<N, A, V, P>::insert_request_in_plan(const P& vehicle_plan, const R& req) {
	const auto& vehicle = vehicle_plan.get_vehicle();

	A pickup_action_data(req.get_pickup());
	A drop_off_action_data(req.get_dropoff());

	IH_vehicle_plan_builder<V, A, P> plan_builder(vehicle_plan);
	if(temporal_pruning_min_plan_length > 0) {
		plan_builder.update_temporal_action_bounds(*this->travel_time_provider());
	}

	// fail fast
	if (can_serve_request(vehicle, pickup_action_data, drop_off_action_data)) {
		SVDARP_solver.insert_request_into_plan_optimally(
			pickup_action_data,
			drop_off_action_data,
			plan_builder,
			std::numeric_limits<unsigned int>::max(),
			temporal_pruning_min_plan_length
		);

		if(plan_builder.get_length() > vehicle_plan.get_length()){
			SVDARP_solver.finalize_plan(plan_builder);
			return plan_builder.to_vehicle_plan();
		}
	}

	return std::nullopt;

//	// reset the solver internals
//	reset();
//
//	// vehicle plan index setup
//	current_vehicle_plan_index = 0;
//
//	// create plan builder
//    vehicle_plan_builders.emplace_back(vehicle_plan);
//
//	// per vehicle resetting
//	min_cost_increment = std::numeric_limits<cost_type>::max();
//    best_plan.reset();
//

//
//	// insert request at best index_in_plan
//	process_request_vehicle_combination(pickup_action_data, drop_off_action_data);
//
//    if(best_plan){
//    	SVDARP_solver.finalize_plan(best_plan.value());
//
//		// return the plan
//		return best_plan->to_vehicle_plan();
//    }
//

}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::reset() {
	this->solution_cost = 0;
	vehicle_plan_builders.clear();
	vehicle_plan_builders.reserve(this->darp_instance->get_vehicles().size());
	dropped_requests.clear();
	best_plan.reset();
	created_vehicles.clear();
	best_vehicle_index = std::numeric_limits<uint_fast32_t>::max();

	if (minimize_used_vehicles) {
		unused_vehicles.clear();
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::set_best_plan() {
//	SVDARP_solver.finalize_plan(best_plan.value());
	best_plan.value().erase_time_adjustments();
	this->solution_cost += min_cost_increment;
	vehicle_plan_builders[best_vehicle_index] = best_plan.value();
	if(temporal_pruning_min_plan_length > 0) {
		vehicle_plan_builders[best_vehicle_index].update_temporal_action_bounds(*this->travel_time_provider());
	}
	else {
		vehicle_plan_builders[best_vehicle_index].clear_temporal_action_bounds();
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::process_request(const Request<N>& request) {
	min_cost_increment = std::numeric_limits<cost_type>::max();
	best_plan.reset();
	best_vehicle_index = std::numeric_limits<uint_fast32_t>::max();

	A pickup_action_data(request.get_pickup());
	A drop_off_action_data(request.get_dropoff());

	// try to add request into all plans
	if(max_parallel_vehicle_trials > 1 && vehicle_plan_builders.size() > 1) {
		process_existing_vehicle_plans_parallel(pickup_action_data, drop_off_action_data);
	}
	else {
		process_existing_vehicle_plans_serial(pickup_action_data, drop_off_action_data);
	}

	// fleet sizing: a new vehicle if no plan can take the request, or if its plan is cheaper with the penalty
	if (fleet_sizing && open_new_vehicles && (!best_plan || new_plan_penalty)) {
		try_new_vehicle(pickup_action_data, drop_off_action_data);
	}

	if (best_plan) {
		set_best_plan();
		return;
	}

	// try to add the request into an empty vehicle
	if (minimize_used_vehicles && !unused_vehicles.empty()) {

		// gen the nearest unused vehicle
		const size_t nearest_vehicle_index = this->nearest_vehicle_provider->get_nearest_vehicle(
			request.get_pickup().get_node(), unused_vehicles
		);
		const auto& nearest_vehicle = *unused_vehicles[nearest_vehicle_index];
		vehicle_plan_builders.emplace_back(
			nearest_vehicle,
			static_cast<unsigned short>(6),
			nearest_vehicle.get_operation_start());
		unused_vehicles.erase(unused_vehicles.begin() + nearest_vehicle_index);

		if(auto insertion = evaluate_request_vehicle_combination(
			static_cast<uint_fast32_t>(vehicle_plan_builders.size() - 1),
			pickup_action_data,
			drop_off_action_data,
			min_cost_increment,
			best_plan
		)) {
			min_cost_increment = insertion->cost_increment;
			best_vehicle_index = insertion->vehicle_plan_index;
		}

		if (best_plan) {
			set_best_plan();
			return;
		}
	}

	dropped_requests.push_back(&request);
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
std::optional<typename Insertion_heuristic_solver<N, A, V, P>::Vehicle_insertion_candidate>
Insertion_heuristic_solver<N, A, V, P>::evaluate_request_vehicle_combination(
	uint_fast32_t vehicle_plan_index,
	A& pickup_action_data,
	A& drop_off_action_data,
	cost_type min_increment,
	std::optional<IH_vehicle_plan_builder<V, A, P>>& evaluated_plan
) {
	evaluated_plan = vehicle_plan_builders[vehicle_plan_index];

	const auto& vehicle = evaluated_plan->get_vehicle();

	// fail fast
	if (can_serve_request(vehicle, pickup_action_data, drop_off_action_data)) {
		const cost_type new_min_cost_increment = SVDARP_solver.insert_request_into_plan_optimally(
			pickup_action_data,
			drop_off_action_data,
			*evaluated_plan,
			min_increment,
			temporal_pruning_min_plan_length
		);
		if (min_increment > new_min_cost_increment) {
			return Vehicle_insertion_candidate{
				vehicle_plan_index,
				new_min_cost_increment
			};
		}
	}

	return std::nullopt;
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::process_existing_vehicle_plans_serial(
	A& pickup_action_data,
	A& drop_off_action_data
) {
	std::optional<IH_vehicle_plan_builder<V, A, P>> evaluated_plan;

	for (current_vehicle_plan_index = 0; current_vehicle_plan_index < vehicle_plan_builders.size();
		 current_vehicle_plan_index++
		) {
		if(auto insertion = evaluate_request_vehicle_combination(
			current_vehicle_plan_index,
			pickup_action_data,
			drop_off_action_data,
			min_cost_increment,
			evaluated_plan
		)) {
			if(insertion->cost_increment < min_cost_increment) {
				min_cost_increment = insertion->cost_increment;
				best_plan = std::move(evaluated_plan);
				best_vehicle_index = insertion->vehicle_plan_index;
			}
		}
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::process_existing_vehicle_plans_parallel(
	A& pickup_action_data,
	A& drop_off_action_data
) {
	const auto plan_count = static_cast<uint_fast32_t>(vehicle_plan_builders.size());
	const auto worker_count = std::min<uint_fast32_t>(
		plan_count,
		static_cast<uint_fast32_t>(max_parallel_vehicle_trials)
	);
	const auto block_size = (plan_count + worker_count - 1) / worker_count;

	std::vector<std::optional<Vehicle_insertion_candidate>> worker_best(worker_count);
	std::vector<std::optional<IH_vehicle_plan_builder<V, A, P>>> worker_best_plans(worker_count);

	{
		std::vector<std::jthread> workers;
		workers.reserve(worker_count);
		for(uint_fast32_t worker_index = 0; worker_index < worker_count; ++worker_index) {
			workers.emplace_back([&, worker_index]() {
				const uint_fast32_t first_plan_index = worker_index * block_size;
				const uint_fast32_t last_plan_index = std::min<uint_fast32_t>(
					plan_count,
					first_plan_index + block_size
				);

				cost_type local_min_cost_increment = std::numeric_limits<cost_type>::max();
				std::optional<Vehicle_insertion_candidate> local_best;
				std::optional<IH_vehicle_plan_builder<V, A, P>> evaluated_plan;
				std::optional<IH_vehicle_plan_builder<V, A, P>> local_best_plan;

				for(uint_fast32_t vehicle_plan_index = first_plan_index;
					vehicle_plan_index < last_plan_index;
					++vehicle_plan_index
				) {
					if(auto insertion = evaluate_request_vehicle_combination(
						vehicle_plan_index,
						pickup_action_data,
						drop_off_action_data,
						local_min_cost_increment,
						evaluated_plan
					)) {
						if(insertion->cost_increment < local_min_cost_increment) {
							local_min_cost_increment = insertion->cost_increment;
							local_best = std::move(insertion);
							local_best_plan = std::move(evaluated_plan);
						}
					}
				}

				worker_best[worker_index] = std::move(local_best);
				worker_best_plans[worker_index] = std::move(local_best_plan);
			});
		}
	}

	for(uint_fast32_t worker_index = 0; worker_index < worker_count; ++worker_index) {
		auto& insertion = worker_best[worker_index];
		if(insertion && insertion->cost_increment < min_cost_increment) {
			min_cost_increment = insertion->cost_increment;
			best_plan = std::move(worker_best_plans[worker_index]);
			best_vehicle_index = insertion->vehicle_plan_index;
		}
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
bool Insertion_heuristic_solver<N, A, V, P>::can_serve_request(
	const V& vehicle,
	const A& pickup_action_data,
	const A& drop_off_action_data
) {
	const auto vehicle_start_time = vehicle.get_operation_start();

	// node identity - vehicle still needs to wait until operation_start
	if (nodes_equal(vehicle.get_init_position(), pickup_action_data.get_node())){
		return vehicle_start_time < pickup_action_data.get_max_time();
	}

	const auto travel_time_to_pickup = this->travel_time_provider().get()->get_travel_time(
		vehicle.get_init_position(), pickup_action_data.get_node());

	// pickup feasibility check: earliest arrival = vehicle_start_time + travel_time
	const auto earliest_pickup_arrival = vehicle_start_time + travel_time_to_pickup;
	const bool can_serve = earliest_pickup_arrival < pickup_action_data.get_max_time();

	if (can_serve) {
		const auto travel_time_to_dropoff = this->travel_time_provider().get()->get_travel_time(
			vehicle.get_init_position(), drop_off_action_data.get_node());
		const auto earliest_dropoff_arrival = vehicle_start_time + travel_time_to_dropoff + pickup_action_data.get_service_duration();

		// drop off feasibility check
		return earliest_dropoff_arrival < drop_off_action_data.get_max_time();
	}
	return false;
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
Solution<N> Insertion_heuristic_solver<N, A, V, P>::export_solution() {
	std::vector<VehiclePlan<N>> vehicle_plans;
	vehicle_plans.reserve(vehicle_plan_builders.size());
	for (const auto& vehicle_plan_builder: vehicle_plan_builders) {
		vehicle_plans.push_back(vehicle_plan_builder.to_vehicle_plan());
	}

	return {std::move(vehicle_plans), this->solution_cost, std::move(dropped_requests)};
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::try_new_vehicle(A& pickup_action_data, A& drop_off_action_data) {
	if constexpr (std::is_same_v<V, Vehicle<N>>) {
		const auto& configuration = *this->darp_instance_configuration();
		created_vehicles.emplace_back(
			static_cast<unsigned int>(created_vehicles.size()),
			pickup_action_data.get_action().get_node_pointer(),
			configuration.get_vehicle_capacity(),
			configuration.get_start_time());
		constexpr unsigned short initial_capacity = 4;
		vehicle_plan_builders.emplace_back(
			created_vehicles.back(), initial_capacity, created_vehicles.back().get_operation_start());
		const auto new_vehicle_plan_index = static_cast<uint_fast32_t>(vehicle_plan_builders.size() - 1);

		std::optional<IH_vehicle_plan_builder<V, A, P>> new_plan;
		const auto insertion = evaluate_request_vehicle_combination(
			new_vehicle_plan_index,
			pickup_action_data,
			drop_off_action_data,
			std::numeric_limits<cost_type>::max(),
			new_plan
		);

		bool use_new_vehicle = insertion.has_value();
		if (use_new_vehicle && best_plan) {
			const cost_type insertion_cost_increase
				= weighted_cost(*best_plan) - weighted_cost(vehicle_plan_builders[best_vehicle_index]);
			use_new_vehicle = weighted_cost(*new_plan) + *new_plan_penalty < insertion_cost_increase;
		}

		if (use_new_vehicle) {
			min_cost_increment = insertion->cost_increment;
			best_plan = std::move(new_plan);
			best_vehicle_index = new_vehicle_plan_index;
		}
		else {
			vehicle_plan_builders.pop_back();
			created_vehicles.pop_back();
		}
	}
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
cost_type Insertion_heuristic_solver<N, A, V, P>::weighted_cost(const IH_vehicle_plan_builder<V, A, P>& plan) const {
	if (plan.get_length() == 0) {
		return 0;
	}
	IH_vehicle_plan_builder<V, A, P> finalized_plan = plan;
	SVDARP_solver.finalize_plan(finalized_plan);
	return cost_evaluator.template evaluate<N>(
		finalized_plan, *this->travel_time_provider(), *this->darp_instance_configuration()).total;
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
double Insertion_heuristic_solver<N, A, V, P>::plan_revenue(const IH_vehicle_plan_builder<V, A, P>& plan) const {
	unsigned int drop_off_count = 0;
	for (size_t index = 0; index < plan.get_length(); ++index) {
		if (plan[index].is_drop_off()) {
			++drop_off_count;
		}
	}
	return drop_off_count * this->darp_instance_configuration()->get_request_revenue();
}

template<typename N, Vehicle_plan_builder_action A, IH_vehicle V, IH_vehicle_plan<V, A> P>
void Insertion_heuristic_solver<N, A, V, P>::repair_short_plans() {
	const double min_plan_revenue = this->darp_instance_configuration()->get_min_plan_revenue();
	if (min_plan_revenue <= 0) {
		return;
	}

	// vehicles of the short plans whose dissolution failed
	std::unordered_set<unsigned int> kept_vehicles;
	unsigned int dissolved_count = 0;
	open_new_vehicles = false;
	while (true) {
		std::optional<size_t> shortest_plan_index;
		for (size_t index = 0; index < vehicle_plan_builders.size(); ++index) {
			const auto& plan = vehicle_plan_builders[index];
			if (plan.get_length() > 0
				&& plan_revenue(plan) < min_plan_revenue
				&& !kept_vehicles.contains(plan.get_vehicle().get_index())
				&& (!shortest_plan_index || plan_revenue(plan) < plan_revenue(vehicle_plan_builders[*shortest_plan_index]))
			) {
				shortest_plan_index = index;
			}
		}
		if (!shortest_plan_index) {
			break;
		}

		const auto& short_plan = vehicle_plan_builders[*shortest_plan_index];
		const unsigned int vehicle_index = short_plan.get_vehicle().get_index();
		// the requests of the plan in the order of their pickups
		std::vector<const Request<N>*> plan_requests;
		for (size_t index = 0; index < short_plan.get_length(); ++index) {
			const auto& action = short_plan[index].get_action();
			if (action.get_action_type() == Action_type::pickup) {
				plan_requests.push_back(&static_cast<const Service_action<N>&>(action).get_request());
			}
		}

		const auto plans_before = vehicle_plan_builders;
		const size_t dropped_count_before = dropped_requests.size();
		const cost_type solution_cost_before = this->solution_cost;
		this->solution_cost -= short_plan.get_cost();
		vehicle_plan_builders.erase(vehicle_plan_builders.begin() + static_cast<std::ptrdiff_t>(*shortest_plan_index));
		for (const Request<N>* request: plan_requests) {
			process_request(*request);
		}

		if (dropped_requests.size() > dropped_count_before) {
			vehicle_plan_builders = plans_before;
			dropped_requests.resize(dropped_count_before);
			this->solution_cost = solution_cost_before;
			kept_vehicles.insert(vehicle_index);
		}
		else {
			++dissolved_count;
		}
	}
	open_new_vehicles = true;

	spdlog::info(
		"Minimum plan revenue {}: {} short plans dissolved, {} short plans kept (their requests fit no other plan)",
		min_plan_revenue, dissolved_count, kept_vehicles.size());
}

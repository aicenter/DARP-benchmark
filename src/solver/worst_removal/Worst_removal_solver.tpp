#pragma once

#include <spdlog/spdlog.h>

template<typename N>
Worst_removal_solver<N>::Worst_removal_solver(
	const DARP_instance<N>& instance, const DARP_benchmark_config& solver_config, const fs::path& out_dir_path
):
	DARP_benchmark_solver<N>(instance, solver_config),
	ih_solver(instance, solver_config, out_dir_path),
	initial_solution_path(solver_config.worst_removal.initial_solution),
	drop_threshold(solver_config.worst_removal.drop_threshold),
	stop_groups(solver_config.worst_removal.stop_groups) {
	if(drop_threshold < 0) {
		throw std::runtime_error(fmt::format("worst_removal.drop_threshold must not be negative, got {}", drop_threshold));
	}
}

template<typename N>
typename DARP_benchmark_solver<N>::solution_impl_ret_val Worst_removal_solver<N>::solve_impl() {
	std::vector<VehiclePlan<N>> plans;
	std::vector<const Request<N>*> dropped_requests;
	if(initial_solution_path.empty()) {
		spdlog::info("Worst removal: computing the initial solution by the insertion heuristic");
		const auto initial_solution = ih_solver.compute(
			this->darp_instance->get_requests(), this->darp_instance->get_vehicles());
		plans = initial_solution->get_plans();
		dropped_requests = initial_solution->get_dropped_requests();
	}
	else {
		spdlog::info("Worst removal: loading the initial solution from {}", initial_solution_path.string());
		const auto initial_solution = deserialize_json(initial_solution_path, *this->darp_instance);
		plans = initial_solution.get_plans();
		dropped_requests = initial_solution.get_dropped_requests();
	}

	Worst_removal_operator<N> worst_removal{this->context, stop_groups};
	worst_removal.init(plans);
	const cost_type initial_cost = worst_removal.get_cost();
	const size_t initial_served_count = worst_removal.get_served_request_count();
	dropped_requests.reserve(dropped_requests.size() + initial_served_count);

	const unsigned removed_count = worst_removal.remove_while_improving(plans, dropped_requests, drop_threshold);

	const size_t served_count = worst_removal.get_served_request_count();
	spdlog::info(
		"Worst removal: {} requests removed, cost per served request {:.4f} -> {:.4f} ({} -> {} served)",
		removed_count,
		initial_served_count == 0 ? 0.0 : initial_cost / static_cast<cost_type>(initial_served_count),
		served_count == 0 ? 0.0 : worst_removal.get_cost() / static_cast<cost_type>(served_count),
		initial_served_count, served_count);

	std::erase_if(plans, [](const VehiclePlan<N>& plan) { return plan.get_length() == 0; });
	// the plans removed from keep their costs, the other plans the costs of the initial solution; the framework
	// evaluates the reported cost of the final plans
	cost_type plans_cost = 0;
	for(const auto& plan: plans) {
		plans_cost += plan.get_cost();
	}
	return std::make_unique<Solution<N>>(std::move(plans), plans_cost, std::move(dropped_requests));
}

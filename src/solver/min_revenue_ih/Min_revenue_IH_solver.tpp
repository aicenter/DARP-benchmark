#pragma once

#include <tuple>

#include <spdlog/spdlog.h>

template<typename N>
Min_revenue_IH_solver<N>::Min_revenue_IH_solver(
	const DARP_instance<N>& instance, const DARP_benchmark_config& solver_config, const fs::path& out_dir_path
):
	DARP_benchmark_solver<N>(instance, solver_config),
	ih_solver(instance, solver_config, out_dir_path),
	new_plan_penalty_step(solver_config.min_revenue_ih.new_plan_penalty_step),
	new_plan_penalty_count(static_cast<unsigned int>(solver_config.min_revenue_ih.new_plan_penalty_count)) {
	if(new_plan_penalty_step < 0) {
		throw std::runtime_error(fmt::format(
			"min_revenue_ih.new_plan_penalty_step must not be negative, got {}", new_plan_penalty_step));
	}
	if(solver_config.min_revenue_ih.new_plan_penalty_count < 1) {
		throw std::runtime_error(fmt::format(
			"min_revenue_ih.new_plan_penalty_count must be at least 1, got {}",
			solver_config.min_revenue_ih.new_plan_penalty_count));
	}
	ih_solver.set_min_plan_revenue_repair(true);
}

template<typename N>
std::span<const problem_type> Min_revenue_IH_solver<N>::supported_problem_types() const {
	static constexpr problem_type types[] = {problem_type::fleet_sizing};
	return types;
}

template<typename N>
unsigned int Min_revenue_IH_solver<N>::count_short_plans(const Solution<N>& solution) const {
	const auto& configuration = *this->darp_instance_configuration();
	unsigned int short_plan_count = 0;
	for(const auto& plan: solution.get_plans()) {
		unsigned int drop_off_count = 0;
		for(const auto& action_data: plan) {
			if(action_data.is_drop_off()) {
				++drop_off_count;
			}
		}
		if(plan.get_length() > 0 && drop_off_count * configuration.get_request_revenue() < configuration.get_min_plan_revenue()) {
			++short_plan_count;
		}
	}
	return short_plan_count;
}

template<typename N>
typename DARP_benchmark_solver<N>::solution_impl_ret_val Min_revenue_IH_solver<N>::solve_impl() {
	std::unique_ptr<Solution<N>> best_solution;
	std::tuple<unsigned int, unsigned int, cost_type> best_key;
	cost_type best_penalty = 0;
	for(unsigned int penalty_index = 0; penalty_index < new_plan_penalty_count; ++penalty_index) {
		const cost_type penalty = penalty_index * new_plan_penalty_step;
		ih_solver.set_new_plan_penalty(penalty);
		auto solution = ih_solver.compute(this->darp_instance->get_requests(), this->darp_instance->get_vehicles());
		solution->evaluate_costs(*this->darp_instance);
		const std::tuple<unsigned int, unsigned int, cost_type> key{
			count_short_plans(*solution), solution->get_dropped_request_count(), solution->get_cost()};
		spdlog::info(
			"Minimum revenue IH, new plan penalty {}: {} plans, {} below the minimum revenue, {} dropped, cost {:.2f}",
			penalty, solution->get_plan_count(), std::get<0>(key), std::get<1>(key), std::get<2>(key));
		if(!best_solution || key < best_key) {
			best_solution = std::move(solution);
			best_key = key;
			best_penalty = penalty;
		}
	}

	spdlog::info("Minimum revenue IH: the solution of the new plan penalty {} is kept", best_penalty);
	if(std::get<0>(best_key) > 0) {
		spdlog::error(
			"Minimum revenue IH: {} plans are below the minimum revenue {}, their requests fit no other plan",
			std::get<0>(best_key), this->darp_instance_configuration()->get_min_plan_revenue());
	}
	return best_solution;
}

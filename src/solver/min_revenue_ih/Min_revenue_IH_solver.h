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

#include <filesystem>
#include <span>

#include "../DARP_benchmark_solver.h"
#include "../IH/Insertion_heuristic_solver.h"

namespace fs = std::filesystem;

/**
 * @brief Fleet-sizing solver keeping the minimum plan revenue of the instance (`min_plan_revenue`): every non-empty
 * plan serves requests worth at least the minimum (`request_revenue` each).
 *
 * For each opening penalty 0, s, ..., (n - 1) s (s = `min_revenue_ih.new_plan_penalty_step`,
 * n = `min_revenue_ih.new_plan_penalty_count`), the insertion heuristic builds the plans, creating a new vehicle when
 * no plan can take a request or when the new plan plus the penalty is cheaper than the best insertion (weighted cost),
 * and then dissolves the plans below the minimum, reinserting their requests into the other plans (see
 * Insertion_heuristic_solver::repair_short_plans). The solution with the fewest plans below the minimum, then the
 * fewest dropped requests, then the lowest cost is kept; plans below the minimum remain only if their requests fit
 * no other plan, and they are reported.
 */
template<typename N>
class Min_revenue_IH_solver: public DARP_benchmark_solver<N> {
public:
	Min_revenue_IH_solver(
		const DARP_instance<N>& instance, const DARP_benchmark_config& solver_config, const fs::path& out_dir_path);

	typename DARP_benchmark_solver<N>::solution_impl_ret_val solve_impl() override;

	[[nodiscard]] std::span<const problem_type> supported_problem_types() const override;

	[[nodiscard]] bool supports_min_plan_revenue() const override {
		return true;
	}

private:
	Insertion_heuristic_solver<N> ih_solver;
	const cost_type new_plan_penalty_step;
	const unsigned int new_plan_penalty_count;

	[[nodiscard]] unsigned int count_short_plans(const Solution<N>& solution) const;
};

#include "Min_revenue_IH_solver.tpp"

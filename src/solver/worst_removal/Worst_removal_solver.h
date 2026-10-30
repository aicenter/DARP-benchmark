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

#include "../DARP_benchmark_solver.h"
#include "../IH/Insertion_heuristic_solver.h"
#include "../operators/Worst_removal_operator.h"

namespace fs = std::filesystem;

/**
 * @brief Solver that drops the requests that do not pay off: from an initial solution, it removes requests (and
 * stops) with the Worst_removal_operator operator while the average cost per served request decreases or, with a positive
 * drop threshold, while the saving per removed request exceeds the threshold.
 *
 * The initial solution is loaded from the file of the `worst_removal.initial_solution` option, or computed by the
 * insertion heuristic when the option is empty. The requests dropped by the initial solution stay dropped.
 *
 * @tparam N node type
 */
template<typename N>
class Worst_removal_solver: public DARP_benchmark_solver<N> {
public:
	Worst_removal_solver(
		const DARP_instance<N>& instance, const DARP_benchmark_config& solver_config, const fs::path& out_dir_path);

	typename DARP_benchmark_solver<N>::solution_impl_ret_val solve_impl() override;

private:
	Insertion_heuristic_solver<N> ih_solver;
	const fs::path initial_solution_path;
	const cost_type drop_threshold;
	const bool stop_groups;
};

#include "Worst_removal_solver.tpp"

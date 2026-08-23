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

#include <random>
#include <optional>

#include "Insertion_heuristic_solver.h"
#include "../DARP_instance.h"

template <typename N>
class Iterative_IH_Solver: public Insertion_heuristic_solver<N> {
public:
	Iterative_IH_Solver(
		const DARP_instance<N>& instance,
		const DARP_benchmark_config& solver_config,
		unsigned short iteration_count = 10000
	);

	[[nodiscard]] Solution<N> solve_randomized_iterations();

private:
    const unsigned short iteration_count;

    std::default_random_engine random_engine{};

    std::optional<Solution<N>> best_solution{std::nullopt};
};



#include "Iterative_IH_Solver.tpp"


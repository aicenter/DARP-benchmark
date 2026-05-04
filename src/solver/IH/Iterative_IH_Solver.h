//
// Created by Fido on 2020-05-14.
//

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


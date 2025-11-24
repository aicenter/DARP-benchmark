//
// Created by Fido on 2020-05-14.
//

#pragma once

#include <random>

#include "Insertion_heuristic_solver.h"
#include "../DARP_instance.h"

template <typename N>
class Iterative_IH_Solver: public Insertion_heuristic_solver<N> {
public:
    Solution<N> solve(std::shared_ptr<DARP_instance<N>> instance) override;

    Iterative_IH_Solver(
		const std::shared_ptr<Travel_time_provider<N>>& travel_time_provider_par,
		const std::shared_ptr<DARP_instance_configuration>& darp_instance_configuration_par,
		unsigned short iteration_count=10000
	);

private:
    const unsigned short iteration_count;

    std::default_random_engine random_engine{};

    std::optional<Solution<N>> best_solution{nullopt};
};



#include "Iterative_IH_Solver.tpp"



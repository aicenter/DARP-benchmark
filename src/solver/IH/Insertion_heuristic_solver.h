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

#include <map>
#include <vector>
#include <limits>
#include <optional>
#include <cstdint>
#include <filesystem>

#include "../DARP_benchmark_solver.h"

#include "../../DARP_instance.h"
#include "IH_vehicle_plan_builder.h"
#include "SVDARP.h"
#include "../../Iterable.h"
#include "../../config/DARP-benchmark_config.h"

namespace fs = std::filesystem;


/**
 * @brief Concept for vehicle types used in the Insertion Heuristic solver.
 * Requires methods for accessing vehicle properties needed during insertion.
 */
template<class V>
concept IH_vehicle = requires(const V& vehicle) {
	{ vehicle.get_init_position() };
	{ vehicle.get_operation_start() } -> std::convertible_to<time_type>;
	{ vehicle.get_index() } -> std::convertible_to<unsigned int>;
	{ vehicle.get_capacity() } -> std::convertible_to<unsigned short>;
};

template<class P, class V, class A>
concept IH_vehicle_plan =
	Vehicle_plan_builder_plan<P, V, A> &&
	std::is_move_constructible_v<P>;

template <
    typename N,
	Vehicle_plan_builder_action A = ActionData<N>,
	IH_vehicle V = Vehicle<N>,
	IH_vehicle_plan<V, A> P = VehiclePlan<N>
>
class Insertion_heuristic_solver: public DARP_benchmark_solver<N> {
public:

    // using DARP_benchmark_solver<N>::DARP_benchmark_solver;
    /**
     * Main constructor that uses the parameters defined by the DARP_benchmark_solver_constructor_interface contract
     * @param instance
     * @param solver_config
     * @param out_dir_path
     * @param minimize_used_vehicles
     * @param nearest_vehicle_provider
     */
    Insertion_heuristic_solver(
		const DARP_instance<N>& instance,
		const DARP_benchmark_config& solver_config,
		const fs::path& out_dir_path,
		bool minimize_used_vehicles = false,
		std::shared_ptr<Nearest_vehicle_provider<N>> nearest_vehicle_provider = nullptr
	);

	DARP_benchmark_solver<N>::solution_impl_ret_val solve_impl() override;

	template<class R, Iterable<V> I>
    std::unique_ptr<Solution<N>> compute(const R& requests, const I& vehicles);

	/**
	 * @brief Insert request on the best place in an existing solution. It means that all positions in all plans are
	 * considered for pickup/drop off actions of the new request and the positions with the smallest cost increment are
	 * chosen.
	 * @param solution current solution
	 * @param req new request
	 * @return new solution with the new request inserted in the best position or nullopt if the request cannot be
	 * inserted into any plan.
	*/
	std::optional<Solution<N>> insert_request_in_solution(const Solution<N>& solution, const Request<N>& req);

	/**
	 * @brief Insert request on the best place in an existing vehicle plan. This function does not rely on the
	 * instance member, nor it use the solver internal, it is just a wrapper for the SVDARP call.
	 * @param vehicle_plan
	 * @param req
	 * @return Vehicle plan with the new request inserted in the best position or nullopt if the request cannot be
	 * inserted into the plan.
	 */
	template<class R, Vehicle_plan_builder_action_with_request AR = A>
    std::optional<P> insert_request_in_plan(const P& vehicle_plan, const R& req);

protected:


private:

	const bool minimize_used_vehicles{false};

    std::vector<IH_vehicle_plan_builder<V, A, P>> vehicle_plan_builders;

    unsigned int min_cost_increment{0};

	std::optional<IH_vehicle_plan_builder<V, A, P>> best_plan{std::nullopt};

	const SVDARP<N, V, A, P> SVDARP_solver;

	const plan_size_type temporal_pruning_min_plan_length;

	const unsigned int max_parallel_vehicle_trials;

    uint_fast32_t best_vehicle_index{std::numeric_limits<uint_fast32_t>::max()};

    uint_fast32_t current_vehicle_plan_index{0};
		
    std::vector<const Request<N>*> dropped_requests{};

	std::shared_ptr<Nearest_vehicle_provider<N>> nearest_vehicle_provider;

	//std::vector<bool> used_vehicles;
	std::vector<const Vehicle<N>*> unused_vehicles;


	void reset();
	void set_best_plan();

    void process_request(const Request<N>& request);

	struct Vehicle_insertion_candidate {
		uint_fast32_t vehicle_plan_index;
		unsigned int cost_increment;
	};

	std::optional<Vehicle_insertion_candidate> evaluate_request_vehicle_combination(
		uint_fast32_t vehicle_plan_index,
        A& pickup_action_data,
		A& drop_off_action_data,
		unsigned int min_increment,
		std::optional<IH_vehicle_plan_builder<V, A, P>>& evaluated_plan
	);

	void process_existing_vehicle_plans_serial(A& pickup_action_data, A& drop_off_action_data);

	void process_existing_vehicle_plans_parallel(A& pickup_action_data, A& drop_off_action_data);

    bool can_serve_request(
        const V& vehicle,
        const A& pickup_action_data,
		const A& drop_off_action_data
	);

	Solution<N> export_solution();

};



#include "Insertion_heuristic_solver.tpp"

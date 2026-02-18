//
// Created by Fido on 2020-03-30.
//

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



template<class P, class V, class A>
concept IH_vehicle_plan =
	Vehicle_plan_builder_plan<P, V, A> &&
	std::is_move_constructible_v<P>;

template <
    typename N,
	Vehicle_plan_builder_action A = ActionData<N>,
	class V = Vehicle<N>,
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

	Insertion_heuristic_solver(
		const std::shared_ptr<Travel_time_provider<N>>& travel_time_provider,
		const std::shared_ptr<DARP_instance_configuration>& instance_configuration,
		const DARP_benchmark_config& solver_config
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

    uint_fast32_t best_vehicle_index{std::numeric_limits<uint_fast32_t>::max()};

    uint_fast32_t current_vehicle_plan_index{0};
		
    std::vector<const Request<N>*> dropped_requests{};

	std::shared_ptr<Nearest_vehicle_provider<N>> nearest_vehicle_provider;

	//std::vector<bool> used_vehicles;
	std::vector<const Vehicle<N>*> unused_vehicles;


	void reset();
	void set_best_plan();

    void process_request(const Request<N>& request);

    void process_request_vehicle_combination(
        A& pickup_action_data,
		A& drop_off_action_data
	);

    bool can_serve_request(
        const V& vehicle,
        const A& pickup_action_data,
		const A& drop_off_action_data
	);

	Solution<N> export_solution();

};



#include "Insertion_heuristic_solver.tpp"

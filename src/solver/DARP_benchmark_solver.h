//
// Created by Fido on 2020-06-10.
//

#pragma once

#include <queue>
#include <optional>
#include <filesystem>

#include "../Solution.h"
#include "../DARP_instance.h"
#include "DARP_solver.h"
#include "../config/DARP-benchmark_config.h"

namespace fs = std::filesystem;


// class DARP_benchmark_solver_interface_node_agnostic {
// public:
// 	virtual ~DARP_benchmark_solver_interface_node_agnostic() = default;
//
// 	virtual std::unique_ptr<Solution_interface<N>>
// 	solve_and_get_final_result(const DARP_instance<N>& instance) = 0;
//
// 	virtual void export_performance(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
// 		const std::string message = "Solver has no performance stats";
// 		writer.String(message.c_str());
// 	}
// };

/**
 * @brief Type erasure interface for DARP benchmark solvers. It can be used to solve the DARP benchmark instances
 * without knowing the specific plan type used.
 * @tparam N
 */
template<typename N>
class DARP_benchmark_solver_interface
// public DARP_benchmark_solver_interface_node_agnostic
{
	public:
	virtual ~DARP_benchmark_solver_interface() = default;

	virtual std::unique_ptr<Solution_interface<N>>
	solve_and_get_final_result(const DARP_instance<N>& instance) = 0;

	virtual void export_performance(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
		const std::string message = "Solver has no performance stats";
		writer.String(message.c_str());
	}
};

template<template<typename, class...> class S, typename N>
concept DARP_benchmark_solver_constructor_interface =
	std::derived_from<S<N>,DARP_benchmark_solver_interface<N>>
	&& requires(const DARP_instance<N>& darp_instance, const DARP_benchmark_config& config, const fs::path& out_dir_path){
		{S(darp_instance, config, out_dir_path)} -> std::same_as<S<N>>;
	};

/**
 * @brief Base class for DARP benchmark solvers. It can solve the DARP like the DARP_solver, but additionally it can
 * also be called from the main benchmark executable, i.e. it can be used as the root solver to solve the
 * DARP benchmark instances.
 *
 * The solver should be configured by the DARP_instance_configuration object supplied to the constructor. For each new
 * configuration, a new solver should be created. The DARP instance to be solved is supplied to the solve method.
 * Because of that, multiple DARP instances can be solved by a single solver instance. Moreover, some helper methods
 * can be provided by a solver, which do not require the DARP instance to be supplied.
 * @tparam N
 * @tparam P
 */
template<typename N, class P = VehiclePlan<N>>
class DARP_benchmark_solver : public DARP_solver<N>, public DARP_benchmark_solver_interface<N> {
public:

	using solution_impl_ret_val = std::unique_ptr<Solution<N, P>>;


	using DARP_solver<N>::DARP_solver;

	DARP_benchmark_solver(const DARP_instance<N>& instance);


	void set_darp_instance(const DARP_instance<N>* darp_instance_par) {
		darp_instance = darp_instance_par;
	}


	/**
	 * @brief Solves the supplied DARP instance. It assigns the instance to the solver and calls the solve_impl method.
	 * @param instance
	 * @return
	 */
	std::unique_ptr<Solution<N, P>> solve(const DARP_instance<N>& instance) requires(Benchmark_plan<P>);

	std::unique_ptr<Solution_interface<N>> solve_and_get_final_result(const DARP_instance<N>& instance) override;

	template<class CheckPlan=P>
	void check_plans(const std::vector<CheckPlan>& plans);

protected:
	enum class Adjustment_reason {
		max_ride_time, max_route_time
	};

	const DARP_instance<N>* darp_instance{nullptr};

	unsigned short max_delay_time{0};

	unsigned long solution_cost{0};

	/**
	 * This method should be implemented by each specific solver class. It should be the main solver method.
	 * @return
	 */
	virtual solution_impl_ret_val solve_impl() = 0;

	/**
	 * Computes the optimal plan for a request-vehicle combination.
	 * @param current_plan The current plan of the vehicle.
	 * @param request The request being added.
	 * @return New plan for vehicle or std::nullopt if there is no feasible plan that can be created by inserting the
	 * new request into the current vehicle plan.
     * result in an infeasible plan.
	 */
	std::optional<VehiclePlan<N>> compute_optimal_plan(
		const VehiclePlan<N>& current_plan,
		const Request<N>& request
	);


	/**
	 * Computes new plan from the current plan by inserting the pickup request and the drop off request on the specified
	 * indexes.
	 * @param current_plan The current plan of the vehicle.
	 * @param pickup_option_index Index of the pick up action in the new plan. It can range from 0 to
	*  current_plan.size().
	 * @param drop_off_option_index Index of the drop off action in the new plan. It can range from 1 to
	*  current_plan.size() + 1.
	 * @param request New request to add into plan.
	 * @return New plan for vehicle or std::nullopt if the inserting the requests' actions at specified indexes would
	 * result in an infeasible plan.
	 */
	std::optional<P> insert_into_plan(
		const P& current_plan,
		unsigned short pickup_option_index,
		unsigned short drop_off_option_index,
		const Request<N>& request
	);

	bool adjust_times(VehiclePlan<N>& vehicle_plan, Adjustment_reason initial_reason, N station_position);
};


#include "DARP_benchmark_solver.tpp"




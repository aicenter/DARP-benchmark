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
#include <optional>
#include <queue>
#include <span>

#include "../DARP_instance.h"
#include "../Solution.h"
#include "DARP_context.h"
#include "../config/DARP-benchmark_config.h"

namespace fs = std::filesystem;


/**
 * @brief Type erasure interface for DARP benchmark solvers. It can be used to solve the DARP benchmark instances
 * without knowing the specific plan type used.
 * @tparam N
 */
template<typename N>
class DARP_benchmark_solver_interface
{
	public:
	virtual ~DARP_benchmark_solver_interface() = default;

	/**
	 * @brief Runs the solver on the instance bound when the concrete solver was constructed.
	 */
	virtual std::unique_ptr<Solution_interface<N>> solve_and_get_final_result() = 0;

	virtual void export_performance(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
		const std::string message = "Solver has no performance stats";
		writer.String(message.c_str());
	}

	/** Problem kinds this solver can handle (instance `config.yaml` `problem` field). */
	[[nodiscard]] virtual std::span<const problem_type> supported_problem_types() const = 0;
};

template<template<typename, class...> class S, typename N>
concept DARP_benchmark_solver_constructor_interface =
	std::derived_from<S<N>,DARP_benchmark_solver_interface<N>>
	&& requires(const DARP_instance<N>& darp_instance, const DARP_benchmark_config& config, const fs::path& out_dir_path){
		{S(darp_instance, config, out_dir_path)} -> std::same_as<S<N>>;
	};

/**
 * @brief Base class for DARP benchmark solvers.
 *
 * Each solver object is bound to exactly one `DARP_instance` for its entire lifetime (the instance passed to the
 * constructor). `solve()` operates on that bound instance only; callers must construct a new solver per instance.
 *
 * @tparam N node type
 * @tparam P plan type (default `VehiclePlan<N>`)
 */
template<typename N, class P = VehiclePlan<N>>
class DARP_benchmark_solver : public DARP_benchmark_solver_interface<N> {
public:

	using solution_impl_ret_val = std::unique_ptr<Solution<N, P>>;

	/**
	 * @brief Binds the solver to the given instance and benchmark configuration.
	 * @param instance problem instance; must outlive the solver if stored by reference elsewhere
	 * @param config solver / benchmark configuration (typically loaded from YAML)
	 */
	DARP_benchmark_solver(const DARP_instance<N>& instance, const DARP_benchmark_config& config);

	/**
	 * @brief Runs the solver on the instance supplied at construction time.
	 * @return typed solution, or implementation-defined empty result on failure
	 */
	std::unique_ptr<Solution<N, P>> solve() requires(Benchmark_plan<P>);

	/**
	 * @brief Type-erased entry point used by the benchmark harness; same semantics as `solve()`.
	 */
	std::unique_ptr<Solution_interface<N>> solve_and_get_final_result() override;

	[[nodiscard]] std::span<const problem_type> supported_problem_types() const override;

	/**
	 * @brief Validates each plan against the bound instance configuration (e.g. time windows).
	 * @tparam CheckPlan plan type supporting `check` against `DARP_instance_configuration`
	 * @param plans plans to validate
	 */
	template<class CheckPlan=P>
	void check_plans(const std::vector<CheckPlan>& plans);

protected:
	enum class Adjustment_reason {
		max_ride_time, max_route_time
	};

	/** Instance pointer set at construction; `solve()` / `solve_impl()` use only this object. */
	const DARP_instance<N>* const darp_instance;

	/** Benchmark YAML configuration; reference must remain valid for the solver lifetime. */
	const DARP_benchmark_config& solver_config;

	/** Travel-time provider and instance configuration derived from the bound instance (immutable for the solver lifetime). */
	const DARP_context<N> context;

	unsigned short max_delay_time{0};

	unsigned long solution_cost{0};

	[[nodiscard]] unsigned long get_max_route_duration() const {
		return context.get_max_route_duration();
	}

	[[nodiscard]] unsigned long get_max_ride_time() const {
		return context.get_max_ride_time();
	}

	[[nodiscard]] bool is_return_to_depot() const {
		return context.is_return_to_depot();
	}

	[[nodiscard]] const std::shared_ptr<Travel_time_provider<N>>& travel_time_provider() const {
		return context.travel_time_provider();
	}

	[[nodiscard]] const std::shared_ptr<DARP_instance_configuration>& darp_instance_configuration() const {
		return context.darp_instance_configuration();
	}

	/**
	 * @brief Solver-specific implementation invoked by `solve()` after any common setup.
	 * @return typed solution in the solver's plan representation
	 */
	virtual solution_impl_ret_val solve_impl() = 0;

	/**
	 * @brief Enumerates pickup/drop-off insertion positions to minimize cost increment for one additional request.
	 * @param current_plan feasible plan for a single vehicle
	 * @param request request to insert
	 * @return best resulting plan, or `std::nullopt` if no feasible insertion exists
	 */
	std::optional<VehiclePlan<N>> compute_optimal_plan(
		const VehiclePlan<N>& current_plan,
		const Request<N>& request
	);

	/**
	 * @brief Inserts a request into a plan of type `P` at given pickup/drop-off indices (used by heuristics on structured plans).
	 */
	std::optional<P> insert_into_plan(
		const P& current_plan,
		unsigned short pickup_option_index,
		unsigned short drop_off_option_index,
		const Request<N>& request
	);

	/**
	 * @brief Adjusts action times on a `VehiclePlan` to restore feasibility (max ride / max route constraints).
	 * @param vehicle_plan plan to modify in place
	 * @param initial_reason which constraint triggered the adjustment pass
	 * @param station_position depot / station node for route-duration checks
	 * @return true if a feasible adjustment was found
	 */
	bool adjust_times(VehiclePlan<N>& vehicle_plan, Adjustment_reason initial_reason, N station_position);
};


#include "DARP_benchmark_solver.tpp"



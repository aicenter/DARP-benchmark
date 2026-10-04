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

#include <unordered_map>
#include <any>
#include <functional>
#include <memory>
#include <future-config/configuration.h>

#include "solver/DARP_benchmark_solver.h"
#include "Solution.h"
#include "DARP_instance.h"
#include "Cordeau_benchmark.h"
#include "DARP_benchmark_node.h"
#include "config/DARP-benchmark_config.h"

namespace fs = std::filesystem;

namespace DARP {
// Variant type for all supported node types
// using Node_variant = std::variant<Amodsim_node, Cordeau_node>;
enum class Node_type {
	DEFAULT,
	CORDEAU
};


/**
 * @brief Singleton class that stores solver factory functions.
 * Uses std::variant and std::visit to handle multiple node types with a single-level map.
 */
template<typename... NO>
class Solver_registry {
public:
	static Solver_registry& get();

	/**
	 * @brief Register a solver factory using std::visit with NodeVariant.
	 * Automatically registers factories for all node types in NodeVariant.
	 * @tparam S The solver template class (e.g., Insertion_heuristic_solver)
	 * @param method The method enum to register
	 */
	template<template<typename, class...> class S>
	requires(DARP_benchmark_solver_constructor_interface<S,NO> && ...)
	void register_solver(const std::string& method);

	/**
	 * @brief Create a solver instance using the registered factory.
	 * @tparam N Node type for the solver
	 * @param method The method to use
	 * @param darp_instance The DARP instance
	 * @param solver_config Solver configuration
	 * @param out_dir_path Output directory path
	 * @return Pointer to the created solver, or nullptr if not registered
	 */
	template<typename N>
	[[nodiscard]] std::unique_ptr<DARP_benchmark_solver_interface<N>> create_solver(
		const std::string& method,
		const DARP_instance<N>& darp_instance,
		const DARP_benchmark_config& solver_config,
		const fs::path& out_dir_path
	) const;

	// Deleted copy and move constructors/assignments for singleton
	Solver_registry(const Solver_registry&) = delete;

	Solver_registry& operator=(const Solver_registry&) = delete;

	Solver_registry(Solver_registry&&) = delete;

	Solver_registry& operator=(Solver_registry&&) = delete;

private:
	Solver_registry() = default;

	~Solver_registry() = default;

	// Factory function type that dispatches based on node type
	template<typename N>
	using Solver_factory = std::function<std::unique_ptr<DARP_benchmark_solver_interface<N>>(
		const DARP_instance<N>& darp_instance,
		const DARP_benchmark_config&,
		const fs::path&
	)>;

	template<typename N>
	using Solver_factory_map = std::unordered_map<std::string, Solver_factory<N>>;

	// Simplified single-level map: Method -> FactoryFunction (uses std::visit internally)
	std::tuple<Solver_factory_map<NO>...> solver_factories{};

};

using Default_solver_registry = Solver_registry<Amodsim_node, Cordeau_node>;

/**
 * @brief Singleton class that stores config definitions.
 * Enables static registration of Config_definition instances.
 */
class Config_registry {
public:
	static Config_registry& get();

	/**
	 * @brief Register a config definition.
	 * @param config Config definition to register
	 */
	void register_config(std::unique_ptr<fc::Config_definition_base> config);

	/**
	 * @brief Get all registered config definitions.
	 * @return Vector of config definition instances (moved out from the registry)
	 */
	[[nodiscard]] std::vector<std::unique_ptr<fc::Config_definition_base>> get_all_configs();

	// Deleted copy and move constructors/assignments for singleton
	Config_registry(const Config_registry&) = delete;
	Config_registry& operator=(const Config_registry&) = delete;
	Config_registry(Config_registry&&) = delete;
	Config_registry& operator=(Config_registry&&) = delete;

private:
	static Config_registry instance;

	Config_registry() = default;
	~Config_registry() = default;

	std::vector<std::unique_ptr<fc::Config_definition_base>> config_definitions;
};

template<class N>
class DARP_benchmark {
public:
	int run(
		const std::filesystem::path& instance_path,
		const fs::path& out_path,
		const std::string& method,
		const DARP_benchmark_config& solver_arguments,
		unsigned short number_of_trials
	) const;

	explicit DARP_benchmark(std::unique_ptr<Reader<N>> reader);

private:
	std::unique_ptr<Reader<N>> reader;

	void process_instance(
		const fs::path& instance_file_path,
		const fs::path& out_dir,
		const std::string& method,
		const DARP_benchmark_config& solver_arguments,
		unsigned short trial_number
	) const;

	[[nodiscard]] static rapidjson::StringBuffer export_performance(
		unsigned long total_time,
		const DARP_benchmark_solver_interface<N>& solver
	);
};
}

#include "DARP_benchmark.tpp"

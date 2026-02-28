// DARP-benchmark.cpp : Defines the entry point for the application.
//

#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <magic_enum/magic_enum.hpp>

#include "solver/Random_solver.h"
#include "benchmark.h"
#include "number_formatter.h"
#include "memory.h"
#include "solver/IH/Insertion_heuristic_solver.h"


namespace DARP {


template<typename... NO>
Solver_registry<NO...> Solver_registry<NO...>::instance{};

template<typename... NO>
Solver_registry<NO...>& Solver_registry<NO...>::get() {
	return instance;
}

template<typename... NO>
template<typename N>
std::unique_ptr<DARP_benchmark_solver_interface<N>> Solver_registry<NO...>::create_solver(
	const std::string& method,
	const DARP_instance<N>& darp_instance,
	const DARP_benchmark_config& solver_config,
	const fs::path& out_dir_path
) const {
	const auto& map = std::get<Solver_factory_map<N>>(solver_factories);
	std::string method_lower_case = method;
	std::transform(method.begin(), method.end(), method_lower_case.begin(), ::tolower);
	if(!map.contains(method_lower_case)) {
		throw std::runtime_error(fmt::format("Unknown solver method: {}", method));
	}
	return std::get<Solver_factory_map<N>>(solver_factories).at(method_lower_case)(darp_instance, solver_config, out_dir_path);
}

template <class N>
rapidjson::StringBuffer DARP_benchmark<N>::export_performance(
	unsigned long total_time, 
	const DARP_benchmark_solver_interface<N>& solver
) {
	rapidjson::StringBuffer s;
	rapidjson::PrettyWriter writer(s);

	writer.StartObject();
	writer.Key("total_time");
	writer.Uint64(total_time);
	writer.Key("peak_memory_KiB");
	writer.Uint64(get_max_memory_usage());
	writer.Key("solver_stats");
	solver.export_performance(writer);
	writer.EndObject();

	return s;
}

template<class N>
void DARP_benchmark<N>::process_instance(
	const fs::path& instance_file_path,
	const fs::path& out_dir,
	const std::string& method,
	const DARP_benchmark_config& solver_arguments,
	unsigned short trial_number
) const
{
	// set working dir to input path
	const auto instance_dir = std::filesystem::path(instance_file_path).remove_filename();
	std::filesystem::current_path(instance_dir);

	const DARP_instance<N> darp_instance = reader->read(instance_file_path);
	
	// set working dir to out path
	std::filesystem::current_path(out_dir);

	auto solver = Default_solver_registry::get().create_solver(method, darp_instance, solver_arguments, out_dir);
	spdlog::info("Running {} solver", method);
	auto result = benchmark(
		&DARP_benchmark_solver_interface<N>::solve_and_get_final_result,
		solver,
		darp_instance
	);

	spdlog::info("Solving time: {}", format_number(result.count()));
	rapidjson::StringBuffer sb = result.return_value->JSON_serialize(60);
	rapidjson::StringBuffer perf_sb = export_performance(result.count(), *solver);

	// creating output directories
	std::error_code fs_error;
	const bool new_dir_result = std::filesystem::create_directories(std::filesystem::path(out_dir), fs_error);

	if (fs_error) {
		//error creating output directories
		std::cerr << "[Error] Failed to create output directory \"" << out_dir << "\": ";
		std::cerr << fs_error.message() << "\n";
	}
	else {
		if (new_dir_result) {
			std::cout << "Creating output directory \"" << out_dir << "\"\n";
		}

		if (solver_arguments.simple_csv_export) {
			std::string csv_suffix = trial_number == 1 ? "-solution.csv" : fmt::format("-solution-{}.csv", trial_number);
			const std::string csv_file_name = std::filesystem::path(instance_file_path).filename().string() + csv_suffix;
			const fs::path csv_file_path = out_dir / csv_file_name;
			spdlog::info("Writing solution to: {}", std::filesystem::absolute(csv_file_path).string());
			std::ofstream csv_file(csv_file_path);
			csv_file << result.return_value->export_simple_csv();
			csv_file.close();
		}
		else {
			std::string suffix = trial_number == 1 ? "-solution.json" : fmt::format("-solution-{}.json", trial_number);
			const std::string out_file_name = std::filesystem::path(instance_file_path).filename().string() + suffix;
			const fs::path out_file_path = out_dir / out_file_name;
			spdlog::info("Writing solution to: {}", std::filesystem::absolute(out_file_path).string());
			std::ofstream test_file(out_file_path);
			test_file << sb.GetString();
			test_file.close();
		}

		std::string performance_suffix = trial_number == 1 ? "-performance.json" : fmt::format("-performance-{}.json", trial_number);
		const std::string performance_file_name = std::filesystem::path(instance_file_path).filename().string() + performance_suffix;
		const fs::path performance_file_path = out_dir / performance_file_name;
		spdlog::info("Writing performance to: {}", std::filesystem::absolute(performance_file_path).string());
		std::ofstream performance_file(performance_file_path);
		performance_file << perf_sb.GetString();
		performance_file.close();
	}
}


template<class N>
int DARP_benchmark<N>::run(
	const fs::path& instance_path,
	const fs::path& out_path,
	const std::string& method,
	const DARP_benchmark_config& solver_arguments,
	unsigned short number_of_trials
) const {
	check_path(instance_path.string());
	if(is_directory(instance_path)) {
		const auto message
			= fmt::format("Input file path points to a directory: {} ", instance_path.string());
		throw std::runtime_error(message);
	}
	spdlog::info("Instance path: {}", instance_path.string());
	spdlog::info("Out path: {}", out_path.string());

	for(unsigned short i = 1; i <= number_of_trials; ++i) {
		process_instance(instance_path, out_path, method, solver_arguments, i);
	}

	return 0;
}

template <class N>
DARP_benchmark<N>::DARP_benchmark(std::unique_ptr<Reader<N>> reader): reader(std::move(reader)){
	Default_solver_registry::get().register_solver<Insertion_heuristic_solver>("ih");
}

template<typename... NO>
template<template <typename, class...> class S>
requires(DARP_benchmark_solver_constructor_interface<S,NO> && ...)
void Solver_registry<NO...>::register_solver(const std::string& method) {
	spdlog::info("Registering solver: {}", method);
	([&] {
		auto& solver_factory_map = std::get<Solver_factory_map<NO>>(solver_factories);
		solver_factory_map[method] = [](
			const DARP_instance<NO>& darp_instance,
			const DARP_benchmark_config& solver_config,
			const fs::path& out_dir_path
		) -> std::unique_ptr<DARP_benchmark_solver_interface<NO>> {
				return std::make_unique<S<NO>>(darp_instance, solver_config, out_dir_path);
			};
	}(), ...);
}

}
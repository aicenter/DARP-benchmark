// DARP-benchmark.cpp : Defines the entry point for the application.
//

#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <any>
#include <magic_enum/magic_enum.hpp>

#include "Cordeau_benchmark.h"
#include "Solution.h"
#include "DARP_benchmark_node.h"
#include "solver/IH/Insertion_heuristic_solver.h"
#include "solver/Random_solver.h"
#include "benchmark.h"
#include "number_formatter.h"
#include "memory.h"


namespace DARP {

template <class N>
rapidjson::StringBuffer DARP_benchmark<N>::export_performance(
	unsigned long total_time, 
	const DARP_benchmark_solver_interface<N>& solver
) {
	rapidjson::StringBuffer s;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);


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
DARP_benchmark_solver_interface<N>* create_solver(
	const Method method, 
	const DARP_instance<N>& instance,
	const DARP_benchmark_config& solver_config,
	const fs::path& out_dir_path
) {
	auto travel_time_provider = instance.get_travelcost_provider();
	auto darp_instance_configuration = instance.get_darp_instance_configuration();
	switch(method) {
		case Method::IH:
		default:
			return new Insertion_heuristic_solver<N>(travel_time_provider, darp_instance_configuration);
		}
}


template<class N>
void DARP_benchmark<N>::process_instance(
	const fs::path& instance_file_path,
	const fs::path& out_dir,
	const Method method,
	//Solver_factory_interface<N>* solver_factory,
	//const std::list<std::string> & solver_arguments,
	const DARP_benchmark_config& solver_arguments,
	unsigned short trial_number
) const
{
	// set working dir to input path
	const auto instance_dir = std::filesystem::path(instance_file_path).remove_filename();
	std::filesystem::current_path(instance_dir);

	const DARP_instance<N> instance = reader->read(instance_file_path);
	
	// set working dir to out path
	std::filesystem::current_path(out_dir);

	auto* solver = create_solver(method, instance, solver_arguments, out_dir);
	spdlog::info("Running {} solver", magic_enum::enum_name(method));
	auto result = benchmark(
        &DARP_benchmark_solver_interface<N>::solve_and_get_final_result,
        solver,
        instance
    );
	//Solution<N> solution = solver->solve(instance);
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

		std::string suffix = trial_number == 1 ? "-solution.json" : fmt::format("-solution-{}.json", trial_number);
		const std::string out_file_name = std::filesystem::path(instance_file_path).filename().string() + suffix;
		const fs::path out_file_path = out_dir / out_file_name;
		spdlog::info("Writing solution to: {}", std::filesystem::absolute(out_file_path).string());
		std::ofstream test_file(out_file_path);
		test_file << sb.GetString();
		test_file.close();

		std::string performance_suffix = trial_number == 1 ? "-performance.json" : fmt::format("-performance-{}.json", trial_number);
		const std::string performance_file_name = std::filesystem::path(instance_file_path).filename().string() + performance_suffix;
		const fs::path performance_file_path = out_dir / performance_file_name;
		spdlog::info("Writing performance to: {}", std::filesystem::absolute(performance_file_path).string());
		std::ofstream performance_file(performance_file_path);
		performance_file << perf_sb.GetString();
		performance_file.close();
	}

	delete solver;
}


template<class N>
int DARP_benchmark<N>::run(
	const fs::path& instance_path,
	const fs::path& out_path,
	const Method method,
	const DARP_benchmark_config& solver_arguments,
	unsigned short number_of_trials
) {
		
	//init_solver_factories();
	//Solver_factory_interface<N>* solver_factory = solver_factories[solver_factory_key];
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
DARP_benchmark<N>::DARP_benchmark(std::unique_ptr<Reader<N>> reader): reader(std::move(reader)){}

}

#pragma once

#include <iostream>
#include <unordered_map>
#include <any>

#include "solver/DARP_benchmark_solver.h"
#include "Solution.h"
#include "DARP_instance.h"
#include "Cordeau_benchmark.h"
#include "DARP_benchmark_node.h"
#include "config/DARP-benchmark_config.h"


namespace DARP {
	
enum class Method {
	IH,
	VGA,
	VGA_CHAINING,
	HALNS
};


template<class N>
class DARP_benchmark
{
public:
	int run(const std::filesystem::path& instance_path,
	        const fs::path& out_path,
	        const Method method,
	        //const std::list<std::string> & solver_arguments,
	        const DARP_benchmark_config& solver_arguments,
	        unsigned short number_of_trials);


	explicit DARP_benchmark(std::unique_ptr<Reader<N>> reader);

private:
	std::unique_ptr<Reader<N>> reader;
	
	void process_instance(
		const fs::path& instance_file_path,
		const fs::path& out_dir,
		const Method method,
		const DARP_benchmark_config& solver_arguments,
		unsigned short trial_number
	) const;

	[[nodiscard]] static rapidjson::StringBuffer export_performance(unsigned long total_time, const DARP_benchmark_solver_interface<N>& solver);
};

}

#include "DARP-benchmark.tpp"


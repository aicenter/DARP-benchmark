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

#include <numeric>
#include <tclap/CmdLine.h>
#include <mimalloc-new-delete.h>

#include "chaining_tester.h"
#include "solver/VGA_chaining/chaining/testing.h"
#include "common.h"
#include "logging.h"


//using namespace DARP;

unsigned int get_variant_count(const std::vector<std::vector<DARP::Chaining_test_plan>>& variants) {
	std::vector<int> counts;
	counts.reserve( variants.size());
	std::ranges::transform(variants, std::back_inserter(counts), 
		[](std::vector<DARP::Chaining_test_plan> x) { return static_cast<unsigned>(x.size()); });
	const unsigned int variant_count = std::accumulate( counts.begin(), counts.end(), 0 );
	return variant_count;
}



void export_result(
	const std::string& out_path,
	const unsigned int max_time_between_plans,
	const unsigned int plan_count,
	const Chaining_result* old_chaining_result,
	const Chaining_result* new_chaining_result

) {
	const auto out_dir = std::filesystem::absolute(out_path).parent_path();
	std::filesystem::create_directories(out_dir);
	spdlog::info("Exporting Chaining results to {}",  std::filesystem::absolute(out_path).string());
	rapidjson::StringBuffer s;
	rapidjson::PrettyWriter writer(s);
	writer.StartObject();
	writer.Key("max_time_between_plans");
	writer.Uint(max_time_between_plans);
	writer.Key("plan_count");
	writer.Uint(plan_count);
	
	if (old_chaining_result != nullptr) {
		writer.Key("old_chaining");
		old_chaining_result->JSON_serialize(writer);
	}
	writer.Key("new_chaining");
	new_chaining_result->JSON_serialize(writer);
	writer.EndObject();

	std::ofstream test_file(out_path);
	test_file << s.GetString();
	test_file.close();
}


int main(int argc, const char** argv) {

	try {

		TCLAP::CmdLine cmd("Chaining tester", ' ', "0.1");

		TCLAP::ValueArg<std::string> plans_path_arg("p", "plans",
			"Path to the JSON file with plans computed bz the VGA method", true, "", "string");
		cmd.add(plans_path_arg);

		TCLAP::ValueArg<std::string> distance_matrix_filepath_arg("d", "dm",
			"Distance matrix filepath", true, "", "string");
		cmd.add(distance_matrix_filepath_arg);

		TCLAP::ValueArg<std::string> out_filepath_arg("o", "outpath",
			"Chaining tester result filepath", true, "chaining_result.json", "string");
		cmd.add(out_filepath_arg);

		TCLAP::ValueArg<unsigned short> max_vehicle_count_arg("v", "vcount",
			"Max vehicle count", true, 1, "positive number");
		cmd.add(max_vehicle_count_arg);

		TCLAP::ValueArg<unsigned int> extra_plan_cost_arg("e", "excost",
			"Cost for each additional vehicle plan", false, 1000, "positive number");
		cmd.add(extra_plan_cost_arg);

		TCLAP::ValueArg<unsigned int> max_time_between_plans_arg("t", "mtbp",
			"Max time between plans", false, 240, "positive number");
		cmd.add(max_time_between_plans_arg);

		TCLAP::ValueArg<unsigned int> plan_count_arg("", "plan-count",
			"The number of plans loaded from the plan file", false, 0, "positive number");
		cmd.add(plan_count_arg);

		TCLAP::SwitchArg run_old_variant_generation_arg("", "run-old",
			"Set this flag to false if you want to skip the old variant generation",
			false);
		cmd.add(run_old_variant_generation_arg);

		cmd.parse(argc, argv);

		const std::string plans_filepath = plans_path_arg.getValue();
		const std::string dm_filepath = distance_matrix_filepath_arg.getValue();
		const std::string out_filepath = out_filepath_arg.getValue();
		const unsigned short max_vehicle_count = max_vehicle_count_arg.getValue();
		const unsigned int extra_plan_cost = extra_plan_cost_arg.getValue();
		const unsigned int max_time_between_plans = max_time_between_plans_arg.getValue();
		const unsigned plan_count = plan_count_arg.getValue();
		const bool run_old_variant_generation = run_old_variant_generation_arg.getValue();

		set_up_logger(out_filepath);

		std::unique_ptr<Chaining_result> old_chaining_result;
		std::unique_ptr<Chaining_result> new_chaining_result;
		if (run_old_variant_generation) {
			auto results = DARP::compare_old_and_new_chaining(plans_filepath, dm_filepath, max_vehicle_count,
				extra_plan_cost, max_time_between_plans, plan_count);

			const auto& chaining_possibilities_old = results.first.return_value.chaining_possibilities;
			const auto& chaining_possibilities = results.second.return_value.chaining_possibilities;

			old_chaining_result = std::make_unique<Chaining_result>(
				chaining_possibilities_old->get_variant_count(),
				static_cast<unsigned>(chaining_possibilities_old->get_connections().size()),
				static_cast<unsigned>(results.first.return_value.routes->size()),
				results.first.count(),
				results.first.return_value.route_cost,
				results.first.return_value.extra_vehicle_cost
				);

			new_chaining_result = std::make_unique<Chaining_result>(
				chaining_possibilities->get_variant_count(),
				static_cast<unsigned>(chaining_possibilities->get_connections().size()),
				static_cast<unsigned>(results.second.return_value.routes->size()),
				results.second.count(),
				results.second.return_value.route_cost,
				results.second.return_value.extra_vehicle_cost
				);
		}
		else {
			auto result = DARP::run_chaining(plans_filepath, dm_filepath, max_vehicle_count,
				extra_plan_cost, max_time_between_plans, plan_count);

			new_chaining_result = std::make_unique<Chaining_result>(
				result.return_value.chaining_possibilities->get_variant_count(),
				static_cast<unsigned>(result.return_value.chaining_possibilities->get_connections().size()),
				static_cast<unsigned>(result.return_value.routes->size()),
				result.count(),
				result.return_value.route_cost,
				result.return_value.extra_vehicle_cost
				);
		}

		if (run_old_variant_generation) {
			spdlog::info("Old variant count: {}", old_chaining_result->variant_count);
		}
		spdlog::info("New variant count: {}", new_chaining_result->variant_count);

		if (run_old_variant_generation) {
			spdlog::info("Old connection count: {}", old_chaining_result->connection_count);
		}
		spdlog::info("New connection count: {}", new_chaining_result->connection_count);

		if (run_old_variant_generation) {
			spdlog::info("old routes count: {}", old_chaining_result->route_count);
		}
		spdlog::info("new routes count: {}", new_chaining_result->route_count);

		if (run_old_variant_generation) {
			spdlog::info(
				R"(Old chaining costs: 
			routes cost: {}
			extra vehicle costs: {}
			total cost: {})", old_chaining_result->route_cost, old_chaining_result->extra_vehicle_cost_cost,
				old_chaining_result->route_cost + old_chaining_result->extra_vehicle_cost_cost);
		}

		spdlog::info(
			R"(New chaining costs: 
		routes cost: {}
		extra vehicle costs: {}
		total cost: {})", new_chaining_result->route_cost, new_chaining_result->extra_vehicle_cost_cost,
			new_chaining_result->route_cost + new_chaining_result->extra_vehicle_cost_cost);

		if (run_old_variant_generation) {
			spdlog::info("Old total computational time: {} ms", old_chaining_result->computational_time);
		}
		spdlog::info("New total computational time: {} ms", new_chaining_result->computational_time);

		export_result(out_filepath, max_time_between_plans, plan_count, old_chaining_result.get(),
			new_chaining_result.get());
	}
	catch(...) {
		ExceptionHandeling::process_unexpected_exception();
		/*std::exception_ptr p = std::current_exception();
		spdlog::error(p.what());*/
	}
}

void Chaining_result::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const{
	writer.StartObject();
	writer.Key("variant_count");
	writer.Uint(variant_count);
	writer.Key("connection_count");
	writer.Uint(connection_count);
	writer.Key("route_count");
	writer.Uint(route_count);
	writer.Key("computational_time");
	writer.Uint(computational_time);
	writer.Key("route_cost");
	writer.Uint(route_cost);
	writer.Key("extra_vehicle_cost");
	writer.Uint(extra_vehicle_cost_cost);
	writer.EndObject();
}

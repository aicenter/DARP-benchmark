#include "gtest_wrapper.h"

#include <filesystem>
#include <sstream>

#include <future-config/configuration.h>

#include "common.h"
#include "../src/Solution.h"
#include "../src/Cordeau_benchmark.h"
#include "../src/solver/IH/Insertion_heuristic_solver.h"
#include "../src/DARP_benchmark_reader.h"
#include "../src/DARP_benchmark_node.h"
#include "../src/config/DARP-benchmark_config.h"

namespace fs = std::filesystem;

namespace {

TEST(Solution_test, export_simple_csv_infeasible_returns_header_only) {
	Solution<Amodsim_node> infeasible_solution;
	ASSERT_FALSE(infeasible_solution.is_feasible());

	std::string csv = infeasible_solution.export_simple_csv();

	EXPECT_EQ(csv, "plan,request,pickup_time,dropoff_time\n");
}

TEST(Solution_test, export_simple_csv_from_deserialized_solution) {
	fs::path instance_path = fs::path("test_resources") / "Chyse" / "config.yaml";
	fs::path solution_path = fs::path("test_resources") / "Chyse" / "expected" / "solution.json";
	if (!fs::exists(instance_path)) {
		instance_path = fs::path("data") / "test_resources" / "Chyse" / "config.yaml";
		solution_path = fs::path("data") / "test_resources" / "Chyse" / "expected" / "solution.json";
	}
	ASSERT_TRUE(fs::exists(instance_path)) << "Instance path: " << fs::absolute(instance_path);
	ASSERT_TRUE(fs::exists(solution_path)) << "Solution path: " << fs::absolute(solution_path);

	const auto original_cwd = fs::current_path();
	const auto abs_instance_path = fs::absolute(instance_path);
	const auto abs_solution_path = fs::absolute(solution_path);
	fs::current_path(abs_instance_path.parent_path());

	DARP_benchmark_reader reader;
	DARP_instance<Amodsim_node> darp_instance = reader.read(abs_instance_path);
	Solution<Amodsim_node> solution = deserialize_json<Amodsim_node>(abs_solution_path, darp_instance);

	fs::current_path(original_cwd);

	ASSERT_TRUE(solution.is_feasible());
	std::string csv = solution.export_simple_csv();

	EXPECT_TRUE(csv.starts_with("plan,request,pickup_time,dropoff_time"));

	// Verify each data line has 4 comma-separated values
	size_t data_line_count = 0;
	std::istringstream iss(csv);
	std::string line;
	std::getline(iss, line);
	while (std::getline(iss, line)) {
		if (!line.empty()) {
			++data_line_count;
			size_t comma_count = 0;
			for (char c : line) {
				if (c == ',') ++comma_count;
			}
			EXPECT_EQ(comma_count, 3u) << "Line: " << line;
		}
	}
	EXPECT_GT(data_line_count, 0u);
}

TEST(Solution_test, export_simple_csv_from_ih_solver) {
	std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short)60);

	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
	std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	Vehicle<Cordeau_node> vehicle{0, vehicle_init_position, 1};
	vehicles->push_back(vehicle);
	std::shared_ptr<Cordeau_node> from{new Cordeau_node{0, 0}};
	std::shared_ptr<Cordeau_node> to{new Cordeau_node{1, 0}};
	requests->emplace_back(
		0u, 1u, 0u, from, 0u, 5u, to, 0u,
		75u, (unsigned short)60u, (unsigned short)5u, (unsigned short)10u
	);

	auto config = std::make_shared<DARP_instance_configuration>(135, 135, true);
	auto instance = std::make_shared<DARP_instance<Cordeau_node>>(
		std::move(requests),
		std::move(vehicles),
		travel_time_provider,
		config
	);
	auto solver_config = fc::load<DARP_benchmark_config>();
	Insertion_heuristic_solver<Cordeau_node> solver(
		travel_time_provider,
		config,
		solver_config
	);
	std::unique_ptr<Solution<Cordeau_node>> solution = solver.solve(*instance);

	ASSERT_TRUE(solution->is_feasible());
	std::string csv = solution->export_simple_csv();

	EXPECT_TRUE(csv.starts_with("plan,request,pickup_time,dropoff_time"));

	VehiclePlan<Cordeau_node> plan = (*solution)[0];
	unsigned int expected_pickup_time = plan[0].get_service_start_time();
	unsigned int expected_dropoff_time = plan[1].get_service_start_time();
	std::string expected_row = "0,0," + std::to_string(expected_pickup_time) + "," +
	                          std::to_string(expected_dropoff_time) + "\n";
	EXPECT_TRUE(csv.find(expected_row) != std::string::npos);
}

}  // namespace

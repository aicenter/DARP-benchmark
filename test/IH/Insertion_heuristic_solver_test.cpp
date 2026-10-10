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
#include "../gtest_wrapper.h"
#include <vector>
#include <filesystem>

#include <future-config/configuration.h>

#include "./common.h"
#include "../../src/solver/IH/Insertion_heuristic_solver.h"
#include "../../src/Cordeau_benchmark.h"
#include "../../src/travel_time_provider/Travel_time_provider.h"
#include "../../src/travel_time_provider/Distance_matrix_travel_time_provider.h"
#include "../../src/config/DARP-benchmark_config.h"
#include "../../src/DARP_instance.h"

namespace fs = std::filesystem;

namespace {

class IH_test_action : public Action_base<unsigned> {
public:
//	IH_test_action(unsigned index, Action_type action_type, time_type max_time, request_index_type request_index):
//		Action_base<unsigned>(index, action_type, max_time), request_index(request_index) {}

	/**
	 * JSON deserialization constructor for action data
	 * @param json_data
	 * @param dm_index
	 */
	IH_test_action(
		const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data,
		unsigned dm_index
	) :
		Action_base<unsigned int>(json_data, dm_index),
		request_index(parse_request_index(json_data)) {}

	static request_index_type parse_request_index(const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data) {
		auto type = action_type_from_string(json_data["type"].GetString());
		if (type == Action_type::depot) {
			return 0;
		}
		return json_data["request"].GetUint();
	}

	/**
	 * JSON deserialization constructor for request
	 * @param json_data
	 * @param dm_index
	 * @param request_index
	 */
	IH_test_action(
		const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data,
		unsigned dm_index,
		request_index_type request_index
	) :
		Action_base<unsigned int>(json_data, dm_index),
		request_index(request_index) {}

	[[nodiscard]] request_index_type get_request_index() const {
		return request_index;
	}

private:
	request_index_type request_index{0};
};

class IH_test_action_data : public Test_action_data<IH_test_action> {

public:
	using Test_action_data<IH_test_action>::Test_action_data;

	[[nodiscard]] request_index_type get_request_index() const {
		return this->get_action().get_request_index();
	}
};

using IH_test_plan = IH_SVDARP_test_plan<IH_test_action_data>;

/** Adapter: exposes Travel_time_provider<unsigned> for Insertion_heuristic_solver<unsigned,...> using raw matrix. */
class Distance_matrix_as_unsigned_tt_provider : public Travel_time_provider<unsigned> {
	public:
		explicit Distance_matrix_as_unsigned_tt_provider(std::shared_ptr<Distance_matrix_travel_time_provider> inner)
			: inner_(std::move(inner)) {}
		travel_time_type get_travel_time(const unsigned& from, const unsigned& to) const override {
			return inner_->get_travel_time(from, to);
		}
		std::tuple<const unsigned&, travel_time_type> get_vehicle_location_info(
			const unsigned& last_action_location,
			const unsigned& next_action_location,
			time_type time_since_last_action_departure) const override {
			auto [idx, t] = inner_->get_vehicle_location_info(last_action_location, next_action_location, time_since_last_action_departure);
			cached_index_ = idx;
			return {cached_index_, t};
		}
	private:
		std::shared_ptr<Distance_matrix_travel_time_provider> inner_;
		mutable unsigned cached_index_{0};
	};

auto load_data_from_json_file(const std::string& path) {
	rapidjson::Document doc = load_json_to_dom(get_test_resource_path(path).string());
	assert(doc.IsObject());

	time_type vehicle_start_time = 0;
	if (doc.HasMember("vehicle_start_time")) {
		vehicle_start_time = doc["vehicle_start_time"].GetUint();
	}
	auto vehicle = std::make_unique<Test_vehicle>(1, vehicle_start_time);

	// load plan
	const auto& plan_data = doc["plan"].GetObj();
	auto plan = std::make_unique<IH_test_plan>(plan_data, *vehicle);

	// load request
	auto request_data = doc["request"].GetObj();
	const auto action_count = static_cast<unsigned>(plan->get_length());
	request_index_type request_index = request_data["index"].GetUint();
	auto request = Test_request<IH_test_action>(
		IH_test_action(request_data["pickup"], action_count, request_index),
		IH_test_action(request_data["dropoff"], action_count + 1, request_index)
	);

	// load dm
	const auto& dm_array = doc["dm"].GetArray();
	const unsigned size = dm_array.Size();
	assert(size == plan->get_actions().size() + 2);
	auto dm = std::make_unique<travel_time_type[]>(size * size);
	for (unsigned i = 0; i < size; ++i) {
		const auto& dm_inner_array = dm_array[i];
		assert(dm_inner_array.Size() == size);
		for (unsigned j = 0; j < size; ++j) {
			dm[i * size + j] = static_cast<travel_time_type>(dm_inner_array[j].GetUint());
		}
	}

	// Expose as Travel_time_provider<unsigned> for Insertion_heuristic_solver<unsigned,...>
	auto travel_time_provider = std::make_shared<Distance_matrix_as_unsigned_tt_provider>(
		std::make_shared<Distance_matrix_travel_time_provider>(size, std::move(dm)));

	// load expected plan
	auto expected_plan_data = doc["expected_plan"].GetObj();
	auto expected_plan = std::make_unique<IH_test_plan>(expected_plan_data, *vehicle);

	auto darp_configuration = std::make_shared<DARP_instance_configuration>(0, 0, false, false, vehicle_start_time);

	const unsigned short ih_vehicle_capacity = vehicle->get_capacity();
	auto ih_requests = std::make_unique<std::vector<Request<unsigned>>>();
	auto ih_vehicles = std::make_unique<std::vector<Vehicle<unsigned>>>();
	const auto depot_node = std::make_shared<unsigned>(0u);
	ih_vehicles->emplace_back(0u, depot_node, ih_vehicle_capacity, vehicle_start_time);
	DARP_instance<unsigned> ih_instance(
		std::move(ih_requests),
		std::move(ih_vehicles),
		travel_time_provider,
		darp_configuration
	);

	return std::tuple{
		std::move(plan),
		std::move(request),
		travel_time_provider,
		std::move(vehicle),
		std::move(expected_plan),
		darp_configuration,
		std::move(ih_instance)
	};
}


TEST(Insertion_heuristic_solver_test, one_car_one_request) {
	std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short) 60);

	std::unique_ptr<std::vector<Vehicle<Cordeau_node>>> vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	std::unique_ptr<std::vector<Request<Cordeau_node>>> requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
	requests->reserve(1);

	std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	Vehicle<Cordeau_node> vehicle{0, vehicle_init_position, 1};
	vehicles->push_back(vehicle);
	std::shared_ptr<Cordeau_node> from{new Cordeau_node{0, 0}};
	std::shared_ptr<Cordeau_node> to{new Cordeau_node{1, 0}};
	/*Request<Cordeau_node> request {0, 1, 0, from, 0, 5, to, 0,
								   75, 60, 5, 10};*/
	requests->emplace_back(
		0u, 1u, 0u, from, 0u, 5u, to, 0u,
		75u, (unsigned short) 60u, (unsigned short) 5u, (unsigned short) 10u
	);

	auto config = std::make_shared<DARP_instance_configuration>(135, 135, true);
	auto instance = std::make_shared<DARP_instance<Cordeau_node>>(
		std::move(requests),
		std::move(vehicles),
		travel_time_provider,
		config
	);
	auto solver_config = fc::load<DARP_benchmark_config>();
	solver_config.ih.temporal_pruning_min_plan_length = 1;
	Insertion_heuristic_solver<Cordeau_node> solver(*instance, solver_config, fs::path{});
	std::unique_ptr<Solution<Cordeau_node>> solution = solver.solve();

	VehiclePlan<Cordeau_node> plan = solution->get_plans()[0];
	ASSERT_DOUBLE_EQ(solution->get_cost(), 120.0);

	ASSERT_EQ(plan[0].get_action().get_action_type(), Action_type::pickup);
	ASSERT_EQ(plan[1].get_action().get_action_type(), Action_type::dropoff);

	ASSERT_EQ(plan[0].get_departure_time(), 5);
	ASSERT_EQ(plan[0].get_arrival_time(), 0u);
	ASSERT_EQ(plan[1].get_departure_time(), 75);
	ASSERT_EQ(plan[1].get_arrival_time(), 65u);
}

TEST(Insertion_heuristic_solver_test, one_car_multiple_requests) {
	std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short) 60);

	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	Vehicle<Cordeau_node> vehicle{0, vehicle_init_position, 2};
	vehicles->push_back(vehicle);
	std::shared_ptr<Cordeau_node> from{new Cordeau_node{0, 0}};
	std::shared_ptr<Cordeau_node> to{new Cordeau_node{1, 0}};
	requests->emplace_back(
		2, 3, 0, from, 0, 300, to, 0,
		500, (unsigned short) 60, (unsigned short) 0, (unsigned short) 0
	);
	std::shared_ptr<Cordeau_node> from2{new Cordeau_node{1, 0}};
	std::shared_ptr<Cordeau_node> to2{new Cordeau_node{1, 2}};
	requests->emplace_back(
		4, 5, 1, from2, 0, 300, to2, 0,
		600, (unsigned short) 120, (unsigned short) 0, (unsigned short) 0
	);
	std::shared_ptr<Cordeau_node> from3{new Cordeau_node{1, 1}};
	std::shared_ptr<Cordeau_node> to3{new Cordeau_node{2, 2}};
	requests->emplace_back(
		6, 7, 2, from3, 0, 300, to3, 0,
		600, (unsigned short) 85, (unsigned short) 0, (unsigned short) 0
	);

	auto config = std::make_shared<DARP_instance_configuration>(600, 400, true);
	auto instance = std::make_shared<DARP_instance<Cordeau_node>>(
		std::move(requests),
		std::move(vehicles),
		travel_time_provider,
		config
	);
	auto solver_config = fc::load<DARP_benchmark_config>();
	solver_config.ih.temporal_pruning_min_plan_length = 1;
	Insertion_heuristic_solver<Cordeau_node> solver(*instance, solver_config, fs::path{});
	std::unique_ptr<Solution<Cordeau_node>> solution = solver.solve();

	VehiclePlan<Cordeau_node> plan = solution->get_plans()[0];

	ASSERT_DOUBLE_EQ(solution->get_cost(), 399.0);
	ASSERT_EQ(plan[0].get_action().get_action_type(), Action_type::pickup);
	ASSERT_EQ(plan[1].get_action().get_action_type(), Action_type::pickup);
	ASSERT_EQ(plan[2].get_action().get_action_type(), Action_type::dropoff);
	ASSERT_EQ(plan[3].get_action().get_action_type(), Action_type::pickup);
	ASSERT_EQ(plan[4].get_action().get_action_type(), Action_type::dropoff);
	ASSERT_EQ(plan[5].get_action().get_action_type(), Action_type::dropoff);

	ASSERT_EQ(dynamic_cast<const Service_action<Cordeau_node>&>(plan[0].get_action()).get_request().get_index(),
		instance->get_requests()[0].get_index());
	ASSERT_EQ(dynamic_cast<const Service_action<Cordeau_node>&>(plan[1].get_action()).get_request().get_index(),
		instance->get_requests()[1].get_index());
	ASSERT_EQ(dynamic_cast<const Service_action<Cordeau_node>&>(plan[2].get_action()).get_request().get_index(),
		instance->get_requests()[0].get_index());
	ASSERT_EQ(dynamic_cast<const Service_action<Cordeau_node>&>(plan[3].get_action()).get_request().get_index(),
		instance->get_requests()[2].get_index());
	ASSERT_EQ(dynamic_cast<const Service_action<Cordeau_node>&>(plan[4].get_action()).get_request().get_index(),
		instance->get_requests()[2].get_index());
	ASSERT_EQ(dynamic_cast<const Service_action<Cordeau_node>&>(plan[5].get_action()).get_request().get_index(),
		instance->get_requests()[1].get_index());
}

TEST(Insertion_heuristic_solver_test, parallel_vehicle_trials_match_serial_solution_cost) {
	std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short) 60);

	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	std::shared_ptr<Cordeau_node> vehicle_0_position{new Cordeau_node{0, 0}};
	std::shared_ptr<Cordeau_node> vehicle_1_position{new Cordeau_node{100, 0}};
	std::shared_ptr<Cordeau_node> vehicle_2_position{new Cordeau_node{200, 0}};
	vehicles->emplace_back(0, vehicle_0_position, 2);
	vehicles->emplace_back(1, vehicle_1_position, 2);
	vehicles->emplace_back(2, vehicle_2_position, 2);

	std::shared_ptr<Cordeau_node> pickup_0{new Cordeau_node{101, 0}};
	std::shared_ptr<Cordeau_node> dropoff_0{new Cordeau_node{102, 0}};
	requests->emplace_back(
		2, 3, 0, pickup_0, 0, 1000, dropoff_0, 0,
		1200, (unsigned short) 180, (unsigned short) 0, (unsigned short) 0
	);

	std::shared_ptr<Cordeau_node> pickup_1{new Cordeau_node{202, 0}};
	std::shared_ptr<Cordeau_node> dropoff_1{new Cordeau_node{203, 0}};
	requests->emplace_back(
		4, 5, 1, pickup_1, 0, 1000, dropoff_1, 0,
		1200, (unsigned short) 180, (unsigned short) 0, (unsigned short) 0
	);

	auto config = std::make_shared<DARP_instance_configuration>(0, 0, false);
	auto instance = std::make_shared<DARP_instance<Cordeau_node>>(
		std::move(requests),
		std::move(vehicles),
		travel_time_provider,
		config
	);

	auto serial_solver_config = fc::load<DARP_benchmark_config>();
	serial_solver_config.tmax = 1;
	serial_solver_config.ih.temporal_pruning_min_plan_length = 1;
	Insertion_heuristic_solver<Cordeau_node> serial_solver(*instance, serial_solver_config, fs::path{});
	std::unique_ptr<Solution<Cordeau_node>> serial_solution = serial_solver.solve();

	auto parallel_solver_config = fc::load<DARP_benchmark_config>();
	parallel_solver_config.tmax = 2;
	parallel_solver_config.ih.temporal_pruning_min_plan_length = 1;
	Insertion_heuristic_solver<Cordeau_node> parallel_solver(*instance, parallel_solver_config, fs::path{});
	std::unique_ptr<Solution<Cordeau_node>> parallel_solution = parallel_solver.solve();

	ASSERT_DOUBLE_EQ(parallel_solution->get_cost(), serial_solution->get_cost());
	ASSERT_EQ(parallel_solution->get_dropped_request_count(), serial_solution->get_dropped_request_count());
	ASSERT_EQ(parallel_solution->get_plans().size(), serial_solution->get_plans().size());
}

/**
 * Test the insertion of a request in a plan: a method used by the HALNS solver. It is base on a failing
 * plan-request combination detected when solving a real instance.
 */
TEST(Insertion_heuristic_solver_test, insert_request_in_plan) {
	// load data
	const auto& [plan, request, travel_time_provider, vehicle, expected_plan, config, ih_instance]
		= load_data_from_json_file("IH_insert_in_plan_data.json");

	// insert in plan (N=unsigned; provider is adapter over Distance_matrix_travel_time_provider)
	auto solver_config = fc::load<DARP_benchmark_config>();
	solver_config.ih.temporal_pruning_min_plan_length = 1;
	Insertion_heuristic_solver<unsigned, IH_test_action_data, Test_vehicle, IH_test_plan> solver(
		ih_instance,
		solver_config,
		fs::path{}
	);

	auto new_plan
		= solver.insert_request_in_plan<Test_request<IH_test_action>>(*plan, request);

	ASSERT_TRUE(new_plan.has_value());

	// compare plans
	new_plan.value().test_check_equal(*expected_plan);
}

/**
 * Another test of the insertion of a request in a plan: a method used by the HALNS solver. It tests the case
 * when there is waiting for min time
 */
TEST(Insertion_heuristic_solver_test, insert_request_in_plan_2) {
	// load data
	const auto& [plan, request, travel_time_provider, vehicle, expected_plan, config, ih_instance]
		= load_data_from_json_file("IH_insert_in_plan_data_2.json");

	// insert in plan
	auto solver_config = fc::load<DARP_benchmark_config>();
	Insertion_heuristic_solver<unsigned, IH_test_action_data, Test_vehicle, IH_test_plan> solver(
		ih_instance,
		solver_config,
		fs::path{}
	);

	auto new_plan
		= solver.insert_request_in_plan<Test_request<IH_test_action>>(*plan, request);

	ASSERT_TRUE(new_plan.has_value());

	// compare plans
	new_plan.value().test_check_equal(*expected_plan);
}

TEST(Insertion_heuristic_solver_test, insert_request_in_plan_3) {
	// load data
	const auto& [plan, request, travel_time_provider, vehicle, expected_plan, config, ih_instance]
		= load_data_from_json_file("IH_insert_in_plan_data_3.json");

	// insert in plan
	auto solver_config = fc::load<DARP_benchmark_config>();
	Insertion_heuristic_solver<unsigned, IH_test_action_data, Test_vehicle, IH_test_plan> solver(
		ih_instance,
		solver_config,
		fs::path{}
	);

	auto new_plan
		= solver.insert_request_in_plan<Test_request<IH_test_action>>(*plan, request);

	ASSERT_TRUE(new_plan.has_value());

	// compare plans
	new_plan.value().test_check_equal(*expected_plan);
}

}

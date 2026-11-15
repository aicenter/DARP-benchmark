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
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <future-config/configuration.h>

#include "gtest_wrapper.h"

#include "../src/DARP_benchmark.h"
#include "../src/Cordeau_benchmark.h"
#include "../src/DARP_instance.h"
#include "../src/solver/IH/Insertion_heuristic_solver.h"
#include "../src/solver/min_revenue_ih/Min_revenue_IH_solver.h"
#include "../src/config/DARP-benchmark_config.h"
#include "../src/travel_time_provider/Euclidean_travel_time_provider.h"

namespace {

std::shared_ptr<Cordeau_node> node(unsigned x) {
	return std::make_shared<Cordeau_node>(static_cast<float>(x), 0.f, x);
}

struct Fleet_sizing_instance_parameters {
	unsigned short capacity{10};
	unsigned long max_route_duration{0};
	double min_plan_revenue{0};
	std::optional<unsigned> max_pickup_delay{};
	bool online{false};
};

/**
 * Fleet-sizing instance with the trips (from, to) on a line, one second per coordinate unit, the default cost (travel
 * time) and the revenue 60 per request.
 */
std::shared_ptr<DARP_instance<Cordeau_node>> fleet_sizing_instance(
	const std::vector<std::pair<unsigned, unsigned>>& trips, const Fleet_sizing_instance_parameters& parameters = {}
) {
	auto travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>(static_cast<unsigned short>(1));
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
	requests->reserve(trips.size());
	for(unsigned index = 0; index < trips.size(); ++index) {
		const auto [from, to] = trips[index];
		requests->emplace_back(
			index, 2 * index, 2 * index + 1, node(from), 0u, 10'000u, node(to), 0u, 10'000u,
			static_cast<unsigned short>(to > from ? to - from : from - to));
	}
	auto configuration = std::make_shared<DARP_instance_configuration>(
		parameters.max_route_duration, 0, false, true, 0, Cost_weights{}, problem_type::fleet_sizing);
	configuration->set_vehicle_capacity(parameters.capacity);
	configuration->set_request_revenue(60);
	configuration->set_min_plan_revenue(parameters.min_plan_revenue);
	configuration->set_max_pickup_delay(parameters.max_pickup_delay);
	if(parameters.online) {
		configuration->set_online_mode(Online_mode{.on = true, .batch_period = 30});
	}
	return std::make_shared<DARP_instance<Cordeau_node>>(
		std::move(requests), std::make_unique<std::vector<Vehicle<Cordeau_node>>>(), travel_time_provider,
		configuration);
}

std::unique_ptr<Solution<Cordeau_node>> solve_ih(
	const DARP_instance<Cordeau_node>& instance,
	std::optional<cost_type> new_plan_penalty = std::nullopt,
	bool repair = false
) {
	const auto solver_config = fc::load<DARP_benchmark_config>();
	Insertion_heuristic_solver<Cordeau_node> solver{instance, solver_config, fs::path{}};
	solver.set_new_plan_penalty(new_plan_penalty);
	solver.set_min_plan_revenue_repair(repair);
	return solver.solve();
}

std::vector<unsigned> plan_lengths(const Solution<Cordeau_node>& solution) {
	std::vector<unsigned> lengths;
	for(const auto& plan: solution.get_plans()) {
		lengths.push_back(static_cast<unsigned>(plan.get_length()));
	}
	std::ranges::sort(lengths);
	return lengths;
}

}

TEST(Insertion_heuristic_fleet_sizing_test, vehicles_start_at_their_first_pickup) {
	const auto instance = fleet_sizing_instance({{10, 20}, {20, 30}});
	const auto solution = solve_ih(*instance);

	ASSERT_EQ(solution->get_dropped_request_count(), 0u);
	ASSERT_EQ(solution->get_plans().size(), 1u);
	const auto& plan = solution->get_plans()[0];
	EXPECT_EQ(plan.get_vehicle().get_init_position().get_index(), 10u);
	EXPECT_EQ(plan.get_departure_time(), 0u);
	// no first leg: 10 -> 20 -> 30
	EXPECT_DOUBLE_EQ(solution->get_cost(), 20.0);
}

TEST(Insertion_heuristic_fleet_sizing_test, vehicle_starts_at_the_first_action_of_its_plan) {
	// the vehicle is created at 20 for the first request, the second request 10 -> 30 is picked up before it
	const auto instance = fleet_sizing_instance({{20, 30}, {10, 30}});
	const auto solution = solve_ih(*instance);

	ASSERT_EQ(solution->get_plans().size(), 1u);
	const auto& plan = solution->get_plans()[0];
	EXPECT_EQ(plan.get_first_action().get_node().get_index(), 10u);
	EXPECT_EQ(plan.get_vehicle().get_init_position().get_index(), 10u);
	EXPECT_EQ(plan.get_departure_time(), plan.get_first_action().get_arrival_time());
	// no first leg: 10 -> 20 -> 30
	EXPECT_DOUBLE_EQ(solution->get_cost(), 20.0);
}

TEST(Insertion_heuristic_fleet_sizing_test, new_vehicle_when_no_plan_can_take_the_request) {
	// one vehicle would need 0 -> 100 -> 200 -> 300 = 300 s
	const auto instance = fleet_sizing_instance({{0, 100}, {200, 300}}, {.max_route_duration = 150});
	const auto solution = solve_ih(*instance);

	EXPECT_EQ(solution->get_dropped_request_count(), 0u);
	EXPECT_EQ(solution->get_plans().size(), 2u);
	EXPECT_DOUBLE_EQ(solution->get_cost(), 200.0);
}

TEST(Insertion_heuristic_fleet_sizing_test, capacity_from_the_instance) {
	// together 0 -> 1 -> 100 -> 101, one by one 0 -> 100 -> 1 -> 101
	const auto shared = solve_ih(*fleet_sizing_instance({{0, 100}, {1, 101}}, {.capacity = 2}));
	EXPECT_DOUBLE_EQ(shared->get_cost(), 101.0);

	const auto single = solve_ih(*fleet_sizing_instance({{0, 100}, {1, 101}}, {.capacity = 1}));
	ASSERT_EQ(single->get_plans().size(), 1u);
	EXPECT_DOUBLE_EQ(single->get_cost(), 299.0);
}

TEST(Insertion_heuristic_fleet_sizing_test, new_plan_penalty) {
	const auto instance = fleet_sizing_instance({{0, 10}, {1000, 1010}});

	// without a penalty, the far request is inserted, as it fits
	EXPECT_EQ(solve_ih(*instance)->get_plans().size(), 1u);

	// its own plan costs 10, the insertion 1000
	const auto cheap_new_plan = solve_ih(*instance, 0);
	EXPECT_EQ(cheap_new_plan->get_plans().size(), 2u);
	EXPECT_DOUBLE_EQ(cheap_new_plan->get_cost(), 20.0);

	EXPECT_EQ(solve_ih(*instance, 2000)->get_plans().size(), 1u);
}

TEST(Insertion_heuristic_fleet_sizing_test, repair_dissolves_short_plans) {
	// the far request gets its own plan (penalty 0), which earns 60 < 120
	const auto instance = fleet_sizing_instance({{0, 10}, {1, 11}, {1000, 1010}}, {.min_plan_revenue = 120});
	EXPECT_EQ(plan_lengths(*solve_ih(*instance, 0)), (std::vector<unsigned>{2, 4}));

	const auto repaired = solve_ih(*instance, 0, true);
	EXPECT_EQ(repaired->get_dropped_request_count(), 0u);
	EXPECT_EQ(plan_lengths(*repaired), (std::vector<unsigned>{6}));
}

TEST(Insertion_heuristic_fleet_sizing_test, repair_merges_short_plans) {
	// four far apart requests: with the penalty 0, each gets its own plan earning 60 < 120
	const auto instance = fleet_sizing_instance(
		{{0, 10}, {1000, 1010}, {2000, 2010}, {3000, 3010}}, {.min_plan_revenue = 120});
	const auto repaired = solve_ih(*instance, 0, true);
	EXPECT_EQ(repaired->get_dropped_request_count(), 0u);
	for(const auto& plan: repaired->get_plans()) {
		EXPECT_GE(plan.get_length(), 4u);
	}
}

TEST(Insertion_heuristic_fleet_sizing_test, repair_keeps_a_plan_whose_requests_fit_nowhere) {
	const auto instance = fleet_sizing_instance(
		{{0, 10}, {1, 11}, {1000, 1010}}, {.max_route_duration = 100, .min_plan_revenue = 120});
	const auto repaired = solve_ih(*instance, 0, true);
	EXPECT_EQ(repaired->get_dropped_request_count(), 0u);
	EXPECT_EQ(plan_lengths(*repaired), (std::vector<unsigned>{2, 4}));
}

TEST(Min_revenue_IH_solver_test, keeps_the_minimum_plan_revenue) {
	const auto instance = fleet_sizing_instance(
		{{0, 10}, {1, 11}, {1000, 1010}, {1001, 1011}, {2000, 2010}}, {.min_plan_revenue = 120});
	auto solver_config = fc::load<DARP_benchmark_config>();
	solver_config.min_revenue_ih.new_plan_penalty_step = 100;
	solver_config.min_revenue_ih.new_plan_penalty_count = 3;

	Min_revenue_IH_solver<Cordeau_node> solver{*instance, solver_config, fs::path{}};
	const auto solution = solver.solve();

	EXPECT_EQ(solution->get_dropped_request_count(), 0u);
	ASSERT_FALSE(solution->get_plans().empty());
	for(const auto& plan: solution->get_plans()) {
		EXPECT_GE(plan.get_length(), 4u);
	}
}

TEST(Min_revenue_IH_solver_test, registry_refuses_solvers_without_the_minimum_plan_revenue) {
	auto& registry = DARP::Default_solver_registry::get();
	registry.register_solver<Insertion_heuristic_solver>("ih");
	registry.register_solver<Min_revenue_IH_solver>("min_revenue_ih");
	const auto solver_config = fc::load<DARP_benchmark_config>();

	const auto instance = fleet_sizing_instance({{0, 10}, {1, 11}}, {.min_plan_revenue = 120});
	EXPECT_THROW((void) registry.create_solver("ih", *instance, solver_config, fs::path{}), std::runtime_error);
	EXPECT_NO_THROW((void) registry.create_solver("min_revenue_ih", *instance, solver_config, fs::path{}));

	const auto without_minimum = fleet_sizing_instance({{0, 10}, {1, 11}});
	EXPECT_NO_THROW((void) registry.create_solver("ih", *without_minimum, solver_config, fs::path{}));
}

TEST(Min_revenue_IH_solver_test, registry_checks_the_pickup_delay_and_online_mode_support) {
	auto& registry = DARP::Default_solver_registry::get();
	registry.register_solver<Insertion_heuristic_solver>("ih");
	registry.register_solver<Min_revenue_IH_solver>("min_revenue_ih");
	const auto solver_config = fc::load<DARP_benchmark_config>();

	// the insertion heuristics check the pickup and the drop-off windows separately
	const auto with_pickup_delay = fleet_sizing_instance({{0, 10}, {1, 11}}, {.max_pickup_delay = 120});
	EXPECT_NO_THROW((void) registry.create_solver("ih", *with_pickup_delay, solver_config, fs::path{}));
	EXPECT_NO_THROW((void) registry.create_solver("min_revenue_ih", *with_pickup_delay, solver_config, fs::path{}));

	// no offline solver accepts an online instance
	const auto online = fleet_sizing_instance({{0, 10}, {1, 11}}, {.online = true});
	EXPECT_THROW((void) registry.create_solver("ih", *online, solver_config, fs::path{}), std::runtime_error);
	EXPECT_THROW((void) registry.create_solver("min_revenue_ih", *online, solver_config, fs::path{}), std::runtime_error);
}

TEST(Min_revenue_IH_solver_test, fleet_sizing_needs_the_vehicle_capacity) {
	const auto instance = fleet_sizing_instance({{0, 10}}, {.capacity = 0});
	const auto solver_config = fc::load<DARP_benchmark_config>();
	EXPECT_THROW(
		(Insertion_heuristic_solver<Cordeau_node>{*instance, solver_config, fs::path{}}), std::runtime_error);
}

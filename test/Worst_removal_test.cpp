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
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <future-config/configuration.h>

#include "gtest_wrapper.h"

#include "../src/Cordeau_benchmark.h"
#include "../src/DARP_instance.h"
#include "../src/cost/Cost_evaluator.h"
#include "../src/plan/VehiclePlan.h"
#include "../src/solver/DARP_context.h"
#include "../src/solver/operators/Worst_removal_operator.h"
#include "../src/solver/worst_removal/Worst_removal_solver.h"
#include "../src/config/DARP-benchmark_config.h"
#include "../src/travel_time_provider/Euclidean_travel_time_provider.h"

namespace {

using Plan = VehiclePlan<Cordeau_node>;

/**
 * Requests on a line with one second per coordinate unit; nodes with the same coordinate share their index, so
 * actions at one coordinate form a stop. The vehicle starts at 0.
 */
struct Line {
	std::shared_ptr<Euclidean_travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>(static_cast<unsigned short>(1));
	std::vector<Request<Cordeau_node>> requests;
	std::vector<Vehicle<Cordeau_node>> vehicles;

	Line() {
		// the actions of the plans refer to the requests, which must not move
		requests.reserve(16);
		vehicles.emplace_back(0u, node(0), static_cast<unsigned short>(70));
	}

	static std::shared_ptr<Cordeau_node> node(unsigned x) {
		return std::make_shared<Cordeau_node>(static_cast<float>(x), 0.f, x);
	}

	const Request<Cordeau_node>& add_request(
		unsigned from, unsigned to, unsigned pickup_min_time = 0, unsigned drop_off_min_time = 0,
		unsigned short service_time = 0
	) {
		const auto index = static_cast<unsigned>(requests.size());
		const unsigned short min_travel_time = static_cast<unsigned short>(to > from ? to - from : from - to);
		return requests.emplace_back(
			index, 2 * index, 2 * index + 1, node(from), pickup_min_time, 10'000u, node(to), drop_off_min_time,
			10'000u, min_travel_time, service_time, service_time);
	}

	/** The plan of the actions (request index, pickup) with the earliest schedule from the departure at 0. */
	[[nodiscard]] Plan make_plan(
		const std::vector<std::pair<unsigned, bool>>& sequence, const DARP_instance_configuration& configuration
	) const {
		Plan plan{vehicles[0], static_cast<unsigned short>(sequence.size())};
		const Cordeau_node* previous = &vehicles[0].get_init_position();
		unsigned time = 0;
		for(const auto& [request_index, pickup]: sequence) {
			const Request<Cordeau_node>& request = requests[request_index];
			const Action<Cordeau_node>& action = pickup ? request.get_pickup() : request.get_dropoff();
			ActionData<Cordeau_node>& action_data = plan.add_action(action);
			time += travel_time_provider->get_travel_time(*previous, action.get_node());
			action_data.set_arrival_time(time);
			time = std::max(time, action.get_min_time()) + action.get_service_duration();
			action_data.set_departure_time(time);
			previous = &action.get_node();
		}
		if(configuration.is_return_to_depot()) {
			time += travel_time_provider->get_travel_time(*previous, vehicles[0].get_init_position());
		}
		plan.set_departure_time(0);
		plan.set_arrival_time(time);
		plan.set_cost(evaluate(plan, configuration));
		return plan;
	}

	[[nodiscard]] cost_type evaluate(const Plan& plan, const DARP_instance_configuration& configuration) const {
		return Cost_evaluator::from_configuration(configuration)
			.evaluate<Cordeau_node>(plan, *travel_time_provider, configuration).total;
	}

	[[nodiscard]] DARP_context<Cordeau_node> context(const DARP_instance_configuration& configuration) const {
		return DARP_context<Cordeau_node>{
			travel_time_provider, std::make_shared<DARP_instance_configuration>(configuration)};
	}
};

DARP_instance_configuration configuration(
	unsigned long max_route_duration = 0, unsigned long max_ride_time = 0, bool return_to_depot = false,
	Cost_weights weights = {}
) {
	return DARP_instance_configuration{max_route_duration, max_ride_time, return_to_depot, false, 0, weights};
}

/** Requests 10 -> 20, 20 -> 30 and the far 30 -> 200, served in this order: travel time 200. */
std::vector<std::pair<unsigned, bool>> far_request_line(Line& line) {
	line.add_request(10, 20);
	line.add_request(20, 30);
	line.add_request(30, 200);
	return {{0, true}, {0, false}, {1, true}, {1, false}, {2, true}, {2, false}};
}

const Worst_removal_operator<Cordeau_node>::Candidate* find_candidate(
	const Worst_removal_operator<Cordeau_node>& removal, plan_size_type first, plan_size_type length
) {
	for(const auto& candidate: removal.get_candidates(0)) {
		if(candidate.first == first && candidate.length == length) {
			return &candidate;
		}
	}
	return nullptr;
}

}

TEST(Worst_removal_test, saving_equals_evaluated_cost_difference) {
	Line line;
	const auto config = configuration();
	std::vector<Plan> plans{line.make_plan(far_request_line(line), config)};
	const Plan without_far_request = line.make_plan({{0, true}, {0, false}, {1, true}, {1, false}}, config);
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};

	removal.init(plans);

	const auto* candidate = find_candidate(removal, 4, 1);
	ASSERT_NE(candidate, nullptr);
	EXPECT_DOUBLE_EQ(candidate->saving, line.evaluate(plans[0], config) - line.evaluate(without_far_request, config));
	EXPECT_DOUBLE_EQ(candidate->saving, 170.0);
	EXPECT_DOUBLE_EQ(removal.get_cost(), 200.0);
	EXPECT_EQ(removal.get_served_request_count(), 3u);
}

TEST(Worst_removal_test, remove_while_improving_drops_the_far_request) {
	Line line;
	const auto config = configuration();
	std::vector<Plan> plans{line.make_plan(far_request_line(line), config)};
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};
	removal.init(plans);
	std::vector<const Request<Cordeau_node>*> removed;

	// average 200 / 3; without the far request 30 / 2, further removals would increase the average
	EXPECT_EQ(removal.remove_while_improving(plans, removed), 1u);

	ASSERT_EQ(removed.size(), 1u);
	EXPECT_EQ(removed[0]->get_index(), 2u);
	EXPECT_EQ(plans[0].get_length(), 4u);
	EXPECT_DOUBLE_EQ(removal.get_cost(), 30.0);
	EXPECT_DOUBLE_EQ(plans[0].get_cost(), 30.0);
	EXPECT_EQ(plans[0].get_arrival_time(), 30u);
	EXPECT_TRUE(plans[0].full_check(config, *line.travel_time_provider, false, false));
}

TEST(Worst_removal_test, threshold_compares_the_saving_per_request) {
	Line line;
	const auto config = configuration();
	const auto sequence = far_request_line(line);
	const auto context = line.context(config);

	// only the far request saves more than 100
	std::vector<Plan> plans{line.make_plan(sequence, config)};
	Worst_removal_operator<Cordeau_node> removal{context};
	removal.init(plans);
	std::vector<const Request<Cordeau_node>*> removed;
	EXPECT_EQ(removal.remove_while_improving(plans, removed, 100), 1u);

	// with 5, the far request (170), then the second (10) and the first (20) are removed
	std::vector<Plan> other_plans{line.make_plan(sequence, config)};
	removal.init(other_plans);
	removed.clear();
	EXPECT_EQ(removal.remove_while_improving(other_plans, removed, 5), 3u);
	EXPECT_EQ(other_plans[0].get_length(), 0u);
	EXPECT_DOUBLE_EQ(removal.get_cost(), 0.0);
}

TEST(Worst_removal_test, remove_worst_removes_the_largest_savings) {
	Line line;
	const auto config = configuration();
	std::vector<Plan> plans{line.make_plan(far_request_line(line), config)};
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};
	removal.init(plans);
	std::vector<const Request<Cordeau_node>*> removed;

	EXPECT_EQ(removal.remove_worst(plans, 2, removed), 2u);

	// the far request saves 170; then the second request saves 10, the first 0
	ASSERT_EQ(removed.size(), 2u);
	EXPECT_EQ(removed[0]->get_index(), 2u);
	EXPECT_EQ(removed[1]->get_index(), 1u);
	EXPECT_TRUE(plans[0].full_check(config, *line.travel_time_provider, false, false));
}

TEST(Worst_removal_test, stop_removal) {
	Line line;
	const auto config = configuration();
	// a near request and three requests picked up at the box at 50, delivered at 51
	line.add_request(1, 2);
	line.add_request(50, 51);
	line.add_request(50, 51);
	line.add_request(50, 51);
	const std::vector<std::pair<unsigned, bool>> sequence{
		{0, true}, {0, false}, {1, true}, {2, true}, {3, true}, {1, false}, {2, false}, {3, false}};
	const auto context = line.context(config);
	std::vector<const Request<Cordeau_node>*> removed;

	// a single request at the box saves nothing, the whole stop saves the trip to it (49 of 51)
	std::vector<Plan> plans{line.make_plan(sequence, config)};
	Worst_removal_operator<Cordeau_node> removal{context};
	removal.init(plans);
	const auto* stop = find_candidate(removal, 2, 3);
	ASSERT_NE(stop, nullptr);
	EXPECT_DOUBLE_EQ(stop->saving, 49.0);
	EXPECT_EQ(removal.remove_while_improving(plans, removed), 3u);
	EXPECT_EQ(plans[0].get_length(), 2u);
	EXPECT_TRUE(plans[0].full_check(config, *line.travel_time_provider, false, false));

	// without the stops, no single removal decreases the average
	std::vector<Plan> single_plans{line.make_plan(sequence, config)};
	Worst_removal_operator<Cordeau_node> single_removal{context, false};
	single_removal.init(single_plans);
	removed.clear();
	EXPECT_EQ(single_removal.remove_while_improving(single_plans, removed), 0u);
}

TEST(Worst_removal_test, plan_duration_and_return_to_depot) {
	Line line;
	Cost_weights weights;
	weights.travel_time_weight = 0;
	weights.plan_duration_weight = 1;
	const auto config = configuration(0, 0, true, weights);
	std::vector<Plan> plans{line.make_plan(far_request_line(line), config)};
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};

	removal.init(plans);

	// the plan ends with the return from 200 (400 s), without the far request with the return from 30 (60 s)
	const auto* candidate = find_candidate(removal, 4, 1);
	ASSERT_NE(candidate, nullptr);
	EXPECT_DOUBLE_EQ(candidate->saving, 340.0);
}

TEST(Worst_removal_test, removing_the_first_request_keeps_the_departure_no_earlier) {
	Line line;
	Cost_weights weights;
	weights.travel_time_weight = 0;
	weights.plan_duration_weight = 1;
	const auto config = configuration(0, 0, false, weights);
	// the second request can be picked up at 100: the plan departs at 0 and waits at 30
	line.add_request(10, 20);
	line.add_request(30, 40, 100);
	std::vector<Plan> plans{line.make_plan({{0, true}, {0, false}, {1, true}, {1, false}}, config)};
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};

	removal.init(plans);

	// without the first request, the vehicle departs at 70 to arrive at 100: duration 40 instead of 110
	const auto* candidate = find_candidate(removal, 0, 1);
	ASSERT_NE(candidate, nullptr);
	EXPECT_DOUBLE_EQ(candidate->saving, 70.0);
}

TEST(Worst_removal_test, candidate_breaking_max_ride_time_is_skipped) {
	Line line;
	// max ride time 14: the second request rides from 10 (picked up at 16) to its drop-off at 30 (min time)
	const auto config = configuration(0, 14);
	line.add_request(5, 2);
	line.add_request(10, 20, 0, 30);
	std::vector<Plan> plans{line.make_plan({{0, true}, {0, false}, {1, true}, {1, false}}, config)};
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};

	removal.init(plans);

	// without the first request, the second is picked up at 10 and would ride 20 s
	EXPECT_EQ(find_candidate(removal, 0, 1), nullptr);
	EXPECT_NE(find_candidate(removal, 2, 1), nullptr);
}

TEST(Worst_removal_test, buffers_are_reused) {
	Line line;
	const auto config = configuration();
	std::vector<Plan> plans{line.make_plan(far_request_line(line), config)};
	const auto context = line.context(config);
	Worst_removal_operator<Cordeau_node> removal{context};
	removal.init(plans);
	const auto capacity = removal.get_candidates(0).capacity();
	const auto* candidates_data = removal.get_candidates(0).data();
	std::vector<const Request<Cordeau_node>*> removed;
	removed.reserve(3);

	removal.remove_worst(plans, 3, removed);

	EXPECT_EQ(removal.get_candidates(0).capacity(), capacity);
	EXPECT_EQ(removal.get_candidates(0).data(), candidates_data);
	EXPECT_EQ(plans[0].get_length(), 0u);
}

TEST(Worst_removal_test, plan_remove_actions_keeps_references) {
	Line line;
	const auto config = configuration();
	// the first request around the second one: P0 P1 D1 D0, then P2 D2
	line.add_request(10, 40);
	line.add_request(20, 30);
	line.add_request(50, 60);
	Plan plan = line.make_plan({{0, true}, {1, true}, {1, false}, {0, false}, {2, true}, {2, false}}, config);
	std::vector<char> removed(plan.get_length(), 0);
	removed[1] = 1;
	removed[2] = 1;
	const auto capacity = plan.get_actions().capacity();

	plan.remove_actions(removed);

	ASSERT_EQ(plan.get_length(), 4u);
	EXPECT_EQ(plan.get_actions().capacity(), capacity);
	EXPECT_EQ(plan[0].get_request_index(), 0u);
	EXPECT_EQ(plan[1].get_request_index(), 0u);
	EXPECT_EQ(plan[0].get_other_action_data_index(), 1);
	EXPECT_EQ(plan[1].get_other_action_data_index(), 0);
	EXPECT_EQ(plan[2].get_other_action_data_index(), 3);
	EXPECT_EQ(plan[3].get_other_action_data_index(), 2);
	EXPECT_EQ(plan.get_pickup_index(plan[3]), 2);
	EXPECT_EQ(&plan.get_other(plan[1]), &plan[0]);
	EXPECT_TRUE(plan.contains_pickup(dynamic_cast<const Service_action<Cordeau_node>&>(plan[3].get_action())));
	EXPECT_FALSE(plan.contains_pickup(line.requests[1].get_dropoff()));
}

namespace {

/** Instance with the requests 10 -> 20, 20 -> 30 and the far 30 -> 200 and one vehicle at 0. */
std::shared_ptr<DARP_instance<Cordeau_node>> far_request_instance() {
	auto travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>(static_cast<unsigned short>(1));
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
	requests->reserve(3);
	vehicles->emplace_back(0u, Line::node(0), static_cast<unsigned short>(10));
	const std::vector<std::pair<unsigned, unsigned>> trips{{10, 20}, {20, 30}, {30, 200}};
	for(unsigned index = 0; index < trips.size(); ++index) {
		const auto [from, to] = trips[index];
		requests->emplace_back(
			index, 2 * index, 2 * index + 1, Line::node(from), 0u, 10'000u, Line::node(to), 0u, 10'000u,
			static_cast<unsigned short>(to - from));
	}
	return std::make_shared<DARP_instance<Cordeau_node>>(
		std::move(requests), std::move(vehicles), travel_time_provider,
		std::make_shared<DARP_instance_configuration>());
}

void expect_far_request_dropped(const Solution<Cordeau_node>& solution) {
	ASSERT_TRUE(solution.is_feasible());
	ASSERT_EQ(solution.get_dropped_requests().size(), 1u);
	EXPECT_EQ(solution.get_dropped_requests()[0]->get_index(), 2u);
	ASSERT_EQ(solution.get_plans().size(), 1u);
	EXPECT_EQ(solution.get_plans()[0].get_length(), 4u);
	EXPECT_DOUBLE_EQ(solution.get_cost(), 30.0);
}

}

TEST(Worst_removal_solver_test, drops_the_far_request_of_the_insertion_heuristic_solution) {
	const auto instance = far_request_instance();
	auto solver_config = fc::load<DARP_benchmark_config>();

	Worst_removal_solver<Cordeau_node> solver{*instance, solver_config, fs::path{}};
	const auto solution = solver.solve();

	expect_far_request_dropped(*solution);
}

TEST(Worst_removal_solver_test, initial_solution_from_file) {
	const auto instance = far_request_instance();
	auto solver_config = fc::load<DARP_benchmark_config>();
	Insertion_heuristic_solver<Cordeau_node> ih_solver{*instance, solver_config, fs::path{}};
	const auto initial_solution = ih_solver.solve();
	ASSERT_EQ(initial_solution->get_dropped_requests().size(), 0u);
	const fs::path initial_solution_path = fs::temp_directory_path() / "worst_removal_initial_solution.json";
	{
		std::ofstream file{initial_solution_path};
		file << initial_solution->JSON_serialize().GetString();
	}
	solver_config.worst_removal.initial_solution = initial_solution_path.string();

	Worst_removal_solver<Cordeau_node> solver{*instance, solver_config, fs::path{}};
	const auto solution = solver.solve();
	fs::remove(initial_solution_path);

	expect_far_request_dropped(*solution);
}

TEST(Worst_removal_solver_test, threshold_keeps_requests_saving_less) {
	const auto instance = far_request_instance();
	auto solver_config = fc::load<DARP_benchmark_config>();
	solver_config.worst_removal.drop_threshold = 200;

	Worst_removal_solver<Cordeau_node> solver{*instance, solver_config, fs::path{}};
	const auto solution = solver.solve();

	EXPECT_EQ(solution->get_dropped_requests().size(), 0u);
}

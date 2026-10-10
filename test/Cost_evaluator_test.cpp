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
#include <vector>

#include "gtest_wrapper.h"

#include "../src/Cordeau_benchmark.h"
#include "../src/DARP_instance.h"
#include "../src/Solution.h"
#include "../src/cost/Cost_evaluator.h"
#include "../src/plan/VehiclePlan.h"
#include "../src/travel_time_provider/Euclidean_travel_time_provider.h"

namespace {

/**
 * Instance on a line with one second per coordinate unit: depot at 0, request A 10 -> 30 (desired at 10), request B
 * 40 -> 60 (desired at 35, so its direct ride would arrive at 55). The plan serves both requests in order without
 * waiting: legs 10 + 20 + 10 + 20 = 60 s, drop-off delays 0 + 5 = 5 s, return leg 60 s.
 */
struct Line_instance {
	std::shared_ptr<Euclidean_travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>(static_cast<unsigned short>(1));
	std::shared_ptr<Cordeau_node> depot = std::make_shared<Cordeau_node>(0.f, 0.f, 0u);
	std::shared_ptr<Cordeau_node> a_pickup = std::make_shared<Cordeau_node>(10.f, 0.f, 1u);
	std::shared_ptr<Cordeau_node> a_dropoff = std::make_shared<Cordeau_node>(30.f, 0.f, 2u);
	std::shared_ptr<Cordeau_node> b_pickup = std::make_shared<Cordeau_node>(40.f, 0.f, 3u);
	std::shared_ptr<Cordeau_node> b_dropoff = std::make_shared<Cordeau_node>(60.f, 0.f, 4u);

	std::unique_ptr<std::vector<Request<Cordeau_node>>> requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
	std::unique_ptr<std::vector<Vehicle<Cordeau_node>>> vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();

	Line_instance() {
		requests->emplace_back(0u, 0u, 1u, a_pickup, 10u, 100u, a_dropoff, 30u, 200u, static_cast<unsigned short>(20));
		requests->emplace_back(1u, 2u, 3u, b_pickup, 35u, 100u, b_dropoff, 55u, 200u, static_cast<unsigned short>(20));
		vehicles->emplace_back(0u, depot, static_cast<unsigned short>(4));
	}

	/** The plan serving A then B without waiting; its stored cost is \p stored_cost. */
	[[nodiscard]] VehiclePlan<Cordeau_node> make_plan(cost_type stored_cost, unsigned b_pickup_arrival = 40) const {
		VehiclePlan<Cordeau_node> plan{vehicles->at(0), 4};
		const auto add = [&plan](const Action<Cordeau_node>& action, unsigned arrival, unsigned departure) {
			ActionData<Cordeau_node>& action_data = plan.add_action(action);
			action_data.set_arrival_time(arrival);
			action_data.set_departure_time(departure);
		};
		add(requests->at(0).get_pickup(), 10, 10);
		add(requests->at(0).get_dropoff(), 30, 30);
		add(requests->at(1).get_pickup(), b_pickup_arrival, b_pickup_arrival);
		add(requests->at(1).get_dropoff(), b_pickup_arrival + 20, b_pickup_arrival + 20);
		plan.set_departure_time(0);
		plan.set_arrival_time(b_pickup_arrival + 20);
		plan.set_cost(stored_cost);
		return plan;
	}

	[[nodiscard]] DARP_instance<Cordeau_node> make_instance(const DARP_instance_configuration& configuration) {
		return DARP_instance<Cordeau_node>{
			std::move(requests), std::move(vehicles), travel_time_provider,
			std::make_shared<DARP_instance_configuration>(configuration)};
	}
};

DARP_instance_configuration configuration_with(Cost_weights weights, bool return_to_depot = false,
	problem_type problem = problem_type::darp) {
	return DARP_instance_configuration{0, 0, return_to_depot, false, 0, weights, problem};
}

}

TEST(Cost_evaluator_test, default_weights_equal_travel_time) {
	Line_instance instance;
	const auto configuration = configuration_with({});
	const Cost_evaluator evaluator = Cost_evaluator::from_configuration(configuration);
	const VehiclePlan<Cordeau_node> plan = instance.make_plan(0);

	const Cost_breakdown breakdown = evaluator.evaluate<Cordeau_node>(plan, *instance.travel_time_provider, configuration);

	EXPECT_DOUBLE_EQ(breakdown.travel_time, 60.0);
	EXPECT_DOUBLE_EQ(breakdown.passenger_delay, 5.0);
	EXPECT_DOUBLE_EQ(breakdown.plan_constant, 0.0);
	EXPECT_DOUBLE_EQ(breakdown.total, 60.0);
}

TEST(Cost_evaluator_test, weighted_components) {
	Line_instance instance;
	const auto configuration = configuration_with({1.0, 0.5, 10.0});
	const Cost_evaluator evaluator = Cost_evaluator::from_configuration(configuration);
	const VehiclePlan<Cordeau_node> plan = instance.make_plan(0);

	const Cost_breakdown breakdown = evaluator.evaluate<Cordeau_node>(plan, *instance.travel_time_provider, configuration);

	EXPECT_DOUBLE_EQ(breakdown.plan_constant, 10.0);
	EXPECT_DOUBLE_EQ(breakdown.total, 60.0 + 0.5 * 5.0 + 10.0);

	// the incremental hooks agree with the full evaluation
	const cost_type incremental = evaluator.plan_constant()
		+ evaluator.leg_cost(10) + evaluator.leg_cost(20) + evaluator.leg_cost(10) + evaluator.leg_cost(20)
		+ evaluator.dropoff_cost(30, Cost_evaluator::ideal_dropoff_arrival(instance.requests->at(0)))
		+ evaluator.dropoff_cost(60, Cost_evaluator::ideal_dropoff_arrival(instance.requests->at(1)));
	EXPECT_TRUE(costs_equal(incremental, breakdown.total));
}

TEST(Cost_evaluator_test, empty_plan_has_no_cost) {
	Line_instance instance;
	const auto configuration = configuration_with({1.0, 0.5, 10.0});
	const Cost_evaluator evaluator = Cost_evaluator::from_configuration(configuration);
	const VehiclePlan<Cordeau_node> plan{instance.vehicles->at(0), 4};

	const Cost_breakdown breakdown = evaluator.evaluate<Cordeau_node>(plan, *instance.travel_time_provider, configuration);

	EXPECT_DOUBLE_EQ(breakdown.total, 0.0);
	EXPECT_DOUBLE_EQ(breakdown.plan_constant, 0.0);
}

TEST(Cost_evaluator_test, return_to_depot_leg) {
	Line_instance instance;
	const VehiclePlan<Cordeau_node> plan = instance.make_plan(0);

	const auto darp = configuration_with({}, true, problem_type::darp);
	EXPECT_DOUBLE_EQ(
		Cost_evaluator::from_configuration(darp).evaluate<Cordeau_node>(plan, *instance.travel_time_provider, darp).travel_time,
		120.0);

	// fleet sizing never prices the return leg
	const auto fleet_sizing = configuration_with({}, true, problem_type::fleet_sizing);
	EXPECT_DOUBLE_EQ(
		Cost_evaluator::from_configuration(fleet_sizing).evaluate<Cordeau_node>(plan, *instance.travel_time_provider, fleet_sizing).travel_time,
		60.0);
}

TEST(Cost_evaluator_test, scheduled_leg_longer_than_travel_time_counts_as_driving) {
	Line_instance instance;
	const auto configuration = configuration_with({});
	// the vehicle reaches the pickup of B 5 s later than the direct leg allows (detour of a re-routed vehicle)
	const VehiclePlan<Cordeau_node> plan = instance.make_plan(0, 45);

	const Cost_breakdown breakdown = Cost_evaluator::from_configuration(configuration)
		.evaluate<Cordeau_node>(plan, *instance.travel_time_provider, configuration);

	EXPECT_DOUBLE_EQ(breakdown.travel_time, 65.0);
	EXPECT_DOUBLE_EQ(breakdown.passenger_delay, 10.0);
}

TEST(Cost_evaluator_test, full_check_compares_with_the_evaluated_cost) {
	Line_instance instance;
	const auto configuration = configuration_with({1.0, 0.5, 10.0});

	const VehiclePlan<Cordeau_node> correct = instance.make_plan(72.5);
	EXPECT_TRUE(correct.full_check(configuration, *instance.travel_time_provider, false, false));

	const VehiclePlan<Cordeau_node> wrong = instance.make_plan(60);
	try {
		wrong.full_check(configuration, *instance.travel_time_provider, false, false);
		FAIL() << "full_check accepted a wrong plan cost";
	}
	catch(const std::runtime_error& error) {
		EXPECT_NE(std::string(error.what()).find("computed 72.5"), std::string::npos) << error.what();
	}
}

TEST(Cost_evaluator_test, solution_evaluates_plan_costs) {
	Line_instance instance_data;
	std::vector<VehiclePlan<Cordeau_node>> plans;
	plans.push_back(instance_data.make_plan(12345));
	const DARP_instance<Cordeau_node> instance = instance_data.make_instance(configuration_with({1.0, 0.5, 10.0}));

	const Solution<Cordeau_node> solution{instance, std::move(plans)};

	EXPECT_DOUBLE_EQ(solution.get_plans().at(0).get_cost(), 72.5);
	EXPECT_DOUBLE_EQ(solution.get_cost(), 72.5);
	EXPECT_EQ(solution.get_dropped_request_count(), 0u);
}

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

#include <memory>

#include "gtest_wrapper.h"

#include "../src/travel_time_provider/Travel_time_provider.h"
#include "../src/Cordeau_benchmark.h"
#include "../src/plan/Benchmark_vehicle_plan.h"
#include "../src/plan/VehiclePlan.h"
#include "../src/DARP_benchmark_node.h"
#include "../src/common.h"
#include "../src/solver/IH/IH_vehicle_plan_builder.h"
#include "../src/Solution.h"
#include "../src/resources.h"

namespace fs = std::filesystem;

/*
 * Old test aggregate structures
 */

template<class N>
struct Request_data {

	explicit Request_data(Request<N>& request_par)
		: request(std::move(request_par)),
		  pickup_action_data(request.get_pickup()),
		  drop_off_action_data(request.get_dropoff()) {
	}

	Request<N> request;
	ActionData<N> pickup_action_data;
	ActionData<N> drop_off_action_data;
};

using Request_data_Cordeau_node = Request_data<Cordeau_node>;

template<class N>
class Request_generator {
public:

	explicit Request_generator(
		std::shared_ptr<Travel_time_provider<N>> travel_time_provider_par,
		unsigned short service_time_par
	)
		: travel_time_provider(std::move(travel_time_provider_par)), service_time(service_time_par) {
	}

	unsigned short get_service_time() const {
		return service_time;
	}

	[[nodiscard]] std::shared_ptr<Travel_time_provider<N>> get_travel_time_provider() const {
		return travel_time_provider;
	}

protected:
	const std::shared_ptr<Travel_time_provider<N>> travel_time_provider;
	int request_index{-1};
	int action_id{-2};
	const unsigned short service_time;
};


class Request_generator_cordeau_node : public Request_generator<Cordeau_node> {
public:

	using Request_generator<Cordeau_node>::Request_generator;

	Request<Cordeau_node> generate_request(
		float from_x,
		float from_y,
		unsigned int pickup_min_time,
		unsigned int pickup_max_time,
		float to_x,
		float to_y,
		unsigned int drop_off_min_time,
		unsigned int drop_off_max_time
	) {
		const std::shared_ptr<Cordeau_node> from{new Cordeau_node{from_x, from_y}};
		const std::shared_ptr<Cordeau_node> to{new Cordeau_node{to_x, to_y}};
		const unsigned short min_travel_time = (unsigned short) this->travel_time_provider->get_travel_time(
			*from,
			*to
		);
		++request_index;
		action_id += 2;
		return Request<Cordeau_node>{
			(unsigned int) request_index,
			(unsigned int) action_id,
			(unsigned int) action_id + 1,
			from,
			pickup_min_time,
			pickup_max_time,
			to,
			drop_off_min_time,
			drop_off_max_time,
			min_travel_time,
			service_time,
			service_time
		};
	}

	Request_data<Cordeau_node> get_request_action_data(
		float from_x,
		float from_y,
		unsigned int pickup_min_time,
		unsigned int pickup_max_time,
		float to_x,
		float to_y,
		unsigned int drop_off_min_time,
		unsigned int drop_off_max_time
	) {
		Request<Cordeau_node> request = generate_request(
			from_x, from_y, pickup_min_time,
			pickup_max_time, to_x, to_y, drop_off_min_time, drop_off_max_time
		);
		return Request_data<Cordeau_node>(request);
	}
};


class Request_generator_amodsim_node : public Request_generator<Amodsim_node> {
public:

	using Request_generator<Amodsim_node>::Request_generator;

	Request<Amodsim_node> generate_request(
		unsigned int from_index,
		unsigned int pickup_min_time,
		unsigned int pickup_max_time,
		unsigned int to_index,
		unsigned int drop_off_min_time,
		unsigned int drop_off_max_time
	) {
		const std::shared_ptr<Amodsim_node> from{new Amodsim_node{from_index}};
		const std::shared_ptr<Amodsim_node> to{new Amodsim_node{to_index}};
		const unsigned short min_travel_time = (unsigned short) this->travel_time_provider->get_travel_time(
			*from,
			*to
		);
		++request_index;
		action_id += 2;
		return Request<Amodsim_node>{
			(unsigned int) request_index,
			(unsigned int) action_id,
			(unsigned int) action_id + 1,
			from,
			pickup_min_time,
			pickup_max_time,
			to,
			drop_off_min_time,
			drop_off_max_time,
			min_travel_time,
			service_time,
			service_time
		};
	}

	Request_data<Amodsim_node> get_request_action_data(
		unsigned from_index,
		unsigned int pickup_min_time,
		unsigned int pickup_max_time,
		unsigned to_index,
		unsigned int drop_off_min_time,
		unsigned int drop_off_max_time
	) {
		Request<Amodsim_node> request = generate_request(
			from_index, pickup_min_time,
			pickup_max_time, to_index, drop_off_min_time, drop_off_max_time
		);
		return Request_data<Amodsim_node>(request);
	}
};

static_assert(Pointer_iterable<std::vector<const Request<Cordeau_node>*>, Request<Cordeau_node>>);

template<class P, typename N>
class Test_plan_data {

public:

	[[nodiscard]] const P& get_plan() const {
		return plan;
	}

	[[nodiscard]] const std::vector<Request<N>>& get_requests() const {
		return *requests;
	}

	[[nodiscard]] const std::vector<Vehicle<N>>& get_vehicles() const {
		return *vehicles;
	}


	void set_plan(P&& plan_par) {
		this->plan = plan_par;
	}

	Test_plan_data(
		P plan_par, std::unique_ptr<std::vector<Request<N>>> requests_par,
		std::unique_ptr<std::vector<Vehicle<N>>> vehicles_par
	) :
		plan{std::move(plan_par)}, requests(std::move(requests_par)), vehicles(std::move(vehicles_par)) {}

	[[nodiscard]] std::vector<const Request<N>*> get_request_pointers() const {
		std::vector<const Request<N>*> out;
		out.reserve(requests->size());
		for (const Request<N>& request: *requests) {
			out.push_back(&request);
		}

		return out;
	}

private:
	P plan;
	std::unique_ptr<std::vector<Request<N>>> requests;
	std::unique_ptr<std::vector<Vehicle<N>>> vehicles;
};

template<class N, class R>
class Instance_data {
public:

	Instance_data(
		unsigned service_time,
		const std::shared_ptr<Travel_time_provider<N>> travel_time_provider,
		R&& request_generator
	)
		:
		request_generator(request_generator),
		service_time(service_time),
		travel_time_provider(travel_time_provider) {
	}

	R request_generator;

	unsigned int service_time;

	std::shared_ptr<Travel_time_provider<N>> travel_time_provider;
};



/*
 * New test dummy structures
 */

/**
 * Default action data for testing purposes. Other classes can be used instead if less/more functionality is needed.
 * They should inherit from Action_data_base_template, or Action_data_base if nodes are not needed.
 */
template<class A = Action_base<unsigned>>
class Test_action_data :
	public Action_data_base {
public:
	Test_action_data(unsigned index, Action_type action_type, unsigned long max_time)
		: action(index, 0, max_time, action_type) {
	}

	/**
	 * JSON Deserialization constructor.
	 * @param json_data
	 * @param position_in_plan
	 * @param other_action_data_index
	 */
	Test_action_data(
		const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data,
		index_in_plan position_in_plan,
		index_in_plan other_action_data_index
	)
		: Action_data_base(json_data, position_in_plan, other_action_data_index),
			action(json_data, position_in_plan) {}
//		  action_type(action_type_from_string(json_data["type"].GetString())),
//		  max_time(json_data["max_time"].GetUint()) {}

	explicit Test_action_data(const A& action_par)
		: action(action_par) {}

	[[nodiscard]] unsigned get_node() const {
		return action.get_node();
	}

	[[nodiscard]] unsigned get_index() const {
		return get_node();
	}

	[[nodiscard]] unsigned get_min_time() const {
		return action.get_min_time();
	}

	[[nodiscard]] unsigned get_max_time() const {
		return action.get_max_time();
	}

	[[nodiscard]] Action_type get_action_type() const {
		return action.get_action_type();
	}

	[[nodiscard]] const A& get_action() const {
		return action;
	}

	[[nodiscard]] unsigned short get_service_duration() const {
		return 0;
	}
private:
	A action;
};

template<class A = Test_action_data<>>
struct Test_request {
	A pickup_action_data;
	A drop_off_action_data;

	const A& get_pickup() const;

	const A& get_dropoff() const;
};


/*
 * Functions
 */


template<typename N>
void compare_action_data(const ActionData<N>& computed_action_data, const ActionData<N>& expected_action_data) {
	ASSERT_EQ(computed_action_data.get_action_id(), expected_action_data.get_action_id());
	if (dynamic_cast<const Service_action<N>*>(&computed_action_data.get_action()) != nullptr) {
		EXPECT_EQ(dynamic_cast<const Service_action<N>&>(computed_action_data.get_action()).get_request().get_index(),
			dynamic_cast<const Service_action<N>&>(expected_action_data.get_action()).get_request().get_index());
	}
	EXPECT_EQ(computed_action_data.get_action().get_action_type(), expected_action_data.get_action().get_action_type());
	EXPECT_EQ(computed_action_data.get_arrival_time(), expected_action_data.get_arrival_time());
	EXPECT_EQ(computed_action_data.get_departure_time(), expected_action_data.get_departure_time());
}

/**
 * @brief Checks if two plans are equal
 * @tparam N node type
 * @tparam P plan type
 * @param computed_plan computed plan 
 * @param expected_plan expected plan
*/
template<class N, class P, class A = ActionData<N>>
void check_plans_equal(
	const P& computed_plan,
	const P& expected_plan
) {
	check_plans_equal<N, P, A>(computed_plan, expected_plan, compare_action_data<N>);
}

template<class P>
void check_plan_basics_equal(const P& computed_plan, const P& expected_plan);

/**
 * @brief Checks if two plans are equal
 * @tparam N node type
 * @tparam P plan type
 * @param computed_plan computed plan 
 * @param expected_plan expected plan
*/
template<class N, class P, class A = ActionData<N>>
void check_plans_equal(
	const P& computed_plan,
	const P& expected_plan,
	const std::function<void(const A&, const A&)> action_data_comparator
) {
	ASSERT_EQ(computed_plan.is_feasible(), expected_plan.is_feasible());

	// the plans are compared completely even if infeasible: not all solvers mark their plans as feasible
	check_plan_basics_equal(computed_plan, expected_plan);
	EXPECT_DOUBLE_EQ(computed_plan.get_cost(), expected_plan.get_cost());
	ASSERT_EQ(computed_plan.get_length(), expected_plan.get_length());
	for (unsigned short i = 0; i < computed_plan.get_length(); i++) {
		action_data_comparator(computed_plan[i], expected_plan[i]);
	}
}

template<typename N, class P>
void check_plan_list_equal(const std::vector<P>& computed_plans, const std::vector<P> expected_plans) {
	ASSERT_EQ(computed_plans.size(), expected_plans.size());
	for (unsigned i = 0; i < computed_plans.size(); ++i) {
		check_plans_equal<N>(computed_plans[i], expected_plans[i]);
	}
}

/**
 * @brief Checks if two solutions are equal
 * @tparam N node type
 * @tparam P plan type
 * @param computed_solution computed solution
 * @param expected_solution expected solution
 */
template<typename N, class P = VehiclePlan<N>>
void check_solutions_equal(
	const Solution<N, P>& computed_solution,
	const Solution<N, P>& expected_solution
) {
	ASSERT_EQ(computed_solution.is_feasible(), expected_solution.is_feasible());
	EXPECT_DOUBLE_EQ(computed_solution.get_cost(), expected_solution.get_cost());
	ASSERT_EQ(computed_solution.get_dropped_request_count(), expected_solution.get_dropped_request_count());
	
	// Compare dropped requests by index
	const auto& computed_dropped = computed_solution.get_dropped_requests();
	const auto& expected_dropped = expected_solution.get_dropped_requests();
	for (size_t i = 0; i < computed_dropped.size(); ++i) {
		EXPECT_EQ(computed_dropped[i]->get_index(), expected_dropped[i]->get_index());
	}
	
	// Compare plans
	check_plan_list_equal<N, P>(computed_solution.get_plans(), expected_solution.get_plans());
}

template<class N>
void check_plan_valid(const IH_vehicle_plan_builder<Vehicle<N>, ActionData<N>, VehiclePlan<N>>& vehicle_plan_builder) {
	for (plan_size_type i = 0; i < vehicle_plan_builder.get_action_data_used_length(); ++i) {

		if (vehicle_plan_builder.get_action_order().at(i) >= 0) {
			const ActionData<N>& action_data = vehicle_plan_builder[i];
			EXPECT_EQ(action_data.get_position_in_plan(), i);
		}
	}
}

template<size_t S>
std::unique_ptr<travel_time_type[]> two_D_array_to_flat_array(travel_time_type matrix[][S]) {
	//std::unique_ptr<travel_time_type[]> array_of_pointers_to_array = new unsigned int* [S * S];
	auto flat_array = std::make_unique<travel_time_type[]>(S * S);
	for (unsigned int i = 0; i < S; ++i) {
		for (unsigned int j = 0; j < S; ++j) {
			flat_array[i * S + j] = matrix[i][j];
		}
	}
	return flat_array;
}

std::unique_ptr<travel_time_type[]> load_dm_from_json(const rapidjson::Document& doc);

/**
 * @brief Returns the absolute path of a test resource. The resource is searched in the test resource directories of
 * the downstream projects (CMake variable DARP_BENCHMARK_EXTRA_TEST_RESOURCE_DIRS) first, then in those of this project.
 * @param relative_path path relative to a test resource directory
 */
fs::path get_test_resource_path(const fs::path& relative_path);

[[nodiscard]] DARP_instance<Amodsim_node> load_test_instance_amodsim(const std::string& instance_id);

#include "common.tpp"



//
// Created by david on 2023-09-19.
//

#pragma once

#include "../common.h"
#include "../../src/solver/IH/IH_SVDARP_interfaces.h"
#include "../../src/plan/Base_plan.h"


class Test_vehicle {
public:
	explicit Test_vehicle(unsigned short capacity)
		: capacity(capacity) {
	}

	[[nodiscard]] unsigned short get_capacity() const {
		return capacity;
	}

	[[nodiscard]] const unsigned& get_init_position() const {
		return init_position;
	}

private:
	unsigned short capacity;

	unsigned init_position{0};
};

template<class A = Test_action_data<>>
class IH_SVDARP_test_plan :
	public DARP_benchmark_plan_template<A, Test_vehicle> {
public:
	/**
	 * @brief Construct an empty plan.
	 * @param vehicle vehicle
	 */
	explicit IH_SVDARP_test_plan(const Test_vehicle& vehicle)
		: DARP_benchmark_plan_template<A, Test_vehicle>(vehicle) {}

	/**
	 * @brief Construct a plan with the given actions. Used for manual plan creation in tests.
	 * @param vehicle vehicle
	 * @param actions actions
	 */
	IH_SVDARP_test_plan(const Test_vehicle& vehicle, const std::vector<A>& actions, unsigned cost)
		: DARP_benchmark_plan_template<A, Test_vehicle>(actions, vehicle, cost) {}

	/**
	 * @brief Constructor for vehicle plan builder
	 * @param vehicle
	 * @param cost
	 * @param vehicle_plan_actions
	 * @param departure_time
	 * @param arrival_time
	 */
	IH_SVDARP_test_plan(
		const Test_vehicle& vehicle,
		unsigned cost,
		const std::vector<A>& vehicle_plan_actions,
		time_type departure_time,
		time_type arrival_time
	) :
		DARP_benchmark_plan_template<A, Test_vehicle>(vehicle_plan_actions, vehicle, cost, departure_time, arrival_time) {}

	explicit IH_SVDARP_test_plan(const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data, const Test_vehicle& vehicle);

	[[nodiscard]] const A& get_other(const A&) const { return this->actions[0]; };

	void test_check_equal(const IH_SVDARP_test_plan<A>& other) const {
		check_plan_basics_equal(*this, other);
		EXPECT_EQ(this->get_cost(), other.get_cost());
		ASSERT_EQ(this->get_length(), other.get_length());
		for (unsigned short i = 0; i < this->get_length(); i++) {
			this->operator[](i).test_check_equal(other[i]);
		}
	}

private:
	auto parse_actions_from_json(const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data);
};


#include "common.tpp"


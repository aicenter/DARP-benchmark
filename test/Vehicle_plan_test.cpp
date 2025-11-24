#include "gtest_wrapper.h"

#include "common.h"
#include "Single_vehicle_DARP_common.h"
#include "../src/Cordeau_benchmark.h"

TEST(Vehicle_plan_remove_from_plan, one_action_incomplete_plan) {
	const std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short)60);

	const unsigned short service_time = 10;
	Request_generator_cordeau_node rg{travel_time_provider, service_time};

	// base plan
	Test_plan_data_cordeau_node td = generate_plan_for_removal_with_one_request(rg, service_time);
	VehiclePlan<Cordeau_node> base_plan = td.get_plan();

	// computed plan
	base_plan.remove_last_action(false);

	// expected plan
	VehiclePlan<Cordeau_node> expected_plan{td.get_vehicles().at(0), 1};

	check_plans_equal<Cordeau_node>(base_plan, expected_plan);
}

TEST(Vehicle_plan_remove_from_plan, one_request_complete_plan) {
	const std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
		= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short)60);
	unsigned short service_time = 10;
	Request_generator_cordeau_node rg{travel_time_provider, service_time};

	// base plan
	Test_plan_data_cordeau_node td = generate_complete_plan_for_removal(rg, service_time, *travel_time_provider);
	VehiclePlan<Cordeau_node> base_plan = td.get_plan();

	// computed plan
	base_plan.remove_last_action(true);

	// expected plan
	VehiclePlan<Cordeau_node> expected_plan{td.get_vehicles().at(0), 1};
	ActionData<Cordeau_node>& expected_pickup_action_data = expected_plan.add_action(td.get_requests().at(0).get_pickup());
	expected_pickup_action_data.set_departure_time(service_time);

	check_plans_equal<Cordeau_node>(base_plan, expected_plan);
}


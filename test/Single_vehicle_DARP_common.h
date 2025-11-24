
#pragma once

#include "common.h"

typedef Test_plan_data<VehiclePlan<Cordeau_node>, Cordeau_node> Test_plan_data_cordeau_node;

std::unique_ptr<travel_time_type[]> generate_distance_matrix_for_search_amodsim_node();

Test_plan_data_cordeau_node generate_plan_for_removal_with_one_request(Request_generator_cordeau_node& rg, unsigned short service_time);

Test_plan_data_cordeau_node generate_complete_plan_for_removal(
	Request_generator_cordeau_node& rg, 
	unsigned short service_time,
	const Travel_time_provider<Cordeau_node>& travel_time_provider
);

Test_plan_data_cordeau_node generate_data_for_search(Request_generator_cordeau_node& rg);

Test_plan_data_cordeau_node generate_data_for_search_second_test(Request_generator_cordeau_node& rg);

Test_plan_data_cordeau_node generate_data_for_search_third_test(Request_generator_cordeau_node& rg);

Test_plan_data_cordeau_node generate_data_for_search_three_requests(Request_generator_cordeau_node& rg);

Test_plan_data_cordeau_node generate_data_for_search_three_requests_case_2(Request_generator_cordeau_node& rg);

Test_plan_data<VehiclePlan<Amodsim_node>, Amodsim_node> generate_data_for_search_amodsim_node(Request_generator_amodsim_node& rg);

Test_plan_data_cordeau_node generate_data_for_search_four_requests(Request_generator_cordeau_node& rg);

VehiclePlan<Cordeau_node> generate_expected_plan(
	const Test_plan_data_cordeau_node& td,
	unsigned short service_time,
	const Travel_time_provider<Cordeau_node>& travel_time_provider);

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_second_test(
	const Test_plan_data_cordeau_node& td,
	unsigned short service_time,
	const Travel_time_provider<Cordeau_node>& travel_time_provider);

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_third_test(const Test_plan_data_cordeau_node& td);

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_three_requests(
	const Test_plan_data_cordeau_node& td
);

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_three_requests_case_2(
	const Test_plan_data_cordeau_node& td
);

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_four_requests(
	const Test_plan_data_cordeau_node& td
);

VehiclePlan<Amodsim_node> generate_expected_plan_amodsim_node(
	const Test_plan_data<VehiclePlan<Amodsim_node>, Amodsim_node>& td,
	unsigned short service_time,
	const Travel_time_provider<Amodsim_node>& travel_time_provider
);




#include "common.h"
#include "../src/plan/VehiclePlan.h"
#include "Single_vehicle_DARP_common.h"
#include "../src/DARP_benchmark_node.h"

typedef Test_plan_data<VehiclePlan<Cordeau_node>, Cordeau_node> Test_plan_data_cordeau_node;



std::unique_ptr<travel_time_type[]> generate_distance_matrix_for_search_amodsim_node() {
	//unsigned int* first_row = {0, 5663, 4294967295};
	//unsigned int second_row[3] = {5666, 0, 4294967295};
	//unsigned int third_row[3] = {2433, 5849, 0 };
	
	//return new unsigned int* [3] {
	//	(unsigned int[3]) {0, 5663, 4294967295},
	//	(unsigned int[3]) {5666, 0, 4294967295},
	//	(unsigned int[3]) {2433, 5849, 0}
	//};
	travel_time_type matrix[3][3] = {
		{0, 5663, 39000},
		{5666, 0, 39000},
		{2433, 5849, 0}
	};

	return two_D_array_to_flat_array(matrix);
}

Test_plan_data_cordeau_node generate_plan_for_removal_with_one_request(Request_generator_cordeau_node& rg, const unsigned short service_time) {
	const std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short) 1);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	requests->push_back(rg.generate_request(0, 0, 0, 1000, 1,
		1, 15, 2000));

	// base plan
	VehiclePlan<Cordeau_node> base_plan{vehicle, 1};
	ActionData<Cordeau_node>& pickup_action_data = base_plan.add_action(requests->back().get_pickup());
	pickup_action_data.set_departure_time(service_time);

	return Test_plan_data_cordeau_node{base_plan, std::move(requests), std::move(vehicles)};
}

Test_plan_data_cordeau_node generate_complete_plan_for_removal(
	Request_generator_cordeau_node& rg, 
	unsigned short service_time,
	const Travel_time_provider<Cordeau_node>& travel_time_provider
) {
	const std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)1);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	requests->push_back(rg.generate_request(0, 0, 0, 1000, 1,
		1, 15, 2000));
	const Request<Cordeau_node>& r = requests->back();

	// base plan
	VehiclePlan<Cordeau_node> base_plan{vehicle, 1};
	ActionData<Cordeau_node>& pickup_action_data = base_plan.add_action(r.get_pickup());
	pickup_action_data.set_departure_time(service_time);
	ActionData<Cordeau_node>& drop_off_action_data = base_plan.add_action(r.get_dropoff());
	const unsigned int travel_time = travel_time_provider.get_travel_time(r.get_pickup().get_node(),
		r.get_dropoff().get_node());
	drop_off_action_data.set_arrival_time(travel_time + service_time);
	drop_off_action_data.set_departure_time(travel_time + service_time * 2);
	const unsigned int travel_time_to_depot = travel_time_provider.get_travel_time(r.get_dropoff().get_node(),
		*vehicle_init_position);
	base_plan.set_cost(travel_time + travel_time_to_depot);
	base_plan.set_arrival_time(travel_time + travel_time_to_depot + 2 * service_time);
	
	return Test_plan_data_cordeau_node{base_plan, std::move(requests), std::move(vehicles)};
}

Test_plan_data_cordeau_node generate_data_for_search(Request_generator_cordeau_node& rg) {

	const std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)1);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	requests->push_back(rg.generate_request(0, 0, 0, 1000, 1,
		1, 15, 2000));

	const VehiclePlan<Cordeau_node> base_plan{vehicle, 1};

	return Test_plan_data_cordeau_node{base_plan, std::move(requests), std::move(vehicles)};
}

Test_plan_data_cordeau_node generate_data_for_search_second_test(Request_generator_cordeau_node& rg) {
	const std::shared_ptr<Cordeau_node> vehicle_init_position{new Cordeau_node{0, 0}};
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)2);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	requests->push_back(rg.generate_request(0, 0, 0, 1000, 1,
		1, 15, 2000));
	requests->push_back(rg.generate_request(0, 1, 0, 1000, 1,
		0, 15, 2000));

	const VehiclePlan<Cordeau_node> base_plan{vehicle, 1};

	return Test_plan_data_cordeau_node{base_plan, std::move(requests), std::move(vehicles)};
}

Test_plan_data_cordeau_node generate_data_for_search_third_test(Request_generator_cordeau_node& rg) {
	const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.044003f, 2} };
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)6);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	requests->push_back(rg.generate_request(5.16400003f, 0.546999991f, 0, 86400, 5.73999977f,
		2.38199997f, 12540, 15120));
	requests->push_back(rg.generate_request(2.97300005f, 6.41400003f, 0, 86400, -5.4799983f,
		1.43700004f, 15480, 17220));

	const VehiclePlan<Cordeau_node> base_plan{ vehicle, 1 };

	return Test_plan_data_cordeau_node{ base_plan, std::move(requests), std::move(vehicles) };
}


Test_plan_data_cordeau_node generate_data_for_search_three_requests(Request_generator_cordeau_node& rg) {
	const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.044003f, 2} };
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)6);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	// request 0
	requests->push_back(rg.generate_request(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220));
	// request 3
	requests->push_back(rg.generate_request(-1.31700003f, 6.93400002f, 0, 86400,
		-2.27500010f, 5.54099989f, 24960, 27600));
	// request 11
	requests->push_back(rg.generate_request(-4.26100016f, -2.63899994f, 0, 86400, 
		-2.64000010f, 2.95300007f, 22860, 23820));

	const VehiclePlan<Cordeau_node> base_plan{ vehicle, 6 };

	return Test_plan_data_cordeau_node{ base_plan, std::move(requests), std::move(vehicles) };
}

Test_plan_data_cordeau_node generate_data_for_search_three_requests_case_2(Request_generator_cordeau_node& rg) {
	const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.044003f, 2} };
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)6);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();

	// request 0
	requests->push_back(rg.generate_request(-2.97300005f, 6.41400003f, 0, 86400,
		-5.47599983f, 1.43700004f, 15480, 17220));
	// request 1
	requests->push_back(rg.generate_request(-3.06599998f, 0.546000004f, 0, 86400,
		-4.93300009f, 3.3369989f, 19740, 21660));
	// request 5
	requests->push_back(rg.generate_request(4.89099979f, 0.626999974f, 0, 86400,
		-3.85599995f, -0.370000005f, 25920, 27480));

	const VehiclePlan<Cordeau_node> base_plan{ vehicle, 6 };

	return Test_plan_data_cordeau_node{ base_plan, std::move(requests), std::move(vehicles) };
}

Test_plan_data_cordeau_node generate_data_for_search_four_requests(Request_generator_cordeau_node& rg) {
	auto vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
	const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.044003f, 2} };
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)6);
	const Vehicle<Cordeau_node>& vehicle = vehicles->at(0);
	
	auto requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
	// request 5
	requests->push_back(rg.generate_request(4.891f, 0.627f, 0, 86400,
		-3.856f, -0.370f, 25920, 27480));
	// request 3
	requests->push_back(rg.generate_request(-1.317f, 6.934f, 0, 86400,
		-2.275f, 5.541f, 24960, 27600));
	// request 4
	requests->push_back(rg.generate_request(-6.741f, 6.832f, 0, 86400,
		-5.662f, 7.334f, 18300, 20940));
	// request 2
	requests->push_back(rg.generate_request(5.164f, 0.547f, 0, 86400,
		5.740f, 2.382f, 12540, 15120));
	

	const VehiclePlan<Cordeau_node> base_plan{ vehicle, 6 };

	return Test_plan_data_cordeau_node{ base_plan, std::move(requests), std::move(vehicles) };
}

Test_plan_data<VehiclePlan<Amodsim_node>, Amodsim_node> generate_data_for_search_amodsim_node(Request_generator_amodsim_node& rg) {

	const std::shared_ptr<Amodsim_node> vehicle_init_position{new Amodsim_node{0} };
	auto vehicles = std::make_unique<std::vector<Vehicle<Amodsim_node>>>();
	vehicles->emplace_back(0, vehicle_init_position, (unsigned short)1);
	const Vehicle<Amodsim_node>& vehicle = vehicles->at(0);
	auto requests = std::make_unique<std::vector<Request<Amodsim_node>>>();

	requests->push_back(rg.generate_request(1, 25209, 25389, 2, 90744, 90924));

	const VehiclePlan<Amodsim_node> base_plan{vehicle, 1};

	return Test_plan_data<VehiclePlan<Amodsim_node>, Amodsim_node>{ base_plan, std::move(requests), std::move(vehicles) };
}

VehiclePlan<Cordeau_node> generate_expected_plan(
	const Test_plan_data_cordeau_node& td, 
	unsigned short service_time,
	const Travel_time_provider<Cordeau_node>& travel_time_provider
) {
	const Request<Cordeau_node>& r = td.get_requests().at(0);
	VehiclePlan<Cordeau_node> expected_plan{td.get_vehicles().at(0), 1};
	ActionData<Cordeau_node>& pickup_action_data = expected_plan.add_action(r.get_pickup());
	pickup_action_data.set_departure_time(service_time);
	ActionData<Cordeau_node>& drop_off_action_data = expected_plan.add_action(r.get_dropoff());
	const unsigned int travel_time = travel_time_provider.get_travel_time(r.get_pickup().get_node(),
		r.get_dropoff().get_node());
	drop_off_action_data.set_arrival_time(travel_time + service_time);
	drop_off_action_data.set_departure_time(travel_time + service_time * 2);

	const unsigned int travel_time_to_depot = travel_time_provider.get_travel_time(r.get_dropoff().get_node(),
		td.get_vehicles().at(0).get_init_position());
	expected_plan.set_arrival_time(drop_off_action_data.get_departure_time() + travel_time_to_depot);
	expected_plan.set_cost(travel_time + travel_time_to_depot);

	return expected_plan;
}

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_second_test(
	const Test_plan_data_cordeau_node& td,
	unsigned short service_time, 
	const Travel_time_provider<Cordeau_node>& travel_time_provider
) {
	const Request<Cordeau_node>& r1 = td.get_requests().at(0);
	const Request<Cordeau_node>& r2 = td.get_requests().at(1);
	
	VehiclePlan<Cordeau_node> expected_plan{td.get_vehicles().at(0), 1};
	ActionData<Cordeau_node>& pickup_action_data = expected_plan.add_action(r1.get_pickup());
	pickup_action_data.set_departure_time(service_time);

	ActionData<Cordeau_node>& second_pickup_action_data = expected_plan.add_action(r2.get_pickup());
	unsigned int travel_time = travel_time_provider.get_travel_time(r1.get_pickup().get_node(),
		r2.get_dropoff().get_node());
	second_pickup_action_data.set_arrival_time(travel_time + service_time);
	second_pickup_action_data.set_departure_time(travel_time + service_time * 2);

	ActionData<Cordeau_node>& drop_off_action_data = expected_plan.add_action(r1.get_dropoff());
	travel_time += travel_time_provider.get_travel_time(r2.get_pickup().get_node(),
		r1.get_dropoff().get_node());
	drop_off_action_data.set_arrival_time(travel_time + 2 * service_time);
	drop_off_action_data.set_departure_time(travel_time + 3 * service_time);

	ActionData<Cordeau_node>& second_drop_off_action_data = expected_plan.add_action(r2.get_dropoff());
	travel_time += travel_time_provider.get_travel_time(r1.get_dropoff().get_node(),
		r2.get_dropoff().get_node());
	second_drop_off_action_data.set_arrival_time(travel_time + 3 * service_time);
	second_drop_off_action_data.set_departure_time(travel_time + 4 * service_time);

	const unsigned int travel_time_to_depot = travel_time_provider.get_travel_time(r2.get_dropoff().get_node(),
		td.get_vehicles().at(0).get_init_position());
	expected_plan.set_arrival_time(second_drop_off_action_data.get_departure_time() + travel_time_to_depot);
	expected_plan.set_cost(travel_time + travel_time_to_depot);

	return expected_plan;
}

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_third_test(const Test_plan_data_cordeau_node& td) {
	const Request<Cordeau_node>& r1 = td.get_requests().at(0);
	const Request<Cordeau_node>& r2 = td.get_requests().at(1);

	VehiclePlan<Cordeau_node> expected_plan{ td.get_vehicles().at(0), 1 };

	// pickup 1
	ActionData<Cordeau_node>& pickup_action_data = expected_plan.add_action(r1.get_pickup());
	pickup_action_data.set_arrival_time(383u);
	pickup_action_data.set_departure_time(7140);

	// drop off 1
	ActionData<Cordeau_node>& drop_off_action_data = expected_plan.add_action(r1.get_dropoff());
	drop_off_action_data.set_arrival_time(7255u);
	drop_off_action_data.set_departure_time(13140);

	// piuckup 2
	ActionData<Cordeau_node>& second_pickup_action_data = expected_plan.add_action(r2.get_pickup());
	second_pickup_action_data.set_arrival_time(13433u);
	second_pickup_action_data.set_departure_time(14033);

	// drop off 2
	ActionData<Cordeau_node>& second_drop_off_action_data = expected_plan.add_action(r2.get_dropoff());
	second_drop_off_action_data.set_arrival_time(14622u);
	second_drop_off_action_data.set_departure_time(16080);

	expected_plan.set_arrival_time(16348);
	expected_plan.set_cost(1648);

	return expected_plan;
}

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_three_requests (
	const Test_plan_data_cordeau_node& td
)
{
	VehiclePlan<Cordeau_node> expected_plan = td.get_plan();
	const Request<Cordeau_node>& r1 = td.get_requests().at(0);
	const Request<Cordeau_node>& r2 = td.get_requests().at(1);
	const Request<Cordeau_node>& r3 = td.get_requests().at(2);

	// pickup 1
	ActionData<Cordeau_node>& pickup_action_data = expected_plan.add_action(r1.get_pickup());
	pickup_action_data.set_arrival_time(289u);
	pickup_action_data.set_departure_time(10080);

	// drop off 1
	ActionData<Cordeau_node>& drop_off_action_data = expected_plan.add_action(r1.get_dropoff());
	drop_off_action_data.set_arrival_time(10414u);
	drop_off_action_data.set_departure_time(16080);

	// pickup 2
	ActionData<Cordeau_node>& second_pickup_action_data = expected_plan.add_action(r3.get_pickup());
	second_pickup_action_data.set_arrival_time(16335u);
	second_pickup_action_data.set_departure_time(17460);

	// drop off 2
	ActionData<Cordeau_node>& second_drop_off_action_data = expected_plan.add_action(r3.get_dropoff());
	second_drop_off_action_data.set_arrival_time(17809u);
	second_drop_off_action_data.set_departure_time(23460);

	// pickup 3
	ActionData<Cordeau_node>& third_pickup_action_data = expected_plan.add_action(r2.get_pickup());
	third_pickup_action_data.set_arrival_time(23712u);
	third_pickup_action_data.set_departure_time(24312);

	// drop off 3
	ActionData<Cordeau_node>& third_drop_off_action_data = expected_plan.add_action(r2.get_dropoff());
	third_drop_off_action_data.set_arrival_time(24413u);
	third_drop_off_action_data.set_departure_time(25560);

	expected_plan.set_arrival_time(25785);
	expected_plan.set_cost(1805);

	return expected_plan;
}

VehiclePlan<Cordeau_node> generate_expected_plan_exhaustive_search_three_requests_case_2(const Test_plan_data_cordeau_node& td) {
	VehiclePlan<Cordeau_node> expected_plan = td.get_plan();
	const Request<Cordeau_node>& r1 = td.get_requests().at(0);
	const Request<Cordeau_node>& r2 = td.get_requests().at(1);
	const Request<Cordeau_node>& r3 = td.get_requests().at(2);

	// pickup 1
	ActionData<Cordeau_node>& pickup_action_data = expected_plan.add_action(r1.get_pickup());
	pickup_action_data.set_arrival_time(289u);
	pickup_action_data.set_departure_time(10080);

	// pickup 2
	ActionData<Cordeau_node>& second_pickup_action_data = expected_plan.add_action(r2.get_pickup());
	second_pickup_action_data.set_arrival_time(10432);
	second_pickup_action_data.set_departure_time(14340);

	// drop off 1
	ActionData<Cordeau_node>& drop_off_action_data = expected_plan.add_action(r1.get_dropoff());
	drop_off_action_data.set_arrival_time(14494u);
	drop_off_action_data.set_departure_time(16080);

	// drop off 2
	ActionData<Cordeau_node>& second_drop_off_action_data = expected_plan.add_action(r2.get_dropoff());
	second_drop_off_action_data.set_arrival_time(16199u);
	second_drop_off_action_data.set_departure_time(20340);

	// pickup 3
	ActionData<Cordeau_node>& third_pickup_action_data = expected_plan.add_action(r3.get_pickup());
	third_pickup_action_data.set_arrival_time(20951u);
	third_pickup_action_data.set_departure_time(21551);

	// drop off 3
	ActionData<Cordeau_node>& third_drop_off_action_data = expected_plan.add_action(r3.get_dropoff());
	third_drop_off_action_data.set_arrival_time(22079u);
	third_drop_off_action_data.set_departure_time(26520);

	expected_plan.set_arrival_time(26741);
	expected_plan.set_cost(2274);

	return expected_plan;
}

VehiclePlan<Cordeau_node>
generate_expected_plan_exhaustive_search_four_requests(const Test_plan_data_cordeau_node& td) {
	VehiclePlan<Cordeau_node> expected_plan = td.get_plan();
	const Request<Cordeau_node>& r2 = td.get_requests().at(3);
	const Request<Cordeau_node>& r3 = td.get_requests().at(1);
	const Request<Cordeau_node>& r4 = td.get_requests().at(2);
	const Request<Cordeau_node>& r5 = td.get_requests().at(0);

	// pickup req 2
	ActionData<Cordeau_node>& pickup_action_data = expected_plan.add_action(r2.get_pickup());
	pickup_action_data.set_arrival_time(383u);
	pickup_action_data.set_departure_time(7140);

	// drop off req 2
	ActionData<Cordeau_node>& second_pickup_action_data = expected_plan.add_action(r2.get_dropoff());
	second_pickup_action_data.set_arrival_time(7255u);
	second_pickup_action_data.set_departure_time(13140);

	// pickup req 3
	ActionData<Cordeau_node>& drop_off_action_data = expected_plan.add_action(r3.get_pickup());
	drop_off_action_data.set_arrival_time(13644u);
	drop_off_action_data.set_departure_time(19560);

	// pickup req 4
	ActionData<Cordeau_node>& second_drop_off_action_data = expected_plan.add_action(r4.get_pickup());
	second_drop_off_action_data.set_arrival_time(19885u);
	second_drop_off_action_data.set_departure_time(20485);

	// drop off req 4
	ActionData<Cordeau_node>& third_pickup_action_data = expected_plan.add_action(r4.get_dropoff());
	third_pickup_action_data.set_arrival_time(20556u);
	third_pickup_action_data.set_departure_time(21156);

	// drop off req 3
	ActionData<Cordeau_node>& third_drop_off_action_data = expected_plan.add_action(r3.get_dropoff());
	third_drop_off_action_data.set_arrival_time(21386u);
	third_drop_off_action_data.set_departure_time(25560);

	// pick up req 5
	ActionData<Cordeau_node>& action_data_7 = expected_plan.add_action(r5.get_pickup());
	action_data_7.set_arrival_time(26081u);
	action_data_7.set_departure_time(26681);

	// drop off req 3
	ActionData<Cordeau_node>& action_data_8 = expected_plan.add_action(r5.get_dropoff());
	action_data_8.set_arrival_time(27209u);
	action_data_8.set_departure_time(27809);

	expected_plan.set_arrival_time(28030);
	expected_plan.set_cost(2898);

	return expected_plan;
}

VehiclePlan<Amodsim_node> generate_expected_plan_amodsim_node(
	const Test_plan_data<VehiclePlan<Amodsim_node>, Amodsim_node>& td,
	unsigned short service_time,
	const Travel_time_provider<Amodsim_node>& travel_time_provider
) {
	const Request<Amodsim_node>& r = td.get_requests().at(0);
	VehiclePlan<Amodsim_node> expected_plan{ td.get_vehicles().at(0), 1 };
	ActionData<Amodsim_node>& pickup_action_data = expected_plan.add_action(r.get_pickup());
	pickup_action_data.set_departure_time(pickup_action_data.get_min_time() + service_time);
	ActionData<Amodsim_node>& drop_off_action_data = expected_plan.add_action(r.get_dropoff());
	const unsigned int travel_time = travel_time_provider.get_travel_time(r.get_pickup().get_node(),
		r.get_dropoff().get_node());
	drop_off_action_data.set_arrival_time(drop_off_action_data.get_min_time());
	drop_off_action_data.set_departure_time(drop_off_action_data.get_arrival_time() + service_time);

	const unsigned int travel_time_to_depot = travel_time_provider.get_travel_time(r.get_dropoff().get_node(),
		td.get_vehicles().at(0).get_init_position());
	expected_plan.set_arrival_time(drop_off_action_data.get_departure_time() + travel_time_to_depot);
	expected_plan.set_cost(travel_time + travel_time_to_depot);

	return expected_plan;
}





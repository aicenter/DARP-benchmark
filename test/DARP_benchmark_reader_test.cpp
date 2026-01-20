//
// Created by Fido on 2023-03-31.
//

#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <fstream>   // Add for ifstream
#include <sstream>   // Add for stringstream
#include <string>    // Add for string and getline
#include <vector>    // Add for vector
#include <tuple>     // Add for tuple

#include "gtest_wrapper.h"
#include "../src/DARP_benchmark_reader.h"
#include "../src/travel_time_provider/Travel_time_provider.h"
#include "../src/Request.h"
#include "../src/Action.h"
#include "../src/DARP_benchmark_node.h"

namespace {

// Simple travel time provider that always returns 0
class Zero_travel_time_provider : public Travel_time_provider<Amodsim_node> {
public:
	[[nodiscard]] travel_time_type get_travel_time(const Amodsim_node&, const Amodsim_node&) const override {
		return 0;
	}

	[[nodiscard]] std::tuple<const Amodsim_node&, time_type> get_vehicle_location_info(
		[[maybe_unused]] const Amodsim_node& last_action_location,
		[[maybe_unused]] const Amodsim_node& next_action_location,
		[[maybe_unused]] time_type time_since_last_action_departure
	) override {
		return std::make_tuple(Amodsim_node(0),0);
	}
};

// Helper function to compare two Request objects
void assert_requests_equal(const Request<Amodsim_node>& actual, const Request<Amodsim_node>& expected) {
	ASSERT_EQ(actual.get_index(), expected.get_index());
	ASSERT_EQ(actual.get_min_travel_time(), expected.get_min_travel_time());

	// Compare pickup action
	const auto& actual_pickup = actual.get_pickup();
	const auto& expected_pickup = expected.get_pickup();
	ASSERT_EQ(actual_pickup.get_action_id(), expected_pickup.get_action_id());
	ASSERT_EQ(actual_pickup.get_node().get_index(), expected_pickup.get_node().get_index());
	ASSERT_EQ(actual_pickup.get_min_time(), expected_pickup.get_min_time());
	ASSERT_EQ(actual_pickup.get_max_time(), expected_pickup.get_max_time());
	ASSERT_EQ(actual_pickup.get_action_type(), expected_pickup.get_action_type());
	ASSERT_EQ(actual_pickup.get_service_duration(), expected_pickup.get_service_duration());

	// Compare dropoff action
	const auto& actual_dropoff = actual.get_dropoff();
	const auto& expected_dropoff = expected.get_dropoff();
	ASSERT_EQ(actual_dropoff.get_action_id(), expected_dropoff.get_action_id());
	ASSERT_EQ(actual_dropoff.get_node().get_index(), expected_dropoff.get_node().get_index());
	ASSERT_EQ(actual_dropoff.get_min_time(), expected_dropoff.get_min_time());
	ASSERT_EQ(actual_dropoff.get_max_time(), expected_dropoff.get_max_time());
	ASSERT_EQ(actual_dropoff.get_action_type(), expected_dropoff.get_action_type());
	ASSERT_EQ(actual_dropoff.get_service_duration(), expected_dropoff.get_service_duration());
}

}
TEST(DARP_benchmark_reader_test, darp_instance_configuration_loading) {
	auto path = std::filesystem::path{"test_resources/test_instance.yaml"};
	YAML::Node config = YAML::LoadFile(path.string());
	auto darp_config = internal::load_instance_configuration(config);

	ASSERT_EQ(darp_config->get_max_ride_time(), 0);
	ASSERT_EQ(darp_config->get_max_route_duration(), 0);
	ASSERT_EQ(darp_config->is_return_to_depot(), false);
	ASSERT_EQ(darp_config->use_virtual_vehicles(), false);
	ASSERT_EQ(darp_config->get_start_time(), 63900);
}

TEST(DARP_benchmark_reader_test, request_loading_csv) {
	std::string request_filepath = "test_resources/requests.csv"; // Correct path
	unsigned short service_time = 0; // Assuming default service time

	// Hardcoded YAML config string pointing to CSV
	std::string yaml_string = std::format(R"(
	{{
		max_prolongation: 600,
		demand: {{
			filepath: "{}"
		}}
	}})", request_filepath);
	YAML::Node config = YAML::Load(yaml_string);
	const auto max_prolongation = config["max_prolongation"].as<unsigned short>();

	// Use the Zero travel time provider
	std::shared_ptr<Travel_time_provider<Amodsim_node>> travel_cost_provider
		= std::make_shared<Zero_travel_time_provider>();

	// Call static load_requests with config (should dispatch to CSV loader)
	auto requests_ptr = DARP_benchmark_reader::load_requests(config, travel_cost_provider);
	const auto& actual_requests = *requests_ptr;

    ASSERT_EQ(actual_requests.size(), 3); // Expect 3 requests from requests.csv

	// Expected Request 0 (from requests.csv line 1)
	unsigned int expected_time_0 = 64813; // 64813000 / 1000
	unsigned short min_travel_time_0 = 0; // From Zero_travel_time_provider
	Request<Amodsim_node> expected_req_0(
		0, 0, 1, // Expected index 0, action IDs 0, 1
        std::make_shared<Amodsim_node>(19357),
        expected_time_0, expected_time_0 + max_prolongation,
        std::make_shared<Amodsim_node>(26464),
        expected_time_0 + min_travel_time_0,
        expected_time_0 + min_travel_time_0 + max_prolongation,
        min_travel_time_0, service_time, service_time
	);
	assert_requests_equal(actual_requests[0], expected_req_0);

	// Expected Request 1 (from requests.csv line 2)
	unsigned int expected_time_1 = 64841; // 64841000 / 1000
	unsigned short min_travel_time_1 = 0;
	Request<Amodsim_node> expected_req_1(
		1, 2, 3, // Expected index 1, action IDs 2, 3
        std::make_shared<Amodsim_node>(307),
        expected_time_1, expected_time_1 + max_prolongation,
        std::make_shared<Amodsim_node>(4198),
        expected_time_1 + min_travel_time_1,
        expected_time_1 + min_travel_time_1 + max_prolongation,
        min_travel_time_1, service_time, service_time
	);
	assert_requests_equal(actual_requests[1], expected_req_1);

	// Expected Request 2 (from requests.csv line 3)
	unsigned int expected_time_2 = 64858; // 64858000 / 1000
	unsigned short min_travel_time_2 = 0;
	Request<Amodsim_node> expected_req_2(
		2, 4, 5, // Expected index 2, action IDs 4, 5
        std::make_shared<Amodsim_node>(50825),
        expected_time_2, expected_time_2 + max_prolongation,
        std::make_shared<Amodsim_node>(28165),
        expected_time_2 + min_travel_time_2,
        expected_time_2 + min_travel_time_2 + max_prolongation,
        min_travel_time_2, service_time, service_time
	);
	assert_requests_equal(actual_requests[2], expected_req_2);
}

// DI test unchanged from before
TEST(DARP_benchmark_reader_test, request_loading_di) {
	std::string request_filepath = "test_resources/trips.di"; // Use dedicated test file
	unsigned short service_time = 0;

	// Hardcoded YAML config string
	std::string yaml_string = std::format(R"(
	{{
		max_prolongation: 600,
		demand: {{
			filepath: "{}"
		}}
	}})", request_filepath);
	YAML::Node config = YAML::Load(yaml_string);
	const auto max_prolongation = config["max_prolongation"].as<unsigned short>();

	std::shared_ptr<Travel_time_provider<Amodsim_node>> travel_cost_provider
		= std::make_shared<Zero_travel_time_provider>();

	auto requests_ptr = DARP_benchmark_reader::load_requests(config, travel_cost_provider);
	const auto& actual_requests = *requests_ptr;

	ASSERT_EQ(actual_requests.size(), 3);

	// Expected Request 0
	unsigned int expected_time_0 = 64800;
	unsigned short min_travel_time_0 = 0;
	Request<Amodsim_node> expected_req_0(
		0, 0, 1, std::make_shared<Amodsim_node>(3572), expected_time_0, expected_time_0 + max_prolongation,
		std::make_shared<Amodsim_node>(2715), expected_time_0 + min_travel_time_0, expected_time_0 + min_travel_time_0 + max_prolongation,
		min_travel_time_0, service_time, service_time
	);
	assert_requests_equal(actual_requests[0], expected_req_0);

	// Expected Request 1
	unsigned int expected_time_1 = 64800;
	unsigned short min_travel_time_1 = 0;
	Request<Amodsim_node> expected_req_1(
		1, 2, 3, std::make_shared<Amodsim_node>(5660), expected_time_1, expected_time_1 + max_prolongation,
		std::make_shared<Amodsim_node>(1779), expected_time_1 + min_travel_time_1, expected_time_1 + min_travel_time_1 + max_prolongation,
		min_travel_time_1, service_time, service_time
	);
	assert_requests_equal(actual_requests[1], expected_req_1);

	// Expected Request 2
	unsigned int expected_time_2 = 64800;
	unsigned short min_travel_time_2 = 0;
	Request<Amodsim_node> expected_req_2(
		2, 4, 5, std::make_shared<Amodsim_node>(2077), expected_time_2, expected_time_2 + max_prolongation,
		std::make_shared<Amodsim_node>(110), expected_time_2 + min_travel_time_2, expected_time_2 + min_travel_time_2 + max_prolongation,
		min_travel_time_2, service_time, service_time
	);
	assert_requests_equal(actual_requests[2], expected_req_2);
}

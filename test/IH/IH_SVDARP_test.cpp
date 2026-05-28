#include "../gtest_wrapper.h"

#include <utility>
#include <rapidjson/document.h>

#include "./common.h"
#include "../../src/inout.h"
#include "../../src/Cordeau_benchmark.h"
#include "../../src/Vehicle.h"
#include "../../src/solver/IH/IH_vehicle_plan_builder.h"
#include "../../src/solver/IH/SVDARP.h"
#include "../../src/solver/DARP_context.h"
#include "../../src/travel_time_provider/Distance_matrix_travel_time_provider.h"

namespace {

std::pair<
		std::shared_ptr<Distance_matrix_node_travel_time_provider<Test_action_data<>>>,
		std::unique_ptr<std::vector<Test_request<>>>
> load_data(std::string path) {
	rapidjson::Document doc = load_json_to_dom(get_test_resource_path(path).string());
	assert(doc.IsObject());

	// load requests
	auto requests = std::make_unique<std::vector<Test_request<>>>();
	const auto& request_array = doc["requests"].GetArray();
	for(unsigned i = 0; i < request_array.Size(); ++i) {
		requests->emplace_back(
			Test_action_data(i * 2 + 1, Action_type::pickup, 0), 
			Test_action_data(i * 2 + 2, Action_type::dropoff, request_array[i].GetUint()));
	}

	// load dm
	const auto& dm_array = doc["dm"].GetArray();
	const unsigned size = dm_array.Size();
	assert(size == requests->size() * 2 + 1);
	auto dm = std::make_unique<travel_time_type[]>(size * size);
	for(unsigned i = 0; i < size; ++i) {
		const auto& dm_inner_array = dm_array[i];
		assert(dm_inner_array.Size() == size);
		for(unsigned j = 0; j < size; ++j) {
			dm[i * size + j] = static_cast<travel_time_type>(dm_inner_array[j].GetUint());
		}
	}

	return {
		std::make_shared<Distance_matrix_node_travel_time_provider<Test_action_data<>>>(size, std::move(dm)),
		std::move(requests)};
}

	/*class Test_travel_time_provider: public Distance_matrix_node_travel_time_provider<Test_action_data>{
	public:
		[[nodiscard]] unsigned
		get_travel_time(const Test_action_data& from, const Test_action_data& to) const override {
			return Distance_matrix_node_travel_time_provider::get_travel_time(from.get_index(), to.get_index());
		}

		const Vehicle<Test_action_data>& get_nearest_vehicle(
			const Test_action_data& location,
			const std::unordered_set<const Vehicle<Test_action_data>*>& vehicles
		) override;
	};*/


	class Instance_data {
	public:

		Instance_data(unsigned service_time,
			const std::shared_ptr<Travel_time_provider<Cordeau_node>>& travel_time_provider,
			const std::shared_ptr<DARP_instance_configuration>& darp_instance_configuration,
			const SVDARP<Cordeau_node, Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>>& solver,
			Request_generator_cordeau_node& request_generator)
			: service_time(service_time),
			  travel_time_provider(travel_time_provider),
			  darp_instance_configuration(darp_instance_configuration),
			  solver(solver),
			request_generator(request_generator){
		}

		unsigned int service_time;
		
		std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider;

		std::shared_ptr<DARP_instance_configuration> darp_instance_configuration;

		SVDARP<Cordeau_node, Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> solver;

		Request_generator_cordeau_node request_generator;

		Request_data_Cordeau_node get_request_action_data(
			float from_x,
			float from_y,
			unsigned int pickup_min_time,
			unsigned int pickup_max_time,
			float to_x,
			float to_y,
			unsigned int drop_off_min_time,
			unsigned int drop_off_max_time
		) {
			Request<Cordeau_node> request = request_generator.generate_request(from_x, from_y, pickup_min_time,
				pickup_max_time, to_x, to_y, drop_off_min_time, drop_off_max_time);
			return Request_data_Cordeau_node(request);
		}
		
	};

	Instance_data get_instance_data() {
		unsigned short service_time = 10 * 60u;
		const std::shared_ptr<Travel_time_provider<Cordeau_node>> travel_time_provider
			= std::make_shared<Euclidean_travel_time_provider<Cordeau_node>>((unsigned short)60);
		const std::shared_ptr<DARP_instance_configuration> darp_instance_configuration
			= std::make_shared<DARP_instance_configuration>(480 * 60u, 90 * 60u, true);
		const DARP_context<Cordeau_node> context(travel_time_provider, darp_instance_configuration);
		SVDARP<Cordeau_node, Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> solver{context};
		Request_generator_cordeau_node rg{ travel_time_provider, service_time };
		return Instance_data{service_time, travel_time_provider, darp_instance_configuration, solver, rg};
	}

	
	template<class N>
	void check_plan_builders_equal(const IH_vehicle_plan_builder<Vehicle<N>, ActionData<N>, VehiclePlan<N>>& computed_plan, const IH_vehicle_plan_builder<Vehicle<N>, ActionData<N>, VehiclePlan<N>>& expected_plan) {
		EXPECT_EQ(computed_plan.get_cost(), expected_plan.get_cost());

		ASSERT_EQ(computed_plan.get_action_data_used_length(), expected_plan.get_action_data_used_length());
		ASSERT_EQ(computed_plan.get_active_length(), expected_plan.get_active_length());

		EXPECT_EQ(computed_plan.get_departure_time(), expected_plan.get_departure_time());
		EXPECT_EQ(computed_plan.get_arrival_time(), expected_plan.get_arrival_time());

		for (unsigned int i = 0; i < computed_plan.get_active_length(); i++) {
			const ActionData<N>& computed_action_data = computed_plan[i];
			const ActionData<N>& expected_action_data = expected_plan[i];

			EXPECT_EQ(dynamic_cast<const Service_action<N>&>(computed_action_data.get_action()).get_request().get_index(),
				dynamic_cast<const Service_action<N>&>(expected_action_data.get_action()).get_request().get_index());
			EXPECT_EQ(computed_action_data.get_action().get_action_type(), expected_action_data.get_action().get_action_type());
			EXPECT_EQ(computed_action_data.get_arrival_time(), expected_action_data.get_arrival_time());
			EXPECT_EQ(computed_action_data.get_departure_time(), expected_action_data.get_departure_time());
		}

		// time adjustments check
		const std::vector<int>& time_adjustemnts_computed = computed_plan.get_time_adjustments();
		const std::vector<int>& time_adjustemnts_expected = expected_plan.get_time_adjustments();
		ASSERT_EQ(time_adjustemnts_computed.size(), time_adjustemnts_expected.size());
		for(unsigned int i = 0; i < time_adjustemnts_expected.size(); ++i) {
			EXPECT_EQ(time_adjustemnts_computed[i], time_adjustemnts_expected[i]);
		}
	}

	template<class V, class A, class P>
	void check_test_plan_builders_equal(
		const IH_vehicle_plan_builder<V, A, P>& computed_plan,
		const IH_vehicle_plan_builder<V, A, P>& expected_plan
	) {
		EXPECT_EQ(computed_plan.get_cost(), expected_plan.get_cost());
		ASSERT_EQ(computed_plan.get_action_data_used_length(), expected_plan.get_action_data_used_length());
		ASSERT_EQ(computed_plan.get_active_length(), expected_plan.get_active_length());
		EXPECT_EQ(computed_plan.get_departure_time(), expected_plan.get_departure_time());
		EXPECT_EQ(computed_plan.get_arrival_time(), expected_plan.get_arrival_time());

		for(unsigned int i = 0; i < computed_plan.get_active_length(); ++i) {
			const A& computed_action_data = computed_plan[i];
			const A& expected_action_data = expected_plan[i];

			EXPECT_EQ(computed_action_data.get_node(), expected_action_data.get_node());
			EXPECT_EQ(computed_action_data.get_action_type(), expected_action_data.get_action_type());
			EXPECT_EQ(computed_action_data.get_arrival_time(), expected_action_data.get_arrival_time());
			EXPECT_EQ(computed_action_data.get_departure_time(), expected_action_data.get_departure_time());
		}
	}

	class Unit_travel_time_provider : public Travel_time_provider<unsigned> {
	public:
		travel_time_type get_travel_time(const unsigned& from, const unsigned& to) const override {
			return from == to ? 0 : 1;
		}

		std::tuple<const unsigned&, travel_time_type> get_vehicle_location_info(
			const unsigned& last_action_location,
			const unsigned& next_action_location,
			time_type time_since_last_action_departure
		) const override {
			cached_location = time_since_last_action_departure == 0 ? last_action_location : next_action_location;
			return {cached_location, 0};
		}

	private:
		mutable unsigned cached_location{0};
	};

	Test_request<> make_test_request(
		unsigned pickup_node,
		time_type pickup_min_time,
		time_type pickup_max_time,
		unsigned drop_off_node,
		time_type drop_off_min_time,
		time_type drop_off_max_time
	) {
		return {
			Test_action_data<>(Action_base<unsigned>(
				pickup_node, pickup_min_time, pickup_max_time, Action_type::pickup)),
			Test_action_data<>(Action_base<unsigned>(
				drop_off_node, drop_off_min_time, drop_off_max_time, Action_type::dropoff))
		};
	}

	TEST(IH_SVDARP_temporal_pruning_test, insertion_position_range_uses_temporal_overlap) {
		using Solver = SVDARP<unsigned, Test_vehicle, Test_action_data<>, IH_SVDARP_test_plan<>>;

		const std::vector<std::int64_t> earliest{10, 20, 30};
		const std::vector<std::int64_t> latest{15, 25, 35};

		auto range = Solver::compute_temporal_insertion_position_range(earliest, latest, 18, 28);
		EXPECT_EQ(range.first, 1);
		EXPECT_EQ(range.last, 2);
		EXPECT_FALSE(range.empty());

		range = Solver::compute_temporal_insertion_position_range(earliest, latest, 15, 20);
		EXPECT_EQ(range.first, 0);
		EXPECT_EQ(range.last, 2);
		EXPECT_FALSE(range.empty());

		range = Solver::compute_temporal_insertion_position_range(earliest, latest, 40, 50);
		EXPECT_EQ(range.first, 3);
		EXPECT_EQ(range.last, 3);
		EXPECT_FALSE(range.empty());

		range = Solver::compute_temporal_insertion_position_range(earliest, latest, 16, 18);
		EXPECT_EQ(range.first, 1);
		EXPECT_EQ(range.last, 1);
		EXPECT_FALSE(range.empty());

		range = Solver::compute_temporal_insertion_position_range(earliest, latest, 36, 9);
		EXPECT_TRUE(range.empty());

		range = Solver::compute_temporal_insertion_position_range({}, {}, 16, 18);
		EXPECT_EQ(range.first, 0);
		EXPECT_EQ(range.last, 0);
		EXPECT_FALSE(range.empty());
	}

	TEST(IH_SVDARP_temporal_pruning_test, pruned_insertion_matches_exhaustive_in_long_plan) {
		using Plan_builder = IH_vehicle_plan_builder<Test_vehicle, Test_action_data<>, IH_SVDARP_test_plan<>>;
		using Solver = SVDARP<unsigned, Test_vehicle, Test_action_data<>, IH_SVDARP_test_plan<>>;

		auto travel_time_provider = std::make_shared<Unit_travel_time_provider>();
		auto config = std::make_shared<DARP_instance_configuration>(0, 0, false);
		const DARP_context<unsigned> context(travel_time_provider, config);
		Solver solver(context);
		Test_vehicle vehicle(80);
		Plan_builder plan(vehicle, 80);

		for(unsigned i = 0; i < 40; ++i) {
			const time_type action_time = i * 100;
			Test_request<> request = make_test_request(
				i * 2 + 1,
				action_time,
				action_time + 10,
				i * 2 + 2,
				action_time + 20,
				action_time + 30
			);
			solver.insert_request_into_plan_optimally(
				request.pickup_action_data,
				request.drop_off_action_data,
				plan,
				std::numeric_limits<unsigned long>::max(),
				0
			);
		}

		Plan_builder exhaustive_plan = plan;
		Plan_builder pruned_plan = plan;
		Test_request<> exhaustive_request = make_test_request(1001, 2050, 2060, 1002, 2070, 2080);
		Test_request<> pruned_request = make_test_request(1001, 2050, 2060, 1002, 2070, 2080);

		const auto exhaustive_increment = solver.insert_request_into_plan_optimally(
			exhaustive_request.pickup_action_data,
			exhaustive_request.drop_off_action_data,
			exhaustive_plan,
			std::numeric_limits<unsigned long>::max(),
			0
		);
		const auto pruned_increment = solver.insert_request_into_plan_optimally(
			pruned_request.pickup_action_data,
			pruned_request.drop_off_action_data,
			pruned_plan,
			std::numeric_limits<unsigned long>::max(),
			1
		);

		EXPECT_EQ(pruned_increment, exhaustive_increment);
		check_test_plan_builders_equal(pruned_plan, exhaustive_plan);
	}


	
	TEST(IH_SVDARP_test, insert_into_empty_plan) {
		Instance_data id = get_instance_data();

		// request data
		Request_data_Cordeau_node rd0 = id.get_request_action_data(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220);
		
		// empty plan builder
		const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.04400003f, 2} }; //TODO coords
		Vehicle<Cordeau_node> vehicle(0, vehicle_init_position, (unsigned short)6);
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> plan(vehicle, 4);
		plan.add_new_request_data(rd0.pickup_action_data, rd0.drop_off_action_data);
		
		// expected plan builder
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> expected_plan = plan;
		std::vector<short>& action_order = expected_plan.get_action_order();
		action_order[0] = 0;
		ActionData<Cordeau_node>& ad = expected_plan[0];
		const unsigned int first_segment_traveltime
			= id.travel_time_provider->get_travel_time(expected_plan.get_vehicle().get_init_position(), ad.get_node());
		ad.set_arrival_time(first_segment_traveltime);
		ad.set_departure_time(ad.get_arrival_time() + id.service_time);
		expected_plan.set_departure_time(0);
		expected_plan.set_cost(first_segment_traveltime * 2);

		// insert into plan plan
		bool success = id.solver.insert_into_plan(plan, 0, true);
		ASSERT_TRUE(success);

		check_plan_valid(plan);
		
		check_plan_builders_equal(plan, expected_plan);
	}

	TEST(IH_SVDARP_test, insert_request_to_empty_plan) {
		Instance_data id = get_instance_data();

		// request data
		Request_data_Cordeau_node rd0 = id.get_request_action_data(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220);
		
		// empty plan builder
		const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.04400003f, 2} }; //TODO coords
		Vehicle<Cordeau_node> vehicle(0, vehicle_init_position, (unsigned short)6);
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> plan(vehicle, 4);
		
		// expected plan builder
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> expected_plan = plan;
		expected_plan.add_new_request_data(rd0.pickup_action_data, rd0.drop_off_action_data);
		std::vector<short>& action_order = expected_plan.get_action_order();
		expected_plan.set_departure_time(0);


		// dropoff
		action_order[1] = 1;
		ActionData<Cordeau_node>& dropoff = expected_plan[1];
		dropoff.set_departure_time(dropoff.get_min_time() + id.service_time);

		// pickup
		action_order[0] = 0;
		ActionData<Cordeau_node>& pickup = expected_plan[0];
		const unsigned int first_segment_traveltime
			= id.travel_time_provider->get_travel_time(expected_plan.get_vehicle().get_init_position(), pickup.get_node());
		const unsigned int second_segment_traveltime
			= id.travel_time_provider->get_travel_time(pickup.get_node(), dropoff.get_node());
		pickup.set_arrival_time(first_segment_traveltime);
		pickup.set_departure_time(dropoff.get_departure_time() - id.service_time - id.darp_instance_configuration->get_max_ride_time());
		expected_plan.set_departure_time(0);
		expected_plan.set_cost(first_segment_traveltime * 2);
		expected_plan.set_cost_before_drop_off();
		dropoff.set_arrival_time(pickup.get_departure_time() + second_segment_traveltime);
		const unsigned int third_segment_traveltime
			= id.travel_time_provider->get_travel_time(dropoff.get_node(), expected_plan.get_vehicle().get_init_position());
		expected_plan.set_cost(first_segment_traveltime + second_segment_traveltime + third_segment_traveltime);
		std::vector<int>& time_adjustments = expected_plan.get_time_adjustments();
		unsigned int time_adjustment = pickup.get_departure_time() - pickup.get_arrival_time() - id.service_time;
		time_adjustments[4] = 1;
		time_adjustments[7] = time_adjustment;
		//time_adjustments[8] = time_adjustment;

		// insert into plan plan
		id.solver.insert_request_into_plan_optimally(rd0.pickup_action_data, rd0.drop_off_action_data, 
			plan, std::numeric_limits<unsigned long>::max());

		check_plan_valid(plan);
		
		check_plan_builders_equal(plan, expected_plan);
	}

	TEST(IH_SVDARP_test, time_adjustment_test) {
		Instance_data id = get_instance_data();

		// request data
		Request_data_Cordeau_node rd0 = id.get_request_action_data(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220);

		Request_data_Cordeau_node rd1 = id.get_request_action_data(-3.06599998f, 0.546000004f, 0, 86400,
		-4.93300009f, 3.3369989f, 19740, 21660);
		
		// empty plan builder
		const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.04400003f, 2} };
		Vehicle<Cordeau_node> vehicle(0, vehicle_init_position, (unsigned short)6);
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> plan(vehicle, 4);
		// insert first request into plan plan
		id.solver.insert_request_into_plan_optimally(rd0.pickup_action_data, rd0.drop_off_action_data, 
			plan, std::numeric_limits<unsigned long>::max());
		plan.add_new_request_data(rd1.pickup_action_data, rd1.drop_off_action_data);

		check_plan_valid(plan);
		
		/*
		 * expected plan
		 */ 
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> expected_plan = plan;
		expected_plan.set_departure_time(0);

		// reordering actions
		std::vector<short>& action_order = expected_plan.get_action_order();
		action_order[0] = 2;
		action_order[1] = 0;
		action_order[2] = 1;

		// change action times
		ActionData<Cordeau_node>& r1_pickup = expected_plan[0];
		const unsigned int first_segment_traveltime
			= id.travel_time_provider->get_travel_time(expected_plan.get_vehicle().get_init_position(), r1_pickup.get_node());
		r1_pickup.set_arrival_time(first_segment_traveltime);
		r1_pickup.set_departure_time(r1_pickup.get_arrival_time() + id.service_time);
		ActionData<Cordeau_node>& r0_drop_off = expected_plan[2];
		r0_drop_off.set_departure_time(r0_drop_off.get_min_time() + id.service_time);
		ActionData<Cordeau_node>& r0_pickup = expected_plan[1];
		const unsigned int second_segment_traveltime
			= id.travel_time_provider->get_travel_time(r1_pickup.get_node(), r0_pickup.get_node());
		r0_pickup.set_arrival_time(r1_pickup.get_departure_time() + second_segment_traveltime);
		r0_pickup.set_departure_time(r0_drop_off.get_departure_time() - id.service_time - id.darp_instance_configuration->get_max_ride_time());
		const unsigned int third_segment_traveltime
			= id.travel_time_provider->get_travel_time(r0_pickup.get_node(), r0_drop_off.get_node());
		r0_drop_off.set_arrival_time(r0_pickup.get_departure_time() + third_segment_traveltime);

		const unsigned int fourth_segment_traveltime
			= id.travel_time_provider->get_travel_time(r0_drop_off.get_node(), expected_plan.get_vehicle().get_init_position());
		expected_plan.set_cost(first_segment_traveltime + second_segment_traveltime + third_segment_traveltime + fourth_segment_traveltime);

		// time adjustments
		std::vector<int>& time_adjustments = expected_plan.get_time_adjustments();
		unsigned int adjustment = first_segment_traveltime + second_segment_traveltime + id.service_time
			- id.travel_time_provider->get_travel_time(vehicle.get_init_position(), r0_pickup.get_node());
		time_adjustments[0] = 1;
		time_adjustments[2] = adjustment;
		//time_adjustments[3] = adjustment;
		//time_adjustments[4] = adjustment;
		//unsigned int max_ride_time_time_adjustment = r0_pickup.get_departure_time() - r0_pickup.get_arrival_time() - id.service_time;
		//time_adjustments[4] = 1;
		//time_adjustments[7] = max_ride_time_time_adjustment;
		//time_adjustments[8] = max_ride_time_time_adjustment;
		

		// insert into plan plan
		id.solver.insert_into_plan(plan, 0, true);

		check_plan_valid(plan);
		
		check_plan_builders_equal(plan, expected_plan);
	}

	TEST(IH_SVDARP_test, insert_into_plan_length_4) {
		Instance_data id = get_instance_data();

		// request data
		Request_data_Cordeau_node rd0 = id.get_request_action_data(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220);

		Request_data_Cordeau_node rd1 = id.get_request_action_data(-3.06599998f, 0.546000004f, 0, 86400,
		-4.93300009f, 3.3369989f, 19740, 21660);
		
		// empty plan builder
		const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.04400003f, 2} };
		Vehicle<Cordeau_node> vehicle(0, vehicle_init_position, (unsigned short)6);
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> plan(vehicle, 4);
		// insert first request into plan plan
		id.solver.insert_request_into_plan_optimally(rd0.pickup_action_data, rd0.drop_off_action_data, 
			plan, std::numeric_limits<unsigned long>::max());
		id.solver.finalize_plan(plan);
		plan.add_new_request_data(rd1.pickup_action_data, rd1.drop_off_action_data);
		id.solver.insert_into_plan(plan, 0, true);

		/*
		 * expected plan
		 */ 
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> expected_plan = plan;

		//// reordering actions
		//std::vector<short>& action_order = expected_plan.get_action_order();
		//action_order[0] = 2;
		//action_order[1] = 3;
		//action_order[2] = 0;
		//action_order[3] = 1;

		//// change action times
		//ActionData<Cordeau_node>& r1_pickup = expected_plan[0];
		//const unsigned int first_segment_traveltime
		//	= id.travel_time_provider->get_travel_time(expected_plan.get_vehicle().get_init_position(), r1_pickup.get_node());
		//ActionData<Cordeau_node>& r1_dropoff = expected_plan[1];
		//const unsigned int second_segment_traveltime
		//	= id.travel_time_provider->get_travel_time(r1_pickup.get_node(), r1_dropoff.get_node());
		//r1_dropoff.set_arrival_time(r1_pickup.get_departure_time() + second_segment_traveltime);
		//r1_dropoff.set_departure_time(r1_dropoff.get_min_time() + id.service_time);
		//ActionData<Cordeau_node>& r0_pickup = expected_plan[2];
		//const unsigned int third_segment_traveltime
		//	= id.travel_time_provider->get_travel_time(r1_dropoff.get_node(), r0_pickup.get_node());
		//r0_pickup.set_arrival_time(r1_dropoff.get_departure_time() + third_segment_traveltime);
		//r0_pickup.set_departure_time(r0_pickup.get_arrival_time() + id.service_time);
		//ActionData<Cordeau_node>& r0_drop_off = expected_plan[3];
		//const unsigned int fourth_segment_traveltime
		//	= id.travel_time_provider->get_travel_time(r0_pickup.get_node(), r0_drop_off.get_node());
		//r0_drop_off.set_arrival_time(r0_pickup.get_departure_time() + fourth_segment_traveltime);
		//r0_drop_off.set_departure_time(r0_drop_off.get_arrival_time() + id.service_time);

		//const unsigned int fifth_segment_traveltime
		//	= id.travel_time_provider->get_travel_time(r0_drop_off.get_node(), expected_plan.get_vehicle().get_init_position());
		//expected_plan.set_cost_before_drop_off();
		//expected_plan.set_cost(first_segment_traveltime + second_segment_traveltime + third_segment_traveltime 
		//	+ fourth_segment_traveltime + fifth_segment_traveltime);

		//// time adjustments
		//std::vector<int>& time_adjustments = expected_plan.get_time_adjustments();
		//unsigned int adjustment = first_segment_traveltime
		//	+ id.travel_time_provider->get_travel_time(r1_pickup.get_node(), r0_pickup.get_node()) + id.service_time
		//	- id.travel_time_provider->get_travel_time(vehicle.get_init_position(), r0_pickup.get_node());
		//time_adjustments[0] = 1;
		//time_adjustments[2] = adjustment;
		//time_adjustments[3] = adjustment;
		//time_adjustments[4] = adjustment;
		//time_adjustments[5] = adjustment;
		//unsigned int second_adjustment = second_segment_traveltime + third_segment_traveltime + id.service_time
		//	- id.travel_time_provider->get_travel_time(r1_pickup.get_node(), r0_pickup.get_node());
		//time_adjustments[8] = 1;
		//time_adjustments[10] = second_adjustment;
		//time_adjustments[11] = second_adjustment;
		//time_adjustments[12] = second_adjustment;
		//time_adjustments[13] = second_adjustment;
		
		// insert into plan plan
		bool success = id.solver.insert_into_plan(plan, 1, false);
		ASSERT_FALSE(success);

		check_plan_valid(plan);
		
		check_plan_builders_equal(plan, expected_plan);
	}

	TEST(IH_SVDARP_test, remove_from_plan) {
		Instance_data id = get_instance_data();

		// request data
		Request_data_Cordeau_node rd0 = id.get_request_action_data(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220);

		Request_data_Cordeau_node rd1 = id.get_request_action_data(-3.06599998f, 0.546000004f, 0, 86400,
		-4.93300009f, 3.3369989f, 19740, 21660);
		
		// plan builder
		const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.04400003f, 2} };
		Vehicle<Cordeau_node> vehicle(0, vehicle_init_position, (unsigned short)6);
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> plan(vehicle, 4);
		// insert first request into plan plan
		id.solver.insert_request_into_plan_optimally(rd0.pickup_action_data, rd0.drop_off_action_data, 
			plan, std::numeric_limits<unsigned long>::max());
		plan.add_new_request_data(rd1.pickup_action_data, rd1.drop_off_action_data);
		id.solver.insert_into_plan(plan, 0, true);
		id.solver.insert_into_plan(plan, 3, false);

		/*
		 * expected plan
		 */ 
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> expected_plan = plan;

		ActionData<Cordeau_node>& r1_drop_off = expected_plan[3];
		r1_drop_off.set_arrival_time(0);
		r1_drop_off.delete_departure_time();

		// reordering actions
		std::vector<short>& action_order = expected_plan.get_action_order();
		action_order[3] = -1;

		// change action times
		std::vector<int>& time_adjustments = expected_plan.get_time_adjustments();
		unsigned int time_adjustment = time_adjustments[11];
		ActionData<Cordeau_node>& r1_pickup = expected_plan[0];
		r1_pickup.set_departure_time(r1_pickup.get_departure_time() - time_adjustments[15]);
		ActionData<Cordeau_node>& r0_pickup = expected_plan[1];
		r0_pickup.set_arrival_time(r0_pickup.get_arrival_time() - time_adjustments[10]);
		r0_pickup.set_departure_time(r0_pickup.get_departure_time() - time_adjustment);
		ActionData<Cordeau_node>& r0_drop_off = expected_plan[2];
		r0_drop_off.set_arrival_time(r0_drop_off.get_arrival_time() - time_adjustment);
		r0_drop_off.set_departure_time(r0_drop_off.get_departure_time() - time_adjustments[13]);
		expected_plan.set_cost(0);
		expected_plan.set_cost_before_drop_off();
		expected_plan.set_cost(1103);
		

		// time adjustments
		time_adjustments[8] = 0;
		time_adjustments[9] = 0;
		time_adjustments[10] = 0;
		time_adjustments[11] = 0;
		time_adjustments[12] = 0;
		time_adjustments[13] = 0;
		time_adjustments[14] = 0;
		time_adjustments[15] = 0;
		
		// insert into plan plan
		plan.remove_lastly_added_action(false, true);

		check_plan_valid(plan);
		
		check_plan_builders_equal(plan, expected_plan);
	}

	TEST(IH_SVDARP_test, adjust_times) {
		Instance_data id = get_instance_data();

		// request data
		Request_data_Cordeau_node rd0 = id.get_request_action_data(-2.97300005f, 6.41400003f, 0, 86400, 
		-5.4799983f, 1.43700004f, 15480, 17220);

		Request_data_Cordeau_node rd1 = id.get_request_action_data(-3.06599998f, 0.546000004f, 0, 86400,
		-4.93300009f, 3.3369989f, 19740, 21660);

		// plan before adjustment
		const std::shared_ptr<Cordeau_node> vehicle_init_position{ new Cordeau_node{-1.04400003f, 2} };
		Vehicle<Cordeau_node> vehicle(0, vehicle_init_position, (unsigned short)6);
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> plan(vehicle, 4);
		// insert first request into plan plan
		id.solver.insert_request_into_plan_optimally(rd0.pickup_action_data, rd0.drop_off_action_data, 
			plan, std::numeric_limits<unsigned long>::max());
		plan.add_new_request_data(rd1.pickup_action_data, rd1.drop_off_action_data);
		id.solver.insert_into_plan(plan, 0, true);
		// reordering actions
		std::vector<short>& action_order = plan.get_action_order();
		//action_order[0] = 2;
		//action_order[1] = 0;
		//action_order[2] = 1;
		action_order[3] = 3;
		plan[3].set_position_in_plan(3);
		ActionData<Cordeau_node>& r0_drop_off = plan[2];
		ActionData<Cordeau_node>& r1_dropoff = plan[3];
		const unsigned int fourth_segment_traveltime
			= id.travel_time_provider->get_travel_time(r0_drop_off.get_node(), r1_dropoff.get_node());
		r1_dropoff.set_arrival_time(r0_drop_off.get_departure_time() + fourth_segment_traveltime);
		r1_dropoff.set_departure_time(r1_dropoff.get_min_time() + id.service_time);
		plan.set_cost_before_drop_off();

		/*
		 * expected plan
		 */ 
		IH_vehicle_plan_builder<Vehicle<Cordeau_node>, ActionData<Cordeau_node>, VehiclePlan<Cordeau_node>> expected_plan = plan;
		std::vector<int>& time_adjustments = expected_plan.get_time_adjustments();
		ActionData<Cordeau_node>& r1_pickup = expected_plan[0];
		ActionData<Cordeau_node>& r1_dropoff_expected = expected_plan[3];
		unsigned int time_adjustment
			= r1_dropoff_expected.get_departure_time() - id.service_time - r1_pickup.get_departure_time() - id.darp_instance_configuration->get_max_ride_time();
		// change action times
		r1_pickup.set_departure_time(r1_pickup.get_departure_time() + time_adjustment);
		ActionData<Cordeau_node>& r0_pickup = expected_plan[1];
		r0_pickup.set_arrival_time(r0_pickup.get_arrival_time() + time_adjustment);
		unsigned int new_time_adjustment = r0_pickup.get_arrival_time() + id.service_time - r0_pickup.get_departure_time();
		r0_pickup.set_departure_time(r0_pickup.get_departure_time() + new_time_adjustment);
		ActionData<Cordeau_node>& r0_drop_off_expected = expected_plan[2];
		r0_drop_off_expected.set_arrival_time(r0_drop_off_expected.get_arrival_time() + new_time_adjustment);
		unsigned int second_time_adjustment = r0_drop_off_expected.get_arrival_time() + id.service_time - r0_drop_off_expected.get_departure_time();
		r0_drop_off_expected.set_departure_time(r0_drop_off_expected.get_departure_time() + second_time_adjustment);
		r1_dropoff_expected.set_arrival_time(r1_dropoff_expected.get_arrival_time() + second_time_adjustment);

		//// time adjustments
		time_adjustments[8] = 1;
		time_adjustments[15] = time_adjustment;
		time_adjustments[10] = time_adjustment;
		time_adjustments[11] = new_time_adjustment;
		time_adjustments[12] = new_time_adjustment;
		time_adjustments[13] = second_time_adjustment;
		
		// insert into plan plan
		id.solver.adjust_times(3, plan, Adjustment_reason::max_ride_time);

		check_plan_valid(plan);
		
		check_plan_builders_equal(plan, expected_plan);
	}

	/** Adapter: exposes Travel_time_provider<unsigned> by wrapping Distance_matrix_node_travel_time_provider<Test_action_data<>>. */
	class Unsigned_from_node_provider_adapter : public Travel_time_provider<unsigned> {
	public:
		explicit Unsigned_from_node_provider_adapter(
			std::shared_ptr<Distance_matrix_node_travel_time_provider<Test_action_data<>>> inner)
			: inner_(std::move(inner)) {}
		travel_time_type get_travel_time(const unsigned& from, const unsigned& to) const override {
			Test_action_data<> from_node(from, Action_type::depot, 0);
			Test_action_data<> to_node(to, Action_type::depot, 0);
			return inner_->get_travel_time(from_node, to_node);
		}
		std::tuple<const unsigned&, travel_time_type> get_vehicle_location_info(
			const unsigned& last_action_location,
			const unsigned& next_action_location,
			time_type time_since_last_action_departure) const override {
			Test_action_data<> last_node(last_action_location, Action_type::depot, 0);
			Test_action_data<> next_node(next_action_location, Action_type::depot, 0);
			auto [loc, t] = inner_->get_vehicle_location_info(last_node, next_node, time_since_last_action_departure);
			cached_index_ = loc.get_index();
			return {cached_index_, t};
		}
	private:
		std::shared_ptr<Distance_matrix_node_travel_time_provider<Test_action_data<>>> inner_;
		mutable unsigned cached_index_{0};
	};

	TEST(IH_SVDARP_test_insert_request_into_plan_optimally, capacity_test) {
		// get requests action data
		const std::pair<
			std::shared_ptr<Distance_matrix_node_travel_time_provider<Test_action_data<>>>,
			std::unique_ptr<std::vector<Test_request<>>>
		> instance = load_data("instance_capacity_test.json");

		// Expose as Travel_time_provider<unsigned> for SVDARP<unsigned, ...>
		auto travel_time_provider = std::make_shared<Unsigned_from_node_provider_adapter>(instance.first);

		// create empty plan builder
		Test_vehicle test_vehicle(1);
		IH_vehicle_plan_builder<Test_vehicle, Test_action_data<>, IH_SVDARP_test_plan<>> plan_builder(test_vehicle, 4);

		// create IH SVDARP
		auto config = std::make_shared<DARP_instance_configuration>(0, 0, false);
		const DARP_context<unsigned> context(travel_time_provider, config);
		SVDARP<unsigned, Test_vehicle, Test_action_data<>, IH_SVDARP_test_plan<>> solver(context);

		// try to add both requests
		solver.insert_request_into_plan_optimally(
			instance.second->at(0).pickup_action_data, 
			instance.second->at(0).drop_off_action_data, 
			plan_builder, 
			std::numeric_limits<unsigned>::max()
		);

		solver.insert_request_into_plan_optimally(
			instance.second->at(0).pickup_action_data, 
			instance.second->at(0).drop_off_action_data, 
			plan_builder, 
			std::numeric_limits<unsigned>::max()
		);

		// check that it did not work
		ASSERT_EQ(plan_builder.get_completed_length(), 2);
	}
}



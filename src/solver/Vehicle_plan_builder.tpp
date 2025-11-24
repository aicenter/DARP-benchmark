

template <class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V,A> P>
template<Vehicle_plan_builder_action_with_request AR>
Vehicle_plan_builder<V, A, P>::Vehicle_plan_builder(
	const P& plan,
	plan_size_type init_time_adjustments_size
):
	action_order(plan.get_service_action_length()),
	iterator_transform_function([this](unsigned short index) -> const A& {return this->action_data[index]; }),
	vehicle(plan.get_vehicle()),
	departure_time(plan.get_departure_time()),
	arrival_time(plan.get_arrival_time()),
	action_data(),
	time_adjustments(init_time_adjustments_size, 0),
	id(id_counter++),
	cost(plan.get_cost())
{
	const auto length = plan.get_service_action_length();
	action_data.reserve(length);
	std::unordered_map<unsigned, unsigned short> pickup_indices;
	pickup_indices.reserve(length / 2);

	// i is the index in plan including depot action
	for(unsigned short i = 1; i < plan.get_length(); ++i){
		const auto& action = plan.get_actions()[i];
		const unsigned short order_index = i - 1; // index in the plan builder, not including the depot action
		if(action.get_action_type() == Action_type::pickup) {
			pickup_indices[action.get_request_index()] = order_index;
		}
		else if(action.get_action_type() == Action_type::dropoff) {
			const auto pickup_index = pickup_indices[action.get_request_index()];
			const auto& pickup = plan[pickup_index + 1];

			// push to action data and set the other action index
			add_action_data(pickup, action);

			// action order setup
			action_order[pickup_index] = static_cast<short>(action_data.size() - 2); // pickup
			action_order[order_index] = static_cast<short>(action_data.size() - 1); // drop off
		}
	}
}


template <class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V, A> P>
void Vehicle_plan_builder<V, A, P>::add_action_data(const A& pickup_action_data, const A& drop_off_action_data) {
	auto& inserted_pickup_action_data = action_data.emplace_back(pickup_action_data);
	auto& inserted_drop_off_action_data = action_data.emplace_back(drop_off_action_data);

	inserted_pickup_action_data.set_other_action_data_index(static_cast<index_in_plan>(action_data.size() - 1));
	inserted_drop_off_action_data.set_other_action_data_index(static_cast<index_in_plan>(action_data.size() - 2));
}


template <class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V,A> P>
bool Vehicle_plan_builder<V, A, P>::check(const DARP_instance_configuration& configuration) const {
    unsigned int time = this->get_departure_time();

	assert(time >= configuration.get_start_time());

	for (const A& action: *this) {
		unsigned int new_time = action.get_arrival_time();
		assert(new_time >= time);
		time = new_time;
		new_time = action.get_departure_time();
		assert(new_time >= time);
		time = new_time;
		if (configuration.get_max_ride_time() && action.get_action_type() == Action_type::dropoff) {
			assert(get_ride_time(action) <= configuration.get_max_ride_time());
		}
	}
	assert(this->get_arrival_time() >= time);

    return true;
}


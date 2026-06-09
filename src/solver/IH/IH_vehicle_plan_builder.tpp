
template <class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V, A> P>
template<Vehicle_plan_builder_action_with_request AR>
IH_vehicle_plan_builder<V, A, P>::IH_vehicle_plan_builder(const P& plan):
	Vehicle_plan_builder<V, A, P>(plan, plan.get_service_action_length() * 4 + 2),
	action_data_used_length(plan.get_service_action_length()),
	operating_start(plan.get_vehicle().get_operation_start())
{
	assert(action_data_used_length == this->action_data.size());
}

template <class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V, A> P>
template<class Travel_time_provider>
void IH_vehicle_plan_builder<V, A, P>::update_temporal_action_bounds(
	const Travel_time_provider& travel_time_provider
) {
	const auto existing_action_count = static_cast<index_in_plan>(action_data_used_length);
	const auto previous_capacity_bounds_size = free_capacities_before_positions.size();
	const bool update_capacity_bounds_incrementally =
		action_data_used_length >= 2
		&& previous_capacity_bounds_size == action_data_used_length - 1
		&& first_capacity_blocking_pickups.size() == previous_capacity_bounds_size;

	earliest_service_starts.resize(existing_action_count);
	latest_service_starts.resize(existing_action_count);

	for(index_in_plan i = 0; i < existing_action_count; ++i) {
		const A& current_action = (*this)[i];
		time_type earliest_arrival;
		if(i == 0) {
			earliest_arrival = operating_start
				+ travel_time_provider.get_travel_time(
					this->get_vehicle().get_init_position(),
					current_action.get_node()
				);
		}
		else {
			const A& previous_action = (*this)[i - 1];
			earliest_arrival = earliest_service_starts[i - 1]
				+ previous_action.get_service_duration()
				+ travel_time_provider.get_travel_time(
					previous_action.get_node(),
					current_action.get_node()
				);
		}
		earliest_service_starts[i] = std::max<time_type>(
			current_action.get_min_time(),
			earliest_arrival
		);
	}

	for(index_in_plan i = existing_action_count - 1; i >= 0; --i) {
		const A& current_action = (*this)[i];
		if(i == existing_action_count - 1) {
			latest_service_starts[i] = current_action.get_max_time();
		}
		else {
			const A& next_action = (*this)[i + 1];
			const std::int64_t latest_before_next =
				static_cast<std::int64_t>(latest_service_starts[i + 1])
				- current_action.get_service_duration()
				- travel_time_provider.get_travel_time(
					current_action.get_node(),
					next_action.get_node()
				);
			assert(latest_before_next >= 0);
			latest_service_starts[i] = std::min<time_type>(
				current_action.get_max_time(),
				static_cast<time_type>(latest_before_next)
			);
		}

		if(i == 0) {
			break;
		}
	}

	if(update_capacity_bounds_incrementally) {
		const auto previous_free_capacities_before_positions = std::move(free_capacities_before_positions);

		const A& new_pickup = this->action_data[action_data_used_length - 2];
		const A& new_drop_off = this->action_data[action_data_used_length - 1];
		assert(new_pickup.get_action_type() == Action_type::pickup);
		assert(new_drop_off.get_action_type() == Action_type::dropoff);

		const index_in_plan pickup_position = new_pickup.get_position_in_plan();
		const index_in_plan drop_off_position = new_drop_off.get_position_in_plan();
		assert(pickup_position >= 0);
		assert(drop_off_position > pickup_position);
		assert(drop_off_position < existing_action_count);

		free_capacities_before_positions.resize(existing_action_count + 1);
		for(index_in_plan i = 0; i <= pickup_position; ++i) {
			free_capacities_before_positions[i] = previous_free_capacities_before_positions[i];
		}
		for(index_in_plan i = pickup_position + 1; i <= drop_off_position; ++i) {
			assert(previous_free_capacities_before_positions[i - 1] > 0);
			free_capacities_before_positions[i] = previous_free_capacities_before_positions[i - 1] - 1;
		}
		for(index_in_plan i = drop_off_position + 1; i <= existing_action_count; ++i) {
			free_capacities_before_positions[i] = previous_free_capacities_before_positions[i - 2];
		}
	}
	else {
		free_capacities_before_positions.resize(existing_action_count + 1);

		unsigned short free_capacity = this->get_vehicle().get_capacity();
		for(index_in_plan i = 0; i < existing_action_count; ++i) {
			free_capacities_before_positions[i] = free_capacity;
			if((*this)[i].get_action_type() == Action_type::pickup) {
				assert(free_capacity > 0);
				--free_capacity;
			}
			else {
				++free_capacity;
			}
		}
		free_capacities_before_positions[existing_action_count] = free_capacity;
	}

	first_capacity_blocking_pickups.resize(existing_action_count + 1);
	index_in_plan first_capacity_blocking_pickup = existing_action_count;
	first_capacity_blocking_pickups[existing_action_count] = first_capacity_blocking_pickup;
	for(index_in_plan i = existing_action_count - 1; i >= 0; --i) {
		if(
			(*this)[i].get_action_type() == Action_type::pickup
			&& free_capacities_before_positions[i] == 1
		) {
			first_capacity_blocking_pickup = i;
		}
		first_capacity_blocking_pickups[i] = first_capacity_blocking_pickup;

		if(i == 0) {
			break;
		}
	}
}

//
// Created by Fido on 2020-06-10.
//

template<typename N, class P>
DARP_benchmark_solver<N, P>::DARP_benchmark_solver(
	const std::shared_ptr<Travel_time_provider<N>>& travel_time_provider_par,
	const std::shared_ptr<DARP_instance_configuration>& darp_instance_configuration_par
)
	: DARP_solver<N>(travel_time_provider_par, darp_instance_configuration_par) {}

template<typename N, class P>
std::unique_ptr<Solution<N, P>> DARP_benchmark_solver<N, P>::solve(const DARP_instance<N>& instance) requires(Benchmark_plan<P>)
{
	darp_instance = &instance;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
	return solve_impl();
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

template<typename N, class P>
std::unique_ptr<Solution_interface<N>>
	DARP_benchmark_solver<N, P>::solve_and_get_final_result(const DARP_instance<N>& instance) {
	return solve(instance);
}

template<typename N, class P>
template<class CheckPlan>
void DARP_benchmark_solver<N, P>::check_plans(const std::vector<CheckPlan>& plans) {
	for (const CheckPlan& plan: plans) {
		plan.check(*this->darp_instance_configuration);
	}
}

template<typename N, class P>
std::optional<VehiclePlan<N>> DARP_benchmark_solver<N, P>::compute_optimal_plan(
	const VehiclePlan<N>& current_plan,
	const Request<N>& request
) {
	std::optional<VehiclePlan<N>> best_plan{};
	unsigned int min_cost_increment = std::numeric_limits<unsigned int>::max();

	const Vehicle<N>& vehicle = current_plan.get_vehicle();
	unsigned short free_capacity = vehicle.get_capacity();

	for (unsigned short pickup_option_index = 0;
		 pickup_option_index <= current_plan.get_length(); pickup_option_index++) {

		// continue if the vehicle is full
		if (free_capacity > 0) {
			for (unsigned short drop_off_option_index = pickup_option_index + 1;
				 drop_off_option_index <= current_plan.get_length() + 1;
				 drop_off_option_index++) {
				std::optional<VehiclePlan<N>> potential_plan = insert_into_plan(
					current_plan, pickup_option_index,
					drop_off_option_index, request
				);
				if (potential_plan) {
					const unsigned int cost_increment = potential_plan->get_cost() - current_plan.get_cost();
					if (cost_increment < min_cost_increment) {
						min_cost_increment = cost_increment;
						best_plan = potential_plan;
					}
				}
			}
		}

		// change free capacity for next index
		if (pickup_option_index < current_plan.get_length()) {
			if (current_plan[pickup_option_index].get_action().get_action_type() == Action_type::pickup) {
				--free_capacity;
			} else {
				++free_capacity;
			}
		}
	}

	return best_plan;
}

template<typename N, class P>
std::optional<P> DARP_benchmark_solver<N, P>::insert_into_plan(
	const P& current_plan,
	unsigned short pickup_option_index,
	unsigned short drop_off_option_index,
	const Request<N>& request
) {

	const Vehicle<N>& vehicle = current_plan.get_vehicle();

	/*std::vector<ActionData<N>> new_plan_tasks;
	new_plan_tasks.reserve(current_plan.get_lenght() + 2);*/

	VehiclePlan<N> vehicle_plan{vehicle, (unsigned short) (current_plan.get_length() + 2)};

	// travel time of the new plan in seconds
	unsigned int current_time = 0;

	unsigned int new_plan_cost = 0;

	// induced delay of the new plan in seconds
	int new_plan_delay = 0;

	const Action<N>* previous_action = nullptr;

	// index of the lastly added action from the old plan
	unsigned short index_in_current_plan = 0;

	unsigned short free_capacity = vehicle.get_capacity();

	std::unordered_map<unsigned int, unsigned int> pickup_map;

	for (int new_plan_index = 0; new_plan_index <= current_plan.get_length() + 1; new_plan_index++) {

		/* get new task */
		const Action<N>* new_action;
		if (new_plan_index == pickup_option_index || new_plan_index == drop_off_option_index) {
			if (new_plan_index == pickup_option_index) {
				new_action = &request.get_pickup();
			} else {
				new_action = &request.get_dropoff();
			}
		} else {
			new_action = &current_plan[index_in_current_plan++].get_action();
		}
		//new_plan_tasks.emplace_back(*new_action);
		//ActionData<N>& new_action_data = new_plan_tasks.back();
		ActionData<N>& new_action_data = vehicle_plan.add_action(*new_action);

		// travel time increment
		unsigned int travel_time;
		if (new_plan_index == 0) {
			travel_time = this->travel_time_provider.get()->get_travel_time(
				vehicle.get_init_position(), new_action->get_node());
		} else {
			travel_time =
				this->travel_time_provider.get()->get_travel_time(previous_action->get_node(), new_action->get_node());
		}
		current_time += travel_time;
		new_plan_cost += travel_time;

		// check max time check for the new action
		unsigned int max_time = new_action->get_max_time();
		if (max_time < current_time) {
			//std::cout << "Max time check for new action failed\n";
			return std::nullopt;
		}

		// for the first action, we can wait in depot
		if (new_plan_index == 0) {
			unsigned int arrival_time = std::max(travel_time, new_action->get_min_time());
			new_action_data.set_arrival_time(arrival_time);
			vehicle_plan.set_departure_time(arrival_time - travel_time);
			current_time = arrival_time;
		} else {

			// arrival time
			new_action_data.set_arrival_time(current_time);

			// min time check for new action
			if (new_action->get_min_time() > current_time) {
				current_time = new_action->get_min_time();
			}
		}

		// add the new task to list
//        new_plan_tasks.emplace_back(
//                *new_action_data.release());

		if (new_action->get_action_type() == Action_type::dropoff) {

			// pickup / drop off referencing
			ActionData<N>& pickup_action_data = vehicle_plan[
				pickup_map[new_action_data.get_action().get_request().get_index()]];
			pickup_action_data.set_other(&new_action_data);
			new_action_data.set_other(&pickup_action_data);

			// max ride time check
			if (current_time - pickup_action_data.get_departure_time() > this->max_ride_time) {
				if (!adjust_times(vehicle_plan, Adjustment_reason::max_ride_time, vehicle.get_init_position())) {
					//std::cout << "Adjust time failed\n";
					return std::nullopt;
				}
			}
		}

		// service time addition
		current_time += new_action->get_service_duration();

		// check max route time
		if (current_time - vehicle_plan.get_departure_time() > this->max_route_duration) {
			if (!adjust_times(vehicle_plan, Adjustment_reason::max_route_time, vehicle.get_init_position())) {
				//std::cout << "Adjust time failed\n";
				return std::nullopt;
			}
		}

		// check max time for actions in the current plan
		for (int index = index_in_current_plan; index < current_plan.get_length(); index++) {
			const Action<N>& remaining_action = current_plan[index].get_action();
			if (remaining_action.get_max_time() < current_time) {
				//std::cout << "Max time exceeded for action " << remaining_action.get_request().get_index() << "\n";
				return std::nullopt;
			}
		}

		// check max time for pick up action
		if (new_plan_index < pickup_option_index) {
			if (request.get_pickup().get_max_time() < current_time) {
				//std::cout << "Max time for pick up action exceeded \n";
				return std::nullopt;
			}
		}

		// check max time for drop off action
		if (new_plan_index < drop_off_option_index) {
			if (request.get_dropoff().get_max_time() < current_time) {
				//std::cout << "Max time for drop off exceeded \n";
				return std::nullopt;
			}
		}

		/* capacity  handling */
		if (new_action->get_action_type() == Action_type::dropoff) {
			free_capacity++;

			// discomfort increment
			unsigned int task_execution_time =
				new_action_data.get_arrival_time() - vehicle_plan.get_other(new_action_data)->get_departure_time();
			new_plan_delay += task_execution_time - new_action->get_request().get_min_travel_time();
		} else {
			// capacity check
			if (free_capacity == 0) {
				//std::cout << "Capacity exceeded \n";
				return std::nullopt;
			}
			free_capacity--;
		}

		// departure time
		new_action_data.set_departure_time(current_time);

		// pickup map
		pickup_map[new_action->get_request().get_index()] = new_plan_index;

		previous_action = new_action;
	}

	// add cost of returning to depot
	const unsigned int travel_time_to_depot = this->travel_time_provider.get()->get_travel_time(
		previous_action->get_node(), vehicle.get_init_position());
	new_plan_cost += travel_time_to_depot;
	current_time += travel_time_to_depot;
	vehicle_plan.set_arrival_time(current_time);

	// max route time check
	if (current_time - vehicle_plan.get_departure_time() > this->max_route_duration) {
		if (!adjust_times(vehicle_plan, Adjustment_reason::max_route_time, vehicle.get_init_position())) {
			//std::cout << "Adjust time failed\n";
			return std::nullopt;
		}
	}

	vehicle_plan.set_cost(new_plan_cost);

	return vehicle_plan;
}

template<typename N, class P>
bool DARP_benchmark_solver<N, P>::adjust_times(
	VehiclePlan<N>& vehicle_plan,
	Adjustment_reason initial_reason, N station_position
) {
	//ActionData<N>* pickup_of_last_action = new_plan_tasks[new_plan_tasks.size() - 1].get_other();
	std::queue<std::pair<ActionData<N>*, Adjustment_reason>> drop_offs_to_resolve;
	drop_offs_to_resolve.emplace(&vehicle_plan[vehicle_plan.get_length() - 1], initial_reason);

	while (!drop_offs_to_resolve.empty()) {
		std::pair to_resolve = drop_offs_to_resolve.front();
		drop_offs_to_resolve.pop();
		ActionData<N>* drop_off_action_data = to_resolve.first;
		Adjustment_reason reason = to_resolve.second;

		unsigned int drop_off_service_time
			= std::max(drop_off_action_data->get_arrival_time(), drop_off_action_data->get_action().get_min_time());

		// difference between current ride time (which breaks constraint) and max ride/route time
		int diff;
		ActionData<N>* pickup_action_data;
		unsigned int travel_time_to_depot = 0;

		if (reason == Adjustment_reason::max_ride_time) {
			pickup_action_data = vehicle_plan->get_other(drop_off_action_data);
			diff = drop_off_service_time - pickup_action_data->get_departure_time() - this->max_ride_time;
		} else {
			pickup_action_data = &vehicle_plan[0];
			travel_time_to_depot = this->travel_time_provider->get_travel_time(
				drop_off_action_data->get_action().get_node(), station_position
			);
			diff = drop_off_service_time + drop_off_action_data->get_action().get_service_duration()
				   + travel_time_to_depot - this->max_route_duration;
		}

		// it can happen that the problem was already solved by moving different action
		if (diff <= 0) {
			continue;
		}

		const Action<N>& pickup_action = pickup_action_data->get_action();


		if (reason == Adjustment_reason::max_ride_time) {
			// set new pickup departure time
			pickup_action_data->set_departure_time(pickup_action_data->get_departure_time() + diff);
		} else {
			// check that we can postpone the arrival to the first action
			if (pickup_action_data->get_arrival_time() + diff > pickup_action.get_max_time()) {
				return false;
			}

			// set new pickup arrival time
			pickup_action_data->set_arrival_time(pickup_action_data->get_arrival_time() + diff);
			vehicle_plan.set_departure_time(vehicle_plan.get_departure_time() + diff);
			unsigned int old_departure_time = pickup_action_data->get_departure_time();
			pickup_action_data->set_departure_time(
				std::max(
					old_departure_time, pickup_action_data->get_arrival_time()
										+ pickup_action.get_service_duration()));
			diff = pickup_action_data->get_departure_time() - old_departure_time;
		}

		// fail if pick up cannot be delayed
		if (pickup_action_data->get_departure_time() - pickup_action.get_service_duration()
			> (int) pickup_action.get_max_time()) {
			//std::cout << "[at] Pick up cannot be delayed\n";
			//std::cout << pickup_action_data->get_departure_time() << "-" << pickup_action.get_service_duration() << "+" << diff << " > ";
			//std::cout << pickup_action.get_max_time() << "\n";
			//std::cout << "diff: " << drop_off_service_time << " - " << pickup_action_data->get_departure_time() << " - " << this->max_ride_time << "\n";
			return false;
		}

		// adjust times for actions from pickup to the end of the plan
		bool solve = false;
		for (ActionData<N>& action_data: vehicle_plan.get_actions()) {
			// solve only actions after adjusted pickup
			if (solve) {
				const Action<N>& action = action_data.get_action();

				unsigned int new_arrival_time = action_data.get_arrival_time() + diff;

				// check max arrival time
				if (new_arrival_time > action.get_max_time()) {
					return false;
				}

				action_data.set_arrival_time(new_arrival_time);

				unsigned int departure_time = action_data.get_departure_time();
				action_data.set_departure_time(
					std::max(departure_time, action_data.get_arrival_time() + action.get_service_duration()));

				// delay update
				diff = action_data.get_departure_time() - departure_time;

				if (action.get_action_type() == Action_type::dropoff) {
					unsigned int service_time
						= std::max(action_data.get_arrival_time(), action_data.get_action().get_min_time());

					// max ride time check
					if (service_time - vehicle_plan.get_other(action_data)->get_departure_time()
						> this->max_ride_time) {
						drop_offs_to_resolve.emplace(&action_data, Adjustment_reason::max_ride_time);
					}

					// max route time check
					if (&action_data == &vehicle_plan[vehicle_plan.get_length() - 1]) {
						if (travel_time_to_depot == 0) {
							travel_time_to_depot = this->travel_time_provider->get_travel_time(
								drop_off_action_data->get_action().get_node(), station_position
							);
						}

						if (action_data.get_departure_time() + travel_time_to_depot - vehicle_plan.get_departure_time()
							> this->max_route_duration) {
							drop_offs_to_resolve.emplace(&action_data, Adjustment_reason::max_route_time);
						}
					}
				}

				// if the delay is 0, we can stop
				if (diff <= 0) {
					break;
				}
			} else if (&action_data == pickup_action_data) {
				solve = true;
			}
		}

	}

	return true;
}



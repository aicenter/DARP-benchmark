
#include <fstream>

//
// Created by Fido on 2020-04-02.
//

template <typename N, class P, class V>
DARP_vehicle_plan<N,P,V>::DARP_vehicle_plan(
    const V& vehicle, 
    unsigned int cost, 
    std::vector<ActionData<N>> actions,
    unsigned int departure_time,
    unsigned int arrival_time
):
	DARP_benchmark_plan_template<ActionData<N>, V>(actions, vehicle, cost, departure_time, arrival_time),
	free_capacity{vehicle.get_capacity()}
{
	for(unsigned short i = 0; i < this->actions.size(); ++i) {
		
        const ActionData<N>& action_data = actions[i];
		if(action_data.get_action_type() == Action_type::pickup) {
            // pickup map
            pickup_map[dynamic_cast<const Service_action<N>&>(action_data.get_action()).get_request().get_index()] = i;
		}
	}
}

template <typename N, class P, class V>
DARP_vehicle_plan<N,P,V>::DARP_vehicle_plan(const V& vehicle, unsigned short size) :
	DARP_benchmark_plan_template<ActionData<N>, V>(vehicle),
    free_capacity{vehicle.get_capacity()}
{
    this->actions.reserve(size);
}

template <typename N, class P, class V>
unsigned short DARP_vehicle_plan<N, P, V>::get_vehicle_capacity() const {
	return this->vehicle.get().get_capacity();
}

template <typename N, class P, class V>
plan_size_type DARP_vehicle_plan<N, P, V>::get_length() const {
	return DARP_benchmark_plan_template<ActionData<N>, V>::get_length();
}

template <typename N, class P, class V>
P DARP_vehicle_plan<N,P,V>::create_bigger(unsigned short increase) const {
    P vehicle_plan = P{ this->vehicle, static_cast<unsigned short>(this->get_length() + increase)};
    vehicle_plan.cost = this->cost;
    vehicle_plan.departure_time = this->departure_time;
    vehicle_plan.arrival_time = this->arrival_time;
    vehicle_plan.free_capacity = free_capacity;
    vehicle_plan.pickup_map = pickup_map;

    for (ActionData<N> action : this->actions) {
        vehicle_plan.actions.emplace_back(action);
    }

    return vehicle_plan;
}


template <typename N, class P, class V>
void DARP_vehicle_plan<N,P,V>::set_vehicle(const std::reference_wrapper<const V>& vehicle_par) {
	this->vehicle = vehicle_par;
}

template <typename N, class P, class V>
void DARP_vehicle_plan<N,P,V>::set_departure_time(unsigned long new_departure_time) {
    this->departure_time = new_departure_time;
}

template <typename N, class P, class V>
void DARP_vehicle_plan<N,P,V>::set_arrival_time(unsigned long new_arrival_time) {
    this->arrival_time = new_arrival_time;
}

template <typename N, class P, class V>
void DARP_vehicle_plan<N,P,V>::set_cost(unsigned int new_cost) {
    this->cost = new_cost;
}

template <typename N, class P, class V>
const std::vector<ActionData<N>>& DARP_vehicle_plan<N, P, V>::get_actions() const {
    return DARP_benchmark_plan_template<ActionData<N>, V>::get_actions();
}

template <typename N, class P, class V>
std::vector<ActionData<N>>& DARP_vehicle_plan<N,P,V>::get_actions() {
    return this->actions;
}

template <typename N, class P, class V>
ActionData<N>& DARP_vehicle_plan<N,P,V>::operator[](plan_size_type index) {
    return this->actions[index];
}

//template <typename N, class P>
//Plan_evaluator_action_interface& DARP_vehicle_plan<N,P>::operator[](unsigned short index) {
//    return this->actions[index];
//}

template <typename N, class P, class V>
const ActionData<N>& DARP_vehicle_plan<N,P,V>::get_first_action() const {
    assert(!this->actions.empty());
    return this->actions.front();
}

template <typename N, class P, class V>
ActionData<N>& DARP_vehicle_plan<N,P,V>::get_first_action() {
    assert(!this->actions.empty());
    return this->actions.front();
}

template <typename N, class P, class V>
const ActionData<N>& DARP_vehicle_plan<N,P,V>::get_last_action() const {
    assert(!this->actions.empty());
    return this->actions.back();
}

template <typename N, class P, class V>
ActionData<N>* DARP_vehicle_plan<N,P,V>::get_pickup(const Service_action<N>& drop_off_action) {
	const unsigned int request_index = drop_off_action.get_request().get_index();
	assert(pickup_map.contains(request_index));
	return &this->actions[pickup_map[request_index]];
}

template <typename N, class P, class V>
const ActionData<N>* DARP_vehicle_plan<N,P,V>::get_pickup(const Service_action<N>& drop_off_action) const {
	const unsigned int request_index = drop_off_action.get_request().get_index();
    assert(pickup_map.contains(request_index));
	return &this->actions[pickup_map.at(request_index)];
}

template <typename N, class P, class V>
ActionData<N>* DARP_vehicle_plan<N,P,V>::get_pickup(const ActionData<N>& drop_off_action_data) {
    return get_pickup(dynamic_cast<const Service_action<N>&>(drop_off_action_data.get_action()));
}

template <typename N, class P, class V>
const ActionData<N>* DARP_vehicle_plan<N,P,V>::get_pickup(const ActionData<N>& drop_off_action_data) const {
    return get_pickup(drop_off_action_data.get_action());
}

template <typename N, class P, class V>
index_in_plan DARP_vehicle_plan<N, P, V>::get_pickup_index(const ActionData<N>& drop_off_action_data) const {
	const unsigned int request_index = dynamic_cast<const Service_action<N>&>(drop_off_action_data.get_action()).get_request().get_index();
    assert(pickup_map.contains(request_index));
	return pickup_map.at(request_index);
}

template <typename N, class P, class V>
bool DARP_vehicle_plan<N,P,V>::contains_pickup(const Service_action<N>& drop_off_action) const {
    const unsigned int request_index = drop_off_action.get_request().get_index();
    return pickup_map.contains(request_index);
}

//template <typename N, class P>
//unsigned int DARP_vehicle_plan<N,P>::get_servicing_start() const {
//    return get_first_action().get_service_start_time();
//}
//
//template <typename N, class P>
//unsigned int DARP_vehicle_plan<N,P>::get_servicing_end() const {
//    return get_last_action().get_departure_time();
//}

template <typename N, class P, class V>
typename std::vector<ActionData<N>>::iterator DARP_vehicle_plan<N,P,V>::begin() {
    return this->actions.begin();
}

template <typename N, class P, class V>
typename std::vector<ActionData<N>>::iterator DARP_vehicle_plan<N,P,V>::end() {
    return this->actions.end();
}


template <typename N, class P, class V>
const N& DARP_vehicle_plan<N,P,V>::get_first_action_node() const {
    return this->actions[0].get_action().get_node();
}

template <typename N, class P, class V>
const N& DARP_vehicle_plan<N,P,V>::get_last_action_node() const {
    return this->get_last_action().get_node();
}

template <typename N, class P, class V>
ActionData<N>& DARP_vehicle_plan<N,P,V>::add_action(const Action<N>& action)
{
    ActionData<N>& new_action_data = this->actions.emplace_back(action);

	if(action.get_action_type() == Action_type::pickup) {
        assert(free_capacity > 0);
        --free_capacity;

        // pickup map
        pickup_map[dynamic_cast<const Service_action<N>&>(action).get_request().get_index()]
			= static_cast<unsigned short>(this->actions.size()) - 1;
    }
    else if(action.get_action_type() == Action_type::dropoff) {
        ++free_capacity;

        // pickup / drop off referencing
        ActionData<N>* pickup_action_data = get_pickup(new_action_data);
        pickup_action_data->set_other_action_data_index(static_cast<index_in_plan>(this->actions.size()) - 1);
        new_action_data.set_other_action_data_index(get_pickup_index(new_action_data));
    }
	
    return new_action_data;
}

template <typename N, class P, class V>
void DARP_vehicle_plan<N,P,V>::remove_last_action() {
    assert(this->get_length() > 0);

    if (this->actions.back().get_action().get_action_type() == Action_type::pickup) {
        ++free_capacity;
    	
        // pickup map
        pickup_map.erase(dynamic_cast<const Service_action<N>&>(this->actions.back().get_action()).get_request().get_index());
    }
    else {
        --free_capacity;
    }

    this->actions.erase(this->actions.end() - 1);
}

template <typename N, class P, class V>
unsigned short DARP_vehicle_plan<N,P,V>::get_free_capacity() const {
    return free_capacity;
}

template<typename N, class P, class V>
void DARP_vehicle_plan<N,P,V>::remove_last_action(const bool was_last) {

    if (this->get_length() > 1) {
        const ActionData<N>& new_last_action = this->actions[this->get_length() - 2];
        const unsigned int travel_time = get_last_action().get_arrival_time() - new_last_action.get_departure_time();

        // cost adjustment
        unsigned int new_plan_cost = this->get_cost() - travel_time;

        if (get_last_action().get_action().get_action_type() == Action_type::dropoff) {
            // pickup / drop off dereferencing
            ActionData<N>* pickup_action_data = get_pickup(get_last_action());
            pickup_action_data->set_other_action_data_index(-1);
        }

        if (was_last) {
            // remove cost of returning to depot
            const unsigned int travel_time_to_depot
                = this->get_arrival_time() - get_last_action().get_departure_time();
            new_plan_cost -= travel_time_to_depot;
            set_arrival_time(0);
        }

        set_cost(new_plan_cost);
    }
    else {
        set_cost(0);
    }

	// actual removal of the last action
    remove_last_action();
}

template <typename N, class P, class V>
ActionData<N>& DARP_vehicle_plan<N,P,V>::get_other(const ActionData<N>& source_action_data) {
    return this->actions[source_action_data.get_other_action_data_index()];
}

template <typename N, class P, class V>
const ActionData<N>& DARP_vehicle_plan<N,P,V>::get_other(const ActionData<N>& source_action_data) const {
	return this->actions[source_action_data.get_other_action_data_index()];
}

template <typename N, class P, class V>
unsigned DARP_vehicle_plan<N,P,V>::get_ride_time(const ActionData<N>& action_data_par) const  {
	if(action_data_par.get_service_start_time() > 0) {
        return action_data_par.get_service_start_time() - get_other(action_data_par).get_departure_time();
	}
    else {
	    return action_data_par.get_arrival_time() - get_other(action_data_par).get_departure_time();
    }
}

template <typename N, class P, class V>
bool DARP_vehicle_plan<N,P,V>::check(const DARP_instance_configuration& configuration) const {
    unsigned int time = this->get_departure_time();
	for (const ActionData<N>& action_data : this->actions) {
        if(action_data.get_action_type() == Action_type::depot) {
            continue;
        }

		unsigned int new_time = action_data.get_arrival_time();
		if(new_time < time){
            throw std::runtime_error("Time is not increasing");
        }
		time = new_time;
		new_time = action_data.get_departure_time();
		if(new_time < time){
            throw std::runtime_error("Time is not increasing");
        }
		time = new_time;
		if (configuration.get_max_ride_time() && action_data.get_action_type() == Action_type::dropoff) {
			if(get_ride_time(action_data) > configuration.get_max_ride_time()){
                throw std::runtime_error("Max ride time exceeded");
            }
		}
	}
	if(this->get_arrival_time() < time){
        throw std::runtime_error("Time is not increasing");
    }

    return true;
}


template<typename N, class P, class V>
bool DARP_vehicle_plan<N, P, V>::full_check(
	const DARP_instance_configuration& instance_configuration,
	const Travel_time_provider<N>& travel_time_provider,
	bool skip_time_check,
	bool skip_cost_check,
	std::vector<bool>* served_or_dropped_requests
) const {
	auto start_time = instance_configuration.get_start_time();

	// check that the vehicles do not start too early
	if(is_feasible() && start_time > this->departure_time){
		throw std::runtime_error("Vehicle starts too early");
	}

	unsigned time = this->departure_time;
	unsigned plan_cost_computed = 0;

	// Track current location for travel time calculations (nullptr for virtual vehicles on first action)
	const N* current_location = nullptr;
	bool is_first_action = true;
	for(const auto& action: this->actions) {
		if(action.get_action_type() == Action_type::depot) {
			continue;
		}

		// arrival time check
		travel_time_type travel_time = 0;
		if (is_first_action) {
			// Use travel time provider's vehicle-aware method for first action
			travel_time = travel_time_provider.get_travel_time_from_vehicle(this->get_vehicle(), action.get_node());
			is_first_action = false;
		} else {
			travel_time = travel_time_provider.get_travel_time(*current_location, action.get_node());
		}
		time += travel_time;
		plan_cost_computed += travel_time;

		if(!skip_time_check && time != action.get_arrival_time()) {
			throw std::runtime_error("Arrival time does not match travel time");
		}

		// waiting to min time
		if(time < action.get_min_time()){
			time = action.get_min_time();
		}

		time += action.get_service_duration();

		// departure time check
		if(!skip_time_check && time != static_cast<unsigned>(action.get_departure_time())) {
			throw std::runtime_error("Departure time does not match the computed departure time");
		}

		time = action.get_departure_time();

		if(served_or_dropped_requests != nullptr && action.get_action_type() == Action_type::dropoff) {
			served_or_dropped_requests->operator[](action.get_request_index()) = true;
		}

		// update current location to this action node
		current_location = &action.get_node();
	}

	if(!skip_cost_check && plan_cost_computed != this->get_cost()) {
		throw std::runtime_error("plan cost does not match the computed plan cost");
	}

	return true;
}

template <typename N, class P, class V>
std::optional<P> DARP_vehicle_plan<N,P,V>::get_delayed_plan(unsigned delay, unsigned variant_id_par) const {
    P delayed_plan = *((P*) this);

	// we do not have to modify the departure time, it serves no purpose when the plans are successfully connected
	//delayed_plan.set_departure_time(delayed_plan.get_departure_time() + delay);

	// delay first action departure
	ActionData<N>& first_action = delayed_plan.get_first_action();
	unsigned int new_departure_time = first_action.get_departure_time() + delay;
	if(new_departure_time - first_action.get_action().get_service_duration() > first_action.get_max_time()) {
		return std::nullopt;
	}
	delayed_plan.get_first_action().set_departure_time(new_departure_time);
	
	// for the other actions, follow this procedure
	for (unsigned short i = 1; i < delayed_plan.get_length(); ++i) {
		ActionData<N>& action_data = delayed_plan[i];
		unsigned int new_arrival_time = action_data.get_arrival_time() + delay;
		if (new_arrival_time > action_data.get_max_time()) {
			return std::nullopt;
		}
		action_data.set_arrival_time(new_arrival_time);

		const unsigned int old_departure_time = action_data.get_departure_time();
		// delay service time if needed
		unsigned int new_service_time = std::max<unsigned long>(action_data.get_service_start_time(),
			action_data.get_arrival_time());
		new_departure_time = new_service_time + action_data.get_action().get_service_duration();
		action_data.set_departure_time(new_departure_time);

		// recompute the delay
		delay = new_departure_time - old_departure_time;

		if (delay == 0) {
			break;
		}
	}

	if (delay > 0) {
		delayed_plan.set_arrival_time(delayed_plan.get_arrival_time() + delay);
	}

    delayed_plan.variant_id = variant_id_par;

	return delayed_plan;
}

template<typename N, class P, class V>
void DARP_vehicle_plan<N, P, V>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
	writer.StartObject();
    writer.Key("cost");
    writer.Uint(this->cost);
    writer.Key("vehicle");
    this->vehicle.get().JSON_serialize(writer);
    writer.Key("departure_time");
    writer.Uint(this->departure_time);
    writer.Key("arrival_time");
    writer.Uint(this->arrival_time);
    writer.Key("actions");
    writer.StartArray();
    for (const ActionData<N>& action : this->actions) {
        action.JSON_serialize(writer);
    }
    writer.EndArray();
    writer.EndObject();
}

template <typename N, class P, class V>
void export_plans(const std::vector<DARP_vehicle_plan<N,P,V>>& plans, std::string file_path) {
	rapidjson::StringBuffer string_buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(string_buffer);
	writer.StartArray();
    for(const DARP_vehicle_plan<N,P,V>& plan: plans){
        plan.JSON_serialize(writer);
    }
    writer.EndArray();

	std::ofstream out_file(file_path);
	out_file << string_buffer.GetString();
	out_file.close();
}

template<typename N, class V>
index_in_plan VehiclePlan<N, V>::get_last_service_action_index() const {
	if(this->get_length() == 1) {
		return 0;
	}
	if(this->actions[this->get_length() - 1].get_action_type() == Action_type::depot) {
		return static_cast<index_in_plan>(this->get_length()) - 2;
	}

	return static_cast<index_in_plan>(this->get_length()) - 1;
}



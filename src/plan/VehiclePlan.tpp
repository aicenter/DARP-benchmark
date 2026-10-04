
#include <algorithm>
#include <concepts>
#include <fstream>
#include <cstring>
#include <memory>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

//
// Created by Fido on 2020-04-02.
//

template <typename N>
std::vector<ActionData<N>> deserialize_action_data_list(
	const rapidjson::Value& actions_array,
	const DARP_instance<N>& darp_instance
) {
	if (!actions_array.IsArray()) {
		throw std::runtime_error("deserialize_action_data_list: expected a JSON array");
	}
	const auto& json_actions = actions_array.GetArray();
	std::vector<ActionData<N>> actions;
	actions.reserve(json_actions.Size());
	std::unordered_map<unsigned, index_in_plan> pickups;
	for (index_in_plan i = 0; i < static_cast<index_in_plan>(json_actions.Size()); ++i) {
		const auto& json_action_data = json_actions[i];
		const auto& json_action = json_action_data["action"];
		const char* const type_str = json_action["type"].GetString();
		const Action_type action_type = action_type_from_string(type_str);
		if (action_type == Action_type::depot) {
			if (std::strcmp(type_str, "depot") == 0) {
				throw std::runtime_error("deserialize_action_data_list: depot rows in actions are not supported");
			}
			throw std::runtime_error(
				std::string("deserialize_action_data_list: unsupported action type: ") + type_str
			);
		}

		if (!json_action.HasMember("request_index")) {
			throw std::runtime_error("deserialize_action_data_list: pickup and drop_off actions require request_index");
		}
		const unsigned request_index = json_action["request_index"].GetUint();
		if (request_index >= static_cast<unsigned>(darp_instance.get_requests().size())) {
			throw std::runtime_error("deserialize_action_data_list: request_index does not match any request in the instance");
		}
		const Request<N>& request = darp_instance.get_requests()[request_index];

		index_in_plan other_index = 0;
		const Action<N>* action_ptr = nullptr;
		if (action_type == Action_type::dropoff) {
			const auto pickup_it = pickups.find(request_index);
			if (pickup_it != pickups.end()) {
				other_index = pickup_it->second;
				actions[other_index].set_other_action_data_index(static_cast<index_in_plan>(i));
			} else {
				other_index = static_cast<index_in_plan>(-1);
			}
			action_ptr = &request.get_dropoff();
		} else {
			action_ptr = &request.get_pickup();
			pickups[request_index] = i;
		}
		actions.emplace_back(json_action_data, i, other_index, *action_ptr);
	}
	return actions;
}

inline bool vehicle_plan_json_references_virtual_vehicle(const rapidjson::Value& vehicle_json) {
	if (vehicle_json.HasMember("actions")) {
		throw std::runtime_error(
			"vehicle_plan_json_references_virtual_vehicle: value looks like a full plan (has \"actions\"); pass the vehicle object only");
	}
	if (!vehicle_json.HasMember("type")) {
		return false;
	}
	if (!vehicle_json["type"].IsString()) {
		throw std::runtime_error("vehicle_plan_json_references_virtual_vehicle: vehicle type must be a string");
	}
	return std::strcmp(vehicle_json["type"].GetString(), "virtual") == 0;
}

inline void validate_virtual_vehicle_plan_reference(
	const rapidjson::Value& vehicle_json,
	const Virtual_vehicle& expected_virtual_vehicle
) {
	if (!vehicle_plan_json_references_virtual_vehicle(vehicle_json)) {
		throw std::runtime_error("validate_virtual_vehicle_plan_reference: plan vehicle does not reference a virtual vehicle");
	}
	Virtual_vehicle::JSON_deserialize(vehicle_json, expected_virtual_vehicle);
}

template <typename N>
const Vehicle<N>& validate_normal_vehicle_plan_reference(
	const rapidjson::Value& vehicle_json,
	const std::vector<Vehicle<N>>& vehicles
) {
	if (vehicle_plan_json_references_virtual_vehicle(vehicle_json)) {
		throw std::runtime_error("validate_normal_vehicle_plan_reference: virtual vehicle is not Vehicle<N>");
	}
	if (vehicle_json.HasMember("type")) {
		throw std::runtime_error("validate_normal_vehicle_plan_reference: normal vehicle references must not declare type");
	}
	if (!vehicle_json.HasMember("index")) {
		throw std::runtime_error("validate_normal_vehicle_plan_reference: normal vehicle reference requires index");
	}
	const unsigned vehicle_index = vehicle_json["index"].GetUint();
	const auto it = std::find_if(vehicles.begin(), vehicles.end(),
		[&](const Vehicle<N>& v) { return v.get_index() == vehicle_index; });
	if (it != vehicles.end()) {
		return *it;
	}
	throw std::runtime_error("validate_normal_vehicle_plan_reference: index not found in provided vehicle collection");
}

template <typename N>
std::vector<Vehicle<N>> deserialize_vehicle_list_from_json_array(
	const rapidjson::Value& vehicles_array,
	time_type operation_start
) {
	if (!vehicles_array.IsArray()) {
		throw std::runtime_error("deserialize_vehicle_list_from_json_array: expected a JSON array");
	}
	std::vector<Vehicle<N>> vehicles;
	const auto arr = vehicles_array.GetArray();
	vehicles.reserve(arr.Size());
	for (const auto& vehicle_json : arr) {
		vehicles.push_back(Vehicle<N>::JSON_deserialize(vehicle_json, operation_start));
	}
	return vehicles;
}

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
P DARP_vehicle_plan<N, P, V>::JSON_deserialize(
	const rapidjson::Value& plan_data,
	const DARP_instance<N>& darp_instance,
	const std::vector<Vehicle<N>>& vehicles,
	const Virtual_vehicle* virtual_vehicle
) {
	const auto& veh_json = plan_data["vehicle"];
	const V* vehicle = nullptr;
	if (vehicle_plan_json_references_virtual_vehicle(veh_json)) {
		if constexpr (std::derived_from<Virtual_vehicle, V>) {
			if (virtual_vehicle == nullptr) {
				throw std::runtime_error("DARP_vehicle_plan::JSON_deserialize: plan references virtual vehicle but no virtual_vehicle was provided");
			}
			validate_virtual_vehicle_plan_reference(veh_json, *virtual_vehicle);
			vehicle = virtual_vehicle;
		} else {
			throw std::runtime_error(
				"DARP_vehicle_plan::JSON_deserialize: plan references virtual vehicle; use a plan type with Vehicle_base as the vehicle type"
			);
		}
	} else {
		vehicle = &validate_normal_vehicle_plan_reference<N>(veh_json, vehicles);
	}

	std::vector<ActionData<N>> actions = deserialize_action_data_list<N>(plan_data["actions"], darp_instance);
	const unsigned int cost = plan_data["cost"].GetUint();
	const unsigned int departure_time = plan_data["departure_time"].GetUint();
	const unsigned int arrival_time = plan_data["arrival_time"].GetUint();
	return P(*vehicle, cost, std::move(actions), departure_time, arrival_time);
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
void DARP_vehicle_plan<N,P,V>::append_simple_csv_rows(int plan_index, std::ostringstream& csv) const {
	for (const ActionData<N>& action : this->get_actions()) {
		if (action.is_drop_off()) {
			const ActionData<N>* pickup = get_pickup(action);
			if (pickup) {
				csv << plan_index << "," << action.get_request_index() << ","
					<< pickup->get_service_start_time() << ","
					<< action.get_service_start_time() << "\n";
			}
		}
	}
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
    return get_pickup(dynamic_cast<const Service_action<N>&>(drop_off_action_data.get_action()));
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
unsigned int DARP_vehicle_plan<N, P, V>::get_cost() const {
	return Base_plan<ActionData<N>, V>::get_cost();
}

template <typename N, class P, class V>
unsigned long DARP_vehicle_plan<N, P, V>::get_driving_time() const {
	unsigned long total_route_time = this->get_arrival_time() - this->get_departure_time();
	unsigned long time_at_stops = 0;
	for (const ActionData<N>& action_data : this->actions) {
		int dep = action_data.get_departure_time();
		if (dep >= 0) {
			unsigned int arr = action_data.get_arrival_time();
			if (static_cast<unsigned>(dep) >= arr) {
				time_at_stops += static_cast<unsigned>(dep) - arr;
			}
		}
	}
	return total_route_time - time_at_stops;
}

template <typename N, class P, class V>
unsigned long DARP_vehicle_plan<N, P, V>::get_ride_time() const {
	unsigned long total = 0;
	for (const ActionData<N>& action_data : this->actions) {
		if (action_data.get_action_type() == Action_type::dropoff) {
			total += get_ride_time(action_data);
		}
	}
	return total;
}

template <typename N, class P, class V>
unsigned long DARP_vehicle_plan<N, P, V>::get_passenger_delay() const {
	unsigned long total_delay = 0;
	for (const ActionData<N>& action_data : this->actions) {
		if (action_data.get_action_type() == Action_type::dropoff) {
			unsigned long drop_off_time = action_data.get_arrival_time();
			unsigned long min_time = action_data.get_min_time();
			if (drop_off_time > min_time) {
				total_delay += drop_off_time - min_time;
			}
		}
	}
	return total_delay;
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
	if constexpr (requires { this->vehicle.get().get_operation_start(); }) {
		start_time = this->vehicle.get().get_operation_start();
	}

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

template <typename N, Benchmark_plan P>
void deserialize_vehicle_plans_from_json_array(
	const rapidjson::Value& plans_array,
	const DARP_instance<N>& darp_instance,
	const std::vector<Vehicle<N>>& vehicles,
	const Virtual_vehicle* virtual_vehicle,
	std::vector<P>& plans_out
) {
	if (!plans_array.IsArray()) {
		throw std::runtime_error("deserialize_vehicle_plans_from_json_array: expected a JSON array");
	}

	const auto arr = plans_array.GetArray();
	plans_out.clear();
	plans_out.reserve(arr.Size());
	for (const auto& plan_data : arr) {
		plans_out.push_back(P::JSON_deserialize(plan_data, darp_instance, vehicles, virtual_vehicle));
	}
}

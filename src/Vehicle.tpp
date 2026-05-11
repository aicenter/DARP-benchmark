//
// Created by Fido on 2020-04-02.
//

#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "serialization.h"

// Concept to check if a type has JSON_serialize method
template<typename T>
concept HasJSONSerialize = requires(const T& t, rapidjson::PrettyWriter<rapidjson::StringBuffer>& w) {
    { t.JSON_serialize(w) } -> std::same_as<void>;
};

template <typename N>
Vehicle<N>::Vehicle(unsigned int index, std::shared_ptr<N> initial_position, unsigned short capacity, time_type operation_start)
    : Vehicle_base(capacity)
    , index(index)
    , init_position(std::move(initial_position))
    , operation_start(operation_start) {
}

template <typename N>
const N& Vehicle<N>::get_init_position() const {
    return *init_position;
}

template <typename N>
const std::shared_ptr<N>& Vehicle<N>::get_init_position_ptr() const {
    return init_position;
}

template <typename N>
unsigned int Vehicle<N>::get_index() const {
    return index;
}

template <typename N>
time_type Vehicle<N>::get_operation_start() const {
    return operation_start;
}

template <typename N>
const Vehicle<N>& Vehicle<N>::JSON_deserialize(
	const rapidjson::Value& vehicle_json,
	const std::vector<Vehicle<N>>& vehicles
) {
	if (vehicle_json.HasMember("actions")) {
		throw std::runtime_error(
			"Vehicle::JSON_deserialize: value looks like a full plan (has \"actions\"); pass the vehicle object only");
	}
	if (vehicle_json.HasMember("type") && std::strcmp(vehicle_json["type"].GetString(), "virtual") == 0) {
		throw std::runtime_error("Vehicle::JSON_deserialize(collection): virtual vehicle is not Vehicle<N>");
	}
	if (!vehicle_json.HasMember("index")) {
		throw std::runtime_error("Vehicle::JSON_deserialize(collection): missing index");
	}
	const unsigned vehicle_index = vehicle_json["index"].GetUint();
	for (const Vehicle<N>& v : vehicles) {
		if (v.get_index() == vehicle_index) {
			return v;
		}
	}
	throw std::runtime_error("Vehicle::JSON_deserialize(collection): index not found in vehicle collection");
}

template <typename N>
Vehicle<N> Vehicle<N>::JSON_deserialize(const rapidjson::Value& vehicle_json, const time_type operation_start) {
	if (vehicle_json.HasMember("actions")) {
		throw std::runtime_error(
			"Vehicle::JSON_deserialize: value looks like a full plan (has \"actions\"); pass the vehicle object only");
	}
	if (vehicle_json.HasMember("type") && std::strcmp(vehicle_json["type"].GetString(), "virtual") == 0) {
		throw std::runtime_error("Vehicle::JSON_deserialize(materialize): virtual vehicle object cannot be materialized as Vehicle<N>");
	}
	if constexpr (!std::is_constructible_v<N, unsigned>) {
		throw std::runtime_error(
			"Vehicle::JSON_deserialize(materialize): fleet-sized vehicle materialization is not supported for this node type");
	} else {
		if (!vehicle_json.HasMember("init_position") || !vehicle_json["init_position"].HasMember("index")) {
			throw std::runtime_error(
				"Vehicle::JSON_deserialize(materialize): fleet-sizing vehicle requires init_position.index in JSON");
		}
		unsigned short cap = 4;
		if (vehicle_json.HasMember("capacity")) {
			cap = static_cast<unsigned short>(vehicle_json["capacity"].GetUint());
		}
		const auto vehicle_index = vehicle_json["index"].GetUint();
		const unsigned init_idx = vehicle_json["init_position"]["index"].GetUint();
		std::shared_ptr<N> pos = std::make_shared<N>(init_idx);
		return Vehicle<N>(vehicle_index, std::move(pos), cap, operation_start);
	}
}

template <typename N>
std::unique_ptr<Vehicle_base> Vehicle<N>::JSON_deserialize_real_or_virtual(
	const rapidjson::Value& vehicle_json,
	const time_type operation_start_for_concrete
) {
	if (vehicle_json.HasMember("type") && std::strcmp(vehicle_json["type"].GetString(), "virtual") == 0) {
		return std::make_unique<Virtual_vehicle>(Virtual_vehicle::JSON_deserialize(vehicle_json));
	}
	return std::make_unique<Vehicle<N>>(Vehicle<N>::JSON_deserialize(vehicle_json, operation_start_for_concrete));
}

template <typename N>
const Vehicle_base& Vehicle<N>::JSON_deserialize_real_or_virtual(
	const rapidjson::Value& vehicle_json,
	const std::vector<Vehicle<N>>& vehicles,
	const Virtual_vehicle& expected_when_virtual
) {
	if (vehicle_json.HasMember("type") && std::strcmp(vehicle_json["type"].GetString(), "virtual") == 0) {
		Virtual_vehicle::JSON_deserialize(vehicle_json, expected_when_virtual);
		return expected_when_virtual;
	}
	return Vehicle<N>::JSON_deserialize(vehicle_json, vehicles);
}

template<typename N>
void Vehicle<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("index");
    writer.Uint(index);
    writer.Key("init_position");
    if constexpr (HasJSONSerialize<N>) {
        if(init_position){
            serialize_node(writer, *init_position);
        } else {
            writer.Null();
        }
    } else {
        writer.String("not serializable");
    }
    writer.Key("capacity");
    writer.Uint(get_capacity());
    writer.EndObject();
}

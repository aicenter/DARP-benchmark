//
// Created by Fido on 2020-04-02.
//

#include "Vehicle.h"

#include <cstring>
#include <stdexcept>
#include <utility>

Virtual_vehicle::Virtual_vehicle(
	unsigned short capacity,
	unsigned int time_to_start,
	unsigned int vehicle_count,
	std::optional<std::unordered_set<request_index_type>>&& allowed_requests
) :
	Vehicle_base(capacity),
	time_to_start(time_to_start),
	vehicle_count(vehicle_count),
	allowed_requests(allowed_requests)
{
}

unsigned int Virtual_vehicle::get_time_to_start() const {
    return time_to_start;
}

unsigned int Virtual_vehicle::get_vehicle_count() const {
    return vehicle_count;
}

/** When set, only these request indices may appear on plans for this virtual vehicle (e.g. new batch only). */
[[nodiscard]] const std::optional<std::unordered_set<request_index_type>>& Virtual_vehicle::get_allowed_requests() const {
	return allowed_requests;
}

namespace {

void require_virtual_type_in_json(const rapidjson::Value& vehicle_json) {
	if (!vehicle_json.HasMember("type") || std::strcmp(vehicle_json["type"].GetString(), "virtual") != 0) {
		throw std::runtime_error("Virtual_vehicle::JSON_deserialize: expected type \"virtual\"");
	}
}

} // namespace

Virtual_vehicle Virtual_vehicle::JSON_deserialize(const rapidjson::Value& vehicle_json) {
	require_virtual_type_in_json(vehicle_json);
	const unsigned short capacity = vehicle_json.HasMember("capacity")
		? static_cast<unsigned short>(vehicle_json["capacity"].GetUint())
		: static_cast<unsigned short>(4);
	const unsigned int time_to_start = vehicle_json.HasMember("time_to_start") ? vehicle_json["time_to_start"].GetUint() : 0;
	const unsigned int vehicle_count = vehicle_json.HasMember("vehicle_count") ? vehicle_json["vehicle_count"].GetUint() : 1;
	return Virtual_vehicle(capacity, time_to_start, vehicle_count);
}

void Virtual_vehicle::JSON_deserialize(const rapidjson::Value& vehicle_json, const Virtual_vehicle& expected) {
	require_virtual_type_in_json(vehicle_json);
	if (vehicle_json.HasMember("capacity")) {
		const auto c = static_cast<unsigned short>(vehicle_json["capacity"].GetUint());
		if (c != expected.get_capacity()) {
			throw std::runtime_error("Virtual_vehicle::JSON_deserialize: JSON capacity does not match expected vehicle");
		}
	}
	if (vehicle_json.HasMember("time_to_start")) {
		const auto t = vehicle_json["time_to_start"].GetUint();
		if (t != expected.get_time_to_start()) {
			throw std::runtime_error("Virtual_vehicle::JSON_deserialize: JSON time_to_start does not match expected vehicle");
		}
	}
	if (vehicle_json.HasMember("vehicle_count")) {
		const auto vc = vehicle_json["vehicle_count"].GetUint();
		if (vc != expected.get_vehicle_count()) {
			throw std::runtime_error("Virtual_vehicle::JSON_deserialize: JSON vehicle_count does not match expected vehicle");
		}
	}
}

void Virtual_vehicle::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
	writer.StartObject();
	writer.Key("type");
	writer.String("virtual");
	writer.Key("capacity");
	writer.Uint(get_capacity());
	writer.Key("time_to_start");
	writer.Uint(time_to_start);
	writer.Key("vehicle_count");
	writer.Uint(vehicle_count);
	writer.EndObject();
}
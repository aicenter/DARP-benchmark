//
// Created by Fido on 2020-04-02.
//

#include "Vehicle.h"

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
/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

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

void reject_non_virtual_type_in_json(const rapidjson::Value& vehicle_json) {
	if (vehicle_json.HasMember("type") && std::strcmp(vehicle_json["type"].GetString(), "virtual") != 0) {
		throw std::runtime_error("Virtual_vehicle::JSON_deserialize: expected type \"virtual\"");
	}
}

} // namespace

Virtual_vehicle Virtual_vehicle::JSON_deserialize(const rapidjson::Value& vehicle_json) {
	reject_non_virtual_type_in_json(vehicle_json);
	if (!vehicle_json.HasMember("capacity")
		|| !vehicle_json.HasMember("time_to_start")
		|| !vehicle_json.HasMember("vehicle_count")) {
		throw std::runtime_error(
			"Virtual_vehicle::JSON_deserialize: expected capacity, time_to_start, and vehicle_count");
	}
	const auto capacity = static_cast<unsigned short>(vehicle_json["capacity"].GetUint());
	const unsigned int time_to_start = vehicle_json["time_to_start"].GetUint();
	const unsigned int vehicle_count = vehicle_json["vehicle_count"].GetUint();
	return Virtual_vehicle(capacity, time_to_start, vehicle_count);
}

void Virtual_vehicle::JSON_deserialize(const rapidjson::Value& vehicle_json, const Virtual_vehicle& expected) {
	reject_non_virtual_type_in_json(vehicle_json);
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

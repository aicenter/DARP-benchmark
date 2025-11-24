#pragma once

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

struct Chaining_result{
	const unsigned int variant_count;

	const unsigned int connection_count;

	const unsigned int route_count;

	const unsigned int computational_time;

	const unsigned int route_cost;

	const unsigned int extra_vehicle_cost_cost;

	void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
};

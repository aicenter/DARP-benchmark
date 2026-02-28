#pragma once

#include <sstream>

#include "../ActionData.h"
#include "../Vehicle.h"
#include "Base_plan.h"


/**
 * @brief Base class for all plans that are used in the benchmark executable. It is parametrized only by the type of
 * the nodes, but actions are always of type ActionData<N> and vehicles can be any Vehicle_base derived type.
 * @tparam N node type
 */
class Benchmark_vehicle_plan{
public:
	virtual void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const = 0;

	virtual void append_simple_csv_rows(int plan_index, std::ostringstream& csv) const = 0;

};

#include "Benchmark_vehicle_plan.tpp"

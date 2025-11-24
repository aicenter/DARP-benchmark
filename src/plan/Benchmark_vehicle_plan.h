#pragma once

#include "../ActionData.h"
#include "../Vehicle.h"
#include "Base_plan.h"


/**
 * @brief Base class for all plans that are used in the benchmark executable. It is parametrized only by the type of
 * the nodes, but actions are always of type ActionData<N> and vehicles are always of type Vehicle<N>.
 * @tparam N node type
 */
class Benchmark_vehicle_plan{
public:
	virtual void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const = 0;

};

#include "Benchmark_vehicle_plan.tpp"

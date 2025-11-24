#pragma once

#include <vector>
#include "Vehicle.h"

template<typename N>
class Nearest_vehicle_provider {
public:
	virtual ~Nearest_vehicle_provider() = default;
	virtual size_t get_nearest_vehicle(
		const N& location,
		const std::vector<const Vehicle<N>*>& vehicles
	) = 0;
};

#include "DARP_instance.h"


unsigned Node::get_index() const {
	return index;
}

unsigned long DARP_instance_configuration::get_max_route_duration() const {
	return max_route_duration;
}

unsigned long DARP_instance_configuration::get_max_ride_time() const {
	return max_ride_time;
}

bool DARP_instance_configuration::is_return_to_depot() const {
	return return_to_depot;
}

bool DARP_instance_configuration::use_virtual_vehicles() const {
	return virtual_vehicles;
}

unsigned DARP_instance_configuration::get_start_time() const {
	return start_time;
}

unsigned short DARP_instance_configuration::get_vehicle_capital_cost() const {
	return vehicle_capital_cost;
}

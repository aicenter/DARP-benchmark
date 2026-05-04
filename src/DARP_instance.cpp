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

double DARP_instance_configuration::get_relative_delay_cost() const {
	return relative_delay_cost;
}

problem_type DARP_instance_configuration::get_problem() const {
	return problem;
}

void DARP_instance_configuration::set_max_route_duration(unsigned long value) {
	max_route_duration = value;
}

void DARP_instance_configuration::set_max_ride_time(unsigned long value) {
	max_ride_time = value;
}

void DARP_instance_configuration::set_return_to_depot(bool value) {
	return_to_depot = value;
}

void DARP_instance_configuration::set_virtual_vehicles(bool value) {
	virtual_vehicles = value;
}

void DARP_instance_configuration::set_start_time(unsigned value) {
	start_time = value;
}

void DARP_instance_configuration::set_vehicle_capital_cost(unsigned short value) {
	vehicle_capital_cost = value;
}

void DARP_instance_configuration::set_relative_delay_cost(double value) {
	relative_delay_cost = value;
}

void DARP_instance_configuration::set_problem(problem_type value) {
	problem = value;
}

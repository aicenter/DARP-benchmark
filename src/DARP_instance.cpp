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

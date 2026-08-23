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
#include <cassert>

#include "Action_data_base.h"
#include "gtest/gtest.h"

#include <rapidjson/reader.h>


unsigned int Action_data_base::get_arrival_time() const {
    return arrival_time;
}

int Action_data_base::get_departure_time() const {
    return departure_time;
}

index_in_plan Action_data_base::get_position_in_plan() const {
	return position_in_plan;
}

index_in_plan Action_data_base::get_other_action_data_index() const {
	return other_action_data_index;
}

void Action_data_base::set_arrival_time(unsigned int arrival_time_par) {
    arrival_time = arrival_time_par;
}

void Action_data_base::set_departure_time(unsigned int departure_time_par) {
    assert(departure_time_par >= arrival_time);
    departure_time = departure_time_par;
}

void Action_data_base::set_position_in_plan(index_in_plan position_in_plan_par) {
	position_in_plan = position_in_plan_par;
}

void Action_data_base::set_other_action_data_index(const index_in_plan other_action_data_index_par) {
	other_action_data_index = other_action_data_index_par;
}


unsigned Action_data_base::get_service_start_time() const {
    return departure_time - get_service_duration();
}

void Action_data_base::delete_departure_time() {
    departure_time = -1;
}

unsigned int Action_data_base::get_min_service_end() const {
	return arrival_time + get_service_duration();
}

void Action_data_base::test_check_equal(const Action_data_base& other) const {
	EXPECT_EQ(this->get_arrival_time(), other.get_arrival_time());
	EXPECT_EQ(this->get_departure_time(), other.get_departure_time());
}

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
#pragma once
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>

#include "../src/aliases.h"

/**
 * @brief Base class for action data. Contains non-template functionality required by most DARP benchmark solvers.
 */
class Action_data_base {

public:

	/**
	 * Empty constructor for action data that are not part of a plan yet.
	 */
	Action_data_base() = default;

	/**
	 * @brief Constructor for action data deserialization
	 * @param json_data loaded json data
	 * @param position_in_plan position in the plan
	 * @param other_action_data_index position of the corresponding action
	*/
	Action_data_base(
		const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data, 
		index_in_plan position_in_plan, 
		index_in_plan other_action_data_index
	):
		arrival_time(json_data["arrival_time"].GetUint()),
		departure_time(json_data["departure_time"].GetInt()),
		position_in_plan(position_in_plan),
		other_action_data_index(other_action_data_index) {}


	[[nodiscard]] unsigned int get_arrival_time() const;

    [[nodiscard]] int get_departure_time() const;

    [[nodiscard]] index_in_plan get_position_in_plan() const;

	[[nodiscard]] index_in_plan get_other_action_data_index() const;

	void set_arrival_time(unsigned int arrival_time);

	void set_departure_time(unsigned int departure_time);

	void set_position_in_plan(index_in_plan position_in_plan_par);

	void set_other_action_data_index(index_in_plan other_action_data_index_par);


	/**
	 * Set departure time to -1, i.e., unset.
	 */
	void delete_departure_time();

	/**
	 * @brief Get the service duration of the action.
	 * @return service duration
	 */
	[[nodiscard]] virtual unsigned short get_service_duration() const = 0;

	/**
	 * @brief Get the start time of the action service, i.e, the time at which the pickup/dropoff process starts.
	 * @return service start time
	 */
    [[nodiscard]] unsigned int get_service_start_time() const;

	[[nodiscard]] unsigned int get_min_service_end() const;

	void test_check_equal(const Action_data_base& other) const;


protected:
    /**
     * A in Cordeau and Laport 2003
     */
    time_type arrival_time{0};

    /**
     * D in Cordeau and Laport 2003
     */
    int departure_time{-1};

	/**
	 * @brief Position in the plan, i.e., plan[position_in_plan] should return this action data. Not required in the
	 * output, but used across multiple solvers.
	*/
	index_in_plan position_in_plan{-1};

private:    
	index_in_plan other_action_data_index{-1};
};

#include "Action_data_base.tpp"

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

#include "aliases.h"
#include "functional"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "Action.h"

/**
 * Action data class for the DARP benchmark solvers. It does not share properties with the test classes to avoid
 * the inheritance overhead.
 * @tparam N node type
 */
template<typename N>
class ActionData
{
public:


    explicit ActionData(const Action<N>& action_param);

	explicit ActionData(
		const rapidjson::GenericValue<rapidjson::UTF8<>>& json_action_data,
		index_in_plan position_in_plan, 
		index_in_plan other_action_data_index,
		const Action<N>& action
	);


    /*ActionData(const ActionData& action_data);
	    

    ActionData(ActionData&& action_data) noexcept;
	    

    ActionData& operator=(const ActionData& action_data);
   

    ActionData& operator=(ActionData&& action_data) noexcept;*/

    bool is_pickup() const {
        return this->action.get().get_action_type() == Action_type::pickup;
    }

    bool is_drop_off() const {
        return this->action.get().get_action_type() == Action_type::dropoff;
    }

    const Action<N>& get_action() const;

    //ActionData<N> *get_other() const;

    //void set_other(ActionData<N> *other);

    [[nodiscard]] unsigned long get_action_id() const;

    [[nodiscard]] Action_type get_action_type() const;

    [[nodiscard]] unsigned get_max_time() const;

    [[nodiscard]] unsigned get_min_time() const;

    [[nodiscard]] const N& get_node() const;

    [[nodiscard]] request_index_type get_request_index() const;

    //[[nodiscard]] unsigned int get_ride_time() const;

	unsigned short get_service_duration() const;
	unsigned get_min_service_end() const;

	void set_arrival_time(unsigned int arrival_time);

    void set_departure_time(unsigned int departure_time);

	void set_other_action_data_index(index_in_plan other_action_data_index_par);

	void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;

	[[nodiscard]] unsigned int get_arrival_time() const;

    [[nodiscard]] int get_departure_time() const;

    [[nodiscard]] index_in_plan get_position_in_plan() const;

    [[nodiscard]] index_in_plan get_other_action_data_index() const;

    void set_position_in_plan(index_in_plan position_in_plan_par);

    [[nodiscard]] unsigned int get_service_start_time() const;

    void delete_departure_time();

protected:
    std::reference_wrapper<const Action<N>> action;

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
    index_in_plan other_action_data_index;
};

template<class N>
bool operator==(const ActionData<N>& lhs, const ActionData<N>& rhs) {
	return &lhs.get_action() == &rhs.get_action()
		&& lhs.get_arrival_time() == rhs.get_arrival_time()
		&& lhs.get_departure_time() == rhs.get_departure_time();
}


template<class N>
bool operator!=(const ActionData<N>& lhs, const ActionData<N>& rhs) {
	return !(lhs == rhs);
}

#include "ActionData.tpp"



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

#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <optional>
#include <rapidjson/document.h>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "Benchmark_vehicle_plan.h"
#include "../Vehicle.h"
#include "../cost/Cost_evaluator.h"
#include "../ActionData.h"
#include "../Plan_checker.h"
#include "../solver/Vehicle_plan_builder.h"
#include "Plan_evaluator_plan_interface.h"
#include "DARP_benchmark_plan_template.h"
#include "../DARP_instance.h"
#include "../travel_time_provider/Travel_time_provider.h"

[[nodiscard]] inline bool vehicle_plan_json_references_virtual_vehicle(const rapidjson::Value& vehicle_json);

inline void validate_virtual_vehicle_plan_reference(
	const rapidjson::Value& vehicle_json,
	const Virtual_vehicle& expected_virtual_vehicle);

template <typename N>
[[nodiscard]] const Vehicle<N>& validate_normal_vehicle_plan_reference(
	const rapidjson::Value& vehicle_json,
	const std::vector<Vehicle<N>>& vehicles);

template <typename N>
[[nodiscard]] std::vector<Vehicle<N>> deserialize_vehicle_list_from_json_array(
	const rapidjson::Value& vehicles_array,
	time_type operation_start);

/**
 * @brief Class for vehicle plan for solving the DARP problem. It contains the functionality that should be accessible
 * for all DARP benchmark solvers. It is parametrized by the node type, but actions are always
 * of type ActionData<N>. The vehicle type V defaults to Vehicle<N> but can be set to Vehicle_base
 * for solvers that support virtual vehicles.
 *
 * Note that there is also a P template parameter, which is the plan type. This is because this class use the CRTP
 * in methods like create_bigger or get_delayed_plan - methods which should return the same type as the type of the
 * plan, while using a unified implementation. In result, this class template cannot be used directly, but it should
 * be inherited by a class that specifies the plan type.
 * @tparam N node type
 * @tparam P plan type - self type
 * @tparam V vehicle holder type - defaults to Vehicle<N>, use Vehicle_base for virtual vehicle support
 */
template <typename N, class P, class V = Vehicle<N>>
class DARP_vehicle_plan:
	public Benchmark_vehicle_plan,
	public DARP_benchmark_plan_template<ActionData<N>, V>,
	public Checkable_plan_interface,
	public Vehicle_plan_builder_plan_interface<V, ActionData<N>>,
	public Plan_evaluator_plan_interface<ActionData<N>>
{
public:

    /**
     * @brief Constructor for complete plan
     * @param vehicle 
     * @param cost 
     * @param actions
     * @param departure_time
     * @param arrival_time 
    */
    DARP_vehicle_plan(
        const V& vehicle, 
        cost_type cost, 
        std::vector<ActionData<N>> actions, 
        unsigned int departure_time,
        unsigned int arrival_time);

    /**
     * @brief Constructor of an empty plan of a known size
     * @param vehicle 
     * @param size 
     * @return 
    */
    DARP_vehicle_plan(const V& vehicle, unsigned short size);

	/**
	 * @brief Deserialize plan JSON (vehicle_plan.schema.json in the Ridesharing_DARP_instances repository).
	 * Plan-local vehicle JSON only identifies or validates a vehicle; it never creates backing storage: a normal
	 * vehicle is looked up by index in \p vehicles, a virtual vehicle is validated against \p virtual_vehicle.
	 * Virtual vehicle plans are supported only if the vehicle type V can hold a virtual vehicle (Vehicle_base).
	 */
	[[nodiscard]] static P JSON_deserialize(
		const rapidjson::Value& plan_data,
		const DARP_instance<N>& darp_instance,
		const std::vector<Vehicle<N>>& vehicles,
		const Virtual_vehicle* virtual_vehicle = nullptr);

	DARP_vehicle_plan(const DARP_vehicle_plan& other) = default;
	DARP_vehicle_plan(DARP_vehicle_plan&& other) noexcept = default;
	DARP_vehicle_plan& operator=(const DARP_vehicle_plan& other) = default;
	DARP_vehicle_plan& operator=(DARP_vehicle_plan&& other) noexcept = default;

    ~DARP_vehicle_plan() override = default;

	[[nodiscard]] unsigned short get_vehicle_capacity() const;

	[[nodiscard]] plan_size_type get_length() const;

	//Plan_evaluator_action_interface& operator[](unsigned short index) const override;


	[[nodiscard]] P create_bigger(unsigned short increase = 1) const;


    void set_vehicle(const std::reference_wrapper<const V>& vehicle_par);

    void set_departure_time(unsigned long departure_time);

    void set_arrival_time(unsigned long arrival_time);

    bool is_feasible() const {
        return this->feasible;
    }

    void set_feasible(bool feasible_par) {
        this->feasible = feasible_par;
    }

    void set_cost(cost_type new_cost);

    //using Benchmark_vehicle_plan<N>::get_actions;

    std::vector<ActionData<N>>& get_actions();

	[[nodiscard]] const std::vector<ActionData<N>>& get_actions() const override;

    [[nodiscard]] unsigned short get_free_capacity() const;

	[[nodiscard]] variant_id_type get_variant_id() const {
		return variant_id;
	}

	using Base_plan<ActionData<N>,V>::operator[];

    ActionData<N>& operator[](plan_size_type index);

	//Plan_evaluator_action_interface& operator[](unsigned short index) override;

	friend bool operator==(const DARP_vehicle_plan& lhs, const DARP_vehicle_plan& rhs) {
	    return &lhs.vehicle == &rhs.vehicle
		    && lhs.cost == rhs.cost
		    && lhs.actions == rhs.actions
		    && lhs.departure_time == rhs.departure_time
		    && lhs.arrival_time == rhs.arrival_time
		    && lhs.free_capacity == rhs.free_capacity;
    }

    friend bool operator!=(const DARP_vehicle_plan& lhs, const DARP_vehicle_plan& rhs) {
	    return !(lhs == rhs);
    }

    const ActionData<N>& get_first_action() const;

    ActionData<N>& get_first_action();

    const ActionData<N>& get_last_action() const;

    ActionData<N>* get_pickup(const Service_action<N>& drop_off_action_data);

	ActionData<N>* get_pickup(const ActionData<N>& drop_off_action_data);

	const ActionData<N>* get_pickup(const Service_action<N>& drop_off_action_data) const;

	const ActionData<N>* get_pickup(const ActionData<N>& drop_off_action_data) const;

    index_in_plan get_pickup_index(const ActionData<N>& drop_off_action_data) const;

    bool contains_pickup(const Service_action<N>& drop_off_action_data) const;


 /*   unsigned int get_servicing_start() const;


    unsigned int get_servicing_end() const;*/

    using Base_plan<ActionData<N>,V>::begin;

    using Base_plan<ActionData<N>,V>::end;

    typename std::vector<ActionData<N>>::iterator begin();

    typename std::vector<ActionData<N>>::iterator end();

    const N& get_first_action_node() const;

    const N& get_last_action_node() const;
    
    /**
    * Adds a new action to the plan and returns a reference to the newly created action data.
    * @param  action The new action to be added.
    * @return Reference to the ActionData object created for the new action.
    */
    ActionData<N>& add_action(const Action<N>& action);

    void remove_last_action();

    void remove_last_action(bool was_last);

	ActionData<N>& get_other(const ActionData<N>& source_action_data);

	const ActionData<N>& get_other(const ActionData<N>& source_action_data) const override;

	unsigned get_ride_time(const ActionData<N>& action_data_par) const;

    bool check(const DARP_instance_configuration& configuration) const override;

	bool full_check(
		const DARP_instance_configuration& instance_configuration,
		const Travel_time_provider<N>& travel_time_provider,
		bool skip_time_check,
		bool skip_cost_check,
		std::vector<bool>* served_or_dropped_requests = nullptr
	) const;

	[[nodiscard]] std::optional<P> get_delayed_plan(unsigned int delay, unsigned variant_id_par) const;

	void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;

	void append_simple_csv_rows(int plan_index, std::ostringstream& csv) const override;

	[[nodiscard]] cost_type get_cost() const override;

	[[nodiscard]] unsigned long get_driving_time() const override;

	[[nodiscard]] unsigned long get_ride_time() const override;

	[[nodiscard]] unsigned long get_passenger_delay() const override;

protected:

private:
    bool feasible {false};
    std::unordered_map<unsigned int,short> pickup_map;
    unsigned short free_capacity;

    variant_id_type variant_id{0};
};


template<typename N, class P, class V = Vehicle<N>>
void export_plans(const std::vector<DARP_vehicle_plan<N,P,V>>& plans, std::string file_path);


/**
 * @brief Basic vehicle plan class for DARP benchmark solvers. In future, it should have no additional functionality
 * over the DARP vehicle plan.
 * @tparam N node type
 * @tparam V vehicle holder type - defaults to Vehicle<N>
 */
template<typename N, class V = Vehicle<N>>
class VehiclePlan:
	public DARP_vehicle_plan<N, VehiclePlan<N, V>, V>
{
    using DARP_vehicle_plan<N, VehiclePlan<N, V>, V>::DARP_vehicle_plan;

public:

	using vehicle_type = V;

	/**
	 * @brief Temporary method for HALNS solver that returns the last service action index. Later, the vehicle
	 * plan should be replaced by some plan builder in HALNS.
	 * @return last index of action that is not a depot action.
	*/
	[[nodiscard]] index_in_plan get_last_service_action_index() const;

//    using Benchmark_vehicle_plan<N>::operator[];
    using DARP_vehicle_plan<N, VehiclePlan<N, V>, V>::operator[];
};

/**
 * Deserializes a JSON array of action data objects (same shape as plan \c "actions"). Actions are resolved from
 * the instance requests by \c request_index and \c type.
 * Used by \c DARP_vehicle_plan::JSON_deserialize and by loaders of standalone action data lists.
 */
template <typename N>
[[nodiscard]] std::vector<ActionData<N>> deserialize_action_data_list(
	const rapidjson::Value& actions_array,
	const DARP_instance<N>& darp_instance);

/**
 * Deserializes a JSON array of plan objects (same shape as solution \c "plans").
 * Vehicle rows are resolved against \p vehicles or validated against \p virtual_vehicle; this function never creates
 * vehicles from plan-local JSON.
 */
template <typename N, Benchmark_plan P>
void deserialize_vehicle_plans_from_json_array(
	const rapidjson::Value& plans_array,
	const DARP_instance<N>& darp_instance,
	const std::vector<Vehicle<N>>& vehicles,
	const Virtual_vehicle* virtual_vehicle,
	std::vector<P>& plans_out);

#include "VehiclePlan.tpp"

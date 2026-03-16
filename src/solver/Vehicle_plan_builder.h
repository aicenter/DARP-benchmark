#pragma once

#include <vector>
#include <array>
#include <boost/iterator/transform_iterator.hpp>
#include <functional>

#include "../Adjustment_reason.h"
#include "../aliases.h"
#include "../Plan_checker.h"
#include "../Vehicle.h"


struct Adjust_times_data {
	unsigned short index;
	Adjustment_reason adjustment_reason;
};

template<class A>
concept Vehicle_plan_builder_action	= 
	requires(A action) {
		{action.get_action_type()} -> std::same_as<Action_type>;
		{action.get_position_in_plan()} -> std::same_as<index_in_plan>;
		{action.get_other_action_data_index()} -> std::same_as<index_in_plan>;
	}
	&& requires(A action, index_in_plan other_action_data_index_par) {
		{action.set_other_action_data_index(other_action_data_index_par)} -> std::same_as<void>;
	};

template<class A>
concept Vehicle_plan_builder_action_with_request =
	Vehicle_plan_builder_action<A> &&
	requires(A action) {
		{action.get_request_index()} -> std::same_as<request_index_type>;
	};

template<class V, class A>
class Vehicle_plan_builder_plan_interface {
public:
	Vehicle_plan_builder_plan_interface() = default;

	virtual ~Vehicle_plan_builder_plan_interface() = default;

	[[nodiscard]] virtual const std::vector<A>& get_actions() const = 0;

	[[nodiscard]] virtual const A& get_other(const A& action_data) const = 0;

protected:
	Vehicle_plan_builder_plan_interface(const Vehicle_plan_builder_plan_interface& other) = default;
	Vehicle_plan_builder_plan_interface(Vehicle_plan_builder_plan_interface&& other) noexcept = default;
	Vehicle_plan_builder_plan_interface& operator=(const Vehicle_plan_builder_plan_interface& other) = default;
	Vehicle_plan_builder_plan_interface& operator=(Vehicle_plan_builder_plan_interface&& other) noexcept = default;
};



template<class P, class V, class A>
concept Vehicle_plan_builder_plan =
	requires(const P plan){
		{plan.get_length()} -> std::same_as<plan_size_type>;
		{plan.get_service_action_length()} -> std::same_as<plan_size_type>;
		{plan.get_vehicle()} -> std::same_as<const V&>;
		{plan.get_departure_time()} -> std::same_as<time_type>;
		{plan.get_arrival_time()} -> std::same_as<time_type>;
		{plan.get_cost()} -> std::same_as<unsigned>;
	}
    && requires(const V& vehicle, unsigned cost, const std::vector<A>& actions, time_type departure_time, time_type arrival_time) {
        {P(vehicle, cost, actions, departure_time, arrival_time)} -> std::same_as<P>;
    }
	&& requires(const P plan, index_in_plan index) {
		{plan[index]} -> std::same_as<const A&>;
	}
;



/**
 * @brief Class for fast in place building of vehicle plans. It is meant to be overriden for each specific solver.
 * @tparam V vehicle type
 * @tparam A Action data type
 * @tparam P plan type
*/
template<class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V,A> P>
class Vehicle_plan_builder: public Checkable_plan_interface{
public:
	static constexpr unsigned short adj_stack_size = 25;
	
    [[nodiscard]] const V& get_vehicle() const {
	    return vehicle.get();
    }


    [[nodiscard]] unsigned long get_departure_time() const {
	    return departure_time;
    }

    [[nodiscard]] unsigned long get_arrival_time() const {
	    return arrival_time;
    }

    void set_departure_time(unsigned long departure_time_par) {
	    this->departure_time = departure_time_par;
    }

    void set_arrival_time(unsigned long arrival_time_par) {
	    this->arrival_time = arrival_time_par;
    }

    [[nodiscard]] std::vector<A>& get_action_data() {
	    return action_data;
    }

    [[nodiscard]] unsigned get_cost() const {
	    return cost;
    }

    void set_cost(unsigned cost_par) {
	    this->cost = cost_par;
    }

	[[nodiscard]] std::vector<int>& get_time_adjustments() {
		return time_adjustments;
	}

	[[nodiscard]] const std::vector<int>& get_time_adjustments() const {
		return time_adjustments;
	}

    [[nodiscard]] std::vector<short>& get_action_order() {
	    return action_order;
    }

	[[nodiscard]] const std::vector<short>& get_action_order() const {
	    return action_order;
    }
	
	[[nodiscard]] std::array<Adjust_times_data, adj_stack_size>& get_adj_stack() {
		return adj_stack;
	}

	A& operator[](unsigned int index) {
		assert(action_order[index] >= 0);
        return this->action_data[action_order[index]];
    }

    const A& operator[](unsigned int index) const {
        return this->action_data[action_order[index]];
    }

	/**
	 * @brief Returns the other action data. The action data are sorted, this is still useful however, as
	 * we often do not know the index of the action data in the caller context.
	 * @param source_action_data action data
	 * @return complementary action data
	*/
	const A& get_other(const A& source_action_data) const {
		assert(source_action_data.get_other_action_data_index() >= 0);
    	return action_data[source_action_data.get_other_action_data_index()];
    }

	/**
	 * @brief Returns the other action data. The action data are sorted, this is still useful however, as
	 * we often do not know the index of the action data in the caller context.
	 * @param source_action_data action data
	 * @return complementary action data
	*/
	A& get_other(const A& source_action_data) {
	    return action_data[source_action_data.get_other_action_data_index()];
    }

	unsigned get_ride_time(const A& action_data_par) const {
		if(action_data_par.get_service_start_time() > 0) {
	        return action_data_par.get_service_start_time() - get_other(action_data_par).get_departure_time();
		}
	    else {
		    return action_data_par.get_arrival_time() - get_other(action_data_par).get_departure_time();
	    }
	}

	[[nodiscard]] P to_vehicle_plan() const {
        std::vector<A> vehicle_plan_actions;
        vehicle_plan_actions.reserve(this->action_data.size());
        for(unsigned int i = 0; i < action_order.size(); ++i) {
    		if(action_order[i] == -1) {
    			break;
    		}
            const A& action = this->action_data[action_order[i]];
            vehicle_plan_actions.push_back(action);
        	vehicle_plan_actions.back().set_other_action_data_index(get_other(action).get_position_in_plan());
    	}
        return P(vehicle, cost, vehicle_plan_actions, departure_time, arrival_time);
    }

	/**
	 * @brief Returns the plan length as if it would be exported to Vehicle plan right now.
	 * @return plan length
	*/
	[[nodiscard]] plan_size_type get_length() const {
		unsigned i = 0;
	    for(; i < action_order.size(); ++i) {
    		if(action_order[i] == -1) {
    			return static_cast<plan_size_type>(i);
    		}
    	}
		return static_cast<plan_size_type>(i + 1);
    }

	bool check(const DARP_instance_configuration& configuration) const;

protected:
	/**
	 * @brief indexes to action_data marking the action order
	*/
	std::vector<short> action_order;

private:

	std::function<const A& (unsigned short)> iterator_transform_function;

	using action_data_iterator = boost::transform_iterator<
		decltype(iterator_transform_function),
		decltype(std::declval<const decltype(action_order)>().begin())
	>;

public:

	[[nodiscard]] action_data_iterator begin() const {
	    return action_data_iterator(
			action_order.begin(), iterator_transform_function);
    }

	[[nodiscard]] action_data_iterator end() const {
	    return action_data_iterator(
			action_order.end(), iterator_transform_function);
    }

protected:
	std::reference_wrapper<const V> vehicle;

    /**
     * @brief The working version of the departure time. Note that some solvers may initialize this with the 
     * earliest possible departure (and wait if needed), while others may initialize this with the latest 
     * possible departure.
     */
    unsigned long departure_time{0};
	
    unsigned long arrival_time{0};
	
	/**
     * @brief action data for all actions that should be served by the plan, when plan building is complete.
    */
    std::vector<A> action_data;



	/**
	 * @brief Time adjustments from the adjust times method. They need to be reverted when an action is removed from
	 * plan. The structure is described at:
	 * https://docs.google.com/spreadsheets/d/1FJWO9nRrsYui55tgJv2pCt7ly7NSHfvCPW4keAmXnK4/edit?usp=sharing
	*/
	std::vector<int> time_adjustments;

	unsigned int id;

	std::array<Adjust_times_data, adj_stack_size> adj_stack{};

	Vehicle_plan_builder(
		const V& vehicle,
		plan_size_type init_size,
		plan_size_type init_time_adjustments_size,
		unsigned initial_departure_time = 0
	):
		action_order(init_size, -1),
		iterator_transform_function([this](unsigned short index) -> const A& {return this->action_data[index]; }),
		vehicle(vehicle),
		departure_time(initial_departure_time),
		action_data(),
		time_adjustments(init_time_adjustments_size, 0),
		id(id_counter++)
	{
		action_data.reserve(init_size);
	}

	/**
	 * @brief Constructor from existing plan. Currently works only for plans beginning with a depot action.
	 * @param plan plan
	 * @param init_time_adjustments_size initial size of the time adjustment backup. It depends on the plan builder type.
	*/
	template<Vehicle_plan_builder_action_with_request AR = A>
	Vehicle_plan_builder(const P& plan, plan_size_type init_time_adjustments_size);



    Vehicle_plan_builder(const Vehicle_plan_builder& other) :
		action_order(other.action_order),
		iterator_transform_function([this](unsigned short index) -> const A& {
			return this->action_data[index];
		}),
		vehicle(other.vehicle),
		departure_time(other.departure_time),
		arrival_time(other.arrival_time),
		action_data(other.action_data),
		time_adjustments(other.time_adjustments),
		id(other.id),
		cost(other.cost)
	{
    }

    Vehicle_plan_builder(Vehicle_plan_builder&& other) noexcept :
		action_order(std::move(other.action_order)),
		iterator_transform_function([this](unsigned short index) -> const A& {return this->action_data[index]; }),
		vehicle(std::move(other.vehicle)),
		departure_time(other.departure_time),
		arrival_time(other.arrival_time),
		action_data(std::move(other.action_data)),
		time_adjustments(other.time_adjustments),
		id(other.id),
		cost(other.cost)
	{}

    Vehicle_plan_builder& operator=(const Vehicle_plan_builder& other) {
	    if(this == &other)
		    return *this;
	    vehicle = other.vehicle;
	    cost = other.cost;
	    departure_time = other.departure_time;
	    arrival_time = other.arrival_time;
	    action_data = other.action_data;
	    action_order = other.action_order;
		time_adjustments = other.time_adjustments;
		id = other.id;
		iterator_transform_function = [this](unsigned short index) -> const A& {return this->action_data[index]; };
	    return *this;
    }

    Vehicle_plan_builder& operator=(Vehicle_plan_builder&& other) noexcept {
	    if(this == &other)
		    return *this;
	    vehicle = std::move(other.vehicle);
	    cost = other.cost;
	    departure_time = other.departure_time;
	    arrival_time = other.arrival_time;
	    action_data = std::move(other.action_data);
	    action_order = std::move(other.action_order);
		time_adjustments = other.time_adjustments;
		id = other.id;
		iterator_transform_function = [this](unsigned short index) -> const A& {return this->action_data[index]; };
	    return *this;
    }
    
	/**
	 * @brief Adds action data for request and wire them together.
	 * @param pickup_action_data pickup action data
	 * @param drop_off_action_data drop off action data
	*/
	void add_action_data(const A& pickup_action_data, const A& drop_off_action_data);

private:
	static inline int id_counter = 0;

	unsigned int cost{0};
};

template<class V, class VH, class A, class P>
concept Vehicle_plan_builder_concept =
std::is_base_of_v<Vehicle_plan_builder<VH, A, P>, V>
&& requires(V vpb){{vpb.compute_time_adjustment_start_index()} -> std::convertible_to<unsigned short>;};

//static_assert(std::is_same_v<Vehicle_plan_builder<Cordeau_node>::action_order::begin const&, decltype(action_order.begin())>)

#include "Vehicle_plan_builder.tpp"
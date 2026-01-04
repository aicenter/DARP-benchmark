#pragma once

#include <type_traits>

class Plan_evaluator_action_interface {
public:
	Plan_evaluator_action_interface() = default;

	virtual ~Plan_evaluator_action_interface() = default;

	bool operator==(const Plan_evaluator_action_interface&) const = default;

	[[nodiscard]] virtual unsigned get_min_time() const = 0;

	[[nodiscard]] virtual unsigned get_max_time() const = 0;

	[[nodiscard]] virtual request_index_type get_request_index() const = 0;

	[[nodiscard]] virtual unsigned short get_service_duration() const = 0;

	[[nodiscard]] virtual bool is_pickup() const = 0;

	[[nodiscard]] virtual bool is_drop_off() const = 0;

	virtual void set_arrival_time(unsigned arrival_time) = 0;

	virtual void set_departure_time(const unsigned departure_time) = 0;
protected:
	Plan_evaluator_action_interface(const Plan_evaluator_action_interface& other) = default;
	Plan_evaluator_action_interface(Plan_evaluator_action_interface&& other) noexcept = default;
	Plan_evaluator_action_interface& operator=(const Plan_evaluator_action_interface& other) = default;
	Plan_evaluator_action_interface& operator=(Plan_evaluator_action_interface&& other) noexcept = default;
};

template<typename A>
concept Plan_evaluator_action =
	requires(A action) {
		{action.get_min_time()} -> std::same_as<time_type>;
		{action.get_max_time()} -> std::same_as<time_type>;
		{action.get_request_index()} -> std::same_as<request_index_type>;
		{action.get_service_duration()} -> std::same_as<travel_time_type>;
		{action.is_pickup()} -> std::same_as<bool>;
		{action.is_drop_off()} -> std::same_as<bool>;
	}
	&& requires(A action, time_type time) {
		{action.set_arrival_time(time)} -> std::same_as<void>;
		{action.set_departure_time(time)} -> std::same_as<void>;
	};

template<Plan_evaluator_action A>
class Plan_evaluator_plan_interface {
public:
	Plan_evaluator_plan_interface() = default;

	virtual ~Plan_evaluator_plan_interface() = default;

	[[nodiscard]] virtual unsigned short get_vehicle_capacity() const = 0;
	[[nodiscard]] virtual plan_size_type get_length() const = 0;
	virtual void set_cost(unsigned cost) = 0;
	virtual void set_feasible(bool feasible) = 0;
	virtual void set_arrival_time(unsigned long arrival_time) = 0;
	virtual void set_departure_time(unsigned long departure_time) = 0;


	virtual A& operator[](plan_size_type index) = 0;

protected:
	Plan_evaluator_plan_interface(const Plan_evaluator_plan_interface& other) = default;
	Plan_evaluator_plan_interface(Plan_evaluator_plan_interface&& other) noexcept = default;
	Plan_evaluator_plan_interface& operator=(const Plan_evaluator_plan_interface& other) = default;
	Plan_evaluator_plan_interface& operator=(Plan_evaluator_plan_interface&& other) noexcept = default;
};
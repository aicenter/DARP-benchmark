//
// Created by Fido on 2023-10-13.
//

#pragma once

#include "Base_plan.h"
#include "../Action.h"

/**
 * @brief concept for actions data used in the DARP benchmark solvers.
 * @tparam A action type
 */
template<class A>
concept DARP_benchmark_solver_action_data = requires(A a) {
	{ a.get_action_type() } -> std::same_as<Action_type>;
};

/**
 * @brief Base template class for plans used in the DARP benchmark solvers. It contains the functionality that is common
 * to all DARP benchmark solvers.
 * @tparam A
 * @tparam V
 */
template<DARP_benchmark_solver_action_data A, class V>
class DARP_benchmark_plan_template: public Base_plan<A,V> {

public:
	using Base_plan<A,V>::Base_plan;

	/**
	 * @brief Temporary method to get the plan length without the depot actions. It can be removed when the depot
	 * actions are removed from the plan logic.
	 * @return plan length without the depot actions.
	 */
	[[nodiscard]] plan_size_type get_service_action_length() const;
};

#include "DARP_benchmark_plan_template.tpp"
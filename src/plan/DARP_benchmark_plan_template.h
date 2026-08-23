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
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

#include <type_traits>
#include <vector>

#include "DARP_instance.h"

class Checkable_plan_interface {
public:
	virtual ~Checkable_plan_interface() = default;
	[[nodiscard]] virtual bool check(const DARP_instance_configuration& configuration) const = 0;
};

template<typename P>
concept Checkable_plan = std::is_base_of_v<Checkable_plan_interface, P>;

template<Checkable_plan P>
class Plan_checker {
public:
	explicit Plan_checker(const DARP_instance_configuration& configuration)
		: configuration(configuration) {
	}

	void check_plans(const std::vector<P>& plans) {
		for (const P& plan: plans) {
			check_plan(plan);
		}
	}

	void check_plan(const P& plan) {
        plan.check(configuration);
	}

private:
	const DARP_instance_configuration& configuration;
};
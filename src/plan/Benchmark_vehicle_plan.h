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

#include <sstream>
#include <type_traits>

#include "../ActionData.h"
#include "../Vehicle.h"
#include "Base_plan.h"


/**
 * @brief Base class for all plans that are used in the benchmark executable. It is parametrized only by the type of
 * the nodes, but actions are always of type ActionData<N> and vehicles can be any Vehicle_base derived type.
 * @tparam N node type
 */
class Benchmark_vehicle_plan{
public:
	virtual void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const = 0;

	virtual void append_simple_csv_rows(int plan_index, std::ostringstream& csv) const = 0;

	[[nodiscard]] virtual cost_type get_cost() const = 0;

	[[nodiscard]] virtual unsigned long get_driving_time() const = 0;

	[[nodiscard]] virtual unsigned long get_ride_time() const = 0;

	[[nodiscard]] virtual unsigned long get_passenger_delay() const = 0;

};

template<class P>
concept Benchmark_plan = std::is_base_of_v<Benchmark_vehicle_plan, P>;

#include "Benchmark_vehicle_plan.tpp"

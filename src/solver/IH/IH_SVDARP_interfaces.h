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

#include "../../Action.h"
#include "../../../test/Action_data_base.h"

template<class V, typename N>
concept IH_SVDARP_vehicle =
	requires(V vehicle) {
		{vehicle.get_capacity()} -> std::same_as<unsigned short>;
		{vehicle.get_init_position()} -> std::same_as<const N&>;
	};

template<class A, typename N>
concept IH_SVDARP_action = 
	requires(A action) {
		{action.get_action_type()} -> std::same_as<Action_type>;
		{action.get_node()} -> std::convertible_to<N>;
		{action.get_max_time()} -> std::same_as<time_type>;
		{action.get_min_time()} -> std::same_as<time_type>;
		{action.get_min_service_end()} -> std::same_as<time_type>;
	};


template<class R, typename N, class A>
concept IH_request =
IH_SVDARP_action<A, N>
&& requires(R request) {
    {request.get_pickup()} -> std::same_as<const A&>;
    {request.get_dropoff()} -> std::same_as<const A&>;
};
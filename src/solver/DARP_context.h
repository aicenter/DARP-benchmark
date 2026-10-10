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

#include "../travel_time_provider/Travel_time_provider.h"
#include "../DARP_instance.h"
#include "../cost/Cost_evaluator.h"

/**
 * @brief Travel time provider + DARP configuration shared by solvers and helpers.
 * @tparam T travel time provider type
 */
template<typename T>
class DARP_context_nontyped {
public:
	DARP_context_nontyped(
		std::shared_ptr<T> travel_time_provider_par,
		std::shared_ptr<DARP_instance_configuration> darp_instance_configuration_par
	):	travel_time_provider_(std::move(travel_time_provider_par)),
		darp_instance_configuration_(std::move(darp_instance_configuration_par)),
		cost_evaluator_(Cost_evaluator::from_configuration(*darp_instance_configuration_))
	{}

	[[nodiscard]] const std::shared_ptr<T>& travel_time_provider() const {
		return travel_time_provider_;
	}

	[[nodiscard]] const std::shared_ptr<DARP_instance_configuration>& darp_instance_configuration() const {
		return darp_instance_configuration_;
	}

	[[nodiscard]] unsigned long get_max_route_duration() const {
		return darp_instance_configuration_->get_max_route_duration();
	}

	[[nodiscard]] unsigned long get_max_ride_time() const {
		return darp_instance_configuration_->get_max_ride_time();
	}

	[[nodiscard]] bool is_return_to_depot() const {
		return darp_instance_configuration_->is_return_to_depot();
	}

	/** Evaluator of the weighted plan cost configured by the instance. */
	[[nodiscard]] const Cost_evaluator& cost_evaluator() const {
		return cost_evaluator_;
	}

private:
	std::shared_ptr<T> travel_time_provider_;
	std::shared_ptr<DARP_instance_configuration> darp_instance_configuration_;
	Cost_evaluator cost_evaluator_;
};

template<class N>
class DARP_context: public DARP_context_nontyped<Travel_time_provider<N>> {
public:
	using DARP_context_nontyped<Travel_time_provider<N>>::DARP_context_nontyped;

	explicit DARP_context(const DARP_instance<N>& darp_instance)
		: DARP_context_nontyped<Travel_time_provider<N>>(
			darp_instance.get_travelcost_provider(),
			darp_instance.get_darp_instance_configuration()
		)
	{
	}
};

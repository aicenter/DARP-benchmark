#pragma once

#include "../travel_time_provider/Travel_time_provider.h"
#include "../DARP_instance.h"

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
		darp_instance_configuration_(std::move(darp_instance_configuration_par))
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

private:
	std::shared_ptr<T> travel_time_provider_;
	std::shared_ptr<DARP_instance_configuration> darp_instance_configuration_;
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

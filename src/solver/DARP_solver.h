#pragma once

#include "../travel_time_provider/Travel_time_provider.h"
#include "../DARP_instance.h"

/**
 * @brief Template for any object that needs to solve a DARP problem. It contains the DARP configuration (parameters
 * like max ride time...) and also a travel time provider. 
 * @tparam T type of the travel time provider.
*/
template<typename T>
class DARP_solver_nontyped {
public:
	
	DARP_solver_nontyped(
		std::shared_ptr<T> travel_time_provider_par,
		std::shared_ptr<DARP_instance_configuration> darp_instance_configuration
	):	travel_time_provider(std::move(travel_time_provider_par)),
		darp_instance_configuration(std::move(darp_instance_configuration))
	{}

	[[nodiscard]] unsigned long get_max_route_duration() const {
		return darp_instance_configuration->get_max_route_duration();
	}

	[[nodiscard]] unsigned long get_max_ride_time() const {
		return darp_instance_configuration->get_max_ride_time();
	}

	[[nodiscard]] bool is_return_to_depot() const {
		return darp_instance_configuration->is_return_to_depot();
	}
	
protected:
	/**
	 * @brief Travel time provider, shared with other components of DARP benchmark
	*/
	std::shared_ptr<T> travel_time_provider;

	/**
	 * @brief Shared DARP instance configuration 
	*/
	std::shared_ptr<DARP_instance_configuration> darp_instance_configuration;
};

/**
 * @brief Template for any object that needs to solve a DARP problem. It contains the DARP configuration (parameters
 * like max ride time...) and also a travel time provider. This abstract class is aware of the node type N. Therefore,
 * if you work with a solver that is ignorant to the node type, you need to inherit from the nontyped version of
 * the DARP_solver.
 * @tparam N type of the travel time provider node.
*/
template<class N>
class DARP_solver: public DARP_solver_nontyped<Travel_time_provider<N>>{
public:

	DARP_solver(
		const std::shared_ptr<Travel_time_provider<N>>& travel_time_provider_par, 
		const std::shared_ptr<DARP_instance_configuration>& darp_instance_configuration_par
	):	DARP_solver_nontyped<Travel_time_provider<N>>(travel_time_provider_par, darp_instance_configuration_par)
	{}

	explicit DARP_solver(const DARP_instance<N>& darp_instance)
		: DARP_solver(darp_instance.get_travelcost_provider(), darp_instance.get_darp_instance_configuration())
	{
	}

	
};



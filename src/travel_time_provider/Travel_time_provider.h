#pragma once

#include <iostream>
#include "../aliases.h"


/**
 * @brief Computes travel time between two locations. These locations are arbitrary for this template interface, they
 * are expected to be specified in the Travel_time_provider implementations.
 * @tparam L location type 
*/
template<typename L>
class Travel_time_provider {
public:
	Travel_time_provider() = default;

	virtual ~Travel_time_provider() = default;

	/**
	 * Provides travel time in seconds between location \p from and \p to.
	 * @param from from
	 * @param to to
	 * @return Travel time between location \p from and \p to in seconds.
	 */
	[[nodiscard]] virtual travel_time_type get_travel_time(const L& from, const L& to) const = 0;

	/**
	 * Method that provides location info when the vehicle is on the way from last_action_location to next_action_location.
	 * @param last_action_location
	 * @param next_action_location
	 * @param time_since_last_action_departure
	 * @return tuple of next location on the path and travel time needed to reach that location
	 */
	[[nodiscard]] virtual std::tuple<const L&, time_type> get_vehicle_location_info(
		const L& last_action_location,
		const L& next_action_location,
		time_type time_since_last_action_departure
	) = 0;

protected:
	Travel_time_provider(const Travel_time_provider& other) = default;

	Travel_time_provider(Travel_time_provider&& other) noexcept = default;

	Travel_time_provider& operator=(const Travel_time_provider& other) = default;

	Travel_time_provider& operator=(Travel_time_provider&& other) noexcept = default;

};



#include "Travel_time_provider.tpp"
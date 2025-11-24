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

protected:
	Travel_time_provider(const Travel_time_provider& other) = default;

	Travel_time_provider(Travel_time_provider&& other) noexcept = default;

	Travel_time_provider& operator=(const Travel_time_provider& other) = default;

	Travel_time_provider& operator=(Travel_time_provider&& other) noexcept = default;

};



#include "Travel_time_provider.tpp"
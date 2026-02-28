#pragma once

#include <vector>
#include <memory>

#include "travel_time_provider/Travel_time_provider.h"
#include "Vehicle.h"
#include "Request.h"

class Node {
public:

	explicit Node(unsigned index_par)
		: index(index_par) {
	}

	//virtual bool operator==(const Node& other) const = 0;
	
	//bool operator!=(const Node& other) const{
	//	return !(*this == other);
	//}

	[[nodiscard]] unsigned int get_index() const;
private:
	const unsigned int index;
};


class DARP_instance_configuration {
public:
	explicit DARP_instance_configuration(
		unsigned long max_route_duration = 0, 
		unsigned long max_ride_time = 0, 
		bool return_to_depot = false,
		bool virtual_vehicles = false, 
		unsigned start_time = 0,
		unsigned short vehicle_capital_cost = 0
	):
		max_route_duration(max_route_duration),
		max_ride_time(max_ride_time),
		return_to_depot(return_to_depot),
		virtual_vehicles(virtual_vehicles),
		start_time(start_time),
		vehicle_capital_cost(vehicle_capital_cost) {
	}

	[[nodiscard]] unsigned long get_max_route_duration() const;
	[[nodiscard]] unsigned long get_max_ride_time() const;
	[[nodiscard]] bool is_return_to_depot() const;
	[[nodiscard]] bool use_virtual_vehicles() const;
	[[nodiscard]] unsigned get_start_time() const;
	[[nodiscard]] unsigned short get_vehicle_capital_cost() const;

private:
    const unsigned long max_route_duration{0};

	/**
	 * Maximum ride time for a request. The ride time is computed as the interval between the departure from the pickup
	 * location and the start of the service at the drop off location.
	 */
    const unsigned long max_ride_time{0};

	/**
	* @brief Specifies whether vehicles have to return to the depot after serving the last request
	*/
	const bool return_to_depot{true};
	const bool virtual_vehicles{false};
    const unsigned start_time{0};
	const unsigned short vehicle_capital_cost{0};
};

class DARP_instance_interface {
public:
	virtual ~DARP_instance_interface() = default;
};


template <typename N>
class DARP_instance: public DARP_instance_interface
{
public:
    DARP_instance(
        std::unique_ptr<std::vector<Request<N>>> requests, 
        std::unique_ptr<std::vector<Vehicle<N>>> vehicles,
        std::shared_ptr<Travel_time_provider<N>> travel_time_provider,
		std::shared_ptr<DARP_instance_configuration> darp_instance_configuration
    );

	DARP_instance(DARP_instance&& other);

    const std::vector<Vehicle<N>>& get_vehicles() const;

    const std::vector<Request<N>>& get_requests() const;

    const std::shared_ptr<Travel_time_provider<N>>& get_travelcost_provider() const;

	[[nodiscard]] const std::shared_ptr<DARP_instance_configuration>& get_darp_instance_configuration() const;

	[[nodiscard]] unsigned long get_max_route_duration() const;

    [[nodiscard]] unsigned long get_max_ride_time() const;

    [[nodiscard]] bool is_return_to_depot() const;

    [[nodiscard]] bool is_virtual_vehicles() const;
private:
    std::unique_ptr<std::vector<Vehicle<N>>> vehicles;
    std::unique_ptr<std::vector<Request<N>>> requests;
    const std::shared_ptr<Travel_time_provider<N>> travel_cost_provider;
	const std::shared_ptr<DARP_instance_configuration> darp_instance_configuration;
};

template <typename N>
const std::shared_ptr<DARP_instance_configuration>& DARP_instance<N>::get_darp_instance_configuration() const {
	return darp_instance_configuration;
}

template <typename N>
bool DARP_instance<N>::is_return_to_depot() const {
	return darp_instance_configuration->is_return_to_depot();
}

template <typename N>
bool DARP_instance<N>::is_virtual_vehicles() const {
	return darp_instance_configuration->use_virtual_vehicles();
}

#include "DARP_instance.tpp"

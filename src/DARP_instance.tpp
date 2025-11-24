//
// Created by Fido on 2020-03-23.
//
#include <memory>
#include <utility>


template <typename N> DARP_instance<N>::DARP_instance(
    std::unique_ptr<std::vector<Request<N>>> requests,
    std::unique_ptr<std::vector<Vehicle<N>>> vehicles, 
    std::shared_ptr<Travel_time_provider<N>> travel_time_provider,
    std::shared_ptr<DARP_instance_configuration> darp_instance_configuration
):
		vehicles{std::move(vehicles)},
		requests{std::move(requests)},
		travel_cost_provider {travel_time_provider},
		darp_instance_configuration(darp_instance_configuration)
		{}

template <typename N>
DARP_instance<N>::DARP_instance(DARP_instance&& other):
	vehicles(std::move(other.vehicles)),
	requests(std::move(other.requests)),
	travel_cost_provider(other.travel_cost_provider),
	darp_instance_configuration(std::move(other.darp_instance_configuration))
{}

template<typename N>
unsigned long DARP_instance<N>::get_max_route_duration() const {
    return darp_instance_configuration->get_max_route_duration();
}

template<typename N>
unsigned long DARP_instance<N>::get_max_ride_time() const {
    return darp_instance_configuration->get_max_ride_time();
}

template<typename N>
const std::vector<Vehicle<N>>& DARP_instance<N>::get_vehicles() const {
    return *vehicles;
}

template<typename N>
const std::vector<Request<N>>& DARP_instance<N>::get_requests() const {
    return *requests;
}

template<typename N>
const std::shared_ptr<Travel_time_provider<N>>& DARP_instance<N>::get_travelcost_provider() const {
    return travel_cost_provider;
}

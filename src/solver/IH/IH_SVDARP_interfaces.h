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
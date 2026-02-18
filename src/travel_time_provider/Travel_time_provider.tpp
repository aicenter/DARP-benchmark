//
// Created by david on 2023-09-13.
//
#pragma once

template<typename L>
time_type Travel_time_provider<L>::get_travel_time_from_vehicle(const Vehicle_base& vehicle, const L& target) const {
	if (const auto* virtual_veh = dynamic_cast<const Virtual_vehicle*>(&vehicle)) {
		return virtual_veh->get_time_to_start();
	}
	
	const auto& regular_veh = dynamic_cast<const Vehicle<L>&>(vehicle);
	return get_travel_time(regular_veh.get_init_position(), target);
}


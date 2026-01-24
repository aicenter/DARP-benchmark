//
// Created by Fido on 2020-04-02.
//

#include "Vehicle.h"

Virtual_vehicle::Virtual_vehicle(unsigned short capacity, unsigned int time_to_start, unsigned int vehicle_count)
    : Vehicle_base(capacity)
    , time_to_start(time_to_start)
    , vehicle_count(vehicle_count) {
}

unsigned int Virtual_vehicle::get_time_to_start() const {
    return time_to_start;
}

unsigned int Virtual_vehicle::get_vehicle_count() const {
    return vehicle_count;
}

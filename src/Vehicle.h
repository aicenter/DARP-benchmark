//
// Created by Fido on 2020-04-02.
//

#pragma once

#include <memory>
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "solver/IH/IH_SVDARP_interfaces.h"


template <typename N>
class Vehicle {

public:
    Vehicle(unsigned int index, std::shared_ptr<N> initial_position, unsigned short capacity);
    const N& get_init_position() const;

    [[nodiscard]] const std::shared_ptr<N>& get_init_position_ptr() const;

    [[nodiscard]] unsigned short get_capacity() const;

    void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;

    [[nodiscard]] unsigned int get_index() const;

    void make_virtual(unsigned int start_time){
        is_virtual_ = true;
        time_to_start = start_time;
    }

    [[nodiscard]] bool is_virtual() const{
        return is_virtual_;
    }

    [[nodiscard]] unsigned int get_time_to_start() const;

private:
    unsigned int index;
    const std::shared_ptr<N> init_position;
    const unsigned short capacity;
    bool is_virtual_{false};
    unsigned int time_to_start{0};

};

#include "Vehicle.tpp"



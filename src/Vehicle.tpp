//
// Created by Fido on 2020-04-02.
//

#include "serialization.h"

// Concept to check if a type has JSON_serialize method
template<typename T>
concept HasJSONSerialize = requires(const T& t, rapidjson::PrettyWriter<rapidjson::StringBuffer>& w) {
    { t.JSON_serialize(w) } -> std::same_as<void>;
};

template <typename N>
Vehicle<N>::Vehicle(unsigned int index, std::shared_ptr<N> initial_position, unsigned short capacity, time_type operation_start)
    : Vehicle_base(capacity)
    , index(index)
    , init_position(std::move(initial_position))
    , operation_start(operation_start) {
}

template <typename N>
const N& Vehicle<N>::get_init_position() const {
    return *init_position;
}

template <typename N>
const std::shared_ptr<N>& Vehicle<N>::get_init_position_ptr() const {
    return init_position;
}

template <typename N>
unsigned int Vehicle<N>::get_index() const {
    return index;
}

template <typename N>
time_type Vehicle<N>::get_operation_start() const {
    return operation_start;
}

template<typename N>
void Vehicle<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("index");
    writer.Uint(index);
    writer.Key("init_position");
    if constexpr (HasJSONSerialize<N>) {
        if(init_position){
            serialize_node(writer, *init_position);
        } else {
            writer.Null();
        }
    } else {
        writer.String("not serializable");
    }
    writer.Key("capacity");
    writer.Uint(get_capacity());
    writer.EndObject();
}

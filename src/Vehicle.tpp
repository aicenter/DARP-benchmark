//
// Created by Fido on 2020-04-02.
//

#include "serialization.h"

template <typename N>
Vehicle<N>::Vehicle(unsigned int index, std::shared_ptr<N> initial_position, unsigned short capacity)
    : Vehicle_base(capacity)
    , index(index)
    , init_position(std::move(initial_position)) {
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

template<typename N>
void Vehicle<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("index");
    writer.Uint(index);
    if(init_position){
		writer.Key("init_position");
		serialize_node(writer, *init_position);
	}
    writer.Key("capacity");
    writer.Uint(get_capacity());
    writer.EndObject();
}

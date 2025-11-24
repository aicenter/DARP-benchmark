//
// Created by david on 2023-10-17.
//

#pragma once

#include "rapidjson/prettywriter.h"

template<typename N>
void serialize_node(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const N& node);

template<>
void serialize_node(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const unsigned& node);


#include "serialization.tpp"
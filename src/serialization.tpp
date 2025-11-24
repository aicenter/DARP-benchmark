//
// Created by david on 2023-10-17.
//

#pragma once

template<typename N>
void serialize_node(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const N& node) {
	node.JSON_serialize(writer);
}


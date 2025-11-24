//
// Created by Fido on 2023-10-17.
//

#include "serialization.h"

template<>
void serialize_node(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const unsigned& node) {
	writer.Uint64(node);
}

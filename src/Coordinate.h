//
// Created by Fido on 2020-03-27.
//

#pragma once

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "DARP_instance.h"

class Coordinate: public Node {
public:
	using Node::Node;
	
    [[nodiscard]] virtual double getX() const = 0;
    [[nodiscard]] virtual double getY() const = 0;


	bool operator==(const Coordinate& other) const;

	bool operator!=(const Coordinate& other) const{
		return !(*this == other);
	}
	
    void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
};



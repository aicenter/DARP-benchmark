//
// Created by olga on 28.5.20.
//
#pragma once

#include "Coordinate.h"


class Amodsim_node final: public Node {
public:
    using Node::Node;


    bool operator==(const Amodsim_node& other) const;
	
    void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;

private:


};




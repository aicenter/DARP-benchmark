//
// Created by olga on 28.5.20.
//

#include <fstream>
#include <vector>
#include <queue>
#include <memory>
#include <tuple>
#include <filesystem>

#include <sstream>

#include <string>


#include "DARP_benchmark_node.h"
#include "DARP_instance.h"



bool Amodsim_node::operator==(const Amodsim_node& other) const {
    if (const Amodsim_node* other_amodsim_node = dynamic_cast<const Amodsim_node*>(&other)) {
        return this->get_index() == other_amodsim_node->get_index();
    }
    return false;
}

void Amodsim_node::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("index");
    writer.Uint64(get_index());
    writer.EndObject();
}



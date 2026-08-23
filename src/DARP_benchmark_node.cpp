/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

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



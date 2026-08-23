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

#include <rapidjson/istreamwrapper.h>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>

#include "inout.h"




std::ifstream get_file_stream(std::string filepath) {
	const bool ok = std::filesystem::exists(filepath);
    if(!ok) {
        throw std::runtime_error(fmt::format("File {} does not exists", std::filesystem::absolute(filepath).string()));
    }

    return std::ifstream(filepath);
}

rapidjson::Document load_json_to_dom(std::string path)
{
	auto ifs = get_file_stream(path);
	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document d;
	d.ParseStream(isw);

	return d;
}

std::filesystem::path check_path(std::string path) {
	const auto abs_path = std::filesystem::absolute(path);
	
	if(!std::filesystem::exists(abs_path)) {
		auto message
			= fmt::format("File does not exists: {} ", path);
		if(abs_path != path) {
			message += fmt::format(" absolute path: {}", abs_path.string());
		}
		throw std::runtime_error(message);
	}

	return abs_path;
}

#pragma once

#include <rapidjson/document.h>

std::ifstream get_file_stream(std::string filepath);

rapidjson::Document load_json_to_dom(std::string path);

std::filesystem::path check_path(std::string path);
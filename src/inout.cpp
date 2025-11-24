
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

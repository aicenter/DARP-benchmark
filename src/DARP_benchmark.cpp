//
// Created by Fido on 2025-12-23.
//

#include "DARP_benchmark.h"

namespace DARP {

Config_registry Config_registry::instance{};

Config_registry& Config_registry::get() {
	return instance;
}

void Config_registry::register_config(std::unique_ptr<fc::Config_definition_base> config) {
	config_definitions.push_back(std::move(config));
}

std::vector<std::unique_ptr<fc::Config_definition_base>> Config_registry::get_all_configs() {
	std::vector<std::unique_ptr<fc::Config_definition_base>> configs;
	configs.reserve(config_definitions.size());
	for (auto& config : config_definitions) {
		configs.push_back(std::move(config));
	}
	config_definitions.clear();
	return configs;
}

}

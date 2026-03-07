#include "common.h"

std::unique_ptr<travel_time_type[]> load_dm_from_json(const rapidjson::Document& doc) {
	const auto& dm_array = doc["dm"].GetArray();
	const unsigned size = dm_array.Size();
	auto dm = std::make_unique<travel_time_type[]>(static_cast<size_t>(size) * size);
	for(unsigned i = 0; i < size; ++i) {
		const auto& dm_inner_array = dm_array[i];
		for(unsigned j = 0; j < size; ++j) {
			dm[i* size + j] = static_cast<travel_time_type>(dm_inner_array[j].GetUint());
		}
	}

	return dm;
}

fs::path get_test_resource_path(const fs::path& relative_path) {
	// Test runner copies contents of data/ to executable dir, so test_resources is next to exe
	return get_running_executable_path() / "test_resources" / relative_path;
}

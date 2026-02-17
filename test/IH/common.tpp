//
// Created by Fido on 2023-10-20.
//


template<class A, class V>
auto IH_SVDARP_test_plan<A, V>::parse_actions_from_json(const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data) {
	const auto& json_actions = json_data["actions"].GetArray();

	// first pass to fill pickup and drop off maps
	std::unordered_map<request_index_type, unsigned short> pickup_map;
	std::unordered_map<request_index_type, unsigned short> drop_off_map;
	for (unsigned short i = 0; i < static_cast<unsigned short>(json_actions.Size()); ++i) {
		const auto& action_data_json = json_actions[i];
		const auto type = action_type_from_string(action_data_json["type"].GetString());
		if (type == Action_type::depot) {
			continue;
		}

		const request_index_type request_index = action_data_json["request"].GetUint();
		if (type == Action_type::pickup) {
			pickup_map[request_index] = i;
		} else {
			drop_off_map[request_index] = i;
		}
	}

	// second pass to create the actions
	std::vector<A> actions;
	for (unsigned i = 0; i < json_actions.Size(); ++i) {
		const auto& action_data_json = json_actions[i];
		const auto type = action_type_from_string(action_data_json["type"].GetString());
		const request_index_type request_index = type == Action_type::depot ? 0: action_data_json["request"].GetUint();
		index_in_plan other_index;
		if (type == Action_type::depot) {
			other_index = -1;
		} else if (type == Action_type::pickup) {
			other_index = static_cast<index_in_plan>(drop_off_map.at(request_index));
		} else {
			other_index = static_cast<index_in_plan>(pickup_map.at(request_index));
		}
		actions.emplace_back(action_data_json, i, other_index);
	}

	return actions;
}


template<class A, class V>
IH_SVDARP_test_plan<A, V>::IH_SVDARP_test_plan(
	const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data,
	const V& vehicle
):
	DARP_benchmark_plan_template<A, V>(
		parse_actions_from_json(json_data),
		vehicle,
		json_data["cost"].GetUint(),
		json_data["departure_time"].GetUint(),
		json_data["arrival_time"].GetUint()
	) {}


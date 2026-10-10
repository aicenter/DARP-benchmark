//
// Created by david on 2023-10-27.
//

template<typename L>
void export_action(
	const Action<L>& action,
	rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer
)  {
	writer.Key("type");
	writer.String(action_type_to_string(action.get_action_type()).c_str());
	writer.Key("min_time");
	writer.Uint64(action.get_min_time());
	writer.Key("max_time");
	writer.Uint64(action.get_max_time());
	auto sa = dynamic_cast<const Service_action<L>*>(&action);
	if(sa != nullptr) {
		writer.Key("request");
		writer.Uint64(sa->get_request().get_index());
	}

}


template<typename L>
void export_plan_and_request(
	const Travel_time_provider<L>& travel_time_provider,
	const VehiclePlan<L>& plan,
	const Request<L>& request,
	const std::string& file_path
) {
	std::vector<const L*> locations;
	locations.push_back(&plan.get_vehicle().get_init_position());

	rapidjson::StringBuffer s;
	rapidjson::PrettyWriter writer(s);

	writer.StartObject();

	// vehicle plan
	writer.Key("plan");
	writer.StartObject();
	writer.Key("departure_time");
	writer.Uint(plan.get_departure_time());
	writer.Key("arrival_time");
	writer.Uint(plan.get_arrival_time());
	writer.Key("cost");
	writer.Double(plan.get_cost());
	writer.Key("actions");
	writer.StartArray();
	for(const auto& action_data: plan) {
		writer.StartObject();
		writer.Key("arrival_time");
		writer.Uint64(action_data.get_arrival_time());
		writer.Key("departure_time");
		writer.Uint64(action_data.get_departure_time());

		const auto& action = action_data.get_action();
		export_action(action, writer);

		writer.EndObject();
		if(action_data.get_action_type() == Action_type::depot) {
			continue;
		}
		else{
			locations.push_back(&action_data.get_node());
		}
	}
	writer.EndArray();
	writer.EndObject();

	// request
	writer.Key("request");
	writer.StartObject();
	writer.Key("index");
	writer.Uint64(request.get_index());
	writer.Key("pickup");
	writer.StartObject();
	export_action(request.get_pickup(), writer);
	writer.EndObject();
	writer.Key("dropoff");
	writer.StartObject();
	export_action(request.get_dropoff(), writer);
	writer.EndObject();
	locations.push_back(&request.get_pickup().get_node());
	locations.push_back(&request.get_dropoff().get_node());
	writer.EndObject();

	// dm
	writer.Key("dm");
	writer.StartArray();
	for(const auto& l1: locations) {
		writer.StartArray();
		for(const auto& l2: locations) {
			writer.Uint64(travel_time_provider.get_travel_time(*l1, *l2));
		}
		writer.EndArray();
	}
	writer.EndArray();

	writer.EndObject();

	std::ofstream test_file(file_path);
	test_file << s.GetString();
	test_file.close();
}

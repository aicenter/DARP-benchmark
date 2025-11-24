//
// Created by Fido on 2020-04-02.
//
template<typename N>
Action_base<N>::Action_base(const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data, N node):
	node(node),
	min_time(json_data["min_time"].GetUint()),
	max_time(json_data["max_time"].GetUint()),
	action_type(action_type_from_string(json_data["type"].GetString()))
{}

template<typename N>
Action<N>::Action(
	std::shared_ptr<N> node,
	unsigned int action_id,
	unsigned int min_time,
	unsigned int max_time,
	enum Action_type action_type,
	unsigned short service_time
):
	Action_base<std::shared_ptr<N>>(node, min_time, max_time, action_type),
	action_id{action_id},
   	service_duration{service_time} {}



//template <typename N> Action<N>::Action(const Action& action)
//        : node{action.node}, action_id{action.action_id}, min_time{action.min_time}, max_time{action.max_time},
//        action_type{action.action_type}, service_duration{action.service_duration}, request{action.request}{}


//template<typename N>
//template<typename T, std::enable_if_t<Dereferenceable<T>>>
//Action_base<N>::node_ret_type Action_base<N>::get_node() const {
//	return *node;
//}
//
template<typename N>
//template<typename T, std::enable_if_t<!Dereferenceable<T>>>
Action_base<N>::node_ret_type Action_base<N>::get_node() const {
	if constexpr(Dereferenceable<N>) {
		return *node;
	}
	else{
		return node;
	}
}

template<typename N>
std::shared_ptr<N> Action<N>::get_node_pointer() const {
	return this->node;
}

template<typename N>
unsigned int Action_base<N>::get_min_time() const {
	return min_time;
}


template<typename N>
unsigned int Action_base<N>::get_max_time() const {
	return max_time;
}

template<typename N>
unsigned int Action<N>::get_action_id() const {
	return action_id;
}

template<typename N>
const Action_type& Action_base<N>::get_action_type() const {
	return action_type;
}

template<typename N>
void Action<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
	writer.StartObject();
	writer.Key("id");
	writer.Uint64(action_id);
	writer.Key("type");
	writer.String(action_type_to_string(this->action_type).c_str());
	writer.Key("position");
//    node->JSON_serialize(writer);
	serialize_node(writer, this->get_node());
	writer.Key("min_time");
	writer.Uint64(this->min_time);
	writer.Key("max_time");
	writer.Uint64(this->max_time);
	writer.Key("service_duration");
	writer.Uint(service_duration);
	writer.EndObject();
}


template<typename N>
unsigned short Action<N>::get_service_duration() const {
	return service_duration;
}


template<class N>
Service_action<N>::Service_action(
	std::shared_ptr<N> node,
	unsigned action_id,
	unsigned min_time,
	unsigned max_time,
	Action_type action_type,
	const Request<N>& request,
	unsigned short service_time
):
	Action<N>(node, action_id, min_time, max_time, action_type, service_time),
	request(&request) {}

template<typename N>
const Request<N>& Service_action<N>::get_request() const {
	return *request;
}


template<typename N>
void Service_action<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
	writer.StartObject();
	writer.Key("id");
	writer.Uint64(this->action_id);
	writer.Key("request_index");
	writer.Uint(request->get_index());
	writer.Key("type");
	writer.String(action_type_to_string(this->action_type).c_str());
	writer.Key("position");
	serialize_node(writer, *this->node);
	writer.Key("min_time");
	writer.Uint64(this->min_time);
	writer.Key("max_time");
	writer.Uint64(this->max_time);
	writer.Key("service_duration");
	writer.Uint(this->service_duration);
	writer.EndObject();
}

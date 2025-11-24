//
// Created by Fido on 2020-06-20.
//



template<typename N>
ActionData<N>::ActionData(const Action<N>& action_param) :
    action(std::ref(action_param))
    {}


template<typename N>
ActionData<N>::ActionData(
	const rapidjson::GenericValue<rapidjson::UTF8<>>& json_action_data,
	index_in_plan position_in_plan, 
	index_in_plan other_action_data_index,
	const Action<N>& action
) :
	action(std::ref(action)),
	arrival_time(json_action_data["arrival_time"].GetUint()),
	departure_time(json_action_data["departure_time"].GetInt()),
	position_in_plan(position_in_plan),
	other_action_data_index(other_action_data_index)
{}


//template <typename N>
//ActionData<N>::ActionData(const ActionData& action_data) :
//	Action_data_base(action_data),
//    action{ action_data.action }
//{
//}

//template <typename N>
//ActionData<N>::ActionData(ActionData&& action_data) noexcept :
//	Action_data_base(action_data),
//    action{ std::move(action_data.action) }
//{
//}

//template <typename N>
//ActionData<N>& ActionData<N>::operator=(const ActionData& action_data)
//{
//	if (this == &action_data)
//        return *this;
//    Action_data_base::operator=(action_data);
//    action = action_data.action;
//    return *this;
//}
//
//template <typename N>
//ActionData<N>& ActionData<N>::operator=(ActionData&& action_data) noexcept
//{
//    if (this == &action_data)
//        return *this;
//    Action_data_base::operator=(action_data);
//    action = std::move(action_data.action);
//    return *this;
//}

template<typename N>
void ActionData<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("arrival_time");
    writer.Uint64(this->arrival_time);
    writer.Key("departure_time");
    writer.Uint64(this->departure_time);
    writer.Key("action");
    action.get().JSON_serialize(writer);
    writer.EndObject();
}

template<typename N>
const Action<N>& ActionData<N>::get_action() const {
    return action.get();
}

template<typename N>
void ActionData<N>::set_arrival_time(unsigned int arrival_time_par) {
    assert(arrival_time_par <= action.get().get_max_time());
    arrival_time = arrival_time_par;
}

template<typename N>
void ActionData<N>::set_departure_time(unsigned int departure_time_par) {
	assert(departure_time_par - action.get().get_service_duration() >= 0);
    assert(departure_time_par - action.get().get_service_duration() <= get_max_time());
    assert(departure_time_par - action.get().get_service_duration() >= get_min_time());
    departure_time = departure_time_par;
}

template <typename N>
void ActionData<N>::set_other_action_data_index(index_in_plan other_action_data_index_par) {
	other_action_data_index = other_action_data_index_par;
}


//template<typename N>
//ActionData<N> *ActionData<N>::get_other() const {
//    return other;
//}
//
//template<typename N>
//void ActionData<N>::set_other(ActionData<N> *other_par) {
//    other = other_par;
//}


template<typename N>
unsigned long ActionData<N>::get_action_id() const {
    return this->action.get().get_action_id();
}

template<typename N>
Action_type ActionData<N>::get_action_type() const {
    return this->action.get().get_action_type();
}

template<typename N>
unsigned ActionData<N>::get_max_time() const {
    return this->action.get().get_max_time();
}

template<typename N>
unsigned ActionData<N>::get_min_time() const {
    return this->action.get().get_min_time();
}

template<typename N>
const N& ActionData<N>::get_node() const {
    return this->action.get().get_node();
}

template <typename N>
request_index_type ActionData<N>::get_request_index() const {
    try {
        const Service_action<N>& sa = dynamic_cast<const Service_action<N>&>(action.get());
        return sa.get_request().get_index();
    }
    catch(const std::bad_cast&) {
        throw std::runtime_error("Request index can be only returned from a service action");
    }
}

template <typename N>
unsigned short ActionData<N>::get_service_duration() const {
	return action.get().get_service_duration();
}

template <typename N>
unsigned ActionData<N>::get_min_service_end() const {
	return arrival_time + get_service_duration();
}

template <typename N>
unsigned int ActionData<N>::get_arrival_time() const {
    return arrival_time;
}

template <typename N>
int ActionData<N>::get_departure_time() const {
    return departure_time;
}

template <typename N>
index_in_plan ActionData<N>::get_position_in_plan() const {
	return position_in_plan;
}

template <typename N>
index_in_plan ActionData<N>::get_other_action_data_index() const {
	return other_action_data_index;
}

template <typename N>
void ActionData<N>::set_position_in_plan(index_in_plan position_in_plan_par) {
	position_in_plan = position_in_plan_par;
}

template <typename N>
unsigned ActionData<N>::get_service_start_time() const {
    return departure_time - get_service_duration();
}

template <typename N>
void ActionData<N>::delete_departure_time() {
    departure_time = -1;
}



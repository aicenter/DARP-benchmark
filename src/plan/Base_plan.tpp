//
// Created by Fido on 2023-10-13.
//

template<class A, class V>
Base_plan<A, V>::Base_plan(const V& vehicle_par): vehicle(vehicle_par){}

template<class A, class V>
const std::vector<A>& Base_plan<A, V>::get_actions() const {
	return this->actions;
}

template<class A, class V>
cost_type Base_plan<A, V>::get_cost() const {
	return cost;
}

template<class A, class V>
const V& Base_plan<A, V>::get_vehicle() const {
	return vehicle;
}

template<class A, class V>
time_type Base_plan<A, V>::get_departure_time() const {
	return departure_time;
}

template<class A, class V>
time_type Base_plan<A, V>::get_arrival_time() const {
	return arrival_time;
}

template<class A, class V>
const A& Base_plan<A,V>::operator[](plan_size_type index) const {
	assert(index < actions.size());
	return this->actions[index];
}

template<class A, class V>
plan_size_type Base_plan<A,V>::get_length() const {
	return static_cast<plan_size_type>(this->actions.size());
}

template<class A, class V>
typename std::vector<A>::const_iterator Base_plan<A,V>::begin() const {
	return this->actions.begin();
}

template<class A, class V>
typename std::vector<A>::const_iterator Base_plan<A,V>::end() const {
	return this->actions.end();
}

template<class A, class V>
bool Plan_size_comparator<A, V>::operator()(const Base_plan<A, V>& plan_a, const Base_plan<A, V>& plan_b) {
	return plan_a.get_length() > plan_b.get_length();
}

//
// Created by Fido on 2023-10-13.
//
template <DARP_benchmark_solver_action_data A, class V>
plan_size_type DARP_benchmark_plan_template<A,V>::get_service_action_length() const {
	if(this->get_length() == 0){
		return 0;
	}

	// first action is a depot action
	if(this->actions[0].get_action_type() == Action_type::depot) {
		// last action is also a depot action
		if(this->actions[this->get_length() - 1].get_action_type() == Action_type::depot) {
			return std::max(
				static_cast<index_in_plan>(this->get_length() - 2), static_cast<index_in_plan>(0u));
		}

		return std::max(
			static_cast<index_in_plan>(this->get_length() - 1), static_cast<index_in_plan>(0u));
	}

	// no depot actions
	return this->get_length();
}



template <class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V, A> P>
template<Vehicle_plan_builder_action_with_request AR>
IH_vehicle_plan_builder<V, A, P>::IH_vehicle_plan_builder(const P& plan):
	Vehicle_plan_builder<V, A, P>(plan, plan.get_service_action_length() * 4 + 2),
	action_data_used_length(plan.get_service_action_length())
{
	assert(action_data_used_length == this->action_data.size());
}

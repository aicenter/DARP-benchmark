#pragma once

template<class P>
void check_plan_basics_equal(const P& computed_plan, const P& expected_plan)
{
	EXPECT_EQ(computed_plan.get_departure_time(), expected_plan.get_departure_time());
	EXPECT_EQ(computed_plan.get_arrival_time(), expected_plan.get_arrival_time());
}

template<typename A>
const A& Test_request<A>::get_pickup() const {
	return pickup_action_data;
}

template<typename A>
const A& Test_request<A>::get_dropoff() const {
	return drop_off_action_data;
}
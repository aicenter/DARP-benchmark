//
// Created by Fido on 2023-04-02.
//

#include "gtest_wrapper.h"

#include "../src/travel_time_provider/Distance_matrix_travel_time_provider.h"

namespace {


TEST(Distance_matrix_travelcost_provider_test, get_travel_time){
	std::unique_ptr<travel_time_type[]> dm(new travel_time_type[]{0, 5, 5, 0});
	Distance_matrix_travel_time_provider travel_cost_provider{2, std::move(dm)};
	ASSERT_EQ(travel_cost_provider.get_travel_time(0, 0), 0);
	ASSERT_EQ(travel_cost_provider.get_travel_time(0, 1), 5);
}


TEST(Distance_matrix_travelcost_provider_test, DISABLED_get_travel_time_big_indices){
	size_t size = 100'000;
	std::unique_ptr<travel_time_type[]> dm = std::make_unique<travel_time_type[]>(size * size);
	unsigned origin = 80000;
	unsigned destination = 90000;
	dm[origin * size + destination] = 1;
	Distance_matrix_travel_time_provider travel_cost_provider{static_cast<unsigned>(size), std::move(dm)};
	ASSERT_EQ(travel_cost_provider.get_travel_time(0, 0), 0);
	ASSERT_EQ(travel_cost_provider.get_travel_time(origin, destination), 1);
}

}

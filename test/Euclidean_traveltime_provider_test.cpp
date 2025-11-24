//
// Created by Fido on 2020-03-27.
//

#include <memory>
#include "gtest_wrapper.h"

#include "../src/travel_time_provider/Euclidean_travel_time_provider.h"
#include "../src/Cordeau_benchmark.h"

namespace {
    TEST(Euclidean_travelcost_provider_test, ZeroDistance){
        Euclidean_travel_time_provider<Cordeau_node> travelcost_provider{60};
        std::shared_ptr<Cordeau_node> from {new Cordeau_node{5.1f, 4.2f}};
        std::shared_ptr<Cordeau_node> to {new Cordeau_node{5.1f, 4.2f}};
        double cost = travelcost_provider.get_travel_time(*from, *to);
        ASSERT_DOUBLE_EQ(cost, 0);
    }

    TEST(Euclidean_travelcost_provider_test, DistanceOne){
        Euclidean_travel_time_provider<Cordeau_node> travelcost_provider{60};
        std::shared_ptr<Cordeau_node> from {new Cordeau_node{5.1f, 4.2f}};
        std::shared_ptr<Cordeau_node> to {new Cordeau_node{5.1f, 3.2f}};
        double cost = travelcost_provider.get_travel_time(*from, *to);
        ASSERT_EQ(cost, 60);
    }

    TEST(Euclidean_travelcost_provider_test, DistanceFromData){
        Euclidean_travel_time_provider<Cordeau_node> travelcost_provider{60};
        std::shared_ptr<Cordeau_node> from {new Cordeau_node{-2.973f, 6.414f}};
        std::shared_ptr<Cordeau_node> to {new Cordeau_node{-5.476f, 1.437f}};
        double cost = travelcost_provider.get_travel_time(*from, *to);
        ASSERT_NEAR(cost, 334, 1);
    }
}


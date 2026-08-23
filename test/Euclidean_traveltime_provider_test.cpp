/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

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


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

#include "gtest_wrapper.h"
#include "common.h"
#include "../src/Cordeau_benchmark.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace {

    TEST(Cordeau_reader_test, CorrectOutput){
        Cordeau_reader reader{};
        auto instance_path = get_test_resource_path("pr01");
        DARP_instance<Cordeau_node> instance = reader.read(instance_path.string());

        ASSERT_FALSE(instance.get_requests().empty());
        ASSERT_FALSE(instance.get_vehicles().empty());
        ASSERT_NE(instance.get_travelcost_provider().get(), nullptr);
//        ASSERT_FALSE();

        ASSERT_EQ(instance.get_vehicles().size(), 3);
        ASSERT_EQ(instance.get_requests().size(), 24);

        Vehicle<Cordeau_node> first_veh = instance.get_vehicles()[0];
        EXPECT_EQ(first_veh.get_capacity(), 6);
        EXPECT_NEAR(first_veh.get_init_position().getX(), -1.044, 0.001);
        EXPECT_NEAR(first_veh.get_init_position().getY(), 2.0, 0.001);
        Vehicle<Cordeau_node> last_veh = instance.get_vehicles()[2];
        EXPECT_EQ(first_veh.get_capacity(), 6);
        EXPECT_NEAR(first_veh.get_init_position().getX(), -1.044, 0.001);
        EXPECT_NEAR(first_veh.get_init_position().getY(), 2.0, 0.001);

        const Request<Cordeau_node>& first_request = instance.get_requests()[0];
        EXPECT_NEAR(first_request.get_pickup().get_node().getX(), -2.973, 0.001);
        EXPECT_NEAR(first_request.get_pickup().get_node().getY(), 6.414, 0.001);
        EXPECT_EQ(first_request.get_pickup().get_min_time(), 0u);
        EXPECT_EQ(first_request.get_pickup().get_max_time(), (unsigned int) (1440 * 60));
        const Request<Cordeau_node>& last_request = instance.get_requests()[23];
        EXPECT_NEAR(last_request.get_pickup().get_node().getX(), -3.530, 0.001);
        EXPECT_NEAR(last_request.get_pickup().get_node().getY(), -2.490, 0.001);
        EXPECT_EQ(last_request.get_pickup().get_min_time(), (unsigned int) (321 * 60));
        EXPECT_EQ(last_request.get_pickup().get_max_time(), (unsigned int) (346 * 60));
        EXPECT_NEAR(last_request.get_dropoff().get_node().getX(), 4.288, 0.001);
        EXPECT_NEAR(last_request.get_dropoff().get_node().getY(), -0.297, 0.001);
        EXPECT_EQ(last_request.get_dropoff().get_min_time(), 0u);
        EXPECT_EQ(last_request.get_dropoff().get_max_time(), (unsigned int) (1440 * 60));
        const Request<Cordeau_node>& request_15 = instance.get_requests()[14];
        EXPECT_NEAR(request_15.get_pickup().get_node().getX(), -5.204, 0.001);
        EXPECT_NEAR(request_15.get_pickup().get_node().getY(), 0.657, 0.001);
        EXPECT_EQ(request_15.get_pickup().get_min_time(), (unsigned int) (395 * 60));
        EXPECT_EQ(request_15.get_pickup().get_max_time(), (unsigned int) (421 * 60));
    }
}


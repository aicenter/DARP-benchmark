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

/**
 * Compilation and behaviour test for Grid_travel_time_provider (C++ port of darpinstances GridTravelTimeProvider)
 * and Grid_instance_reader.
 */
#include "gtest_wrapper.h"
#include "travel_time_provider/Grid_travel_time_provider.h"
#include <filesystem>
#include <DARP_instance.h>

using namespace fleet_sizing;

namespace {
	struct Test_grid_node {
		unsigned vertex_index{0};
		[[nodiscard]] unsigned get_vertex_index() const { return vertex_index; }
		explicit Test_grid_node(unsigned index = 0) : vertex_index(index) {}
	};
}

TEST(GridTravelTimeProvider, matches_python_formula) {
	Grid_travel_time_provider<Test_grid_node> provider(3, 10);
	EXPECT_EQ(provider.get_node_count(), 9u);

	// Vertex 0 = (0,0), vertex 2 = (0,2): Manhattan = 2, time = 2 * 10 = 20
	Test_grid_node from(0);
	Test_grid_node to(2);
	EXPECT_EQ(provider.get_travel_time(from, to), 20u);

	// Vertex 0 to vertex 8 = (2,2): Manhattan = 4, time = 40
	Test_grid_node to_corner(8);
	EXPECT_EQ(provider.get_travel_time(from, to_corner), 40u);

	// Same cell: 0
	EXPECT_EQ(provider.get_travel_time(from, from), 0u);
}

TEST(GridTravelTimeProvider, get_vehicle_location_info) {
	Grid_travel_time_provider<Test_grid_node> provider(3, 10);
	Test_grid_node last(0);
	Test_grid_node next(8);  // (2,2), Manhattan 4, total time 40

	// At departure: at start, time 0
	auto [loc0, t0] = provider.get_vehicle_location_info(last, next, 0);
	EXPECT_EQ(loc0.get_vertex_index(), 0u);
	EXPECT_EQ(t0, 0);

	// After full travel: at end, time 40
	auto [loc_end, t_end] = provider.get_vehicle_location_info(last, next, 40);
	EXPECT_EQ(loc_end.get_vertex_index(), 8u);
	EXPECT_EQ(t_end, 40u);

	// Mid-way: after 20 (2 steps), path 0->1->2 then column; step 0=(0,0), 1=(1,0), 2=(2,0)
	auto [loc_mid, t_mid] = provider.get_vehicle_location_info(last, next, 20);
	EXPECT_EQ(loc_mid.get_vertex_index(), 6u);  // (2,0) = 2*3+0
	EXPECT_EQ(t_mid, 20u);
}

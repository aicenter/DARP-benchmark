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

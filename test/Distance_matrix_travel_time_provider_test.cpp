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

#include <memory>
#include <vector>

#include "../src/travel_time_provider/Distance_matrix_travel_time_provider.h"

namespace {

/** Minimal node for Distance_matrix_node_travel_time_provider (matrix index only). */
struct Indexed_test_node {
	unsigned index_{};

	[[nodiscard]] unsigned get_index() const {
		return index_;
	}
};

[[nodiscard]] std::vector<std::shared_ptr<const Indexed_test_node>> make_two_indexed_nodes() {
	return {
		std::make_shared<const Indexed_test_node>(Indexed_test_node{0u}),
		std::make_shared<const Indexed_test_node>(Indexed_test_node{1u}),
	};
}

TEST(Distance_matrix_node_travel_time_provider_test, get_travel_time) {
	std::unique_ptr<travel_time_type[]> dm(new travel_time_type[]{0, 5, 5, 0});
	Distance_matrix_node_travel_time_provider<Indexed_test_node> provider(
		make_two_indexed_nodes(),
		std::move(dm)
	);
	const Indexed_test_node a0{0u};
	const Indexed_test_node a1{1u};
	ASSERT_EQ(provider.get_travel_time(a0, a0), 0);
	ASSERT_EQ(provider.get_travel_time(a0, a1), 5);
	ASSERT_EQ(provider.get_travel_time(a1, a0), 5);
	ASSERT_EQ(provider.get_travel_time(a1, a1), 0);
}

TEST(Distance_matrix_node_travel_time_provider_test, get_vehicle_location_info) {
	auto nodes = make_two_indexed_nodes();
	const Indexed_test_node* node1 = nodes[1].get();
	std::unique_ptr<travel_time_type[]> dm(new travel_time_type[]{0, 5, 5, 0});
	Distance_matrix_node_travel_time_provider<Indexed_test_node> provider(std::move(nodes), std::move(dm));
	const Indexed_test_node last{0u};
	const Indexed_test_node next{1u};
	const time_type elapsed_since_departure = 2;
	const auto [at_node, time_to_next] = provider.get_vehicle_location_info(last, next, elapsed_since_departure);
	EXPECT_EQ(&at_node, node1);
	EXPECT_EQ(at_node.get_index(), 1u);
	EXPECT_EQ(time_to_next, 3u);
}

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

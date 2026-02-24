#pragma once

/**
 * C++ port of GridTravelTimeProvider from darpinstances.travel_time_provider (Python).
 * Travel time on a 2D square grid. Nodes are indexed in row-major order from 0 to grid_size² - 1.
 * Travel time between horizontally or vertically neighboring nodes is travel_time_between_neighbors;
 * for non-adjacent nodes, travel time is Manhattan distance times that value.
 *
 * Node type N must provide either get_vertex_index() const or get_index() const (see grid_vertex_index_trait).
 * For get_vehicle_location_info, N must be constructible from unsigned (vertex index).
 */

#include <travel_time_provider/Travel_time_provider.h>
#include <memory>
#include <tuple>

namespace fleet_sizing {

/** Trait to get vertex index from node N. Specialize for types that use get_index() (e.g. Amodsim_node). */
template <typename N>
struct grid_vertex_index_trait {
	static unsigned get(const N& n) { return n.get_vertex_index(); }
};

/**
 * Travel time provider for a square grid.
 * Node type N: use grid_vertex_index_trait<N>::get(n) for vertex index (default get_vertex_index(); specialize for get_index()).
 * For get_vehicle_location_info, N must be constructible from unsigned (vertex index).
 */
template <typename N>
class Grid_travel_time_provider : public Travel_time_provider<N> {
public:
	/**
	 * @param grid_size Side of the square grid (grid has grid_size × grid_size vertices).
	 * @param travel_time_between_neighbors Travel time in seconds per grid edge.
	 */
	Grid_travel_time_provider(unsigned grid_size, travel_time_type travel_time_between_neighbors)
		: grid_size_(grid_size)
		, travel_time_between_neighbors_(travel_time_between_neighbors) {}

	[[nodiscard]] travel_time_type get_travel_time(const N& from, const N& to) const override {
		return get_travel_time_impl(grid_vertex_index_trait<N>::get(from), grid_vertex_index_trait<N>::get(to));
	}

	/**
	 * Returns current location of the vehicle on an arbitrary shortest path from last to next,
	 * and the travel time from last_action_location to that position.
	 * Path is chosen arbitrarily among equal-cost Manhattan paths (row-first then column).
	 */
	[[nodiscard]] std::tuple<const N&, time_type> get_vehicle_location_info(
		const N& last_action_location,
		const N& next_action_location,
		time_type time_since_last_action_departure) override {
		const unsigned from_idx = grid_vertex_index_trait<N>::get(last_action_location);
		const unsigned to_idx = grid_vertex_index_trait<N>::get(next_action_location);
		const travel_time_type total_time = get_travel_time_impl(from_idx, to_idx);
		if (total_time == 0) {
			current_location_cache_ = std::make_unique<N>(to_idx);
			return {*current_location_cache_, 0};
		}
		if (time_since_last_action_departure >= total_time) {
			current_location_cache_ = std::make_unique<N>(to_idx);
			return {*current_location_cache_, total_time};
		}
		// Steps completed (each step = one edge = travel_time_between_neighbors_ seconds)
		const unsigned steps_total = static_cast<unsigned>(total_time / travel_time_between_neighbors_);
		const unsigned steps_by_time = static_cast<unsigned>(time_since_last_action_departure / travel_time_between_neighbors_);
		const unsigned steps_done = (steps_by_time <= steps_total) ? steps_by_time : steps_total;
		const unsigned vertex_at_steps = path_vertex_at_step(from_idx, to_idx, steps_done);
		const time_type time_to_current = steps_done * travel_time_between_neighbors_;
		current_location_cache_ = std::make_unique<N>(vertex_at_steps);
		return {*current_location_cache_, time_to_current};
	}

	[[nodiscard]] unsigned get_node_count() const {
		return grid_size_ * grid_size_;
	}

private:
	[[nodiscard]] travel_time_type get_travel_time_impl(unsigned from_index, unsigned to_index) const {
		const unsigned r1 = from_index / grid_size_;
		const unsigned c1 = from_index % grid_size_;
		const unsigned r2 = to_index / grid_size_;
		const unsigned c2 = to_index % grid_size_;
		const auto dr = static_cast<travel_time_type>(r1 > r2 ? r1 - r2 : r2 - r1);
		const auto dc = static_cast<travel_time_type>(c1 > c2 ? c1 - c2 : c2 - c1);
		const travel_time_type manhattan = dr + dc;
		return manhattan * travel_time_between_neighbors_;
	}

	/** Arbitrary Manhattan path: move along row first, then column. Returns vertex index at step s (0 = from). */
	[[nodiscard]] unsigned path_vertex_at_step(unsigned from_index, unsigned to_index, unsigned step) const {
		const unsigned r1 = from_index / grid_size_;
		const unsigned c1 = from_index % grid_size_;
		const unsigned r2 = to_index / grid_size_;
		const unsigned c2 = to_index % grid_size_;
		const int dr = static_cast<int>(r2) - static_cast<int>(r1);
		const int dc = static_cast<int>(c2) - static_cast<int>(c1);
		const unsigned abs_dr = static_cast<unsigned>(dr >= 0 ? dr : -dr);
		const int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
		const int step_c = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);
		unsigned r = r1, c = c1;
		if (step <= abs_dr) {
			r = static_cast<unsigned>(static_cast<int>(r1) + step_r * static_cast<int>(step));
			c = c1;
		} else {
			r = r2;
			c = static_cast<unsigned>(static_cast<int>(c1) + step_c * static_cast<int>(step - abs_dr));
		}
		return r * grid_size_ + c;
	}

	unsigned grid_size_;
	travel_time_type travel_time_between_neighbors_;
	mutable std::unique_ptr<N> current_location_cache_;
};

} // namespace fleet_sizing

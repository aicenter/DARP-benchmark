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
#pragma once

#include <limits>
#include <type_traits>

/** Node index reserved for the virtual node. */
inline constexpr unsigned virtual_node_index = std::numeric_limits<unsigned>::max();

/**
 * @brief The virtual node of the node type \p N: the origin of the first leg of a vehicle without a physical position
 * (Virtual_vehicle). Leg cost components (see Leg_cost_component) contribute nothing on a leg from or to it.
 * Node types not constructible from a node index specialize this function.
 */
template<typename N>
[[nodiscard]] const N& virtual_node() {
	static const N node(virtual_node_index);
	return node;
}

/** Whether \p node is the virtual node; nodes are Node subclasses or plain indices. */
template<typename N>
[[nodiscard]] bool is_virtual_node(const N& node) {
	if constexpr(std::is_arithmetic_v<N>) {
		return node == virtual_node_index;
	}
	else {
		return node.is_virtual();
	}
}

/** Matrix index of \p node; nodes are Node subclasses or plain indices. */
template<typename N>
[[nodiscard]] unsigned node_index(const N& node) {
	if constexpr(std::is_arithmetic_v<N>) {
		return static_cast<unsigned>(node);
	}
	else {
		return node.get_index();
	}
}

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

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace YAML {
class Node;
}

/**
 * @brief A cost component priced per driven leg by a model loaded at startup, e.g., the distance from a distance
 * matrix. The leg cost is weight() * value(from, to). The component values are taken from the model only: unlike the
 * travel time, a detour of a re-routed vehicle does not change them.
 *
 * To add a criterion: implement this interface (or reuse Matrix_leg_cost_component for a matrix-based criterion) and
 * add its loader to leg_cost_component_types().
 */
class Leg_cost_component {
public:
	virtual ~Leg_cost_component() = default;

	/** Key of the component in the `cost_components` breakdown of the solution, e.g., `distance`. */
	[[nodiscard]] virtual const std::string& name() const = 0;

	/** Weight of a unit of the component quantity, from the `cost` section of the instance config. */
	[[nodiscard]] virtual double weight() const = 0;

	/** Unweighted quantity of the leg between the nodes with indices \p from_index and \p to_index. */
	[[nodiscard]] virtual double value(unsigned from_index, unsigned to_index) const = 0;
};

using Leg_cost_components = std::vector<std::shared_ptr<const Leg_cost_component>>;

/** A leg cost component given by a square matrix of non-negative integers indexed by node indices. */
class Matrix_leg_cost_component final: public Leg_cost_component {
public:
	Matrix_leg_cost_component(std::string name, double weight, std::unique_ptr<uint32_t[]> matrix, unsigned size);

	/**
	 * Loads the matrix from \p filepath: HDF5 for the `.h5` extension, CSV otherwise (the formats of the travel time
	 * matrix). The matrix must have \p node_count rows and columns, as the travel time matrix.
	 */
	[[nodiscard]] static std::shared_ptr<const Matrix_leg_cost_component> load(
		std::string name, double weight, const std::filesystem::path& filepath, unsigned node_count);

	[[nodiscard]] const std::string& name() const override {
		return name_;
	}

	[[nodiscard]] double weight() const override {
		return weight_;
	}

	[[nodiscard]] double value(unsigned from_index, unsigned to_index) const override {
		return static_cast<double>(matrix_[static_cast<size_t>(from_index) * size_ + to_index]);
	}

private:
	std::string name_;
	double weight_;
	std::unique_ptr<uint32_t[]> matrix_;
	unsigned size_;
};

/**
 * @brief Registry entry of a leg cost component type: the weight key in the `cost` section of the instance config and
 * the loader called for a non-zero weight with the instance config, the instance directory and the node count of the
 * travel time matrix.
 */
struct Leg_cost_component_type {
	std::string weight_key;
	std::function<std::shared_ptr<const Leg_cost_component>(
		double weight, const YAML::Node& config, const std::filesystem::path& instance_dir, unsigned node_count
	)> load;
};

/** All leg cost component types supported by the benchmark. */
[[nodiscard]] const std::vector<Leg_cost_component_type>& leg_cost_component_types();

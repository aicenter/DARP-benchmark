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

#include <unordered_map>

#include "Travel_time_provider.h"
#include "../Nearest_vehicle_provider.h"
#include "../common.h"
#include "../Vehicle.h"
#include "../aliases.h"

class Distance_matrix_reader {
public:
	virtual ~Distance_matrix_reader() = default;
	virtual std::pair<std::unique_ptr<travel_time_type[]>, unsigned> read_matrix(const std::string& file_path) = 0;

};

/**
 * @brief Travel time provider that uses a distance matrix lookup to determine the travel time.
 * The locations here are indices to the matrix. Does not inherit from Travel_time_provider<unsigned>
 * so that get_vehicle_location_info can return std::tuple<unsigned, travel_time_type> by value.
 */
class Distance_matrix_travel_time_provider {
public:
	Distance_matrix_travel_time_provider(unsigned width, unsigned height, std::unique_ptr<travel_time_type[]> dm);

	Distance_matrix_travel_time_provider(unsigned int size, std::unique_ptr<travel_time_type[]> dm);

	/**
	 * This constructor creates the underlying distance matrix using the \p reader which reads the matrix 
     * from \p dm_filepah
	 * @param reader
	 * @param dm_filepath
	 */
	Distance_matrix_travel_time_provider(Distance_matrix_reader& reader, const std::string& dm_filepath);

	/**
	 * This constructor creates the underlying distance matrix using the \p reader which reads the matrix 
     * from \p dm_filepah
	 * @param reader
	 * @param dm_filepath
	 */
	Distance_matrix_travel_time_provider(Distance_matrix_reader&& reader, const std::string& dm_filepath);

	[[nodiscard]] travel_time_type get_travel_time(const unsigned& from, const unsigned& to) const;

	[[nodiscard]] std::tuple<unsigned, travel_time_type> get_vehicle_location_info(
		const unsigned& last_action_location,
		const unsigned& next_action_location,
		time_type time_since_last_action_departure
	) const;

	[[nodiscard]] unsigned get_matrix_width() const {
		return width;
	}

	[[nodiscard]] unsigned get_matrix_height() const {
		return height;
	}

protected:
	unsigned width;
	unsigned height;
	std::unique_ptr<travel_time_type[]> dm;
private:
	
};

/**
 * @brief Distance matrix travel time provider template that uses a node to get an index to the distance matrix.
 * It can be later transformed to a class with the use of the Node interface.
 * @tparam L
*/
template <typename L>
class Distance_matrix_node_travel_time_provider:
	public Travel_time_provider<L>,
	public Nearest_vehicle_provider<L>,
	public Distance_matrix_travel_time_provider
{

	std::unordered_map<unsigned,std::shared_ptr<const L>> nodes{};


public:
	using Distance_matrix_travel_time_provider::Distance_matrix_travel_time_provider;

	Distance_matrix_node_travel_time_provider(Distance_matrix_reader& reader, const std::string& dm_filepath);

	/**
	 * Creates dm from distances between all nodes. Requires another tt provider to compute the distances.
	 * @param nodes_par
	 * @param travel_time_provider
	 */
	Distance_matrix_node_travel_time_provider(
		const std::vector<std::shared_ptr<const L>>& nodes_par,
		const Travel_time_provider<L>& travel_time_provider
	);

	/**
	 * Builds provider from pre-filled nodes and distance matrix. Nodes must be indexed 0..nodes_par.size()-1
	 * (node at nodes_par[i] is used for matrix index i).
	 */
	Distance_matrix_node_travel_time_provider(
		const std::vector<std::shared_ptr<const L>>& nodes_par,
		std::unique_ptr<travel_time_type[]> dm
	);


	using Distance_matrix_travel_time_provider::get_travel_time;

	travel_time_type get_travel_time(const L& from, const L& to) const override;

	[[nodiscard]] std::tuple<const L&, travel_time_type> get_vehicle_location_info(
		const L& last_action_location,
		const L& next_action_location,
		time_type time_since_last_action_departure
	) const override;

	[[nodiscard]] const L& get_canonical_node(unsigned index) const {
		return *nodes.at(index);
	}

	size_t get_nearest_vehicle(const L& location, const std::vector<const Vehicle<L>*>& vehicles) override;
};

//static_assert(Travel_time_provider<Distance_matrix_node_travel_time_provider<int>, int>);

#include "Distance_matrix_travel_time_provider.tpp"

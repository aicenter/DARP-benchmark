#pragma once

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
 * @brief Travel time provider that use a distance matrix lookup to determine the travel time.
 * The locations here are indices to the matrix.
*/
class Distance_matrix_travel_time_provider: public Travel_time_provider<unsigned> {
public:
	Distance_matrix_travel_time_provider(unsigned width, unsigned height, std::unique_ptr<travel_time_type[]> dm);

	Distance_matrix_travel_time_provider(unsigned int size, std::unique_ptr<travel_time_type[]> dm);

	Distance_matrix_travel_time_provider(Distance_matrix_reader& reader, const std::string& dm_filepath);

	Distance_matrix_travel_time_provider(Distance_matrix_reader&& reader, const std::string& dm_filepath);

	[[nodiscard]] travel_time_type get_travel_time(const unsigned& from, const unsigned& to) const override;

protected:
	unsigned width;
	unsigned height;
	std::unique_ptr<travel_time_type[]> dm;
private:
	
};

/**
 * @brief Distance matrix travel time provider template that uses a node to get an index to the distance matrix.
 * It can be later transformed to a class with the use of the Node interface.
 * @tparam N 
*/
template <typename N>
class Distance_matrix_node_travel_time_provider:
	public Travel_time_provider<N>,
	public Nearest_vehicle_provider<N>,
	public Distance_matrix_travel_time_provider {

public:
	using Distance_matrix_travel_time_provider::Distance_matrix_travel_time_provider;

	Distance_matrix_node_travel_time_provider(
		const std::vector<const N*>& nodes, 
		const Travel_time_provider<N>& travel_time_provider
	);


	using Distance_matrix_travel_time_provider::get_travel_time;

	travel_time_type get_travel_time(const N& from, const N& to) const override;


	size_t get_nearest_vehicle(const N& location, const std::vector<const Vehicle<N>*>& vehicles) override;
};

//static_assert(Travel_time_provider<Distance_matrix_node_travel_time_provider<int>, int>);
static_assert(Pointer_iterable<std::vector<const Vehicle<int>*>,Vehicle<int>>);

#include "Distance_matrix_travel_time_provider.tpp"

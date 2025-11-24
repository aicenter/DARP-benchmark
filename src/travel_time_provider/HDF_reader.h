#pragma once

#include "Distance_matrix_travel_time_provider.h"

class HDF_reader: public Distance_matrix_reader {
	std::pair<std::unique_ptr<travel_time_type[]>, unsigned> read_matrix(const std::string& dm_filepath) override;
};
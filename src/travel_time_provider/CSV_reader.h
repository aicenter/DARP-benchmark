#pragma once

#include "Distance_matrix_travel_time_provider.h"

class CSV_reader: public Distance_matrix_reader {
public:
	std::pair<std::unique_ptr<travel_time_type[]>, unsigned> read_matrix(const std::string& file_path) override;
};
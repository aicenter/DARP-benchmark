
#include "Distance_matrix_travel_time_provider.h"
#include "../inout.h"
#include <limits>

#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4268)
#endif
#include <H5Cpp.h>
#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

Distance_matrix_travel_time_provider::Distance_matrix_travel_time_provider(
	unsigned width,
	unsigned height,
	std::unique_ptr<travel_time_type[]> dm
): width(width), height(height), dm(std::move(dm)) {}

Distance_matrix_travel_time_provider::Distance_matrix_travel_time_provider(
	unsigned int size,
	std::unique_ptr<travel_time_type[]> dm
): Distance_matrix_travel_time_provider(size, size, std::move(dm)) {}

Distance_matrix_travel_time_provider::Distance_matrix_travel_time_provider(
	Distance_matrix_reader& reader, 
	const std::string& dm_filepath
) {
	auto data = reader.read_matrix(dm_filepath);
	width = height = data.second;
	dm = std::move(data.first);
}

Distance_matrix_travel_time_provider::Distance_matrix_travel_time_provider(
	Distance_matrix_reader&& reader,
	const std::string& dm_filepath
): Distance_matrix_travel_time_provider(reader, dm_filepath) {}

travel_time_type Distance_matrix_travel_time_provider::get_travel_time(const unsigned& from, const unsigned& to) const {
	assert(from < height);
	assert(to < width);
	size_t index = static_cast<size_t>(from) * width + to;

	assert((index - to) / width == from);

	return dm[index];
}

std::tuple<unsigned, time_type> Distance_matrix_travel_time_provider::get_vehicle_location_info(
	const unsigned& last_action_location,
	const unsigned& next_action_location,
	time_type time_since_last_action_departure
) const {
	auto lowest_tt_to_i = std::numeric_limits<travel_time_type>::max();
	unsigned lowest_tt_to_i_index = 0;

	for(unsigned int i = 0; i < width; ++i) {
		auto tt_last_to_i = get_travel_time(last_action_location, i);

		if(tt_last_to_i >= time_since_last_action_departure && tt_last_to_i < lowest_tt_to_i) {
			auto tt_last_to_next = get_travel_time(last_action_location, next_action_location);
			auto tt_i_to_next = get_travel_time(i, next_action_location);

			if(tt_last_to_next == tt_last_to_i + tt_i_to_next) {
				lowest_tt_to_i = tt_last_to_i;
				lowest_tt_to_i_index = i;
			}
		}
	}

	// compute the time to next node
	auto time_to_next_node = lowest_tt_to_i - time_since_last_action_departure;

	return {lowest_tt_to_i_index, time_to_next_node};
}

//travel_time_type parse_distance(
//	const std::string& str, 
//	unsigned int nodeFrom, 
//	unsigned int nodeTo, const std::string& inputFile, 
//	double precisionLoss) {
//	double val;
//	try {
//		val = crack_atof(str.c_str(), str.c_str() + str.size());
//	}
//	catch (std::invalid_argument&) {
//		std::cerr << "Warning: Found an unexpected value (" << str << ") in '" << inputFile << "'. It will be interpreted as 'no edge' from node " << nodeFrom << " to node " << nodeTo << "." << std::endl;
//		return std::numeric_limits<travel_time_type>::max();
//	}
//	catch (std::out_of_range&) {
//		std::cerr << "Warning: Found an out of range value (" << str << ") in '" << inputFile << "'. It will be interpreted as 'no edge' from node " << nodeFrom << " to node " << nodeTo << "." << std::endl;
//		return std::numeric_limits<travel_time_type>::max();
//	}
//
//	if (std::isnan(val)) {
//		return std::numeric_limits<travel_time_type>::max();
//	}
//
//	if (val < 0) {
//		std::cerr << "Warning: Found a negative value (" << str << ") in '" << inputFile << "'. It will be interpreted as 'no edge' from node " << nodeFrom << " to node " << nodeTo << "." << std::endl;
//		return std::numeric_limits<travel_time_type>::max();
//	}
//
//	return static_cast<travel_time_type>(round(val / (double)precisionLoss));
//}

//void Distance_matrix_travel_time_provider::read_matrix(std::string dm_filepath){
//
//	csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>, csv2::first_row_is_header<false>,
//		csv2::trim_policy::trim_characters<' ', '\t', '\r', '\n'>> reader;
//
//	if(!std::filesystem::exists(dm_filepath)) {
//		throw std::runtime_error(std::string("Distance matrix file does not exists: ") + dm_filepath + "\n");
//	}
//
//	if (!reader.mmap(dm_filepath)) {
//		throw std::runtime_error(std::string("Error reading file ") + dm_filepath + " using mmap.\n");
//	}
//	
//	size = static_cast<unsigned>(reader.cols());
//	if (size != reader.rows())
//		throw std::runtime_error(dm_filepath + " does not contain a square matrix. Found " +
//			std::to_string(reader.rows()) + " rows and " + std::to_string(size) + " cols.\n");
//	this->dm = std::make_unique<travel_time_type[]>(size * size);
//	unsigned int i = 0;
//
//	constexpr unsigned int step = 200;
//
//	indicators::ProgressBar bar{
//		indicators::option::BarWidth{70},
//		indicators::option::PostfixText{"Loading Distance Matrix"},
//		indicators::option::MaxProgress{size / step}
//	};
//	
//	for (const auto& row: reader) {
//		unsigned int j = 0;
//		for (const auto& cell: row) {
//			std::string val;
//			/*cell.read_value(val);*/
//			cell.read_raw_value(val);
//			const travel_time_type dist = parse_distance(val, i, j, dm_filepath, (double)1);
//			dm[i * size + j] = dist;
//			++j;
//		}
//		if ((i % step) == 0) {
//			bar.tick();
//		}
//		++i;
//	}
//}

//void Distance_matrix_travel_time_provider::read_matrix(std::string dm_filepath){
//		
//	if(!std::filesystem::exists(dm_filepath)) {
//		throw std::runtime_error(std::string("Distance matrix file does not exists: ") + dm_filepath + "\n");
//	}
//
//	csv::CSVReader reader(dm_filepath);
//
//	constexpr unsigned int step = 200;
//	indicators::ProgressBar bar{
//		indicators::option::BarWidth{70},
//		indicators::option::PostfixText{"Loading Distance Matrix"},
//		indicators::option::MaxProgress{size / step}
//	};
//	bool first = true;
//	unsigned int i = 0;
//	for (const auto& row: reader) {
//
//		if(first) {
//			first = false;
//			size = static_cast<unsigned>(row.size());
//			this->dm = std::make_unique<travel_time_type[]>(static_cast<size_t>(size) * size);
//		}
//
//		unsigned int j = 0;
//		for (auto& cell: row) {
//			const auto val = cell.get<short>();
//			dm[i * size + j] = val;
//			++j;
//		}
//		if ((i % step) == 0) {
//			bar.tick();
//		}
//		++i;
//	}
//}


#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <stdexcept>
#if defined(_MSC_VER)
	#define NOMINMAX
#endif
#include <csv2/reader.hpp>

#include "DARP_benchmark_reader.h"

#include "inout.h"
#include "travel_time_provider/CSV_reader.h"
#include "travel_time_provider/Distance_matrix_travel_time_provider.h"
#include "travel_time_provider/Grid_travel_time_provider.h"
#include "travel_time_provider/HDF_reader.h"

namespace fs = std::filesystem;

namespace {
/** Returns true if the file's first line contains a comma (CSV-style). */
bool file_has_commas(const std::string& file_path) {
	std::ifstream f(file_path);
	if (!f) return false;
	std::string first_line;
	while (std::getline(f, first_line) && first_line.find_first_not_of(" \t\r\n") == std::string::npos) {}
	return first_line.find(',') != std::string::npos;
}

/** Detect delimiter: comma or tab (prefers whichever occurs more in the header). */
char detect_tab_or_comma_delimiter(const std::string& header_line) {
	const auto comma_count = static_cast<size_t>(std::count(header_line.begin(), header_line.end(), ','));
	const auto tab_count = static_cast<size_t>(std::count(header_line.begin(), header_line.end(), '\t'));
	if (comma_count == 0 && tab_count == 0) {
		throw std::runtime_error("CSV header must be comma- or tab-delimited");
	}
	return tab_count > comma_count ? '\t' : ',';
}

template<char Delim>
void load_vehicles_csv_impl(std::vector<Vehicle<Amodsim_node>>& vehicles, const std::string& file_path, unsigned instance_start_time) {
	using CsvReader = csv2::Reader<csv2::delimiter<Delim>, csv2::quote_character<'"'>, csv2::first_row_is_header<true>,
		csv2::trim_policy::trim_characters<' ', '\t', '\r', '\n'>>;
	CsvReader reader;
	if (!reader.mmap(file_path)) {
		throw std::runtime_error("Cannot open vehicle CSV file: " + file_path);
	}
	int position_col = -1, capacity_col = -1, operation_start_col = -1;
	int col = 0;
	for (const auto& cell : reader.header()) {
		std::string name;
		cell.read_value(name);
		if (name == "position") position_col = col;
		else if (name == "capacity") capacity_col = col;
		else if (name == "operation_start") operation_start_col = col;
		++col;
	}
	if (position_col < 0 || capacity_col < 0) {
		throw std::runtime_error("Vehicle CSV header must include position and capacity columns");
	}
	if (operation_start_col >= 0 && instance_start_time != 0) {
		throw std::runtime_error(
			"Vehicle CSV has operation_start column but instance configuration also has a nonzero start time. "
			"Use only one: either per-vehicle operation_start in CSV or global vehicles.start_time/operation_start in instance config.");
	}
	unsigned int index = 0;
	for (const auto& row : reader) {
		std::vector<std::string> cells;
		for (const auto& cell : row) {
			std::string val;
			cell.read_value(val);
			cells.push_back(val);
		}
		if (cells.empty()) continue;
		if (static_cast<int>(cells.size()) <= std::max(position_col, capacity_col)) {
			throw std::runtime_error("Too few columns in vehicle CSV row");
		}
		unsigned int position = static_cast<unsigned int>(std::stoul(cells[position_col]));
		unsigned short capacity = static_cast<unsigned short>(std::stoul(cells[capacity_col]));
		time_type operation_start = 0;
		if (operation_start_col >= 0 && static_cast<int>(cells.size()) > operation_start_col && !cells[operation_start_col].empty()) {
			operation_start = static_cast<time_type>(std::stoul(cells[operation_start_col]));
		} else if (instance_start_time != 0) {
			operation_start = static_cast<time_type>(instance_start_time);
		}
		std::shared_ptr<Amodsim_node> initial_position{new Amodsim_node(position)};
		vehicles.emplace_back(index++, initial_position, capacity, operation_start);
	}
}
} // namespace

DARP_instance<Amodsim_node> DARP_benchmark_reader::read(std::filesystem::path instance_filepath) {
    spdlog::info("Reading Amodsim instance from: {}", instance_filepath.string());
    std::ifstream infile(instance_filepath);
	YAML::Node config = YAML::LoadFile(instance_filepath.string());

	auto configuration = internal::load_instance_configuration(config);

	// Grid instance: type == "grid" (same logic as Python load_instance)
	if (config["type"] && config["type"].as<std::string>() == "grid") {
		const unsigned grid_size = config["size"].as<unsigned>();
		const travel_time_type distance = config["distance"].as<travel_time_type>();
		auto travel_cost_provider = std::make_shared<fleet_sizing::Grid_travel_time_provider<Amodsim_node>>(grid_size, distance);
		spdlog::info("Using grid travel time provider (size={}, distance={})", grid_size, distance);
		auto vehicles = std::make_unique<std::vector<Vehicle<Amodsim_node>>>();
		auto requests = load_requests(config, std::static_pointer_cast<Travel_time_provider<Amodsim_node>>(travel_cost_provider));
		return {
			std::move(requests),
			std::move(vehicles),
			std::static_pointer_cast<Travel_time_provider<Amodsim_node>>(travel_cost_provider),
			configuration
		};
	}

    // dm loading (non-grid)
    fs::path dm_filepath;
    if(config["dm_filepath"]) {
	    dm_filepath = fs::path(config["dm_filepath"].as<std::string>());
    }
	else {
		fs::path dm_dir;
		if(config["area_dir"]) {
			dm_dir = fs::path(config["area_dir"].as<std::string>());
		}
		// if not specified, the distance matrix is loaded from file dm.csv located in the same directory as the instance
		// file
		else {
			dm_dir = instance_filepath.parent_path();
		}
		auto hdf_filepath = dm_dir / "dm.h5";
		if(fs::exists(hdf_filepath)) {
			spdlog::info("Found dm.h5 file in area directory, using HDF reader.");
			dm_filepath = hdf_filepath;
		}
		else {
			spdlog::info("dm.h5 file not found in area directory, using CSV reader.");
			dm_filepath = dm_dir / "dm.csv";
		}
	}
    std::unique_ptr<Distance_matrix_reader> dm_reader = dm_filepath.extension() == ".h5"
	    ? static_cast<std::unique_ptr<Distance_matrix_reader>>(std::make_unique<HDF_reader>())
	    : std::make_unique<CSV_reader>();
    auto travel_cost_provider = std::make_shared<Distance_matrix_node_travel_time_provider<Amodsim_node>>(*dm_reader, dm_filepath.string());

    // vehicle loading
	const unsigned instance_start_time = configuration->get_start_time();
	auto vehicles = std::make_unique<std::vector<Vehicle<Amodsim_node>>>();
	if(!configuration->use_virtual_vehicles()){
	    std::string vehicles_filepath = std::filesystem::path(instance_filepath).remove_filename().string() + "vehicles.csv";
	    if (file_has_commas(vehicles_filepath)) {
		    internal::load_vehicles_csv(*vehicles, vehicles_filepath, instance_start_time);
	    } else {
		    load_vehicles(*vehicles, vehicles_filepath, instance_start_time);
	    }
    }
    // When virtual vehicles mode is enabled, the vehicles vector stays empty.
    // Algorithms supporting virtual vehicles will create Virtual_vehicle instances as needed.

    // request loading - Calls dispatcher
    auto requests = load_requests(config, std::static_pointer_cast<Travel_time_provider<Amodsim_node>>(travel_cost_provider));

    return {
		std::move(requests),
		std::move(vehicles),
        std::static_pointer_cast<Travel_time_provider<Amodsim_node>>(travel_cost_provider),
		configuration
	};
}

void DARP_benchmark_reader::load_vehicles(std::vector<Vehicle<Amodsim_node>>& vehicles, std::string file_path, unsigned instance_start_time) const {
    spdlog::info("Reading vehicles from: {}", file_path);

	std::ifstream infile(file_path);
    if(infile.fail()){
        throw std::ios_base::failure(fmt::format("Cannot open vehicle file: {}", file_path));
    }

	unsigned int origin;
	unsigned short capacity;
    unsigned int index = 0;
	while (infile >> origin >> capacity) {
        std::shared_ptr<Amodsim_node> initial_position {new Amodsim_node(origin)};
		vehicles.emplace_back(index++, initial_position, capacity, static_cast<time_type>(instance_start_time));
    }
}



// Dispatcher function
std::unique_ptr<std::vector<Request<Amodsim_node>>> DARP_benchmark_reader::load_requests(
    const YAML::Node& config,
    const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
    const auto request_filepath_str = config["demand"]["filepath"].as<std::string>();
    const auto request_filepath = check_path(request_filepath_str); // Ensure path is valid
	// Grid instances use max_travel_time_delay.seconds; others use max_prolongation (same as Python load_instance)
	const auto max_prolongation = (config["max_travel_time_delay"] && config["max_travel_time_delay"]["seconds"])
		? config["max_travel_time_delay"]["seconds"].as<unsigned short>()
		: config["max_prolongation"].as<unsigned short>();

    if (request_filepath.extension() == ".csv") {
        spdlog::info("Detected .csv format, using CSV loader for requests.");
        return load_requests_csv(request_filepath.string(), max_prolongation, travel_cost_provider);
    } else { // Assuming .di or other format for the original loader
        spdlog::info("Detected non-csv format, using DI loader for requests.");
        return load_requests_di(request_filepath.string(), max_prolongation, travel_cost_provider);
    }
}

// Original loader for .di format (Renamed)
std::unique_ptr<std::vector<Request<Amodsim_node>>> DARP_benchmark_reader::load_requests_di(
    const std::string& request_filepath_str,
    unsigned short max_prolongation,
    const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
    spdlog::info("Loading requests from DI file: {}", request_filepath_str);
    std::ifstream reqfile(request_filepath_str);
    if (!reqfile) {
        throw std::runtime_error("Cannot open request file: " + request_filepath_str);
    }
    auto requests = std::make_unique<std::vector<Request<Amodsim_node>>>();
    unsigned long id;
    unsigned long time_ms; // Read time in milliseconds
    unsigned int from;
    unsigned int to;
    unsigned int action_id = 0;
    while (reqfile >> id >> time_ms >> from >> to) {
        unsigned int time = static_cast<unsigned int>(time_ms / 1000); // Convert to seconds
        std::shared_ptr<Amodsim_node> pickup_node {new Amodsim_node(from)};
        std::shared_ptr<Amodsim_node> drop_off_node {new Amodsim_node(to)};
		auto min_travel_time = static_cast<unsigned short>(travel_cost_provider->get_travel_time(*pickup_node, *drop_off_node));
        requests->emplace_back(id, action_id, action_id + 1,
                pickup_node, time, time + max_prolongation,
                drop_off_node, time + min_travel_time, time + min_travel_time + max_prolongation, min_travel_time);
        action_id += 2;
    }
    return requests;
}

namespace {
template<char Delim>
std::unique_ptr<std::vector<Request<Amodsim_node>>> load_requests_csv_impl(
	const std::string& request_filepath_str,
	unsigned short max_prolongation,
	const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
	using CsvReader = csv2::Reader<csv2::delimiter<Delim>, csv2::quote_character<'"'>, csv2::first_row_is_header<true>,
		csv2::trim_policy::trim_characters<' ', '\t', '\r', '\n'>>;
	CsvReader reader;
	if (!reader.mmap(request_filepath_str)) {
		throw std::runtime_error("Cannot open request CSV file: " + request_filepath_str);
	}
	auto requests = std::make_unique<std::vector<Request<Amodsim_node>>>();
	int time_col = -1, origin_col = -1, dest_col = -1;
	bool time_in_seconds = false;
	int col = 0;
	for (const auto& cell : reader.header()) {
		std::string name;
		cell.read_value(name);
		if (name == "time_ms") {
			time_col = col;
			time_in_seconds = false;
		} else if (name == "time") {
			time_col = col;
			time_in_seconds = true;
		} else if (name == "origin") {
			origin_col = col;
		} else if (name == "dest") {
			dest_col = col;
		} else if (name == "destination") {
			dest_col = col;
		}
		++col;
	}
	if (time_col < 0 || origin_col < 0 || dest_col < 0) {
		throw std::runtime_error("Request CSV header must include time/time_ms, origin, and dest/destination columns");
	}
	const int max_col = std::max({time_col, origin_col, dest_col});
	unsigned int action_id = 0;
	unsigned long request_id_counter = 0;
	for (const auto& row : reader) {
		std::vector<std::string> cells;
		for (const auto& cell : row) {
			std::string val;
			cell.read_value(val);
			cells.push_back(val);
		}
		if (cells.empty()) continue;
		if (static_cast<int>(cells.size()) <= max_col) {
			throw std::runtime_error("Too few columns in request CSV row");
		}
		unsigned long time_val = std::stoul(cells[time_col]);
		unsigned int from = static_cast<unsigned int>(std::stoul(cells[origin_col]));
		unsigned int to = static_cast<unsigned int>(std::stoul(cells[dest_col]));
		unsigned int time = time_in_seconds ? static_cast<unsigned int>(time_val) : static_cast<unsigned int>(time_val / 1000);
		std::shared_ptr<Amodsim_node> pickup_node{new Amodsim_node(from)};
		std::shared_ptr<Amodsim_node> drop_off_node{new Amodsim_node(to)};
		auto min_travel_time = static_cast<unsigned short>(travel_cost_provider->get_travel_time(*pickup_node, *drop_off_node));
		requests->emplace_back(request_id_counter++, action_id, action_id + 1,
			pickup_node, time, time + max_prolongation,
			drop_off_node, time + min_travel_time, time + min_travel_time + max_prolongation, min_travel_time);
		action_id += 2;
	}
	return requests;
}
} // namespace

// Loader for .csv format: supports header "time" (seconds) or "time_ms" (milliseconds), "origin", "dest" or "destination"
std::unique_ptr<std::vector<Request<Amodsim_node>>> DARP_benchmark_reader::load_requests_csv(
	const std::string& request_filepath_str,
	unsigned short max_prolongation,
	const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
	spdlog::info("Loading requests from CSV file: {}", request_filepath_str);
	std::ifstream file(request_filepath_str);
	if (!file) {
		throw std::runtime_error("Cannot open request CSV file: " + request_filepath_str);
	}
	std::string header_line;
	if (!std::getline(file, header_line)) {
		throw std::runtime_error("Request CSV file is empty: " + request_filepath_str);
	}
	const char delimiter = detect_tab_or_comma_delimiter(header_line);
	if (delimiter == '\t') {
		return load_requests_csv_impl<'\t'>(request_filepath_str, max_prolongation, travel_cost_provider);
	} else {
		return load_requests_csv_impl<','>(request_filepath_str, max_prolongation, travel_cost_provider);
	}
}

namespace internal {

static problem_type parse_problem(const YAML::Node& config) {
	if (!config["problem"]) {
		return problem_type::darp;
	}
	const std::string value = config["problem"].as<std::string>();
	if (value == "DARP") {
		return problem_type::darp;
	}
	if (value == "fleet-sizing") {
		return problem_type::fleet_sizing;
	}
	throw std::runtime_error(
		"Invalid 'problem' in instance config: expected 'DARP' or 'fleet-sizing', got: " + value);
}

/** Try to parse a YAML node as start time in seconds: as integer, or as datetime string "%Y-%m-%d %H:%M:%S". */
static unsigned parse_start_time_seconds(const YAML::Node& node) {
	if (!node) return 0;
	unsigned seconds = 0;
	if (YAML::convert<unsigned>::decode(node, seconds)) {
		return seconds;
	}
	const auto str = node.as<std::string>();
	std::tm t{};
	std::stringstream(str) >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
	return t.tm_sec + t.tm_min * 60 + t.tm_hour * 3600;
}

std::shared_ptr<DARP_instance_configuration> load_instance_configuration(const YAML::Node& config) {
	unsigned start_time_seconds = 0;
	unsigned short vehicle_capital_cost = 0;
	double relative_delay_cost = 0.0;

	// vehicle start time: check both keys, try integer and datetime for whichever is present
	if (config["vehicles"]) {
		const auto& vehicles = config["vehicles"];
		if (vehicles["operation_start"]) {
			start_time_seconds = parse_start_time_seconds(vehicles["operation_start"]);
		} else if (vehicles["start_time"]) {
			start_time_seconds = parse_start_time_seconds(vehicles["start_time"]);
		}
	}

	// vehicle capital cost parsing (optional)
	if(config["vehicles"] && config["vehicles"]["capital_cost"]) {
		vehicle_capital_cost = config["vehicles"]["capital_cost"].as<unsigned short>();
	}

	// relative delay cost parsing (optional)
	if(config["demand"] && config["demand"]["relative_delay_cost"]) {
		relative_delay_cost = config["demand"]["relative_delay_cost"].as<double>();
	}

	const problem_type problem = parse_problem(config);

	return std::make_shared<DARP_instance_configuration>(
			0,
			0,
			false,
			false,
			start_time_seconds,
			vehicle_capital_cost,
			relative_delay_cost,
			problem
	);
}

void load_vehicles_csv(std::vector<Vehicle<Amodsim_node>>& vehicles, const std::string& file_path, unsigned instance_start_time) {
	spdlog::info("Reading vehicles from CSV file: {}", file_path);
	std::ifstream file(file_path);
	if (!file) {
		throw std::runtime_error("Cannot open vehicle CSV file: " + file_path);
	}
	std::string header_line;
	if (!std::getline(file, header_line)) {
		throw std::runtime_error("Vehicle CSV file is empty: " + file_path);
	}
	const char delimiter = detect_tab_or_comma_delimiter(header_line);
	if (delimiter == '\t') {
		load_vehicles_csv_impl<'\t'>(vehicles, file_path, instance_start_time);
	} else {
		load_vehicles_csv_impl<','>(vehicles, file_path, instance_start_time);
	}
}

}
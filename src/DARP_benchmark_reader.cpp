#include <spdlog/spdlog.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <filesystem> // Include for path operations
#include "csv.h" // Include the fast-cpp-csv-parser header

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

/** Trim leading/trailing spaces from a string. */
std::string trim_copy(const std::string& s) {
    auto start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t");
    return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
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

/** Parse vehicle CSV header: accept "position", "capacity", and optionally "operation_start". */
void parse_vehicles_csv_header(const std::string& header_line,
    char delimiter,
    int& position_col, int& capacity_col, int& operation_start_col) {
    std::istringstream iss(header_line);
    std::string cell;
    int col = 0;
    position_col = -1;
    capacity_col = -1;
    operation_start_col = -1;
    while (std::getline(iss, cell, delimiter)) {
        std::string name = trim_copy(cell);
        if (name == "position") {
            position_col = col;
        } else if (name == "capacity") {
            capacity_col = col;
        } else if (name == "operation_start") {
            operation_start_col = col;
        }
        ++col;
    }
    if (position_col < 0 || capacity_col < 0) {
        throw std::runtime_error("Vehicle CSV header must include position and capacity columns");
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
	auto vehicles = std::make_unique<std::vector<Vehicle<Amodsim_node>>>();
	if(!configuration->use_virtual_vehicles()){
	    std::string vehicles_filepath = std::filesystem::path(instance_filepath).remove_filename().string() + "vehicles.csv";
	    if (file_has_commas(vehicles_filepath)) {
		    load_vehicles_csv(*vehicles, vehicles_filepath);
	    } else {
		    load_vehicles(*vehicles, vehicles_filepath);
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

void DARP_benchmark_reader::load_vehicles(std::vector<Vehicle<Amodsim_node>>& vehicles, std::string file_path) const {
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
		vehicles.emplace_back(index++, initial_position, capacity);
    }
}

void DARP_benchmark_reader::load_vehicles_csv(std::vector<Vehicle<Amodsim_node>>& vehicles, const std::string& file_path) const {
    spdlog::info("Reading vehicles from CSV file: {}", file_path);
    unsigned int index = 0;

    std::ifstream file(file_path);
    if (!file) {
        throw std::runtime_error("Cannot open vehicle CSV file: " + file_path);
    }

    std::string header_line;
    if (!std::getline(file, header_line)) {
        throw std::runtime_error("Vehicle CSV file is empty: " + file_path);
    }

    const char delimiter = detect_tab_or_comma_delimiter(header_line);

    int position_col, capacity_col, operation_start_col;
    try {
        parse_vehicles_csv_header(header_line, delimiter, position_col, capacity_col, operation_start_col);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Vehicle CSV header: ") + e.what());
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> cells;
        std::istringstream iss(line);
        std::string cell;
        while (std::getline(iss, cell, delimiter)) {
            cells.push_back(trim_copy(cell));
        }
        if (static_cast<int>(cells.size()) <= std::max(position_col, capacity_col)) {
            throw std::runtime_error("Too few columns in vehicle CSV row: " + line);
        }

        unsigned int position = static_cast<unsigned int>(std::stoul(cells[position_col]));
        unsigned short capacity = static_cast<unsigned short>(std::stoul(cells[capacity_col]));
        time_type operation_start = 0;

        if (operation_start_col >= 0 && static_cast<int>(cells.size()) > operation_start_col && !cells[operation_start_col].empty()) {
            operation_start = static_cast<time_type>(std::stoul(cells[operation_start_col]));
        }

        std::shared_ptr<Amodsim_node> initial_position{new Amodsim_node(position)};
        vehicles.emplace_back(index++, initial_position, capacity, operation_start);
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
	const auto max_prolongation = (config["type"] && config["type"].as<std::string>() == "grid")
		? static_cast<unsigned short>(config["max_travel_time_delay"]["seconds"].as<unsigned int>())
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
/** Parse request CSV header: accept "time_ms" or "time" (time), "origin", "dest" or "destination". */
void parse_requests_csv_header(const std::string& header_line,
	char delimiter,
    int& time_col, int& origin_col, int& dest_col, bool& time_in_seconds) {
    std::istringstream iss(header_line);
    std::string cell;
    int col = 0;
    time_col = -1;
    origin_col = -1;
    dest_col = -1;
    time_in_seconds = false;
    while (std::getline(iss, cell, delimiter)) {
        std::string name = trim_copy(cell);
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
}
} // namespace

// Loader for .csv format: supports header "time" (seconds) or "time_ms" (milliseconds), "origin", "dest" or "destination"
std::unique_ptr<std::vector<Request<Amodsim_node>>> DARP_benchmark_reader::load_requests_csv(
    const std::string& request_filepath_str,
    unsigned short max_prolongation,
    const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
    spdlog::info("Loading requests from CSV file: {}", request_filepath_str);
    auto requests = std::make_unique<std::vector<Request<Amodsim_node>>>();
    unsigned int action_id = 0;
    unsigned long request_id_counter = 0;

    std::ifstream file(request_filepath_str);
    if (!file) {
        throw std::runtime_error("Cannot open request CSV file: " + request_filepath_str);
    }

    std::string header_line;
    if (!std::getline(file, header_line)) {
        throw std::runtime_error("Request CSV file is empty: " + request_filepath_str);
    }

	const char delimiter = detect_tab_or_comma_delimiter(header_line);

    int time_col, origin_col, dest_col;
    bool time_in_seconds;
    try {
        parse_requests_csv_header(header_line, delimiter, time_col, origin_col, dest_col, time_in_seconds);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Request CSV header: ") + e.what());
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> cells;
        std::istringstream iss(line);
        std::string cell;
        while (std::getline(iss, cell, delimiter)) {
            cells.push_back(trim_copy(cell));
        }
        const int max_col = std::max({time_col, origin_col, dest_col});
        if (static_cast<int>(cells.size()) <= max_col) {
            throw std::runtime_error("Too few columns in request CSV row: " + line);
        }
        unsigned long time_val = std::stoul(cells[time_col]);
        unsigned int from = static_cast<unsigned int>(std::stoul(cells[origin_col]));
        unsigned int to = static_cast<unsigned int>(std::stoul(cells[dest_col]));
        unsigned int time = time_in_seconds ? static_cast<unsigned int>(time_val) : static_cast<unsigned int>(time_val / 1000);

        std::shared_ptr<Amodsim_node> pickup_node {new Amodsim_node(from)};
        std::shared_ptr<Amodsim_node> drop_off_node {new Amodsim_node(to)};
        auto min_travel_time = static_cast<unsigned short>(travel_cost_provider->get_travel_time(*pickup_node, *drop_off_node));

        requests->emplace_back(request_id_counter++, action_id, action_id + 1,
                pickup_node, time, time + max_prolongation,
                drop_off_node, time + min_travel_time, time + min_travel_time + max_prolongation, min_travel_time);
        action_id += 2;
    }

    return requests;
}

namespace internal {

std::shared_ptr<DARP_instance_configuration> load_instance_configuration(const YAML::Node& config) {
	unsigned start_time_seconds = 0;
	unsigned short vehicle_capital_cost = 0;
	double relative_delay_cost = 0.0;

	// vehicle start time parsing
	if(config["vehicles"] && config["vehicles"]["start_time"]) {
		const auto start_time_string = config["vehicles"]["start_time"].as<std::string>();
		std::tm start_datetime{};
		std::stringstream(start_time_string) >> std::get_time(&start_datetime, "%Y-%m-%d %H:%M:%S");
		start_time_seconds
				= start_datetime.tm_sec + start_datetime.tm_min * 60 + start_datetime.tm_hour * 3600;
	}

	// vehicle capital cost parsing (optional)
	if(config["vehicles"] && config["vehicles"]["capital_cost"]) {
		vehicle_capital_cost = config["vehicles"]["capital_cost"].as<unsigned short>();
	}

	// relative delay cost parsing (optional)
	if(config["demand"] && config["demand"]["relative_delay_cost"]) {
		relative_delay_cost = config["demand"]["relative_delay_cost"].as<double>();
	}

	return std::make_shared<DARP_instance_configuration>(
			0,
			0,
			false,
			false,
			start_time_seconds,
			vehicle_capital_cost,
			relative_delay_cost
	);
}

}
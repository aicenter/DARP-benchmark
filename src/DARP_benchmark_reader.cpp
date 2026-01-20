#include <spdlog/spdlog.h>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem> // Include for path operations
#include "csv.h" // Include the fast-cpp-csv-parser header

#include "DARP_benchmark_reader.h"

#include "inout.h"
#include "travel_time_provider/CSV_reader.h"
#include "travel_time_provider/Distance_matrix_travel_time_provider.h"
#include "travel_time_provider/HDF_reader.h"

namespace fs = std::filesystem;


DARP_instance<Amodsim_node> DARP_benchmark_reader::read(std::filesystem::path instance_filepath) {
    spdlog::info("Reading Amodsim instance from: {}", instance_filepath.string());
    std::ifstream infile(instance_filepath);
	YAML::Node config = YAML::LoadFile(instance_filepath.string());

	auto configuration = internal::load_instance_configuration(config);

    // dm loading
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
	if(configuration->use_virtual_vehicles()){
	    std::shared_ptr<Amodsim_node> depot_node {new Amodsim_node(0)};
		const auto vehicle_capacity = config["vehicles"]["vehicle_capacity"].as<unsigned short>();
	    vehicles->emplace_back(0, depot_node, vehicle_capacity);
	    vehicles->at(0).make_virtual(0);
    }
    else {
	    std::string vehicles_filepath = std::filesystem::path(instance_filepath).remove_filename().string() + "vehicles.csv";
    	load_vehicles(*vehicles, vehicles_filepath);
    }

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

// Dispatcher function
std::unique_ptr<std::vector<Request<Amodsim_node>>> DARP_benchmark_reader::load_requests(
    const YAML::Node& config,
    const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
    const auto request_filepath_str = config["demand"]["filepath"].as<std::string>();
    const auto request_filepath = check_path(request_filepath_str); // Ensure path is valid
    const auto max_prolongation = config["max_prolongation"].as<unsigned short>();

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

// Loader for .csv format using fast-cpp-csv-parser
std::unique_ptr<std::vector<Request<Amodsim_node>>> DARP_benchmark_reader::load_requests_csv(
    const std::string& request_filepath_str,
    unsigned short max_prolongation,
    const std::shared_ptr<Travel_time_provider<Amodsim_node>>& travel_cost_provider
) {
    spdlog::info("Loading requests from CSV file: {}", request_filepath_str);
    auto requests = std::make_unique<std::vector<Request<Amodsim_node>>>();
    unsigned int action_id = 0;
    unsigned long request_id_counter = 0; // Use a counter for unique IDs

    try {
        // Configure CSV reader: 3 columns, trim spaces/tabs, no quote escaping needed for this format, use comma delimiter
        io::CSVReader<3, io::trim_chars<>, io::no_quote_escape<'\t'>> in(request_filepath_str);
        // Read header, ignore extra columns, look for specific names
        in.read_header(io::ignore_extra_column, "time_ms", "origin", "dest");

        unsigned long time_ms;
        unsigned int from;
        unsigned int to;

        while(in.read_row(time_ms, from, to)){
            unsigned int time = static_cast<unsigned int>(time_ms / 1000); // Convert to seconds
            std::shared_ptr<Amodsim_node> pickup_node {new Amodsim_node(from)};
            std::shared_ptr<Amodsim_node> drop_off_node {new Amodsim_node(to)};
            auto min_travel_time = static_cast<unsigned short>(travel_cost_provider->get_travel_time(*pickup_node, *drop_off_node));

            requests->emplace_back(request_id_counter++, action_id, action_id + 1,
                    pickup_node, time, time + max_prolongation,
                    drop_off_node, time + min_travel_time, time + min_travel_time + max_prolongation, min_travel_time);
            action_id += 2;
        }
    } catch (const io::error::can_not_open_file& e) {
        throw std::runtime_error("Cannot open request CSV file: " + request_filepath_str + " (" + e.what() + ")");
    } catch (const std::exception& e) {
        // Catch other potential errors from the CSV library or Request construction
        throw std::runtime_error("Error reading request CSV file " + request_filepath_str + ": " + e.what());
    }

    return requests;
}

namespace internal {

std::shared_ptr<DARP_instance_configuration> load_instance_configuration(const YAML::Node& config) {

	// vehicle start time parsing
	const auto start_time_string = config["vehicles"]["start_time"].as<std::string>();
	std::tm start_datetime{};
	std::stringstream(start_time_string) >> std::get_time(&start_datetime, "%Y-%m-%d %H:%M:%S");
	const unsigned start_time_seconds
			= start_datetime.tm_sec + start_datetime.tm_min * 60 + start_datetime.tm_hour * 3600;

	return std::make_shared<DARP_instance_configuration>(
			0,
			0,
			false,
			false,
			start_time_seconds
	);
}

}

#include <memory>
#include <stdexcept>
#include <string>
#include <H5Cpp.h>

#include "HDF_reader.h"
#include "../inout.h"
#include "../aliases.h"


std::pair<std::unique_ptr<travel_time_type[]>, unsigned> HDF_reader::read_matrix(
	const std::string& dm_filepath_string){
	const auto dm_filepath = check_path(dm_filepath_string);

	spdlog::info("Reading distance matrix from: {}", dm_filepath.string());
	const H5::H5File dm_file(dm_filepath.string(), H5F_ACC_RDONLY);

	std::string dataset_name;
	// by default, the dataset is named "dm"
	if(dm_file.exists("dm")) {
		dataset_name = "dm";
	}
	// otherwise, we take the first dataset in the file
	else {
		dataset_name = dm_file.getObjnameByIdx(0);
	}

	const H5::DataSet dataset = dm_file.openDataSet(dataset_name);
	const H5::DataSpace filespace = dataset.getSpace();
	hsize_t dims[2];    // dataset dimensions
    filespace.getSimpleExtentDims( dims );
	const auto size = static_cast<unsigned>(dims[0]);

	// square matrix check
	if (size != dims[1])
		throw std::runtime_error(dm_filepath.string() + " does not contain a square matrix. Found " +
			std::to_string(size) + " rows and " + std::to_string(dims[1]) + " cols.\n");

	H5::DataSpace mspace(2, dims);
	auto dm = std::make_unique<travel_time_type[]>(static_cast<size_t>(size) * size);

	dataset.read(
		dm.get(),
		H5::PredType::NATIVE_UINT_LEAST16, 
		mspace,
		filespace
	);

	return {std::move(dm), size};
}

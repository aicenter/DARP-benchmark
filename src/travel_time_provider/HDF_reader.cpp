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

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <H5Cpp.h>

#include "HDF_reader.h"
#include "../inout.h"
#include "../aliases.h"


template<typename T>
std::pair<std::unique_ptr<T[]>, unsigned> read_hdf_matrix(const std::string& dm_filepath_string) {
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
	auto dm = std::make_unique<T[]>(static_cast<size_t>(size) * size);

	static_assert(std::is_same_v<T, uint_least16_t> || std::is_same_v<T, uint32_t>, "unsupported matrix element type");
	const H5::PredType& memory_type = std::is_same_v<T, uint_least16_t>
		? H5::PredType::NATIVE_UINT_LEAST16
		: H5::PredType::NATIVE_UINT32;
	dataset.read(
		dm.get(),
		memory_type, 
		mspace,
		filespace
	);

	return {std::move(dm), size};
}

template std::pair<std::unique_ptr<travel_time_type[]>, unsigned> read_hdf_matrix<travel_time_type>(const std::string&);
template std::pair<std::unique_ptr<uint32_t[]>, unsigned> read_hdf_matrix<uint32_t>(const std::string&);

std::pair<std::unique_ptr<travel_time_type[]>, unsigned> HDF_reader::read_matrix(const std::string& dm_filepath) {
	return read_hdf_matrix<travel_time_type>(dm_filepath);
}

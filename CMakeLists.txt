cmake_minimum_required(VERSION 3.31)

project("DARP-benchmark")

enable_language(CXX)

set(CMAKE_CXX_STANDARD 23)

set(DARP_BENCHMARK_SOLVER_PLUGIN_DIRS "" CACHE STRING "Semicolon-separated extra solver dirs")
set(DARP_BENCHMARK_EXTERNAL_MAIN_CONFIG_FILE "" CACHE FILEPATH "Path to external main config file for downstream projects")

find_package(RapidJSON CONFIG REQUIRED)
find_package(OpenMP)
find_package(spdlog CONFIG REQUIRED)
find_path(P_RANAV_CSV2_INCLUDE_DIRS "csv2/mio.hpp")
find_package(nanoflann CONFIG REQUIRED) # for KD trees
find_package(magic_enum CONFIG REQUIRED) # for string to enum conversion
find_package(Boost CONFIG REQUIRED)
find_package(indicators CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS C CXX HL)
find_package(future-config 0.3.0 CONFIG REQUIRED)

# Check boost header-only components 
if (NOT EXISTS ${Boost_INCLUDE_DIRS}/boost/multi_index)
	message(FATAL_ERROR "Boost multi-index not found. Search path: ${Boost_INCLUDE_DIRS}/boost/multi_index")
endif ()
if (NOT EXISTS ${Boost_INCLUDE_DIRS}/boost/algorithm)
	message(FATAL_ERROR "Boost algorithm not found. Search path: ${Boost_INCLUDE_DIRS}/boost/algorithm")
endif ()

# HDF5 debug messages
message(STATUS "HDF libs found: ${HDF5_LIBRARIES}")

find_path(TCLAP_INCLUDE_DIR tclap/CmdLine.h)

# COMPILER OPTIONS
if (MSVC)
	add_compile_options("/W4")
	add_compile_options("/bigobj")
else ()
	add_compile_options("-Wall" "-Wextra")
	# "-Werror"
endif ()

# GOOGLE TEST
include(FetchContent)
FetchContent_Declare(
	googletest
	DOWNLOAD_EXTRACT_TIMESTAMP ON
	URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# GOOGLE TEST END

# Create library with shared files
add_library(DARP_benchmark_lib
	src/common.cpp
	src/Coordinate.cpp
	src/Cordeau_benchmark.cpp
	src/DARP_benchmark_node.cpp
	src/DARP_benchmark_reader.cpp
	src/DARP_instance.cpp
	src/inout.cpp
	src/logging.cpp
	src/resources.cpp
	src/serialization.cpp
	src/Vehicle.cpp
	src/travel_time_provider/CSV_reader.cpp
	src/travel_time_provider/Distance_matrix_travel_time_provider.cpp
	src/travel_time_provider/HDF_reader.cpp
)
target_include_directories(DARP_benchmark_lib
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${RAPIDJSON_INCLUDE_DIRS}
		${Boost_INCLUDE_DIRS}
		${P_RANAV_CSV2_INCLUDE_DIRS}
		${CMAKE_CURRENT_SOURCE_DIR}/fast-cpp-csv-parser
)
target_link_libraries(DARP_benchmark_lib
	PUBLIC
		future-config::future-config
		nanoflann::nanoflann
		OpenMP::OpenMP_CXX
		spdlog::spdlog
		spdlog::spdlog_header_only
		yaml-cpp::yaml-cpp
		${HDF5_LIBRARIES}
		${HDF5_HL_LIBRARIES}
)

# Add source to this project's executable.
add_executable(DARP-benchmark
	natvis/DARP_benchmark.natvis
	src/DARP_benchmark.cpp
	src/Runner.cpp
	src/crack_atof.cpp
	src/memory.cpp
	src/solver/DARP_benchmark_solver.tpp
)
target_include_directories(DARP-benchmark
	PUBLIC
		${RAPIDJSON_INCLUDE_DIRS}
	PRIVATE
		${TCLAP_INCLUDE_DIR}
		include/
)
target_link_libraries(DARP-benchmark
	PRIVATE
		DARP_benchmark_lib
		external_solvers
		indicators::indicators
)

add_library(DARP_benchmark_test_lib
	natvis/DARP_benchmark.natvis
	test/Action_data_base.cpp
	test/common.cpp
	test/IH/common.tpp
	test/Single_vehicle_DARP_common.cpp
	test/functional/functional_test_helpers.cpp
)
target_include_directories(DARP_benchmark_test_lib
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		include/
)
target_link_libraries(DARP_benchmark_test_lib
	PUBLIC
		DARP_benchmark_lib
		gtest_main
)

add_executable(test_run
	test/Cordeau_benchmark_test.cpp
	test/DARP_benchmark_reader_test.cpp
	test/Distance_matrix_travel_time_provider_test.cpp
	test/Euclidean_traveltime_provider_test.cpp
	test/Grid_travel_time_provider_test.cpp
	test/IH/IH_SVDARP_test.cpp
	test/IH/Insertion_heuristic_solver_test.cpp
	test/Solution_test.cpp
	test/Vehicle_plan_test.cpp
	test/functional/functional_tests.cpp
)
target_link_libraries(test_run
	PRIVATE
		DARP_benchmark_test_lib
		GTest::gtest
		GTest::gtest_main
)
target_compile_definitions(test_run PRIVATE IN_TEST)

add_library(external_solvers INTERFACE)

# Set up Future Config
if(DARP_BENCHMARK_EXTERNAL_MAIN_CONFIG_FILE)
	# When external config is provided, pass both default and external main config files
	get_filename_component(DEFAULT_MAIN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/data/config.yaml" ABSOLUTE)
	setup_fconfig(
		MAIN_CONFIG_PATHS ${DEFAULT_MAIN_CONFIG_FILE} ${DARP_BENCHMARK_EXTERNAL_MAIN_CONFIG_FILE}
		TARGET_NAMES DARP-benchmark test_run
	)
else()
	setup_fconfig(TARGET_NAMES DARP-benchmark test_run)
endif()

include(GoogleTest)
gtest_discover_tests(test_run)

add_dependencies(test_run DARP-benchmark)

# Copy the data to build dir
add_custom_command(TARGET test_run POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:test_run>)

foreach(dir IN LISTS DARP_BENCHMARK_SOLVER_PLUGIN_DIRS)
  add_subdirectory(${dir})


endforeach()
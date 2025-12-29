cmake_minimum_required(VERSION 3.15)

project("DARP-benchmark")

enable_language(CXX)

set(CMAKE_CXX_STANDARD 23)

set(DARP_BENCHMARK_SOLVER_PLUGIN_DIRS "" CACHE STRING "Semicolon-separated extra solver dirs")

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
find_package(future-config 0.2.1 CONFIG REQUIRED)

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
	URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# GOOGLE TEST END


# Add source to this project's executable.
add_executable(DARP-benchmark
	src/aliases.h
	src/asserts.h
	src/DARP_benchmark_reader.cpp src/DARP_benchmark_reader.h
	src/Iterable.h
	src/Runner.cpp
	src/DARP_benchmark.cpp
	src/Cordeau_benchmark.cpp src/Cordeau_benchmark.h
	src/DARP_instance.cpp src/DARP_instance.h
	src/crack_atof.cpp
	src/benchmark.h
	src/memory.h src/memory.cpp
	src/Nearest_vehicle_provider.h
	src/Coordinate.cpp src/Coordinate.h
	src/DARP_benchmark_node.cpp src/DARP_benchmark_node.h
	src/Linked_hash_set.h
	src/plan/VehiclePlan.h
	src/solver/DARP_benchmark_solver.tpp src/solver/DARP_benchmark_solver.h
	src/solver/Vehicle_plan_builder.h
	src/solver/IH/Insertion_heuristic_solver.h
	src/solver/IH/IH_vehicle_plan_builder.h
	src/solver/IH/IH_SVDARP_interfaces.h
	src/solver/IH/SVDARP.h
	src/travel_time_provider/CSV_reader.h src/travel_time_provider/CSV_reader.cpp
	src/travel_time_provider/Distance_matrix_travel_time_provider.cpp src/travel_time_provider/Distance_matrix_travel_time_provider.h
	src/travel_time_provider/Euclidean_travel_time_provider.h
	src/travel_time_provider/HDF_reader.cpp src/travel_time_provider/HDF_reader.h
	src/travel_time_provider/Travel_time_provider.h
	src/common.h src/common.cpp
	src/Adjustment_reason.h
	src/KD_tree.h
	src/Plan_checker.h
	src/plan/Benchmark_vehicle_plan.h
	src/progress_bar.h
	src/number_formatter.h
	src/logging.cpp src/logging.h
	src/inout.cpp src/inout.h
	natvis/DARP_benchmark.natvis
	src/plan/Base_plan.h
	src/plan/DARP_benchmark_plan_template.h
	src/serialization.cpp
)

add_executable(test_run
	test/Action_data_base.cpp test/Action_data_base.h
	src/aliases.h
	src/asserts.h
	src/DARP_instance.cpp
	src/Iterable.h
	src/Coordinate.cpp src/Coordinate.h
	src/Cordeau_benchmark.cpp src/Cordeau_benchmark.h
	src/DARP_benchmark_node.cpp src/DARP_benchmark_node.h
	src/DARP_benchmark_reader.cpp src/DARP_benchmark_reader.h
	src/DARP_instance.cpp src/DARP_instance.h
	src/plan/VehiclePlan.tpp src/plan/VehiclePlan.h
	src/Vehicle.tpp src/Vehicle.h
	src/Action.tpp src/Action.h
	src/Request.tpp src/Request.h
	src/common.cpp src/common.h
	src/Adjustment_reason.h
	src/KD_tree.h
	src/Plan_checker.h
	src/plan/Benchmark_vehicle_plan.h
	src/progress_bar.h
	src/Linked_hash_set.h
	src/Nearest_vehicle_provider.h
	src/solver/Vehicle_plan_builder.h
	src/solver/IH/Insertion_heuristic_solver.tpp src/solver/IH/Insertion_heuristic_solver.h
	src/solver/IH/IH_vehicle_plan_builder.h
	src/solver/IH/IH_SVDARP_interfaces.h
	src/solver/IH/SVDARP.h
	src/solver/Random_solver.tpp src/solver/Random_solver.h
	src/travel_time_provider/CSV_reader.cpp src/travel_time_provider/CSV_reader.h
	src/travel_time_provider/Distance_matrix_travel_time_provider.cpp src/travel_time_provider/Distance_matrix_travel_time_provider.h
	src/travel_time_provider/Euclidean_travel_time_provider.h
	src/travel_time_provider/HDF_reader.cpp src/travel_time_provider/HDF_reader.h
	src/travel_time_provider/Travel_time_provider.h
	src/number_formatter.h
	src/logging.cpp src/logging.h
	src/inout.cpp src/inout.h
	natvis/DARP_benchmark.natvis
	test/common.h test/common.cpp
	test/IH/Insertion_heuristic_solver_test.cpp
	test/Euclidean_traveltime_provider_test.cpp
	test/Cordeau_benchmark_test.cpp
	test/Vehicle_plan_test.cpp
	test/IH/IH_SVDARP_test.cpp
	test/gtest_wrapper.h
	test/DARP_benchmark_reader_test.cpp
	test/Distance_matrix_travel_time_provider_test.cpp
	test/IH/common.h
	src/plan/Base_plan.h
	src/plan/DARP_benchmark_plan_template.h
	src/serialization.cpp
	test/IH/common.tpp
	test/Single_vehicle_DARP_common.cpp
	test/functional_tests.cpp
)

#header include
target_include_directories(DARP-benchmark PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
target_include_directories(test_run PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
target_include_directories(DARP-benchmark PRIVATE ${TCLAP_INCLUDE_DIR})
target_include_directories(DARP-benchmark PRIVATE "include/")
target_include_directories(test_run PRIVATE "include/")
target_include_directories(DARP-benchmark PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(DARP-benchmark PRIVATE ${P_RANAV_CSV2_INCLUDE_DIRS})
target_include_directories(DARP-benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fast-cpp-csv-parser)
target_include_directories(test_run PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fast-cpp-csv-parser)

# linking
target_link_libraries(test_run PRIVATE gtest_main)
target_link_libraries(DARP-benchmark PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(test_run PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(DARP-benchmark PRIVATE nanoflann::nanoflann)
target_link_libraries(test_run PRIVATE nanoflann::nanoflann)
target_link_libraries(DARP-benchmark PRIVATE indicators::indicators)
target_link_libraries(DARP-benchmark PRIVATE OpenMP::OpenMP_CXX)
# OpenMP is not active in tests, but it is necessery to link against it to prevent unknown pragma warnings in GCC.
target_link_libraries(test_run PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(DARP-benchmark PRIVATE yaml-cpp::yaml-cpp)
target_link_libraries(test_run PRIVATE yaml-cpp::yaml-cpp)
#target_link_libraries(DARP-benchmark PRIVATE hdf5::hdf5-shared hdf5::hdf5_hl-shared hdf5::hdf5_cpp-shared hdf5::hdf5_hl_cpp-shared)
target_link_libraries(DARP-benchmark PRIVATE ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
target_link_libraries(test_run PRIVATE ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
target_link_libraries(DARP-benchmark PRIVATE future-config::future-config)
target_link_libraries(test_run PRIVATE future-config::future-config)


target_compile_definitions(test_run PRIVATE IN_TEST)

# Run the builder tool that generates the config classes
run_fconfig_builder()

copy_master_config(TARGET_NAMES DARP-benchmark test_run)


include(GoogleTest)
gtest_discover_tests(test_run)

add_dependencies(test_run DARP-benchmark)

# Copy the data to build dir
add_custom_command(TARGET test_run POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:test_run>)

foreach(dir IN LISTS DARP_BENCHMARK_SOLVER_PLUGIN_DIRS)
  add_subdirectory(${dir})


endforeach()
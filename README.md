# Supported Compilers
This project uses C++ 20 features, therefore, compiler fully supporting C++ 20 is required.

# Dependencies
The following libraries are required:

- spdlog
- csv2
- nanoflann
- magic_enum
- boost-multi-index
- boost-algorithm
- indicators
- yaml-cpp
- HDF5
- future-config

All these can be installed via `vcpkg` with the following command:
```bash
vcpkg install spdlog csv2 nanoflann magic_enum boost-multi-index boost-algorithm indicators yaml-cpp HDF5[cpp] future-config
```


# Running the benchmark

The DARP benchmark program accepts the following command line arguments:

| Argument |  Description | Required | Default |
|----------|-------------|----------|---------|
| `--instance` | Path to the instance file (`.yaml` for DARP benchmark instances or classic instance files) | Yes | - |
| `--outdir` | Output directory for results | Yes | - |
| `--method` | Method for solving DARP (`ih`, `vga`, `vga_chaining`, `halns`) | Yes | - |
| `--tcount` | Number of executions for averaging | No | 1 |
| `--tmax` | Maximum number of threads for parallel regions | No | 0 (auto) |



## Usage Examples

```bash
# Basic usage with IH method
./DARP-benchmark --instance data/instances/example.yaml --outdir results/ --method ih
```

## Configuration File Support

You can also provide a local configuration file as the first positional argument (without `-` prefix). The configuration file should contain the same parameters as command line arguments in YAML format. Command line arguments override configuration file values.


# Extending the benchmark
There are two ways to extend the benchmark:

- **public extensions**: you can just develop inside the project and then create a pull request with your extension. This is the most straightforward way how to extend the benchmark.
- **private extensions**: If you need to develop in private, you can create a separate project and plug-in your private DARP solver at compile time.


## Plugging in your private DARP solver
Your private project need to be integrated in two places:
- in the `CMakeLists.txt` the project needs to be connected together
- in one of your `*.cpp` files, you need a static solver registration

### CMakeLists.txt configuration

First, you need to aquire the DARP benchmark source code. This can be automated with the `FetchContent` CMake module:

```cmake
FetchContent_Declare(
    DARP-benchmark
    GIT_REPOSITORY git@github.com:aicenter/DARP-benchmark.git
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(DARP-benchmark)
```

Then youu need to set up your target as library and add the DARP benchmark include directories to it:

```cmake
add_library(<name of your target> STATIC
	<your source files>
)
target_include_directories(<name of your target> PUBLIC
	${DARP-benchmark_SOURCE_DIR}
    <other include directories>
)
```

The integration of your target is then achieve by linking the provided `external_solvers` target to your target:

```cmake
# Plugging into DARP-benchmark
target_link_libraries(external_solvers INTERFACE <name of your target>)
target_link_options(DARP-benchmark PRIVATE
  "/WHOLEARCHIVE:$<TARGET_FILE:<name of your target>>"
)
```

The `/WHOLEARCHIVE` link option is required to ensure that DARP benchmark won't discard your target just because it is not used from the `main` function of the benchmark.


### Static solver registration
The static registration can be anywhere and has the following form:
```cpp

#include <DARP_benchmark.h>
#include "your_solver.h"

namespace DARP {
struct Registrator {
	Registrator() {
		Default_solver_registry::get()
            .register_solver<YourSolver>("your_solver");
	}
};

static Registrator registrator;

}
```

Of course, any Structure can be used and you can register multiple solvers. The important part is calling the `register_solver` function of the `Default_solver_registry` singleton from a static object, so that it is called at the very beginning of the program, before the `main` function is called.


# Implementation
## Data Types
Defined in `aliases.h`

### Times
All times are in one second resolution.

| Name       | Description | Usage |Data type |
|------------|-------------|-------|----------|
| `time` | Specific time of day in seconds, representing times between 00:00:00 and 23:59:59. | `departure time`, `arrival time`, `min_time`, `max_time`, travel times (durations) | `uint_fast32_t` |
| `travel_time` | We expect that no travel will be longer than 18 heours, so we can use a 2 byte type | TODO | `uint_fast16_t` |
| `service_duration` | Duration of the pickup and drop off actions. It is expected to be between 0 and 1h. | `service_time` | `uint_fast16_t` |

###  Other
| Name       | Description | Usage |Data type |
|------------|-------------|-------|----------|
| `vehicle_capacity` | The number of persons that can be transported in the vehicle at the same time | `capacity` | `uint_fast8_t` |
| `request_index` | Index of a requests. It should be between 0 and requests count - 1. | `request_index` | `uint_fast32_t` |
| `index_in_plan` | we expect that each plan can be 24 hours long an that the average trip is at least 2 minutes long. that means 24 * 60 *  2  maximum requests and 24 * 60 = 1440 maximum actions. For that, we need a 2 byte type | `index_in_plan` | `uint_fast16_t` |

## Tests

### Chaning
There are two types of chaining tests: 
- tests validating individual components - *component tests*
    - variant generation tests
    - network generation tests
    - MCFP solver tests
- tests validating the whole chaining process - *chaining tests*

All tests are located in the `Chaining_test.cpp` file. The data structures for component tests are also in this file, but the data stractures for chaining tests are located in `src/solver/VGA_chaining/chaining/testing.h`, in oreder to be accessible from the chaining tester executable.

Each component test type (variant generation, network generation, MCFP solver) has it's own data structures that are the minimal implementations of the component interface. 


# Methodology

## Waiting times
In DARP, we the vehicle sometimes needs to wait to satisfy the time window constraints. It is not specified in the problem definition where exactly we should wait, i.e., we can wait at the action location, or we can wait at the previous location. All methods in this project follow the following strategy:
- departure time of the vehicle/plan is the latest possible so that the vehicle can make it to the min time of the first action (plan departure = first action arrival time - travle time to first action)
- all other times are minimized to minimize the passenger delay, even if it causes more waiting for the vehicle 


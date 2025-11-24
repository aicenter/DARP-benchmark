

# Compilation

# Supported Compilers
Currently, only the MSVC compiler is supported. The reson for that is the usage of GUROBI solver, which is distributed as binary and [only supports MSVC on Windows and gcc on Linux](https://www.gurobi.com/products/gurobi-optimizer/supported-platforms/). However, [gcc still does not support the C++ 20 standard](https://en.cppreference.com/w/cpp/compiler_support), so it also cannot be used to build this project.  

# Dependencies
Following libraries are required:

* GUROBI
* RapidJSON
* TCLAP

Also make sure to add [tqdm.hpp](https://gitlab.com/miguelraggi/tqdm-cpp/-/raw/master/tqdm.hpp) to your include path.


# Running the benchmark

## Command line arguments

The DARP benchmark program accepts the following command line arguments:

### Basic Arguments

| Argument | Short | Description | Required | Default |
|----------|-------|-------------|----------|---------|
| `--instance` | `-i` | Path to the instance file (`.yaml` for DARP benchmark instances or classic instance files) | Yes | - |
| `--outdir` | `-o` | Output directory for results | Yes | - |
| `--method` | `-m` | Method for solving DARP (`ih`, `vga`, `vga_chaining`, `halns`) | Yes | - |
| `--tcount` | `-c` | Number of executions for averaging | No | 1 |
| `--tmax` | - | Maximum number of threads for parallel regions | No | 0 (auto) |

### VGA Method Parameters

| Argument | Short | Description | Default |
|----------|-------|-------------|---------|
| `--max-group` | `-g` | Maximum group size in group generation | 0 |
| `--gglimit` | - | Group generation limit | 0 |
| `--galimit` | - | Group assignment limit | 0 |

### VGA Chaining Parameters

| Argument | Short | Description | Default |
|----------|-------|-------------|---------|
| `--batchl` | `-b` | Batch length in seconds | 300 |
| `--tts` | - | Time estimate from current vehicle position to first action | 120 |
| `--mtbp` | - | Maximum time between plans considered for chaining | 0 |
| `--cmg` | - | Maximum gap for VGA Chaining ILP solver | 0.005 |
| `--cbs` | - | Maximum number of plans to be chained in one level of hierarchical chaining | 0 |
| `--mcc` | - | Maximum cost of the chaining connection (currently implemented only for vehicles) | 0 |
| `--max_delay` | - | Maximum delay parameter | -1 |
| `--skip-vga-plan-export` | - | Skip VGA plan export (boolean flag) | false |

### HALNS Method Parameters

| Argument | Description | Default |
|----------|-------------|---------|
| `--iter` | Number of iterations | 100000 |
| `--init` | Path to initial solution file | - |
| `--time_limit` | Time limit in milliseconds | 0 |

### Usage Examples

```bash
# Basic usage with IH method
./DARP-benchmark --instance data/instances/example.yaml --outdir results/ --method ih

# VGA method with custom parameters
./DARP-benchmark --instance data/instances/example.yaml --outdir results/ --method vga --max-group 4 --tcount 5

# VGA Chaining with batch processing
./DARP-benchmark --instance data/instances/example.yaml --outdir results/ --method vga_chaining --batchl 300 --tts 120

# HALNS with custom iterations
./DARP-benchmark --instance data/instances/example.yaml --outdir results/ --method halns --iter 50000 --init initial_solution.json

# Using a local config file (first positional argument)
./DARP-benchmark config.yaml --instance data/instances/example.yaml --outdir results/ --method ih
```

### Configuration File Support

You can also provide a local configuration file as the first positional argument (without `-` prefix). The configuration file should contain the same parameters as command line arguments in YAML format. Command line arguments override configuration file values.


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


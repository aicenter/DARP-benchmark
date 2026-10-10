<!--
MIT License

Copyright (c) 2026 Czech Technical University in Prague

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.-->
# Instances
In the context of DARP benchmark we call the input unrelated to the specific solution method as *DARP instance* or just *instance*. The instance parameter is formalized in many scientific articles, in context of this manual, we use the formalization used in TODO, which is specified below. 

In addition to the theoretical formalization, there are real data used for the simulation which need to be described. Currently, there are two formats supporded:
- the classic format from Cordeau and Laporte
- the new format specifically designed for the DARP benchmark


## Instance configuration according to formalization
Each DARP instance have the following parameters:
- **max route duration** defining the max length \[s\] for any vehicle plan (plan departure time minus plan arrival time). The default value is 0 (unlimited)
- **max ride time** defining the max length \[s\] a travel request can be serviced (request drop off time minus rquest pickup time). The default value is 0 (unlimited)
- **return to depot** which indicates whether each vehicle has to return to its initial position after serving all requests in the plan. The default value is true.
- **use virtual vehicles** indicates whether the instance uses virtual vehicles, which can arrive at any position in an equal, constant time, or real vehicles. The default value is false.
- **start time** The start time \[s\] of the instance. At this time, the vehicles can depart from theirs initial positions. The default value is 0.



## Classic format
The classicla format uses just a single instance file containing all necessary data. The instance works on Euclidean space, and the travel cost between any two points is the Euclidean distance between them. The specification of the instance file is described in [a google doc](https://docs.google.com/document/d/1twvfUdKq4savEWePCOM8vlHdmIRG_4APw-jSgLTdNio/edit#heading=h.suydp11kwfz3).


## DARP Benchmark format
Each DARP instance consists of the following files:
- **Instance configuration** (`config.yaml`) that defines the parameters of the instance
- **demand data** (`trips.di`) consisting of an entry for each travel request
Optionaly, the instance can also contain:
- **vehicle data** (`vehicles.csv`) defining the individual configuration for each vehicle.
- **Travel times** (`dm.h5`): distance matrix that determines the travel times between each pair of road graph nodes.

There are usually many other files generated during the process, but there are not needed by the DARP benchmark. 

Because the distance matrix file is very large, it is not generated for each instance, but it is shared between instances in the same area. All other map files are also shared between instances in the same area. The resulting structure is following:

**Area Folder**: 

- `experiments`: Folder for the experiments. The hierarchy can be arbitrary.
- `map`
    - `nodes.csv`: map nodes, used for trip generation and dm generation
    - `edges.csv`
    - `shapefiles`
        - `edges.shp`
        - `nodes.shp`
    - `map.xeng`
- `dm.h5`

**Experiment Folder:**
- `trips.di` demand instance file
- `vehicles.csv` vehicle configuration file
- `config.yaml` instance configuration file


### Instance configuration file
Each instance is determined by a configuration file in the YAML format. This file has two purposes
- it is used to configure the generation of the instance
- it is used to configure the DARP-benchmark program when the optimization is run

The parameters in the file can be used to configure either the generation of the instance or the optimization or even both.

The structure is following:
- `area_dir`: directory for the map files. It can be shared between multiple instances in the same area. If the path is relative, the instance is portable. Example: `../../../`
- `area_id`: id of the area used for instance both map and demand will be filtered by this area
- `demand`: object for the demand configuration
    - `dataset`: array of the demand dataset IDs (table `dataset in the database). Example:
        ```YAML
        - 2
        - 3
        - 4
        - 5
        ```
    - `filepath`: The pth to the instance file containing the trips. If the path is relative, the instance is portable. Example: `./trips.di`
    - `min_time`: The minimum datetime for demand origin. Example: `'2022-03-11 18:00:00'`
    - `max_time`: The maximum datetime for demand origin. Example: `'2022-03-11 18:05:00'`
    - `mode`: `load` for using the real demand from the database, otherwise, the demand will be generated. Currently, only the `load` mode is supported
    - `positions_set`: the id of the demand positions set (table `demand_positions_sets` in the database). Example: `1`
- `map`: the objecct for the map configuration
    - `SRID`: The SRID of the map projection. Example: `4326`
    - `SRID_plane`: The SRID of the map planar projection. Example: `32618`
- `cost`: weights of the generalized cost model, see [Cost model](#cost-model) below
- `demand.relative_delay_cost`: legacy default of `cost.passenger_delay_weight`
- `max_delay`: the maximum extra time a request spends over the direct transportation from origin to destination, in seconds, including the waiting times. Either `mode: absolute` with `seconds`, or `mode: relative` with `relative` (a proportion of the minimal travel time). `max_prolongation` (plain seconds) and `max_travel_time_delay` are deprecated aliases. Example: `300`
- `problem`: `DARP` (default) or `fleet-sizing` (vehicles are not loaded, the solver creates them)
- `vehicles.capital_cost`: legacy default of `cost.vehicle_capital_cost`
- `save_shp`: if `true` then the shapefile will be saved for map, demand, and vehicles
- `vehicles`: configuration object for vehicles
    - `start_time`: Each vehicle will be available since this datetime. Example: `'2022-03-11 17:45:00'`
    - `vehicle_capacity`: The capacity of the vehicle. Example: `4`
    `vehicle_to_request_ratio`: The ration between vehicles and requests. Example: `0.5` (meaning 1 vehicle per 2 requests).


### Cost model
The solution cost is the weighted sum of the cost components of its plans, with the weights taken from the `cost` section of the instance configuration (the format is defined by the [DARP instances project](https://github.com/aicenter/Ridesharing_DARP_instances), section "Generalized cost model"). The benchmark evaluates the cost of every plan of the final solution itself (`Cost_evaluator`, `src/cost/Cost_evaluator.h`), so the reported cost does not depend on the solver. The components evaluated by the benchmark are:

| Key | Quantity | Default | Legacy default |
|-----|----------|---------|----------------|
| `travel_time_weight` | vehicle travel time [s]: the legs between the vehicle start, the actions and, when the instance requires the return to the depot, the depot; a leg counts the longer of the matrix travel time and the scheduled time between the departure and the arrival | 1.0 | |
| `passenger_delay_weight` | passenger delay [s]: drop-off arrival minus the arrival of the direct ride started at the desired pickup time, summed over drop-offs | 0.0 | `demand.relative_delay_cost` |
| `vehicle_capital_cost` | constant charged to every non-empty plan | 0.0 | `vehicles.capital_cost` |

The other weights of the instance format (`distance_weight`, `ride_time_weight`, `earliness_weight`, `plan_duration_weight`, `fixed_plan_cost`) are accepted only when zero, and `accounting` only as `per_traveller`; any other value makes the instance loading fail. Unknown keys in the `cost` section are an error.

The weighted cost is a floating-point number; the plan and solution `cost` fields of the solution file are numbers, `cost_minutes` stays an integer. The VGA and rolling horizon VGA solvers optimize the weighted cost; the other methods optimize the travel time and only their reported cost is weighted (the benchmark logs a warning in that case).

### Demand Instance File
Each line has the following structure:
```
<index> <time> <origin index> <destination index>
```
The time in milliseconds from midnight.

### Vehicle File
The vehicle file is a CSV file with the following structure:
```
<start index> <capactity>
```

### Distance Matrix File
The distance matrix file is a HDF5 file containing the travel times in seconds stored in the 16bit integer format. 

#### Travel Time Computation
The travel time computation process has two steps:
- determine the speed on each edge
- compute the travel time on each edge and store it in the distance matrix

There are two variants of the speed computation:
- the speed is computed from the historical data (this is use in the experiments)
- Flat speed of 14 meters per second (about 50km/h).


## Overview Table
| Parameter | Descripton | Implementation prop. name | DARP Benchmark format | Classic format  | 
|-----------|------------|----------------|-----------------------|----------------|
| travel requests | The set of travel requests (origin, destination, min time) | `requests` |`trips.di` file, each row is a request | instance file, each row is a request |
| vehicles | The set of vehicles (start position, capacity) | `vehicles` | `vehicles.csv` file, each row is a vehicle | TODO |
| max delay | The maximum extra time a request spend over the direct transportation from origin to destination, in seconds. | `request.drop_off.max_time` | `max_prolongation` in `config.yaml` | each request has a specific max delay, it is defined in the request row in the instance file |
| max ride time | The maximum ride time of a single request | `darp_insrtance_configuration.max_ride_time` | not present, set to 0 (unconstrained) | TODO |
| max route time | The maximum route time of a single vehicle | `darp_insrtance_configuration.max_route_time` | not present, set to 0 (unconstrained) | TODO |
| return to depot | Whether the vehicle has to return to depot | `darp_insrtance_configuration.return_to_depot` | not present, set to `false` | not present, set to `true` |
| start time | The start time for vehicles. At this time, vehicles can start their trips. | `darp_insrtance_configuration.start_time` | `vehicles.start_time` in `config.yaml` | not present set to 0 |



# Experiments and Results
The result/experiment folder contains both the configuration of the experiment and the results. There should be the following files:
- `config.yaml`: the configuration of the experiment
- `*-solution.json`: the solution file
- `*-performance.json`: the performance file

The properties in the configuration file are identical to the DARP benchmark program command line arguments (long form). 

Note that as of now, the experiment configuration file is not used by the DARP benchmark program. Instead, the configuration is passed as command line arguments. If you want to use the experiment configuration file, you can use the python scripts in the `python/scripts` folder which parse the experiment configuration file and call the DARP benchmark program with the correct arguments.


# Program Arguments

| Parameter | Descripton | Exec. parameter | 
|-----------|------------|--------------------|
| instance path | path to the `config.yaml` file (DARP bench. instance) or  to the instance file (classic instance) |  `-i`, `--instance` | 
| output path | | `-o`, `--outdir` | 
| type | Type of the DARP instance (amodsim or cordeau) | `-t`, `--instype` |
| method | method for solving DARP | `-m`, `--method` |
| execution count | number of executions (for averaging) | `-c`, `tcount` |
| distance matrix path | full path to the distance matrix. Apllicable onlz to the ridesharing instances | `-d`, `--dm` |
| max number of threads | maximum number of threads to be used in paralel regions| `--tmax` |


## VGA parameters
| Parameter | Descripton | Exec. parameter |
|-----------|------------|--------------------|
| max group size | Max size of group in group generation part of the VGA method | `-g`, `--max-group` |



## VGA chaining parameters
| Parameter | Descripton | Exec. parameter |
|-----------|------------|--------------------|
| batch length | Batch length in seconds | `-b`, `--batchl` |
| time to start | A time estimate from the current position of a vehicle to the first action.* | `--tts` | 
| max time between plans | max time between any two plans considered for chaining | `--mtbp` |
| chaining max gap | Max gap for VGA Chaining ILP solver | `--cmg` |
| chaining solver max batch | Max number of plans to be chaind in one level of the hierarchical chaining | `--cbs` |
| max chaining cost | Maximum cost of the chainining connection. Currently implemented onlz for vehicles. | `--mcc` |



### *Time to start parameter explained
It is used in the VGA method, as the method cannot know the possitions of vehicles at the beginning of each batch (with exception of the first batch).This cost estimation is used in two components:

- SVDARP solver, there it adds a fixed cost to each plan. In conclusion, longer plans are prefered later in the vehicle-group assignment
- chaining: here, the tts is used as the cost of the shortuct edge, while the shortut edge represents using an extra vehicle


#include <cmath>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <stdexcept>
#include <functional>
#include <rapidjson/istreamwrapper.h>
#include "inout.h"
#include "rapidjson/prettywriter.h"
#include "Vehicle.h"
#include "plan/VehiclePlan.h"
#include <spdlog/spdlog.h>



template <typename N, Benchmark_plan P>
Solution_iterator_wrapper<N> Solution_iterator_adapter<N, P>::get_copyable_wrapper() {
    return Solution_iterator_wrapper<N>(this);
}

template<typename N>
rapidjson::StringBuffer  Solution_interface<N>::JSON_serialize(unsigned short resolution) const {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter writer(s);
	writer.StartObject();
	writer.Key("feasible");
	writer.Bool(feasible);
	if(feasible){
		writer.Key("cost");
		writer.Uint64(cost);
		writer.Key("cost_minutes");
		writer.Uint64(static_cast<unsigned long>(std::round(static_cast<double>(cost) / resolution)));
		writer.Key("total_passenger_delay");
		writer.Uint64(this->get_total_passenger_delay());
		writer.Key("total_passenger_ride_time");
		writer.Uint64(this->get_total_ride_time());
		writer.Key("total_driving_time");
		writer.Uint64(this->get_total_driving_time());
		writer.Key("plan_count");
		writer.Uint64(this->get_plan_count());
		writer.Key("plans");
		writer.StartArray();
		auto f = std::bind(&Benchmark_vehicle_plan::JSON_serialize, std::placeholders::_1, std::ref(writer));
		std::for_each(this->begin()->get_copyable_wrapper(), this->end()->get_copyable_wrapper(), f);
		writer.EndArray();
		writer.Key("dropped_requests");
		writer.StartArray();
		for (const Request<N>* request : dropped_requests) {
			request->JSON_serialize(writer);
		}
		writer.EndArray();
	}
    writer.EndObject();

    return s;
}

template <typename N>
std::string Solution_interface<N>::export_simple_csv() const {
	std::ostringstream csv;
	csv << "plan,request,pickup_time,dropoff_time\n";
	if (!feasible) {
		return csv.str();
	}
	int plan_index = 0;
	auto it = this->begin();
	auto it_end = this->end();
	auto it_wrap = it->get_copyable_wrapper();
	auto it_end_wrap = it_end->get_copyable_wrapper();
	while (it_wrap != it_end_wrap) {
		const Benchmark_vehicle_plan& plan_ref = *it_wrap;
		plan_ref.append_simple_csv_rows(plan_index, csv);
		++it_wrap;
		++plan_index;
	}
	return csv.str();
}

template <typename N>
Solution_interface<N>::Solution_interface(
	const unsigned long cost,
	std::vector<const Request<N>*>&& dropped_requests,
	bool feasible
):
	cost(cost),
	dropped_requests(dropped_requests),
	feasible(feasible)
{
	assert(check());
}


template<typename N>
unsigned int Solution_interface<N>::get_dropped_request_count() const {
    return static_cast<unsigned>(this->dropped_requests.size());
}


template<typename N, Benchmark_plan P>
Solution<N,P>::Solution(
    std::vector<P>&& vehicle_plans, 
    unsigned long cost, 
    std::vector<const Request<N>*>&& dropped_requests
):
	Solution_interface<N>(cost, std::move(dropped_requests), true),
    plans{vehicle_plans}
{
	assert(check());	
}

template<typename N, Benchmark_plan P>
Solution<N,P>::Solution(
    std::vector<P>&& vehicle_plans,
    unsigned long cost,
    std::vector<const Request<N>*>&& dropped_requests,
    bool is_feasible
):
	Solution_interface<N>(cost, std::move(dropped_requests), is_feasible),
	plans{ vehicle_plans }
{
	assert(check());
}

template<typename N, Benchmark_plan P>
Solution<N,P>::Solution(
	std::vector<P>&& vehicle_plans,
	unsigned long cost,
	std::vector<const Request<N>*>&& dropped_requests,
	std::optional<Virtual_vehicle>&& virtual_vehicle_backing_par,
	std::optional<std::vector<Vehicle<N>>>&& fleet_sizing_vehicle_backing_par
):
	Solution_interface<N>(cost, std::move(dropped_requests), true),
	fleet_sizing_vehicle_backing{std::move(fleet_sizing_vehicle_backing_par)},
	virtual_vehicle_backing{std::move(virtual_vehicle_backing_par)},
	plans{std::move(vehicle_plans)}
{
	assert(check());
}

template<typename N, Benchmark_plan P>
Solution<N,P>::Solution(
	std::vector<P>&& vehicle_plans,
	unsigned long cost,
	std::vector<const Request<N>*>&& dropped_requests,
	std::optional<Virtual_vehicle>&& virtual_vehicle_backing_par
):
	Solution(
		std::move(vehicle_plans),
		cost,
		std::move(dropped_requests),
		std::move(virtual_vehicle_backing_par),
		std::nullopt)
{
}

template<typename N, Benchmark_plan P>
Solution<N,P>::Solution(const Solution<N,P>& other):
	Solution_interface<N>(other),
	fleet_sizing_vehicle_backing(other.fleet_sizing_vehicle_backing),
	virtual_vehicle_backing(other.virtual_vehicle_backing),
	plans()
{
	plans.reserve(other.plans.size());
	for (const P& p : other.plans) {
		P plan_copy = p;
		if (dynamic_cast<const Virtual_vehicle*>(&p.get_vehicle()) != nullptr) {
			if (!virtual_vehicle_backing.has_value()) {
				throw std::runtime_error("Solution copy: virtual plan without virtual_vehicle_backing");
			}
			const Virtual_vehicle& vv = *virtual_vehicle_backing;
			if constexpr (requires {
				std::declval<P&>().set_vehicle(
					std::cref(static_cast<const Vehicle_base&>(std::declval<const Virtual_vehicle&>())));
			}) {
				plan_copy.set_vehicle(std::cref(static_cast<const Vehicle_base&>(vv)));
			} else {
				throw std::runtime_error(
					"Solution copy: virtual vehicle in plan is incompatible with this plan type (use Vehicle_base plans for fleet-sizing JSON)"
				);
			}
		} else {
			const auto* vn = dynamic_cast<const Vehicle<N>*>(&p.get_vehicle());
			if (vn != nullptr && other.fleet_sizing_vehicle_backing.has_value()) {
				for (size_t i = 0; i < other.fleet_sizing_vehicle_backing->size(); ++i) {
					if (vn == &(*other.fleet_sizing_vehicle_backing)[i]) {
						if constexpr (requires {
							std::declval<P&>().set_vehicle(
								std::cref(static_cast<const Vehicle_base&>(std::declval<const Vehicle<N>&>())));
						}) {
							plan_copy.set_vehicle(
								std::cref(static_cast<const Vehicle_base&>((*fleet_sizing_vehicle_backing)[i])));
						} else {
							throw std::runtime_error(
								"Solution copy: fleet-sized materialized vehicle requires Vehicle_base plans");
						}
						break;
					}
				}
			}
		}
		plans.push_back(std::move(plan_copy));
	}
	assert(check());
}

template<typename N, Benchmark_plan P>
Solution<N,P>& Solution<N,P>::operator=(const Solution<N,P>& other) {
	if (this == &other) {
		return *this;
	}
	Solution<N,P> tmp(other);
	*this = std::move(tmp);
	return *this;
}

template<typename N, Benchmark_plan P>
Solution<N,P>::Solution(
    const DARP_instance<N>& instance, 
    std::vector<P>&& vehicle_plans
):
	Solution_interface<N>(0, std::vector<const Request<N>*>{}, true),
    plans{ vehicle_plans }
{

    std::unordered_set<const Request<N>*> served_requests;
	
	
    for(const P& plan : vehicle_plans) {
        this->cost += plan.get_cost();

        for (const ActionData<N>& action_data : plan) {
    		if(action_data.get_action_type() != Action_type::depot){
	            const Request<N>& request = dynamic_cast<const Service_action<N>&>(action_data.get_action()).get_request();
    			if(!served_requests.contains(&request)) {
	                served_requests.insert(&request);
    			}
            }
    	}
    }

    for (const Request<N>& request : instance.get_requests()) {
        if (!served_requests.contains(&request)) {
            this->dropped_requests.push_back(&request);
        }
    }

    assert(check());
}

template <typename N>
bool Solution_interface<N>::check() {
	// duplicity check
	std::unordered_set<request_index_type> dropped_requests_check;
	for(auto req: dropped_requests) {
		assert(!dropped_requests_check.contains(req->get_index()));
		dropped_requests_check.insert(req->get_index());
	}

	return true;
}

template<typename N, Benchmark_plan P>
const std::vector<P>& Solution<N,P>::get_plans() const {
    return plans;
}

template<typename N, Benchmark_plan P>
unsigned int Solution<N,P>::get_cost() const {
    return this->cost;
}

template<typename N, Benchmark_plan P>
unsigned long Solution<N,P>::get_total_passenger_delay() const {
	unsigned long total = 0;
	for (const P& plan : plans) {
		total += plan.get_passenger_delay();
	}
	return total;
}

template<typename N, Benchmark_plan P>
unsigned long Solution<N,P>::get_total_ride_time() const {
	unsigned long total = 0;
	for (const P& plan : plans) {
		total += plan.get_ride_time();
	}
	return total;
}

template<typename N, Benchmark_plan P>
unsigned long Solution<N,P>::get_total_driving_time() const {
	unsigned long total = 0;
	for (const P& plan : plans) {
		total += plan.get_driving_time();
	}
	return total;
}

template<typename N, Benchmark_plan P>
unsigned long Solution<N,P>::get_plan_count() const {
	return static_cast<unsigned long>(plans.size());
}

template<typename N, Benchmark_plan P>
unsigned int Solution<N,P>::get_non_empty_plan_count() {

    if (non_empty_plan_count < 0) {
        for (P plan: plans) {
            if (plan.get_length() > 0) {
                non_empty_plan_count++;
            }
        }
    }

    return non_empty_plan_count;
}

template<typename N, Benchmark_plan P>
const P& Solution<N,P>::operator[](int index) const{
    return plans[index];
}

template<typename N, Benchmark_plan P>
bool Solution<N, P>::is_fleet_sizing_vehicle_backing_engaged() const noexcept {
	return fleet_sizing_vehicle_backing.has_value();
}

template <typename N, Benchmark_plan P>
bool Solution<N, P>::check() {
    Solution_interface<N>::check();


	std::unordered_set<request_index_type> served_requests;
    unsigned cost_sum = 0;
	for(const auto& plan: plans) {
        // check for request in multiple plans
		for(const auto& action: plan) {
			if(action.get_action_type() == Action_type::dropoff) {
				request_index_type request_index = action.get_request_index();
				assert(!served_requests.contains(request_index));
				served_requests.insert(request_index);
			}
		}

        cost_sum += plan.get_cost();
	}

    // check that the solution cost matches the sum of the plan costs
    assert(cost_sum == this->cost);

	return true;
}

template <typename N, Benchmark_plan P>
std::unique_ptr<Solution_iterator_interface<N>> Solution<N, P>::begin() const {
    return std::unique_ptr<Solution_iterator_interface<N>>(
        new Solution_iterator_adapter<N,P>(plans.begin()));
}

template <typename N, Benchmark_plan P>
std::unique_ptr<Solution_iterator_interface<N>> Solution<N, P>::end() const {
    return std::unique_ptr<Solution_iterator_interface<N>>(
        new Solution_iterator_adapter<N,P>(plans.end()));
}

template <typename N, Benchmark_plan P>
Solution<N, P> deserialize_json(std::filesystem::path path, const DARP_instance<N>& darp_instance) {
	spdlog::info("Loading computed VGA plans from: {}", path.string());

	auto ifs = get_file_stream(path.string());
	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document d;
	d.ParseStream(isw);

	std::unordered_map<unsigned, const Request<N>&> requests_mapped_by_id;
	for (const auto& request : darp_instance.get_requests()) {
		requests_mapped_by_id.emplace(request.get_index(), request);
	}

	std::optional<Virtual_vehicle> shared_virtual_vehicle;
	std::vector<Vehicle<N>> fleet_materialized_vehicles;
	std::vector<P> plans;
	const auto& plans_json = d["plans"];
	if (!plans_json.IsArray()) {
		throw std::runtime_error("deserialize_json: expected plans array");
	}
	deserialize_vehicle_plans_from_json_array<N, P>(
		plans_json, darp_instance, shared_virtual_vehicle, fleet_materialized_vehicles, plans);

	std::optional<std::vector<Vehicle<N>>> fleet_sizing_backing;
	if (darp_instance.get_problem() == problem_type::fleet_sizing) {
		fleet_sizing_backing.emplace(std::move(fleet_materialized_vehicles));
	} else if (!fleet_materialized_vehicles.empty()) {
		throw std::runtime_error(
			"deserialize_json: solution JSON materialized vehicles not in the instance fleet, but the instance is not "
			"problem_type::fleet_sizing");
	}

	std::vector<const Request<N>*> dropped_requests;
	for (const auto& request_data : d["dropped_requests"].GetArray()) {
		dropped_requests.emplace_back(&requests_mapped_by_id.at(request_data["index"].GetUint()));
	}
	return Solution<N, P>{
		std::move(plans),
		d["cost"].GetUint(),
		std::move(dropped_requests),
		std::move(shared_virtual_vehicle),
		std::move(fleet_sizing_backing)
	};
}

template <typename N, Benchmark_plan P>
const Benchmark_vehicle_plan& Solution_iterator_adapter<N, P>::dereference() const {
	return *this->base_reference();
}



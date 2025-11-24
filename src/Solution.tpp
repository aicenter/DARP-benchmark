
#include <cmath>
#include <unordered_set>
#include <algorithm>
#include <rapidjson/istreamwrapper.h>
#include "inout.h"
#include "rapidjson/prettywriter.h"
#include <spdlog/spdlog.h>

#include "Solution.h"


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
    std::vector<const Request<N>*>&& dropped_requests,
    std::shared_ptr<std::vector<Vehicle<N>>> virtual_vehicles
):
	Solution_interface<N>(cost, std::move(dropped_requests), true),
    plans{vehicle_plans}, 
	virtual_vehicles(std::move(virtual_vehicles)
) {
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
    const DARP_instance<N>& instance, 
    std::vector<P>&& vehicle_plans,
    std::shared_ptr<std::vector<Vehicle<N>>>&& virtual_vehicles
):
	Solution_interface<N>(0, std::vector<const Request<N>*>{}, true),
    plans{ vehicle_plans },
    virtual_vehicles{ std::move(virtual_vehicles) }
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

template <typename N, class P>
Solution<N, P> deserialize_json(std::filesystem::path path, const DARP_instance<N>& darp_instance) {
    spdlog::info("Loading computed VGA plans from: {}", path.string());

    auto ifs = get_file_stream(path.string());
	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document d;
	d.ParseStream(isw);

    // create map of actions mapped by their id and requests mapped by index
    std::unordered_map<unsigned, const Request<N>&> requests_mapped_by_id;
	std::unordered_map<unsigned, const Action<N>&> actions_mapped_by_id;
	for(const auto& request: darp_instance.get_requests()) {
        requests_mapped_by_id.emplace(request.get_index(), request);

		const auto& pickup = request.get_pickup();
		const auto& drop_off = request.get_dropoff();
		
		actions_mapped_by_id.insert(std::pair<unsigned, const Action<N>&>(pickup.get_action_id(), pickup));
		actions_mapped_by_id.insert(std::pair<unsigned, const Action<N>&>(drop_off.get_action_id(), drop_off));
	}

    // create map of vehicles mapped by vehicle index
    std::unordered_map<unsigned, const Vehicle<N>&> vehicles_mapped_by_index;
    for(const auto& vehicle: darp_instance.get_vehicles()) {
        vehicles_mapped_by_index.insert({vehicle.get_index(), vehicle});
    }

    std::vector<P> plans;

    for(const auto& plan_data: d["plans"].GetArray()) {
        // vehicle
        const auto& veh_json = plan_data["vehicle"];
    	const auto& vehicle_index = veh_json["index"].GetUint();
        const auto& vehicle = vehicles_mapped_by_index.at(vehicle_index);

        auto json_actions = plan_data["actions"].GetArray();
		std::vector<ActionData<N>> actions;
		std::unordered_map<unsigned,index_in_plan> pickups;
		for(index_in_plan i = 0; i < static_cast<index_in_plan>(json_actions.Size()); ++i) {
			const auto& json_action_data = json_actions[i];
			const auto& json_action = json_action_data["action"];
			auto action_type = action_type_from_string(json_action["type"].GetString());
			auto request_index = json_action["request_index"].GetUint();
			index_in_plan other_index = 0;
			if(action_type == Action_type::dropoff) {
				other_index = pickups[request_index];
				actions[other_index].set_other_action_data_index(static_cast<index_in_plan>(i));
			}
			else {
				pickups[request_index] = i;
			}
			const auto& action = actions_mapped_by_id.at(json_action["id"].GetUint());
            actions.emplace_back(json_action_data, i, other_index, action);
        }
        
        plans.emplace_back(
            vehicle,
			plan_data["cost"].GetUint(),
			std::move(actions),
            plan_data["departure_time"].GetUint(),
            plan_data["arrival_time"].GetUint()
        );
    }

    // load dropped requests
    std::vector<const Request<N>*> dropped_requests;
    for(const auto& request_data: d["dropped_requests"].GetArray()) {
        dropped_requests.emplace_back(&requests_mapped_by_id.at(request_data["index"].GetUint()));
    }
    return {
		std::move(plans),
        d["cost"].GetUint(),
        std::move(dropped_requests),
        true
    };
}

template <typename N, Benchmark_plan P>
const Benchmark_vehicle_plan& Solution_iterator_adapter<N, P>::dereference() const {
	return *this->base_reference();
}



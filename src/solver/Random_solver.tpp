#include "Random_solver.h"

#include <algorithm>

template<typename N>
Solution<N> Random_solver<N>::solve() {
    std::random_device device;
    std::mt19937 generator(device());
    this->rand_g = generator;

    return compute(this->darp_instance.get_requests(), this->darp_instance.get_vehicles());
}

template<typename N>
template<class R, class V>
Solution<N> Random_solver<N>::compute(const R &requests, const V &vehicles) {
    this->solution_cost = 0;

    // we start with empty plans for all vehicles
    this->vehicle_plans = std::vector<VehiclePlan<N>>{};
    this->vehicle_plans.reserve(vehicles.size());

    for (const Vehicle<N> &vehicle: vehicles) {
        this->vehicle_plans.emplace_back(VehiclePlan{vehicle, 0});
    }

    for (const Request<N>& request: requests) {
        this->process_request(request, vehicles);
    }

    unsigned long cost = 0;
    for (auto plan : this->vehicle_plans)
        cost += plan.get_cost();

    return Solution<N>{std::move(vehicle_plans), cost, std::move(dropped_requests)};
}

template<typename N>
std::vector<unsigned int> Random_solver<N>::shuffle_vehicles(const std::vector<Vehicle<N>> &vehicles) {
    std::vector<unsigned int> vehicles_indices;

    for (const Vehicle<N>& vehicle : vehicles)
        vehicles_indices.emplace_back(vehicle.get_index());

    std::shuffle(vehicles_indices.begin(), vehicles_indices.end(), this->rand_g);

    //not really that random
    //std::random_shuffle(vehicles_indices.begin(), vehicles_indices.end());

    return vehicles_indices;
}

template<typename N>
void Random_solver<N>::process_request(const Request<N> &request, const std::vector<Vehicle<N>> &vehicles) {
    std::vector<unsigned int> rnd_list = this->shuffle_vehicles(vehicles);

    /*for (auto ind : rnd_list)
        std::cout << ind << " ";
    std::cout << "\n";*/

    for (unsigned int vehicle_index : rnd_list) {
        const Vehicle<N>& vehicle = this->find_vehicle_by_index(vehicle_index, vehicles);

        if (this->can_serve_request(vehicle, request) && vehicle.get_capacity() > 0) {
            VehiclePlan<N> &vp = this->find_plan_by_vehicle(vehicle);
            if (compute_optimal_plan(vehicle, request, vp))
                return;
        }
    }

    dropped_requests.push_back(&request);
    std::cout << "Cannot process request " << request.get_index() << "\n";
}

template<typename N>
bool Random_solver<N>::can_serve_request(const Vehicle<N> &vehicle, const Request<N> &request) {

    // node identity
    if(&vehicle.get_init_position() == &request.get_pickup().get_node()){
        return true;
    }

    // pickup feasibility check
    bool can_serve = this->travel_time_provider.get()->get_travel_time(
	    vehicle.get_init_position(), request.get_pickup().get_node()) < request.get_pickup().get_max_time();

    if(can_serve){
        // drop off feasibility check
        return this->travel_time_provider.get()->get_travel_time(
		        vehicle.get_init_position(), request.get_dropoff().get_node()) + request.get_pickup().get_service_duration()
               < request.get_dropoff().get_max_time();
    }
    return false;
}

template<typename N>
VehiclePlan<N>& Random_solver<N>::find_plan_by_vehicle(const Vehicle<N> &v1) {
    for (VehiclePlan<N> &v2 : this->vehicle_plans)
        if (v1.get_index() == v2.get_vehicle().get_index())
            return v2;
    throw "Couldn't find given vehicle!\n";
}

template <typename N>
const Vehicle<N>& Random_solver<N>::find_vehicle_by_index(unsigned int index, const std::vector<Vehicle<N>> &vehicles) {
    for (const Vehicle<N> &v : vehicles)
        if (v.get_index() == index)
            return v;
    throw "Couldn't find given vehicle!\n";
}


template <typename N>
size_t Random_solver<N>::find_plan_index(const VehiclePlan<N> &vp) {
    size_t i = 0;
    for (VehiclePlan<N> &v2 : this->vehicle_plans) {
        if (vp.get_vehicle().get_index() == v2.get_vehicle().get_index())
            return i;
        i++;
    }
    throw "Couldn't find given vehicleplan!\n";
}

template <typename N>
bool Random_solver<N>::compute_optimal_plan(const Vehicle<N> &vehicle, const Request<N> &request, VehiclePlan<N> &current_plan) {
    unsigned short free_capacity = current_plan.get_vehicle().get_capacity();
    size_t plan_index = this->find_plan_index(current_plan);

    this->best_plan = std::nullopt;

    for (unsigned short pickup_option_index = 0; pickup_option_index <= current_plan.get_length(); pickup_option_index++) {
        //bool updated = false;
        // continue if the vehicle is full
        if (free_capacity > 0) {
            for (unsigned short dropoff_option_index = pickup_option_index + 1; dropoff_option_index <= current_plan.get_length() + 1;
                dropoff_option_index++){
                std::optional<VehiclePlan<N>> potential_plan = insert_into_plan(current_plan, pickup_option_index, dropoff_option_index,
                                                                                vehicle, request);

                if (potential_plan.has_value()) {
                    unsigned int cost_increment = potential_plan->get_cost() - current_plan.get_cost();
                    try_update_best_plan(potential_plan.value(), cost_increment);
                    //std::cout << "Found plan, increment " << cost_increment << "\n";
                }

            }
        }

        // change free capacity for next index
        if(pickup_option_index < current_plan.get_length()){
            if(current_plan[pickup_option_index].get_action().get_action_type() == Action_type::pickup){
                free_capacity--;
            }
            else {
                free_capacity++;
            }
        }
    }

    if (this->best_plan.has_value()) {
        this->vehicle_plans[plan_index] = this->best_plan.value();
        return true;
    }

    //std::cout << "Cannot satisfy conditions\n";
    return false;
}

template<typename N>
std::optional<VehiclePlan<N>> Random_solver<N>::insert_into_plan(const VehiclePlan<N> &current_plan,
unsigned short pickup_option_index, unsigned short dropoff_option_index, const Vehicle<N>& vehicle, const Request<N> &request)
{
    std::vector<ActionData<N>> new_plan_tasks;
    new_plan_tasks.reserve(current_plan.get_length() + 2);

    // travel time of the new plan in seconds
    unsigned int new_plan_travel_time = 0;
    unsigned int new_plan_cost = 0;

    // induced delay of the new plan in seconds
    int new_plan_delay = 0;

    // index of the lastly added action from the old plan
    unsigned short index_in_current_plan = 0;
    unsigned short free_capacity = vehicle.get_capacity();

    std::unordered_map<unsigned int, ActionData<N>*> pickup_map;
    pickup_map.reserve(current_plan.get_length() + 2);

    for (int new_plan_index = 0; new_plan_index < current_plan.get_length() + 2; new_plan_index++) {
        //placeholder
        new_plan_tasks.emplace_back(ActionData<N>{request.get_dropoff()});

        if(new_plan_index == pickup_option_index)
            new_plan_tasks[new_plan_index] = ActionData<N>{request.get_pickup()};
        else if (new_plan_index == dropoff_option_index)
            new_plan_tasks[new_plan_index] = ActionData<N>{request.get_dropoff()};
        else
        {
            new_plan_tasks[new_plan_index] = ActionData<N>{current_plan[index_in_current_plan].get_action()};
            new_plan_tasks[new_plan_index].set_arrival_time(current_plan[index_in_current_plan].get_arrival_time());
            new_plan_tasks[new_plan_index].set_departure_time(current_plan[index_in_current_plan].get_departure_time());
            new_plan_tasks[new_plan_index].set_other(current_plan[index_in_current_plan++].get_other());
        }

        const Action<N>& new_action = new_plan_tasks[new_plan_index].get_action();
        ActionData<N>& new_action_data = new_plan_tasks[new_plan_index];

        // add to pickup map
        if (new_action.get_action_type() == Action_type::pickup)
            pickup_map[new_action.get_request().get_index()] = &new_action_data;

        unsigned int travel_time;

        // travel time increment
        if (new_plan_index == 0)
            travel_time = this->travel_time_provider.get()->get_travel_time(vehicle.get_init_position(), new_action.get_node());
        else
            travel_time = this->travel_time_provider.get()->get_travel_time(new_plan_tasks[new_plan_index - 1].get_action().get_node(), new_action.get_node());

        new_plan_travel_time += travel_time;
        new_plan_cost += travel_time;

        // check max time check for the new action
        if (new_action.get_max_time() < new_plan_travel_time)
            return std::nullopt;

        // arrival time
        new_action_data.set_arrival_time(new_plan_travel_time);

        // min time check for new action
        if (new_action.get_min_time() > new_plan_travel_time)
            new_plan_travel_time = new_action.get_min_time();

        // max ride time check - drop_off
        if (new_action.get_action_type() == Action_type::dropoff) {
            // pickup / drop_off referencing
            ActionData<N>* pickup_action_data = pickup_map[new_action.get_request().get_index()];
            pickup_action_data->set_other(&new_action_data);
            new_action_data.set_other(pickup_action_data);

            if (new_plan_travel_time - pickup_map[new_action.get_request().get_index()]->get_departure_time() > this->max_ride_time){
                if (!this->adjust_times(new_plan_tasks)) {
                    return std::nullopt;
                }
            }
        }

        // service time addition
        new_plan_travel_time += new_action.get_service_duration();

        // check max route time
        if (new_plan_travel_time > this->max_route_duration)
            return std::nullopt;

        // check max time for actions in the current plan
        for (int index = index_in_current_plan; index < current_plan.get_length(); index++){
            const Action<N>& remaining_action = current_plan[index].get_action();

            if (remaining_action.get_max_time() < new_plan_travel_time)
                return std::nullopt;
        }

        // check max time for pick up action
        if (new_plan_index < pickup_option_index && request.get_pickup().get_max_time() < new_plan_travel_time)
            return std::nullopt;

        // check max time for drop off action
        if (new_plan_index < dropoff_option_index && request.get_dropoff().get_max_time() < new_plan_travel_time)
            return std::nullopt;

        // capacity handling
        if(new_action_data.get_action().get_action_type() == Action_type::dropoff) {
            free_capacity++;

            // discomfort increment
            new_plan_delay += new_plan_travel_time - new_action.get_request().get_min_travel_time();
        }
        else {
            if (free_capacity == 0)
                return std::nullopt;

            free_capacity--;
        }

        // departure time
        new_action_data.set_departure_time(new_plan_travel_time + 1);
    }

    // add cost of returning to depot
    unsigned int travel_time_to_depot = this->travel_time_provider.get()->get_travel_time(new_plan_tasks[new_plan_tasks.size() - 1].get_action().get_node(), vehicle.get_init_position());
    new_plan_cost += travel_time_to_depot;
    new_plan_travel_time += travel_time_to_depot;

    // max route time check
    if (new_plan_travel_time > this->max_route_duration)
        return std::nullopt;

    //std::cout << "Found for req " << request.get_index() << "\n";

    return VehiclePlan{vehicle, new_plan_cost, new_plan_tasks, 0, new_plan_travel_time};
}

template <typename N>
bool Random_solver<N>::adjust_times(std::vector<ActionData<N>>& new_plan_tasks) {
    ActionData<N>* pickup_of_last_action = new_plan_tasks[new_plan_tasks.size() - 1].get_other();
    std::queue<ActionData<N>*> pickups_to_resolve;
    pickups_to_resolve.push(pickup_of_last_action);

    while (!pickups_to_resolve.empty()) {
        ActionData<N>* pickup_action_data = pickups_to_resolve.front();
        pickups_to_resolve.pop();
        const Action<N>& pickup_action = pickup_action_data->get_action();

        unsigned long drop_off_arrival_time = pickup_action_data->get_other()->get_arrival_time();

        // difference between current ride time (which breaks constraint) and max ride time
        unsigned long diff = drop_off_arrival_time - pickup_action_data->get_departure_time() - this->max_ride_time;

        // fail if pick up cannot be delayed
        if (pickup_action_data->get_departure_time() - pickup_action.get_service_duration() + diff > pickup_action.get_max_time())
            return false;

        // set new pickup departure time
        pickup_action_data->set_departure_time(pickup_action_data->get_departure_time() + diff);

        // adjust times for actions from pickup to the end of the plan
        bool solve = false;

        for (ActionData<N>& action_data: new_plan_tasks) {
            // solve only actions after adjusted pickup
            if (solve) {
                const Action<N>& action = action_data.get_action();

                // check max arrival time
                if(action_data.get_arrival_time() > action.get_max_time())
                    return false;

                action_data.set_arrival_time(action_data.get_arrival_time() + diff);

                unsigned int departure_time = action_data.get_departure_time();
                action_data.set_departure_time(
                        std::max(departure_time, action_data.get_arrival_time() + action.get_service_duration()));

                // delay update
                diff = action_data.get_departure_time() - departure_time;

                // max ride time check
                if(action.get_action_type() == Action_type::dropoff && action_data.get_arrival_time() - action_data.get_other()->get_departure_time() > this->max_ride_time)
                    pickups_to_resolve.push(action_data.get_other());

                // if the delay is 0, we can stop
                if (diff == 0)
                    break;
            }
            else if (&action_data == pickup_action_data)
                solve = true;
        }
    }
    return false;
}

template <typename N>
bool Random_solver<N>::try_update_best_plan(VehiclePlan<N> &potential_plan, unsigned int cost_increment) {
    if(cost_increment < min_cost_increment || !this->best_plan.has_value()){
        min_cost_increment = cost_increment;
        best_plan = potential_plan;
        return true;
    }
    return false;
}

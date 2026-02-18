//
// Created by Fido on 2020-03-20.
//

#include <fstream>
#include <vector>
#include <queue>
#include <memory>
#include <tuple>
#include <filesystem>
#include <iostream>

#include "Cordeau_benchmark.h"
#include "DARP_instance.h"
#include "travel_time_provider/Euclidean_travel_time_provider.h"
#include "travel_time_provider/Distance_matrix_travel_time_provider.h"
#include "travel_time_provider/Travel_time_provider.h"
#include "inout.h"

//template class std::vector<Request>;

Cordeau_node::Cordeau_node(float x, float y, unsigned int index) : Coordinate(index), x{x}, y{y}{}

Cordeau_node::Cordeau_node(float x, float y) : Cordeau_node(x, y, 0) {};


double Cordeau_node::getX() const {
    return x;
}

double Cordeau_node::getY() const {
    return y;
}

DARP_instance<Cordeau_node> Cordeau_reader::read(std::filesystem::path filepath) {
    //std::cout << "Reading Cordeau DARP instance from: " << std::filesystem::absolute(filepath) << std::endl;

    std::ifstream infile = get_file_stream(filepath.string());
    if(!infile.good()){
        std::cerr << "Could not open file." << std::endl;
    }

    int skip;
    unsigned int num_vehicles;
    unsigned int num_actions;
    unsigned int max_route_duration;
    unsigned short vehicle_capacity;
    unsigned int max_ride_time;


    infile >> num_vehicles >> num_actions >> max_route_duration >> vehicle_capacity >> max_ride_time;

    spdlog::info(
R"(Instance statistics:
	vehicle count: {}
	action count: {}
	max_route duration {}s
	max ride time: {}s
	vehicle capacity: {})", num_vehicles, num_actions, max_route_duration, max_ride_time, vehicle_capacity); 

	int num_requests = num_actions/2;
    float depot_x;
    float depot_y;
    infile >> skip >> depot_x >> depot_y >> skip >> skip >> skip >> skip;

    std::unique_ptr<std::vector<Vehicle<Cordeau_node>>> vehicles = std::make_unique<std::vector<Vehicle<Cordeau_node>>>();
    vehicles->reserve(num_vehicles);
    std::shared_ptr<Cordeau_node> depot_node {new Cordeau_node(depot_x, depot_y, 0)};
    for(unsigned int i = 0; i < num_vehicles; i++){
        vehicles->emplace_back(i, depot_node, vehicle_capacity);
    }

    std::queue<std::tuple<std::shared_ptr<Cordeau_node>, unsigned int, unsigned int, unsigned short>> origin_actions;
    std::unique_ptr<std::vector<Request<Cordeau_node>>> requests = std::make_unique<std::vector<Request<Cordeau_node>>>();
    requests->reserve(num_requests);
    unsigned int id;
    float x;
    float y;
    unsigned short service_time;
    short origin;
    unsigned short min_time;
    unsigned short max_time;

    unsigned int index = 0;

	const std::shared_ptr<Euclidean_travel_time_provider<Cordeau_node>> euclidean_travel_time_provider
        {new Euclidean_travel_time_provider<Cordeau_node>(60u)};
    std::vector<std::shared_ptr<const Cordeau_node>> nodes;
    nodes.push_back(depot_node);

    while (infile >> id >> x >> y >> service_time >> origin >> min_time >> max_time) {
        auto node = std::make_shared<Cordeau_node>(x, y, id);
    	nodes.push_back(node);
        if(origin == 1){
            origin_actions.emplace(node, ((unsigned int) min_time) * 60u, ((unsigned int) max_time) * 60u, (unsigned short) (service_time * 60u));
        }
        else{
            unsigned short min_travel_time = (unsigned short)
                euclidean_travel_time_provider->get_travel_time(*std::get<0>(origin_actions.front()), *node);
            requests->emplace_back(
                    index++,
                    id - num_requests, 
                    id,
                    std::get<0>(origin_actions.front()),
                    std::get<1>(origin_actions.front()),

                std::get<2>(origin_actions.front()),
                node,
                min_time * 60u,
                max_time * 60u,
                min_travel_time,
                std::get<3>(origin_actions.front()),
                (unsigned short) (service_time * 60u));
            origin_actions.pop();
        }
    }

    //return DARP_instance<Cordeau_node>(std::move(requests), std::move(vehicles),
    //        std::static_pointer_cast<Travel_time_provider<Cordeau_node>>(euclidean_travel_time_provider),
    //        max_route_duration * 60u, max_ride_time * 60u);
	return {
        std::move(requests),
		std::move(vehicles),
		std::make_shared<Distance_matrix_node_travel_time_provider<Cordeau_node>>(nodes, *euclidean_travel_time_provider),
        std::make_shared<DARP_instance_configuration>(
			max_route_duration * 60u,
            max_ride_time * 60u
        )
	};
}







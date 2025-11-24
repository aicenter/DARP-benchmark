#pragma once

#include <random>
#include "DARP_benchmark_solver.h"
#include "../DARP_instance.h"

template <typename N>
class Random_solver: public DARP_benchmark_solver<N> {
public:
    using DARP_benchmark_solver<N>::DARP_benchmark_solver;
	
    Solution<N> solve() override;
private:
    unsigned int min_cost_increment{0};
    std::optional<VehiclePlan<N>> best_plan{};
    std::mt19937 rand_g{};


    std::vector<VehiclePlan<N>> vehicle_plans;

    template<class R, class V>
    Solution<N> compute(const R& requests, const V& vehicles);

    std::vector<unsigned int> shuffle_vehicles(const std::vector<Vehicle<N>> &vehicles);

    void process_request(const Request<N> &request, const std::vector<Vehicle<N>> &vehicles);

    bool can_serve_request(const Vehicle<N> &vehicle, const Request<N> &request);

    bool compute_optimal_plan(const Vehicle<N> &vehicle, const Request<N> &request, VehiclePlan<N> &current_plan);

    const Vehicle<N>& find_vehicle_by_index(unsigned int index, const std::vector<Vehicle<N>> &vehicles);

    std::optional<VehiclePlan<N>> insert_into_plan(const VehiclePlan<N> &current_plan,
                                                                     unsigned short pickup_option_index, unsigned short dropoff_option_index, const Vehicle<N>& vehicle,
                                                                     const Request<N> &request);
    VehiclePlan<N>& find_plan_by_vehicle(const Vehicle<N> &v1);
    size_t find_plan_index(const VehiclePlan<N> &vp);
    bool try_update_best_plan(VehiclePlan<N> &potential_plan, unsigned int cost_increment);
    bool adjust_times(std::vector<ActionData<N>> &new_plan_tasks);

    std::vector<const Request<N>*> dropped_requests{};
};

#include "Random_solver.tpp"
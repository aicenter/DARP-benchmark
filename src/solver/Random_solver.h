/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */
#pragma once

#include <random>
#include "DARP_benchmark_solver.h"
#include "../DARP_instance.h"

template <typename N>
class Random_solver: public DARP_benchmark_solver<N> {
public:
    Random_solver(const DARP_instance<N>& instance, const DARP_benchmark_config& config)
        : DARP_benchmark_solver<N>(instance, config) {}

    typename DARP_benchmark_solver<N>::solution_impl_ret_val solve_impl() override;

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

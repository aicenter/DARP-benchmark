#
# MIT License
#
# Copyright (c) 2026 Czech Technical University in Prague
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.#
from typing import Iterable, Optional, List
from tqdm import tqdm
import copy

import darpbenchmark.solver
import darpbenchmark.plan_builder
from darpbenchmark.instance import Request, Vehicle, ActionType
from darpbenchmark.solution import Solution
from darpbenchmark.vehicle_plan import ActionData


class InsertionHeuristicPlanBuilder(darpbenchmark.plan_builder.PlanBuilder):
    def __init__(self, vehicle: Vehicle, init_size: int):
        super().__init__(vehicle, init_size, init_size * 4 + 2)
        self.action_data_used_length = 0
        self.cost_before_drop_off = 0

    def get_active_length(self) -> int:
        if self.cost_before_drop_off:
            return self.action_data_used_length
        else:
            return self.action_data_used_length - 1

    def get_last_action(self) -> ActionData:
        assert self.get_active_length() > 0
        return self.action_data[self.action_order[self.get_active_length() - 1]]

    def get_servicing_end(self) -> int:
        """
        Returns the end time of the request servicing: the departure time of the last action.
        :return:
        """
        return self.get_last_action().departure_time

    def add_new_request_data(self, pickup_action_data: ActionData, drop_off_action_data: ActionData):
        self.action_data_used_length += 2

        # action data size needs to be raised (this plan has been the best plan recently)
        if self.action_data_used_length > len(self.action_data):
            self.action_data.append(pickup_action_data)
            self.action_data.append(drop_off_action_data)
        else:
            self.action_data[self.action_data_used_length - 2] = pickup_action_data
            self.action_data[self.action_data_used_length - 1] = drop_off_action_data

        # action order and time adjustments size needs to be raised (this plan has been the best plan recently,
        # and the initial size was exceeded)
        if self.action_data_used_length > len(self.action_order):
            self.action_order.append(-1)
            self.action_order.append(-1)
            for _ in range(0, self.action_data_used_length * 4 + 2 - len(self.time_adjustments)):
                self.time_adjustments.append(0)


        # pickup-drop off referencing
        # self.action_data[self.action_data_used_length - 1].set_other_action_data_index(self.action_data_used_length - 2)
        # self.action_data[self.action_data_used_length - 2].set_other_action_data_index(self.action_data_used_length - 1)

        # we need to erase this, as it is a drop off indicator
        self.cost_before_drop_off = 0


initial_plan_builder_capacity = 4


class InsertionHeuristicSolver(darpbenchmark.solver.DARPBenchmarkSolver):

    def __init__(self, darp_instance: darpbenchmark.instance.DARPInstance):
        super().__init__(darp_instance)
        self.plan_builders: Optional[List[InsertionHeuristicPlanBuilder]] = None
        self.dropped_requests: List[Request] = []
        self.best_plan: Optional[InsertionHeuristicPlanBuilder] = None
        self.min_cost_increment = 0
        self.best_vehicle_index: Optional[int] = None
        self.current_vehicle_index = 0

    def can_serve_request(self, vehicle: Vehicle, pickup_action_data: ActionData, drop_off_action_data: ActionData):
        # node identity
        if vehicle.initial_position == pickup_action_data.position:
            return True

        # pickup feasibility check
        can_serve_pickup = self.darp_instance.travel_time_provider.get_travel_time(
            vehicle.initial_position, pickup_action_data.position) < pickup_action_data.get_max_time()

        if can_serve_pickup:
            # drop off feasibility check
            return self.darp_instance.travel_time_provider.get_travel_time(
                vehicle.initial_position, drop_off_action_data.position) \
                   + pickup_action_data.get_service_duration() < drop_off_action_data.get_max_time()

        return False

    def insert_request_into_plan_optimally(
            self,
            pickup_action_data: ActionData,
            drop_off_action_data: ActionData,
            plan: InsertionHeuristicPlanBuilder
    ):
        plan.add_new_request_data(pickup_action_data, drop_off_action_data)

        free_capacity = plan.vehicle.capacity
        old_cost = plan.cost
        best_plan = copy.deepcopy(plan)

        for pickup_option_index in range(plan.action_data_used_length - 1):

            # continue if the vehicle is full
            if free_capacity > 0:
                success = self.insert_into_plan(plan, pickup_option_index, True)

                if success:
                    cost_increment = plan.cost - old_cost

                    if cost_increment > self.min_cost_increment:
                        plan.remove_lastly_added_action(True, True)
                    else:
                        for drop_off_option_index in range(pickup_option_index + 1, plan.action_data_used_length):
                            success = self.insert_into_plan(plan, drop_off_option_index, False)
                            if success:
                                cost_increment = plan.cost() - old_cost
                                if cost_increment < self.min_cost_increment:
                                    self.min_cost_increment = cost_increment
                                    best_plan = copy.deepcopy(plan)
                                plan.remove_lastly_added_action(False, True)
                        plan.remove_lastly_added_action(True, True)

            # change free capacity for next index
            # we only need to adjust the capacity before the last possible pickup
            if pickup_option_index < plan.action_data_used_length - 2:
                if plan[pickup_option_index].get_action().get_action_type() == ActionType.PICKUP:
                    free_capacity -= 1
                else:
                    free_capacity += 1

        return best_plan

    def process_request_vehicle_combination(
            self,
            pickup_action_data: ActionData,
            drop_off_action_data: ActionData,
            vehicle: Vehicle
    ):
        # fail fast
        if self.can_serve_request(vehicle, pickup_action_data, drop_off_action_data):
            current_plan = self.plan_builders[self.current_vehicle_index]
            new_min_cost_increment = self.insert_request_into_plan_optimally(
                pickup_action_data, drop_off_action_data, current_plan, self.min_cost_increment)
            if self.min_cost_increment > new_min_cost_increment:
                self.min_cost_increment = new_min_cost_increment
                self.best_plan = current_plan
                self.best_vehicle_index = self.current_vehicle_index


    def compute_best_plan_for_request(self, request: Request, vehicles: Iterable[Vehicle]):
        min_cost_increment = 100_000_000
        best_plan = None

        pickup_action_data = ActionData(request.pickup_action())
        drop_off_action_data = ActionData(request.drop_off_action())

        for self.current_vehicle_index, vehicle in enumerate(vehicles):
            self.process_request_vehicle_combination(pickup_action_data, drop_off_action_data, vehicle)


    def finalize_plan(self, plan: InsertionHeuristicPlanBuilder):
        # compute plan arrival time
        if self.darp_instance.return_to_depot:
            travel_time = self.darp_instance.travel_time_provider.get_travel_time(
                plan.get_last_action().position, plan.vehicle.initial_position)
            plan.arrival_time = plan.get_servicing_end() + travel_time
        else:
            plan.arrival_time = plan.get_servicing_end()

        plan.time_adjustments = [0 for _ in range(len(plan.time_adjustments))]


    def process_request(self, request: Request, vehicles: Iterable[Vehicle]):
        self.compute_best_plan_for_request(request, vehicles)
        if self.best_plan:
            self.finalize_plan(self.best_plan)
            self.solution_cost += self.min_cost_increment
            self.plan_builders[self.best_vehicle_index] = self.best_plan
        else:
            self.dropped_requests.append(request)


    def solve_instance(self, requests: Iterable[Request], vehicles: Iterable[Vehicle]):
        self.solution_cost = 0

        # we start with empty plans for all vehicles
        self.plan_builders = [InsertionHeuristicPlanBuilder(vehicle, initial_plan_builder_capacity) for vehicle in vehicles]

        for request in tqdm(requests, decs="Processing Requests"):
            self.process_request(request, vehicles)

        vehicle_plans = [plan_builder.to_vehicle_plan() for plan_builder in self.plan_builders]

        return Solution(vehicle_plans, self.solution_cost, self.dropped_requests)


    def solve(self):
        return self.solve_instance(self.darp_instance.requests, self.darp_instance.vehicles)

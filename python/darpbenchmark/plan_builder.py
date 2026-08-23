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
from typing import List

from darpbenchmark.instance import Vehicle
from darpbenchmark.vehicle_plan import ActionData, VehiclePlan


class PlanBuilder:
    def __init__(self, vehicle: Vehicle, init_size: int, init_time_adjustments_size: int):
        self.vehicle = vehicle

        # action data for all actions that should be served by the plan, when plan building is complete.
        self.action_data: List[ActionData] = []

        # indexes to action_data marking the action order
        self.action_order: List[int] = [-1 for _ in range(init_size)]

        # Time adjustments from the adjust_times method. They need to be reverted when an action is removed from plan.
        # The semantic structure is described at: https://docs.google.com/spreadsheets/d/1FJWO9nRrsYui55tgJv2pCt7ly7NSHfvCPW4keAmXnK4/edit?usp=sharing
        self.time_adjustments: List[int] = [0 for _ in range(init_time_adjustments_size)]

        self.cost = 0
        self.departure_time = 0
        self.arrival_time = 0

    def get_other(self, source_action_data: ActionData) -> ActionData:
        return self.action_data[source_action_data.other_index]

    def to_vehicle_plan(self):
        vehicle_plan_actions = []
        for i in range(len(self.action_order)):
            if self.action_order[i] == -1:
                break

            action = self.action_data[self.action_order[i]]
            vehicle_plan_actions.append(action)
            action.other_index = self.get_other(action).position

        return VehiclePlan(self.vehicle, self.cost, vehicle_plan_actions, self.departure_time, self.arrival_time)
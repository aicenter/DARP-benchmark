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
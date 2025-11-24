
from abc import ABC, abstractmethod

import darpbenchmark.instance


class DARPBenchmarkSolver(ABC):

    def __init__(self, darp_instance: darpbenchmark.instance.DARPInstance):
        self.darp_instance = darp_instance
        self.solution_cost = 0

    @abstractmethod
    def solve(self):
        pass
from __future__ import annotations

from typing import Callable, Optional, TypeVar, Tuple, Generic, List
from abc import ABC, abstractmethod
import queue


class Solution(ABC):

    @abstractmethod
    def get_cost(self):
        pass


S = TypeVar("S", bound=Solution)


class Node(ABC, Generic[S]):

    @abstractmethod
    def is_solution(self):
        pass

    @abstractmethod
    def get_solution(self) -> Solution:
        pass

    @abstractmethod
    def branch(self) -> List[Node]:
        pass

    @abstractmethod
    def bound(self) -> int:
        pass


IN = TypeVar("IN")
# N = TypeVar("N", bound=Node)


class BB:
    def __init__(self,
                 instance: IN,
                 init_solution_function: Callable[[IN], S],
                 init_node_function: Callable[[], Node]):
        self.instance = instance
        self.init_solution_function = init_solution_function
        self.init_node_function = init_node_function
        self.bound: Optional[int] = None
        self.queue: queue.Queue[Node] = queue.Queue()
        self.best_solution: Optional[S] = None

    def run(self):
        init_sol = self.init_solution_function(self.instance)
        self.bound = init_sol.get_cost()
        self.best_solution = init_sol
        init_node = self.init_node_function()
        self.queue.put(init_node)

        while not self.queue.empty():
            current_node = self.queue.get()
            if current_node.is_solution() and current_node.get_solution().get_cost() < self.bound:
                self.bound = current_node.get_solution().get_cost()
                self.best_solution = current_node.get_solution()
            else:
                nodes = current_node.branch()

                for node in nodes:
                    bound = node.bound()
                    if bound < self.bound:
                        self.queue.put(node)




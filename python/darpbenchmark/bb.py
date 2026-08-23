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




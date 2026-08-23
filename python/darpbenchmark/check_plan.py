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
import darpbenchmark.inout
import darpbenchmark.solution_checker

from darpbenchmark.cordeau_benchmark import CordeauReader


instance_path = r"C:\Workspaces\AIC\DARP-benchmark\data\instances\Cordeau_2003/pr01"

plan_path = r"C:\Workspaces\AIC\DARP-benchmark\data\results\vga\Cordeau_2003/plan_b.json"

# dm_path = r"O:\AIC data\maps/manhattan.csv"

instance = CordeauReader().load(instance_path)
# instance = AmodsimReader().read(instance_path, dm_path)

request_map = dict()
for request in instance.requests:
    request_map[request.index] = request

vehicle_map = dict()
for vehicle in instance.vehicles:
    vehicle_map[vehicle.index] = vehicle

used_vehicles = set()

plan = darpbenchmark.inout.load_plan(plan_path, instance)

solution_ok = True
served_requests = set()
total_cost = 0

cost = darpbenchmark.solution_checker.check_plan(plan, 0, instance, set())



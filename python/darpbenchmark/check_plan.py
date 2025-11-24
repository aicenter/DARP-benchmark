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



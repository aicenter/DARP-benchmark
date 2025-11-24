from __future__ import annotations

import logging
from abc import ABC, abstractmethod
from typing import List, Union, TypeVar, Tuple, Optional, Type

import gurobipy
import numpy as np

import darpbenchmark.log
import darpbenchmark.instance
import darpbenchmark.chaining
import darpbenchmark.inout
from darpbenchmark.instance import MatrixTravelTimeProvider
from darpbenchmark.chaining import Connection, ChainingPossibilities, ChainingTravelTimeProvider, Network, Vertex, Arc, \
    VariantGenerationVehiclePlan, NetworkGenerationVehiclePlan, NetworkGenerator, MCFP_ILP_Solver, chain, \
    VariantGeneration, OldVariantGeneration, NetworkGeneratorVehicle, NetworkGeneratorTravelTimeProvider, PlanJoiner, \
    VehicleToPlanAssigner, P
from darpbenchmark.test.common import assert_len_equal

T = TypeVar('T', bound='TestPlan')


class TestPlan(NetworkGenerationVehiclePlan):
    __test__ = False

    def __init__(self, plan_id: int, variant_id: int = 0):
        self.plan_id = plan_id
        self.variant_id = variant_id

    def get_full_id(self) -> Union[int, str]:
        if self.variant_id == 0:
            return self.plan_id
        return f"{self.plan_id}-{self.variant_id}"

    def is_variant(self):
        return self.variant_id > 0

    def get_variant_id(self) -> int:
        return self.variant_id

    def __key(self):
        return self.plan_id, self.variant_id

    def __eq__(self, other):
        return self.__key() == other.__key()

    def __hash__(self) -> int:
        return hash(self.__key())

    def __str__(self):
        if self.variant_id > 0:
            return f"Plan {self.plan_id}: variant {self.variant_id}"
        else:
            return f"Plan {self.plan_id}"

    def __repr__(self):
        return self.__str__()


class JSONTestPlan(TestPlan, ABC):

    @staticmethod
    @abstractmethod
    def load_from_json(json_plan: dict, plan_id: int = None, variant_id: int = 0) -> T:
        pass


S = TypeVar('S', bound='StringTestPlan')


class StringTestPlan(TestPlan):

    @classmethod
    @abstractmethod
    def load_from_string(cls: Type[S], string_plan: str) -> Optional[T]:
        pass


V = TypeVar('V', bound='VariantGenerationTestPlan')


class VariantGenerationTestPlan(VariantGenerationVehiclePlan, JSONTestPlan):
    __test__ = False

    def get_max_delay(self):
        return self.max_delay

    def get_servicing_end(self):
        return self.arrival_time

    def get_servicing_start(self):
        return self.departure_time

    def __init__(
        self,
        plan_id: int,
        departure_time: int,
        arrival_time: int,
        max_delay: int,
        delay_reserve: int,
        variant_id: int = 0
    ):
        super().__init__(plan_id, variant_id)
        self.departure_time = departure_time
        self.arrival_time = arrival_time
        self.max_delay = max_delay
        self.delay_reserve = delay_reserve

    def _get_delayed_instance(
        self: V,
        variant_id: int,
        new_departure_time: int,
        new_arrival_time: int,
        new_max_delay: int,
        new_delay_reserve: int
    ) -> VariantGenerationTestPlan:
        delayed_plan = VariantGenerationTestPlan(
            self.plan_id,
            new_departure_time,
            new_arrival_time,
            new_max_delay,
            new_delay_reserve,
            variant_id
        )
        return delayed_plan

    def get_delayed_plan(self, min_delay, variant_id: int) -> V:
        new_departure_time = self.departure_time + min_delay
        arrival_delay = max(0, min_delay - self.delay_reserve)
        new_arrival_time = self.arrival_time + arrival_delay
        new_max_delay = self.max_delay - arrival_delay
        new_delay_reserve = max(0, self.delay_reserve - min_delay)
        return self._get_delayed_instance(
            variant_id, new_departure_time, new_arrival_time, new_max_delay, new_delay_reserve
        )

    @staticmethod
    def load_from_json(json_plan: dict, plan_id: int = None, variant_id: int = 0) -> VariantGenerationTestPlan:
        # plan ID par is supplied only for variants
        if not plan_id:
            plan_id = json_plan["id"]

        plan = VariantGenerationTestPlan(
            plan_id,
            int(json_plan["departure_time"]),
            int(json_plan["arrival_time"]),
            int(json_plan["max_delay"]),
            int(json_plan["delay_reserve"]),
            variant_id
        )
        return plan


class VariantGenerationAmodsimTestPlan(VariantGenerationTestPlan):
    def __init__(
        self,
        plan_id: int,
        departure_time: int,
        arrival_time: int,
        max_delay: int,
        delay_reserve: int,
        from_location_index: int,
        to_location_index: int,
        variant_id: int = 0
    ):
        super().__init__(plan_id, departure_time, arrival_time, max_delay, delay_reserve, variant_id)
        self.from_location_index = from_location_index
        self.to_location_index = to_location_index

    def _get_delayed_instance(
        self: VariantGenerationAmodsimTestPlan,
        variant_id: int,
        new_departure_time: int,
        new_arrival_time: int,
        new_max_delay: int,
        new_delay_reserve: int
    ) -> VariantGenerationTestPlan:
        delayed_plan = VariantGenerationAmodsimTestPlan(
            self.plan_id,
            new_departure_time,
            new_arrival_time,
            new_max_delay,
            new_delay_reserve,
            self.from_location_index,
            self.to_location_index,
            variant_id
        )
        return delayed_plan


class NetworkGenerationTestPlan(StringTestPlan):

    @classmethod
    def load_from_json(cls, json_plan: dict, plan_id: int = None, variant_id: int = 0) -> NetworkGenerationTestPlan:
        if plan_id is None:
            plan_id = json_plan["id"]
        return cls(plan_id, variant_id)

    @classmethod
    def load_from_string(cls: Type[S], string_plan: str) -> Optional[S]:
        if string_plan in ["Source", "Sink"]:
            return None

        string_plan = string_plan.split(" - ")[0]
        parts = string_plan.split(": ")
        plan_id = int(parts[0].split(" ")[1])
        variant_id = int(parts[1].split(" ")[1]) if len(parts) > 1 else 0

        return cls(plan_id, variant_id)

    def __init__(self, plan_id: int, variant_id: int = 0):
        super().__init__(plan_id, variant_id)

    def __key(self) -> Tuple[int, int]:
        return self.plan_id, self.variant_id

    def __eq__(self, other: object) -> bool:
        if isinstance(other, NetworkGenerationTestPlan):
            return self.__key() == other.__key()
        return NotImplemented

    def __hash__(self) -> int:
        return hash(self.__key())


class ILPSolverTestPlan(NetworkGenerationTestPlan):
    pass


class RoutesTestPlan(StringTestPlan):

    @classmethod
    def load_from_string(cls, string_plan: str) -> Optional[RoutesTestPlan]:
        parts = string_plan.split(": ")
        plan_id = int(parts[0].split(" ")[1])
        variant_id = int(parts[1].split(" ")[1]) if len(parts) > 1 else 0

        return RoutesTestPlan(plan_id, variant_id)


class ChainingTestPlan(VariantGenerationTestPlan):

    def __init__(
        self,
        plan_id: int,
        departure_time: int,
        arrival_time: int,
        max_delay: int,
        delay_reserve: int,
        variant_id: int = 0,
        original_plans: List[str] = None,
        vehicle: str = None
    ):
        super().__init__(plan_id, departure_time, arrival_time, max_delay, delay_reserve, variant_id)
        self.original_plans = original_plans
        self.vehicle = vehicle

    @staticmethod
    def load_from_json(json_plan: dict, plan_id: int = None, variant_id: int = 0) -> ChainingTestPlan:
        # loaded from expected chaining plans file
        if 'original_plans' in json_plan:
            original_plans = json_plan['original_plans']
            max_delay = 0
            delay_reserve = 0
            vehicle = json_plan['vehicle']
        # loaded from plan set file (variant generation test plans JSON)
        else:
            plan_id = json_plan["id"]
            original_plans = None
            max_delay = int(json_plan["max_delay"])
            delay_reserve = int(json_plan["delay_reserve"])
            vehicle = None

        plan = ChainingTestPlan(
            plan_id,
            int(json_plan["departure_time"]),
            int(json_plan["arrival_time"]),
            max_delay,
            delay_reserve,
            variant_id,
            original_plans,
            vehicle
        )
        return plan


class TestChainingTravelTimeProvider(ChainingTravelTimeProvider, MatrixTravelTimeProvider):
    __test__ = False

    def get_time_between_plans(self, from_plan: VariantGenerationTestPlan, to_plan: VariantGenerationTestPlan):
        return self.get_travel_time(from_plan.plan_id, to_plan.plan_id)


class AmodsimTestChainingTravelTimeProvider(ChainingTravelTimeProvider, MatrixTravelTimeProvider):

    def get_time_between_plans(
        self,
        from_plan: VariantGenerationAmodsimTestPlan,
        to_plan: VariantGenerationAmodsimTestPlan
    ):
        return self.get_travel_time(from_plan.to_location_index, to_plan.from_location_index)


class NetworkGenerationTestVehicle(NetworkGeneratorVehicle):

    def __init__(self, vehicle_id: int):
        self.id = vehicle_id

    def __str__(self):
        return f"Vehicle {self.id}"


class NetworkGenerationTestTravelTimeProvider(NetworkGeneratorTravelTimeProvider):

    def __init__(self, dm: List[List[int]]):
        self.dm = dm

    def get_travel_time(self, vehicle: NetworkGenerationTestVehicle, plan: NetworkGenerationTestPlan):
        return self.dm[vehicle.id][plan.plan_id]


class TestPlanJoiner(PlanJoiner):

    def join_plans(self, routes: List[List[P]]) -> List[P]:
        plans = []
        for route in routes:
            original_plans = []
            for plan in route:
                original_plans.append(plan.__str__())
            plan = route[0]
            plan.original_plans = original_plans
            plan.arrival_time = route[-1].arrival_time
            plans.append(plan)
        return plans


class TestVehiclePlanAssigner(VehicleToPlanAssigner):

    def assign_vehicles_to_plans(self, vehicles: List[NetworkGenerationTestVehicle], plans: List[P]):
        for vehicle, plan in zip(vehicles, plans):
            plan.vehicle = vehicle.__str__()


def _load_plan_data(plan_set_name: str, plan_class: type[T]):
    # load plans
    plan_set_filename = f"{plan_set_name}.json"
    path = darpbenchmark.inout.get_test_resource(plan_set_filename)
    plan_data_json = darpbenchmark.inout.load_json(path.__str__())
    plans = []
    for json_plan in plan_data_json["plans"]:
        plans.append(plan_class.load_from_json(json_plan))

    # load travel times
    dm = np.array(plan_data_json["travel_times"])
    travel_time_provider = TestChainingTravelTimeProvider(dm)

    return plans, travel_time_provider


def _load_chaining_possibilities(chaining_possibilities_name: str, original_plans: List[T], plan_type: type[T]) \
        -> ChainingPossibilities[T]:
    chaining_possibilities_filename = f"{chaining_possibilities_name}.json"
    path = darpbenchmark.inout.get_test_resource(chaining_possibilities_filename)
    chaining_possibilities_json = darpbenchmark.inout.load_json(path.__str__())

    # plan dict
    plan_dict = {}
    for plan in original_plans:
        plan_dict[plan.get_full_id()] = plan

    # variants
    variants = [[] for _ in original_plans]
    for key, variant_list_json in chaining_possibilities_json["variants"].items():
        for variant_json in variant_list_json:
            key = int(key)
            variant = plan_type.load_from_json(variant_json, plan_id=key, variant_id=variant_json["id"])
            variants[key].append(variant)
            plan_dict[variant.get_full_id()] = variant

    # connections
    connections = []
    for connection_json in chaining_possibilities_json["connections"]:
        from_plan = plan_dict[connection_json["from"]]
        to_plan = plan_dict[connection_json["to"]]
        connections.append(Connection(from_plan, to_plan, connection_json["cost"]))

    chaining_possibilities = darpbenchmark.chaining.ChainingPossibilities(original_plans, variants, connections)

    return chaining_possibilities


def _decode_vertex_type(vertex_name: str) -> Vertex.VertexType:
    if vertex_name == "Source":
        return Vertex.VertexType.SOURCE
    if vertex_name == "Sink":
        return Vertex.VertexType.SINK
    if "Vehicle" in vertex_name:
        return Vertex.VertexType.VEHICLE
    if "variant" in vertex_name:
        if "left" in vertex_name:
            return Vertex.VertexType.LEFT_VARIANT
        else:
            return Vertex.VertexType.RIGHT_VARIANT
    else:
        if "left" in vertex_name:
            return Vertex.VertexType.LEFT_PLAN
        else:
            return Vertex.VertexType.RIGHT_PLAN


SP = TypeVar('SP', bound=StringTestPlan)


def _load_network(network_name: str, plan_type: type[SP]):
    network_filename = f"{network_name}.json"
    path = darpbenchmark.inout.get_test_resource(network_filename)
    network_json = darpbenchmark.inout.load_json(path.__str__())

    # load vertices
    required_flow = network_json["required_flow"]
    vertices = []
    vertices_dict = {}
    for vertex_name in network_json["vertices"]:
        vertex_type = _decode_vertex_type(vertex_name)
        if vertex_name in ["Source", "Sink"]:
            gain = required_flow if vertex_name == "Source" else -required_flow
        else:
            gain = 0
        if vertex_name.startswith("Plan"):
            content = plan_type.load_from_string(vertex_name)
        elif vertex_name.startswith("Vehicle"):
            content = NetworkGenerationTestVehicle(int(vertex_name[-1]))
        else:
            content = None

        final_vertex_name = vertex_name.replace(" - ", "-").replace(": ", "-").replace(" ", "_")

        vertex = Vertex(vertex_type, final_vertex_name, content, gain)
        vertices.append(vertex)
        vertices_dict[vertex_name] = vertex

    # load arcs
    arcs = []
    for arc_json in network_json["arcs"]:
        from_vertex = vertices_dict[arc_json["from"]]
        to_vertex = vertices_dict[arc_json["to"]]
        arc_type = Arc.ArcType[str(arc_json["type"]).upper()]
        max_flow = 1
        cost = arc_json["cost"] if "cost" in arc_json else 0
        arcs.append(Arc(from_vertex, to_vertex, arc_type, cost, max_flow))

    network = Network(vertices, arcs)

    network.generate_help_structures()

    return network


def _load_routes(routes_name: str) -> List[List[RoutesTestPlan]]:
    routes_filename = f"{routes_name}.json"
    path = darpbenchmark.inout.get_test_resource(routes_filename)
    routes_json = darpbenchmark.inout.load_json(path.__str__())
    routes = [[RoutesTestPlan.load_from_string(plan_name) for plan_name in route] for route in routes_json]
    return routes


def _check_plans_equal(computed_plan: VariantGenerationTestPlan, expected_plan: VariantGenerationTestPlan):
    assert computed_plan.plan_id == expected_plan.plan_id
    assert computed_plan.variant_id == expected_plan.variant_id
    assert computed_plan.departure_time == expected_plan.departure_time
    assert computed_plan.arrival_time == expected_plan.arrival_time


def _check_connections_equal(
    computed_connection: Connection[VariantGenerationTestPlan],
    expected_connection: Connection[VariantGenerationTestPlan]
):
    assert computed_connection.cost == expected_connection.cost

    _check_plans_equal(computed_connection.from_plan, expected_connection.from_plan)
    _check_plans_equal(computed_connection.to_plan, expected_connection.to_plan)


def _compare_chaining_possibilities(
    computed_possibilities: ChainingPossibilities[VariantGenerationTestPlan],
    expected_possibilities: ChainingPossibilities[VariantGenerationTestPlan]
):
    # compare plan variants
    computed_variants = computed_possibilities.variants
    expected_variants = expected_possibilities.variants
    assert_len_equal(computed_variants, expected_variants)

    for computed_plan_list, expected_plan_list in zip(computed_variants, expected_variants):
        assert_len_equal(computed_plan_list, expected_plan_list)

        for computed_variant, expected_variant in zip(computed_plan_list, expected_plan_list):
            _check_plans_equal(computed_variant, expected_variant)

    # compare connections
    computed_connections = computed_possibilities.connections
    expected_connections = expected_possibilities.connections

    assert_len_equal(computed_connections, expected_connections)

    for computed_connection, expected_connection in zip(computed_connections, expected_connections):
        _check_connections_equal(computed_connection, expected_connection)


def _check_vertices_equal(computed_vertex: Vertex, expected_vertex: Vertex):
    assert computed_vertex.type == expected_vertex.type
    assert str(computed_vertex.content) == str(expected_vertex.content), \
        f"computed vertex content: {computed_vertex.content}, expected vertex content: {expected_vertex.content}"
    assert computed_vertex.flow_gain == expected_vertex.flow_gain


def _check_networks_equal(computed_network: Network, expected_network: Network):
    # COMPARE VERTICES
    computed_vertices = computed_network.vertices
    expected_vertices = expected_network.vertices

    # sort the lists
    def vertex_sorter(vertex: Vertex):
        return vertex.type, str(vertex)

    computed_vertices.sort(key=vertex_sorter)
    expected_vertices.sort(key=vertex_sorter)

    assert_len_equal(computed_vertices, expected_vertices)
    for computed_vertex, expected_vertex in zip(computed_vertices, expected_vertices):
        _check_vertices_equal(computed_vertex, expected_vertex)

    #  COMPARE ARCS
    computed_arcs = computed_network.arcs
    expected_arcs = expected_network.arcs

    # sort the lists
    def arc_sorter(arc: Arc):
        return arc.arc_type, str(arc)

    computed_arcs.sort(key=arc_sorter)
    expected_arcs.sort(key=arc_sorter)

    assert_len_equal(computed_arcs, expected_arcs)
    for computed_arc, expected_arc in zip(computed_arcs, expected_arcs):
        assert expected_arc.arc_type == computed_arc.arc_type
        _check_vertices_equal(computed_arc.from_vertex, expected_arc.from_vertex)
        _check_vertices_equal(computed_arc.to_vertex, expected_arc.to_vertex)
        assert expected_arc.cost == computed_arc.cost
        assert expected_arc.max_flow == computed_arc.max_flow


def _check_routes_equal(
    computed_routes: List[list],
    expected_routes: List[list],
    ignore_variants: bool = False
):
    assert_len_equal(computed_routes, expected_routes)
    for computed_route, expected_route in zip(computed_routes, expected_routes):
        assert_len_equal(computed_route, expected_route)
        for computed_plan, expected_plan in zip(computed_route, expected_route):
            assert computed_plan.plan_id == expected_plan.plan_id
            if not ignore_variants:
                assert computed_plan.variant_id == expected_plan.variant_id


def _check_vehicles_equal(
    computed_vehicles: list,
    expected_vehicles: list
):
    for computed_vehicle, expected_vehicle in zip(computed_vehicles, expected_vehicles):
        assert str(computed_vehicle) == str(expected_vehicle)


def test_old_variant_generation():
    plans, travel_time_provider = _load_plan_data("plan_set_1", VariantGenerationTestPlan)

    chaining = darpbenchmark.chaining.OldVariantGeneration(travel_time_provider, 86_400)
    chaining_possibilities = chaining.generate_chaining_possibilities(plans)

    expected_chaining_possibilities \
        = _load_chaining_possibilities("chaining_possibilities_1-vg", plans, VariantGenerationTestPlan)

    _compare_chaining_possibilities(chaining_possibilities, expected_chaining_possibilities)


def test_variant_generation():
    plans, travel_time_provider = _load_plan_data("plan_set_1", VariantGenerationTestPlan)

    chaining = darpbenchmark.chaining.VariantGeneration(travel_time_provider, 86_400)
    chaining_possibilities = chaining.generate_chaining_possibilities(plans)

    expected_chaining_possibilities \
        = _load_chaining_possibilities("chaining_possibilities_1-vg", plans, VariantGenerationTestPlan)

    _compare_chaining_possibilities(chaining_possibilities, expected_chaining_possibilities)


def _get_firs_example_original_plans(plan_type: type = NetworkGenerationTestPlan):
    plans = [
        plan_type(0),
        plan_type(1),
        plan_type(2),
    ]
    return plans


def _load_vehicles(vehicle_count: int) -> List[NetworkGenerationTestVehicle]:
    vehicles = [NetworkGenerationTestVehicle(vehicle_id) for vehicle_id in range(vehicle_count)]
    return vehicles


def _load_network_generation_travel_time_provider(dm_string: str):
    dm_filename = f"{dm_string}.json"
    path = darpbenchmark.inout.get_test_resource(dm_filename)
    dm_json: List[List[int]] = darpbenchmark.inout.load_json(path.__str__())
    return NetworkGenerationTestTravelTimeProvider(dm_json)


def test_network_generation():
    plans = _get_firs_example_original_plans()

    chaining_possibilities = _load_chaining_possibilities(
        "chaining_possibilities_1-ng",
        plans,
        NetworkGenerationTestPlan
    )
    vehicles = _load_vehicles(2)
    expected_network = _load_network("network_1", NetworkGenerationTestPlan)
    travel_time_provider = _load_network_generation_travel_time_provider("dm")
    network_generator = NetworkGenerator()
    computed_network = network_generator.generate_network(plans, chaining_possibilities, vehicles, travel_time_provider)

    _check_networks_equal(computed_network, expected_network)


def test_network_generation_2():
    """
    Tests network generation with a plan that has active variants only on the right side. The desired network has
    therefore variant nodes for the plan only on the right.
    @return: Void
    """
    plans = _get_firs_example_original_plans()

    chaining_possibilities = _load_chaining_possibilities(
        "chaining_possibilities_2-ng",
        plans,
        NetworkGenerationTestPlan
    )
    vehicles = _load_vehicles(2)
    expected_network = _load_network("network_2", NetworkGenerationTestPlan)
    travel_time_provider = _load_network_generation_travel_time_provider("dm")
    network_generator = NetworkGenerator()
    computed_network = network_generator.generate_network(plans, chaining_possibilities, vehicles, travel_time_provider)

    _check_networks_equal(computed_network, expected_network)


def test_network_generation_3():
    """
    Tests network generation with multiple plans with variants with deferred connection processing.
    @return: Void
    """
    plans = _get_firs_example_original_plans()

    chaining_possibilities = _load_chaining_possibilities(
        "chaining_possibilities_3-ng",
        plans,
        NetworkGenerationTestPlan
    )
    vehicles = _load_vehicles(2)
    expected_network = _load_network("network_3", NetworkGenerationTestPlan)
    travel_time_provider = _load_network_generation_travel_time_provider("dm")
    network_generator = NetworkGenerator()
    computed_network = network_generator.generate_network(plans, chaining_possibilities, vehicles, travel_time_provider)

    _check_networks_equal(computed_network, expected_network)


def _load_ILP_model(model_name: str) -> gurobipy.Model:
    model_filename = f"{model_name}.lp"
    path = darpbenchmark.inout.get_test_resource(model_filename)

    model = gurobipy.read(path.__str__())

    return model


def _check_models_equal(computed_model: gurobipy.Model, expected_model: gurobipy.Model):
    computed_model.update()
    expected_model.update()

    # variables
    assert computed_model.numVars == expected_model.numVars

    sorter = lambda x: x.varName
    for computed_var, expected_var in \
            zip(sorted(computed_model.getVars(), key=sorter), sorted(expected_model.getVars(), key=sorter)):
        assert computed_var.varName == expected_var.VarName
        assert computed_var.obj == expected_var.obj

    # constraints
    assert computed_model.numConstrs == expected_model.numConstrs

    sorter = lambda x: x.constrName
    for computed_constraint, expected_constraint in \
            zip(sorted(computed_model.getConstrs(), key=sorter), sorted(expected_model.getConstrs(), key=sorter)):
        assert computed_constraint.constrName == expected_constraint.constrName
        assert computed_constraint.sense == expected_constraint.sense
        assert computed_constraint.rhs == expected_constraint.rhs

    # objective
    assert computed_model.modelSense == expected_model.modelSense


def test_chaining_generate_MCPF_model():
    network = _load_network("network_4", ILPSolverTestPlan)

    solver = MCFP_ILP_Solver()
    solver._create_ILP_model(network)

    expected_model = _load_ILP_model("model")

    _check_models_equal(solver._model, expected_model)


def test_chaining_ILP_solver():
    plans = _get_firs_example_original_plans(ILPSolverTestPlan)

    chaining_possibilities = _load_chaining_possibilities("chaining_possibilities_1-ng", plans, ILPSolverTestPlan)
    network = _load_network("network_1", ILPSolverTestPlan)

    solver = MCFP_ILP_Solver()
    routes, vehicles = solver.chain_plans_optimally(plans, chaining_possibilities, network)

    expected_routes = _load_routes("routes_1")
    expected_vehicles = [NetworkGenerationTestVehicle(1)]

    _check_routes_equal(routes, expected_routes)
    _check_vehicles_equal(vehicles, expected_vehicles)


def _load_chaining_plans(plan_set_name: str) -> List[ChainingTestPlan]:
    plans_filename = f"{plan_set_name}.json"
    path = darpbenchmark.inout.get_test_resource(plans_filename)
    plans_json: dict = darpbenchmark.inout.load_json(path.__str__())
    plans = [ChainingTestPlan.load_from_json(json_plan) for json_plan in plans_json]
    return plans


def _check_chaining_plans_equal(computed_plans: List[ChainingTestPlan], expected_plans: List[ChainingTestPlan]):
    for computed_plan, expected_plan in zip(computed_plans, expected_plans):
        assert computed_plan.original_plans == expected_plan.original_plans
        assert computed_plan.vehicle == expected_plan.vehicle
        assert computed_plan.departure_time == expected_plan.departure_time
        assert computed_plan.arrival_time == expected_plan.arrival_time


def test_chaining_old_variant_generation():
    plans, travel_time_provider = _load_plan_data("plan_set_1", ChainingTestPlan)
    vehicles = _load_vehicles(2)
    ng_travel_time_provider = _load_network_generation_travel_time_provider("dm_2")

    final_plans = chain(
        travel_time_provider,
        plans,
        vehicles,
        ng_travel_time_provider,
        86_400,
        TestPlanJoiner(),
        TestVehiclePlanAssigner(),
        True
    )

    expected_final_plans = _load_chaining_plans("chained_plans")

    _check_chaining_plans_equal(final_plans, expected_final_plans)


def test_chaining():
    plans, travel_time_provider = _load_plan_data("plan_set_1", ChainingTestPlan)
    vehicles = _load_vehicles(2)
    ng_travel_time_provider = _load_network_generation_travel_time_provider("dm_2")

    final_plans = chain(
        travel_time_provider,
        plans,
        vehicles,
        ng_travel_time_provider,
        86_400,
        TestPlanJoiner(),
        TestVehiclePlanAssigner(),
    )

    expected_final_plans = _load_chaining_plans("chained_plans")

    _check_chaining_plans_equal(final_plans, expected_final_plans)


def run_chaining_and_return_all_structs(
    plans,
    vehicles: List[V],
    ng_travel_time_provider: NetworkGeneratorTravelTimeProvider,
    variant_generation
) -> Tuple[ChainingPossibilities, Network, List[list], List[V]]:
    logging.info("Generating chaining possibilities")
    chaining_possibilities = variant_generation.generate_chaining_possibilities(plans)
    logging.info(
        f"Chaining possibilities generated: {chaining_possibilities.get_variant_count()} plan variants, "
        f"{len(chaining_possibilities.connections)} connections"
    )

    logging.info("Generating network")
    network_generator = NetworkGenerator()
    network = network_generator.generate_network(
        plans, chaining_possibilities, vehicles, ng_travel_time_provider
    )

    logging.info("Solving the Constrained Min-cost Flow Problem")
    solver = MCFP_ILP_Solver()
    routes, assigned_vehicles = solver.chain_plans_optimally(plans, chaining_possibilities, network)
    # assert len(routes) <= max_vehicle_count

    return chaining_possibilities, network, routes, assigned_vehicles


def _load_vga_plans(plans_json: dict, plan_count: int = 0) -> List:
    plans = []
    id_counter = 0
    for json_plan in plans_json:
        max_delay = 1_000_000_000
        delay_reserve = 0
        for action_data in json_plan["actions"]:
            max_delay_for_action = action_data["action"]["max_time"] - action_data["arrival_time"]
            if max_delay_for_action < max_delay:
                max_delay = max_delay_for_action

            delay_reserve_for_action = action_data["action"]["min_time"] - action_data["arrival_time"]
            if delay_reserve_for_action > delay_reserve:
                delay_reserve = delay_reserve_for_action

        plan = VariantGenerationAmodsimTestPlan(
            id_counter,
            json_plan["departure_time"],
            json_plan["arrival_time"],
            max_delay,
            delay_reserve,
            json_plan["actions"][0]["action"]["position"]["idx"],
            json_plan["actions"][-1]["action"]["position"]["idx"]
        )
        plans.append(plan)
        id_counter += 1

        if 0 < plan_count <= id_counter:
            break

    return plans


# def _load_chaining_plans(plan_set_name: str) -> List[ChainingTestPlan]:
#     plan_set_filename = f"{plan_set_name}.json"
#     path = darpbenchmark.inout.get_test_resource(plan_set_filename)
#     plan_data_json = darpbenchmark.inout.load_json(path.__str__())
#     plans = []
#     for json_plan in plan_data_json:
#         plans.append(ChainingTestPlan.load_from_json(json_plan))
#
#     return plans


def get_route_cost(
    route: List[VariantGenerationAmodsimTestPlan],
    travel_time_provider: ChainingTravelTimeProvider
):
    return sum(
        travel_time_provider.get_time_between_plans(from_plan, to_plan) for from_plan, to_plan in zip(route, route[1:])
    )


def _check_all_connections_exists(refrence: List[Connection], to_be_checked: List[Connection]):
    connection_dict = {}
    for connection in to_be_checked:
        key = (connection.from_plan.plan_id, connection.to_plan.plan_id)
        if key in connection_dict:
            connection_dict[key].append(connection)
        else:
            connection_dict[key] = [connection]

    for connection in refrence:
        key = (connection.from_plan.plan_id, connection.to_plan.plan_id)
        assert key in connection_dict
        for validated_connection in connection_dict[key]:
            assert validated_connection.cost == connection.cost


def _check_old_and_new_chaining_possibilities(
    old_chaining_possibilities: ChainingPossibilities,
    new_chaining_possibilities: ChainingPossibilities
):
    _check_all_connections_exists(old_chaining_possibilities.connections, new_chaining_possibilities.connections)
    _check_all_connections_exists(new_chaining_possibilities.connections, old_chaining_possibilities.connections)


def _analyze_differences_between_old_and_new_variant_generation(
    old_chaining_possibilities: ChainingPossibilities,
    old_network: Network,
    old_routes: List[List[VariantGenerationTestPlan]],
    new_chaining_possibilities: ChainingPossibilities,
    new_network: Network,
    new_routes: List[List[VariantGenerationTestPlan]],
    travel_time_provider: ChainingTravelTimeProvider
):
    # isolate routes unique to old chaining
    old_chaining_routes = {}
    for route in old_routes:
        key = tuple(plan.plan_id for plan in route)
        old_chaining_routes[key] = route

    new_chaining_routes = {}
    for route in new_routes:
        key = tuple(plan.plan_id for plan in route)
        new_chaining_routes[key] = route

    unique_old_chaining_routes = {}
    for key in old_chaining_routes:
        if key not in new_chaining_routes:
            unique_old_chaining_routes[key] = old_chaining_routes[key]

    # isolate unique routes that cannot be exactly realized in new chaining, i.e., they at least have a different
    # variant name
    unique_chaining_routes = {}
    new_connection_dict = {}
    for connection in new_chaining_possibilities.connections:
        key = (connection.from_plan, connection.to_plan)
        new_connection_dict[key] = connection

    for _, route in unique_old_chaining_routes.items():
        for plan, next_plan in zip(route, route[1:]):
            key = (plan, next_plan)
            if key not in new_connection_dict:
                unique_chaining_routes[key] = route

    # check all routes unique to old chaining can be realized according to connections from new variant generation
    new_connection_dict = {}
    for connection in new_chaining_possibilities.connections:
        key = (connection.from_plan.plan_id, connection.to_plan.plan_id)
        if key in new_connection_dict:
            new_connection_dict[key].append(connection)
        else:
            new_connection_dict[key] = [connection]

    for _, route in unique_old_chaining_routes.items():
        from_candidates = set()
        to_candidates = set()
        for plan, next_plan in zip(route, route[1:]):
            key = (plan.plan_id, next_plan.plan_id)
            assert key in new_connection_dict
            candidate_connections = new_connection_dict[key]
            for connection in candidate_connections:
                assert connection.cost == travel_time_provider.get_time_between_plans(plan, next_plan)
                # consider only plans with the same delay
                if (connection.from_plan.delay_reserve == plan.delay_reserve
                        and connection.to_plan.delay_reserve == next_plan.delay_reserve):
                    if not from_candidates or connection.from_plan in from_candidates:
                        to_candidates.add(connection.to_plan)

            assert to_candidates
            from_candidates = to_candidates

    # check that all routes unique to old chaining has a corresponding connection in the network
    new_arc_connection_dict = {}
    for arc in new_network.arcs:
        if arc.arc_type == Arc.ArcType.CONNECTION:
            key = (arc.from_vertex.content.plan_id, arc.to_vertex.content.plan_id)
            if key in new_arc_connection_dict:
                new_arc_connection_dict[key].append(arc)
            else:
                new_arc_connection_dict[key] = [arc]

    for _, route in unique_old_chaining_routes.items():
        from_candidates = set()
        to_candidates = set()
        for plan, next_plan in zip(route, route[1:]):
            key = (plan.plan_id, next_plan.plan_id)
            assert key in new_arc_connection_dict
            candidate_arcs = new_arc_connection_dict[key]
            for arc in candidate_arcs:
                assert arc.cost == travel_time_provider.get_time_between_plans(plan, next_plan)
                # consider only plans with the same delay
                if (arc.from_vertex.content.delay_reserve == plan.delay_reserve
                        and arc.to_vertex.content.delay_reserve == next_plan.delay_reserve):
                    if not from_candidates or arc.from_vertex.content in from_candidates:
                        to_candidates.add(arc.to_vertex.content)

            assert to_candidates
            from_candidates = to_candidates


class Result:
    def __init__(
        self,
        chaining_possibilities: ChainingPossibilities,
        network: Network,
        routes: List,
        route_cost: int,
        assigned_vehicles: List[V]
    ):
        self.chaining_possibilities = chaining_possibilities
        self.network = network
        self.routes = routes
        self.route_cost = route_cost
        self.assigned_vehicles = assigned_vehicles


def compare_old_and_new_chaining(
    test_plans_json: dict,
    travel_time_provider: ChainingTravelTimeProvider,
    vehicles: list,
    ng_travel_time_provider: NetworkGeneratorTravelTimeProvider,
    max_time_between_plans: int,
    variant_generation_to_compare: List[Tuple[str, VariantGeneration]],
    plan_count: int = 0
) -> List[Result]:
    logging.info("loading plans")
    plans = _load_vga_plans(test_plans_json, plan_count)

    result = []

    # wanted_plans = {97, 99, 101, 103, 104, 109, 119, 120}
    # plans = [plan for plan in plans if plan.plan_id in wanted_plans]
    logging.info("computing old chaining")
    old_variant_generation = OldVariantGeneration(travel_time_provider, max_time_between_plans)
    chaining_possibilities_old, network_old, routes_old, assigned_vehicles = run_chaining_and_return_all_structs(
        plans, vehicles, ng_travel_time_provider, old_variant_generation
    )
    route_cost_old = sum(get_route_cost(route, travel_time_provider) for route in routes_old)
    total_cost_old = route_cost_old
    result.append(Result(chaining_possibilities_old, network_old, routes_old, route_cost_old, assigned_vehicles))

    for desc, variant_generation in variant_generation_to_compare:
        logging.info("computing %s", desc)
        chaining_possibilities, network, routes, assigned_vehicles = run_chaining_and_return_all_structs(
            plans, vehicles, ng_travel_time_provider, variant_generation
        )

        logging.info("comparing new and old chaining results")

        route_cost = sum(get_route_cost(route, travel_time_provider) for route in routes)
        total_cost = route_cost
        _check_old_and_new_chaining_possibilities(chaining_possibilities_old, chaining_possibilities)
        _analyze_differences_between_old_and_new_variant_generation(
            chaining_possibilities_old, network_old, routes_old, chaining_possibilities, network, routes,
            travel_time_provider
        )
        assert total_cost_old == total_cost

        result.append(Result(chaining_possibilities, network, routes, route_cost, assigned_vehicles))
        # _check_routes_equal(routes, routes_old, True)

    return result

# def test_chaining_result_equal_for_new_and_old_variant_generation():
#     plan_set_filename = "vga_plans-Luze-test.json"
#     resource = load_resource("darpbenchmark.test.resources", plan_set_filename)
#     plan_data_json = json.loads(resource)
#
#     dm_filename = r"dm-Luze.csv"
#     dm_filepath = darpbenchmark.inout.get_resource_absolute_path("darpbenchmark.test.resources", dm_filename)
#     travel_time_provider = darpbenchmark.test.test_chaining.AmodsimTestChainingTravelTimeProvider.from_csv(dm_filepath)
#
#     vehicles = _load_vehicles(20)
#     ng_travel_time_provider = _load_network_generation_travel_time_provider("dm_Luze")
#
#     max_time_between_plans = 86_400
#
#     variant_generation_variants = [
#         ("old variant generation",
#          darpbenchmark.chaining.OldVariantGeneration(travel_time_provider, max_time_between_plans)),
#         ("new variant generation with unique plan departures",
#          darpbenchmark.chaining.VariantGenerationVersion2(travel_time_provider, max_time_between_plans))
#     ]
#
#     compare_old_and_new_chaining(
#         plan_data_json,
#         travel_time_provider,
#         vehicles,
#         ng_travel_time_provider,
#         max_time_between_plans,
#         variant_generation_variants
#     )


# def test_chaining_result_equal_for_new_and_old_variant_generation_max_time_between_plans():
#     plan_set_filename = "vga_plans-Luze-test.json"
#     resource = load_resource("darpbenchmark.test.resources", plan_set_filename)
#     plan_data_json = json.loads(resource)
#
#     dm_filename = r"dm-Luze.csv"
#     dm_filepath = darpbenchmark.inout.get_resource_absolute_path("darpbenchmark.test.resources", dm_filename)
#     travel_time_provider = darpbenchmark.test.test_chaining.AmodsimTestChainingTravelTimeProvider.from_csv(dm_filepath)
#
#     max_time_between_plans = 240
#
#     variant_generation_variants = [
#         ("old variant generation",
#          darpbenchmark.chaining.OldVariantGeneration(travel_time_provider, max_time_between_plans)),
#         ("new variant generation with unique plan departures",
#          darpbenchmark.chaining.VariantGenerationVersion2(travel_time_provider, max_time_between_plans))
#     ]
#
#     compare_old_and_new_chaining(
#         plan_data_json,
#         travel_time_provider,
#         5,
#         1000,
#         max_time_between_plans,
#         variant_generation_variants
#     )

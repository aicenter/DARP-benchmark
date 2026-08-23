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
import logging
import os
from datetime import datetime

import darpbenchmark.instance
import darpbenchmark.inout

from typing import List, Tuple

from darpbenchmark.instance import Request, DARPInstance, DARPInstanceConfiguration, Vehicle
from darpbenchmark.inout import check_file_exists


class AmodsimNode():
    def get_idx(self) -> int:
        return self.idx

    def __init__(self, idx: int):
        self.idx = idx

    def __str__(self):
        return str(self.idx)


def load_vehicles(vehicles_path: str) -> List[Vehicle]:
    veh_data = darpbenchmark.inout.load_csv(vehicles_path, "\t")
    vehicles = []
    for index, veh in enumerate(veh_data):
        vehicles.append(Vehicle(index, AmodsimNode(int(veh[0])), int(veh[1])))

    return vehicles


def read_instance(filepath: str) -> DARPInstance:
    instance_config = darpbenchmark.instance.load_instance_config(filepath)
    instance_dir_path = os.path.dirname(filepath)

    # Here, we are completing the possibly relative paths. Therefore, we need to change the dir because dm path
    # loaded from instance config is relative to the instance dir
    os.chdir(instance_dir_path)
    instance_path = instance_config['demand']['filepath']
    check_file_exists(instance_path)

    if 'dm_filepath' in instance_config:
        dm_filepath = instance_config['dm_filepath']
    # by default, the dm is located in the are folder
    else:
        dm_filepath = os.path.join(instance_config['area_dir'], 'dm.h5')
    check_file_exists(dm_filepath)

    vehicles_path = os.path.join(instance_dir_path, 'vehicles.csv')
    check_file_exists(vehicles_path)

    logging.info("Reading dm from: {}".format(os.path.realpath(dm_filepath)))
    travel_time_provider = darpbenchmark.instance.MatrixTravelTimeProvider.read_from_file(dm_filepath)

    logging.info("Reading Amodsim DARP instance from: {}".format(os.path.realpath(instance_path)))
    with open(instance_path, "r", encoding="utf-8") as infile:
        vehicles = load_vehicles(vehicles_path)

        requests: List[Request] = []

        line_string = infile.readline()
        action_id = 0
        while (line_string):
            line = line_string.split()
            request_id: int = int(line[0])
            request_time: int = int(line[1]) / 1000
            start_node = AmodsimNode(int(line[2]))
            end_node = AmodsimNode(int(line[3]))
            min_travel_time = travel_time_provider.get_travel_time(start_node, end_node)
            max_pickup_time = request_time + int(instance_config['max_prolongation'])
            requests.append(Request(request_id, action_id, start_node, request_time, max_pickup_time,
                            action_id + 1, end_node, request_time + min_travel_time,  max_pickup_time + min_travel_time,
                            min_travel_time))
            line_string = infile.readline()
            action_id += 2

        start_time = instance_config['vehicles']['start_time']
        if not isinstance(start_time, int):
            # start_datetime = datetime.strptime(start_time)
            timeparts = start_time.split(' ')[1].split(':')
            h = timeparts[0]
            m = timeparts[1]
            s = 0 if len(timeparts) == 2 else timeparts[2]
            start_time = int(h) * 3600 + int(m) * 60 + int(s)

        config = DARPInstanceConfiguration(0, 0, False, False, start_time)
        return DARPInstance(requests, vehicles, travel_time_provider, config)


@darpbenchmark.instance.MatrixTravelTimeProvider.get_travel_time.register
def _(self, from_node: AmodsimNode, to_dode: AmodsimNode):
    return self.get_travel_time(from_node.idx, to_dode.idx)

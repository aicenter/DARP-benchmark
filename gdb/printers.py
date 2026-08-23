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
import gdb.printing

def call_method(val, name):
    t = val.type.strip_typedefs()

    # If val is a reference, get the referred object.
    if t.code == gdb.TYPE_CODE_REF:
        val = val.referenced_value()
        t = val.type.strip_typedefs()

    # If val is already a pointer, use it directly.
    if t.code == gdb.TYPE_CODE_PTR:
        ptr = val
    else:
        # Otherwise take its address.
        ptr = val.address
        if ptr is None:
            raise gdb.error("Value is not addressable")
    return gdb.parse_and_eval(f"(({ptr.type}) {int(ptr):#x})->{name}()")

class ActionDataPrinter():
    def __init__(self, value):
        self.__value = value

    def to_string(self):
        # v = self.__value
        # print("TYPE:", v.type)
        # print("STRIPPED:", v.type.strip_typedefs())

        # for f in v.type.strip_typedefs().fields():
        #     print(f.name)
        # return "debug"
        request_index = call_method(self.__value, "get_request_index")
        action_type = call_method(self.__value, "get_action_type")
        return (
            # f"Action_data(request: test)"
            f"Action_data(request: {request_index}, "
            f"type: {action_type}, "
            f"arrival: {self.__value['arrival_time']}, "
            f"departure: {self.__value['departure_time']})"
        )

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("darpbenchmark")
    pp.add_printer('Action_data', '^ActionData<.*>$', ActionDataPrinter)

    return pp

def _remove_existing_printer(objfile, name):
    printers = objfile.pretty_printers if objfile is not None else gdb.pretty_printers
    for i, p in enumerate(list(printers)):
        if getattr(p, "name", None) == name:
            del printers[i]
            return True
    return False

def register_printers(objfile):
    _remove_existing_printer(objfile, "darpbenchmark")
    gdb.printing.register_pretty_printer(objfile, build_pretty_printer())


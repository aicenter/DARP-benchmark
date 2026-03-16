import gdb
import os
import sys
import importlib

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

import printers
importlib.reload(printers)

printers.register_printers(gdb.current_objfile())
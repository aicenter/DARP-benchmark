#
# Copyright (c) 2021 Czech Technical University in Prague.
#
# This file is part of the SiMoD project.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

from enum import Enum


class Method(Enum):
    IH = (0, "grey", "ooo", "IH", 'ih', 'o')
    VGA_CHAINING_BATCH_30_S = (1, "red", "+++", "prop b30", 'vga_chaining-batch_30_s', '+')
    VGA_CHAINING_BATCH_60_S = (2, "green", "///", "prop b60", 'vga_chaining-batch_60_s', 'P')
    VGA_CHAINING_BATCH_120_S = (3, "red", "+++", "prop b120", 'vga_chaining-batch_120_s', '+')
    VGA_CHAINING_BATCH_120_S_LIMITED = (4, "red", "+++", "prop b120 lim.", 'vga_chaining-batch_120_s-limited', '+')
    VGA_CHAINING_BATCH_240_S = (5, "red", "+++", "prop b240", 'vga_chaining-batch_240_s', '+')
    VGA_CHAINING_BATCH_240_S_LIMITED = (6, "red", "+++", "prop b240 lim", 'vga_chaining-batch_240_s-limited', '+')
    VGA_CHAINING_BATCH_480_S = (7, "red", "+++", "prop b480", 'vga_chaining-batch_480_s', '+')
    VGA_CHAINING_BATCH_480_S_LIMITED = (8, "red", "+++", "prop b480 lim", 'vga_chaining-batch_480_s-limited', '+')
    VGA = (9, "blue", "\\\\\\", "VGA", 'vga', 'v')
    HALNS = (10, "orange", "***", "HALNS", 'halns', '*')
    HALNS_IH = (11, "darkcyan", "---", "HALNS-IH", 'halns-ih', 's')
    HALNS_VGA = (12, "darkcyan", "---", "HALNS-IH", 'halns-vga', 's')

    def __init__(self, index, color, pattern, label, folder_name, marker):
        self.color = color
        self.index = index
        self.pattern = pattern
        self.label = label
        self.folder_name = folder_name
        self.marker = marker


methods = {method.label: method for method in Method}
folder_to_method = {method.folder_name: method for method in Method}

# labels = [exp.label for exp in Experiment]
# colors = [exp.color for exp in Experiment]
# hatches = [exp.pattern for exp in Experiment]

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

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
from pathlib import Path
import shutil
import os
import stat
import re

# result_dir = Path(r'C:\Google Drive\AIC Experiment Data\DARP\final\Results')
result_dir = Path(r'/home/fiedlda1/Experiment Data\DARP\final\Results')

dry_run = False
# dry_run = True


delete_sol_path = Path(r'/home/fiedlda1/Experiment Data/DARP/final/todo.txt')

exclude_files = re.compile('config.yaml$')

folders_to_delete = None
# files_to_delete = 'halns*/*'
files_to_delete = []

if delete_sol_path:
    with open(delete_sol_path, 'r') as del_list:
        for sol_to_delete in del_list:
            path = Path(sol_to_delete.rstrip())
            if path.is_file():
                path = path.parent
            for file in path.glob('*'):
                if not exclude_files.match(file.name):
                    files_to_delete.append(file)
else:
    for file in result_dir.rglob(f'{files_to_delete}'):
        if not exclude_files.match(file.name):
            files_to_delete.append(file)

if files_to_delete:
    for file in files_to_delete:
        print(f"Deleting file {file}")

        if not dry_run:
            file.unlink()

if folders_to_delete:
    for file in result_dir.rglob(f'{folders_to_delete}'):
        print(f"Deleting folder {file}")

        if not dry_run:
            shutil.rmtree(file, onerror=lambda func, path, _: (os.chmod(path, stat.S_IWRITE), func(path)))
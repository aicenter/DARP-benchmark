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
import os
import pathlib
import configparser


def read_config(config_paths):
    """
    Read database credentials from .ini file.
    """
    config = configparser.ConfigParser()

    read_configs = config.read(config_paths)
    if read_configs:
        print("Successfully read db config from: {}".format(str(read_configs)))
    else:
        print("Failed to find any config in {}".format([os.path.abspath(path) for path in config_paths]))
    return config


class CredentialsConfig:
    CONFIG_PATHS = [f'{pathlib.Path(__file__).parent.parent.parent}/config.ini',
                    "/home/mrkos/projects/bell/config.ini"]

    def __init__(self):
        config = read_config(self.CONFIG_PATHS)
        self.private_key_path = config['ssh'].get('private_key_path', None)
        self.private_key_phrase = config['ssh'].get('private_key_passphrase', None)

        self.db_server_port = int(config['database'].get('db_server_port', "5432"))
        self.username = config['database']['username']
        self.db_password = config['database']['db_password']


CREDENTIALS = CredentialsConfig()

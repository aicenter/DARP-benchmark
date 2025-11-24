import re


def get_param(name: str, instance_name: str) -> str:
    param_name = f"{name}_(\\d+)"
    sr = re.search(param_name, instance_name)
    value = int(sr.group(1))
    return value



import shutil
from pathlib import Path

from darpbenchmark.sizing import calculate_sizing_for_instance

TEST_INSTANCE_DATA = Path("./test_data/experiment").resolve()


def test_run_sizing():
    experiment_folder = Path("./test_data/tmp").resolve()
    try:
        shutil.copytree(TEST_INSTANCE_DATA, experiment_folder)
    except FileExistsError:
        pass
    calculate_sizing_for_instance(experiment_folder / "result" / "config.yaml")


if __name__ == '__main__':
    test_run_sizing()

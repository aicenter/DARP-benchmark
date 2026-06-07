import csv
import json
import logging
import math
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from scipy.optimize import minimize_scalar
import yaml

try:
    from cppdev.benchmarking import (
        BUILD_PARAMETERS_FILENAME,
        BenchmarkError,
        build_parameters_path_for_binary,
        check_and_copy_build_parameters,
        compare_build_parameters,
    )
except ImportError as error:
    raise ImportError(
        "The cppdev package is required for benchmark build-parameter validation. "
        "Install C:\\Workspaces\\Fido\\cpp-dev-support\\src in the Python environment."
    ) from error

logger = logging.getLogger(__name__)

RESULTS_FILE = "temporal_pruning_min_plan_length_tuning.csv"


@dataclass(frozen=True)
class TemporalPruningRunResult:
    length: int
    status: str
    return_code: int | None
    average_total_time: float | None
    performance_trials: int
    peak_memory_kib: int | None
    cost: int | None
    dropped_requests: int | None
    solution_matches_baseline: bool | None
    experiment_config: Path
    output_dir: Path
    solution_key: str | None = None

    def sort_key(self) -> float:
        if self.status != "ok":
            return math.inf
        return self.average_total_time if self.average_total_time is not None else math.inf


class TemporalPruningMinPlanLengthTuner:
    def __init__(
        self,
        instance_config_file: Path,
        working_dir: Path,
        initial_length_a: int,
        initial_length_b: int,
        executable_path: Path | str = "DARP-benchmark",
        tcount: int = 5,
        tmax: int = 0,
        previous_output_folder: Path | str | None = None,
    ):
        self.instance_config_file = instance_config_file.resolve()
        self.working_dir = working_dir.resolve()
        self.initial_length_a = initial_length_a
        self.initial_length_b = initial_length_b
        self.executable_path = self.resolve_executable_path(executable_path)
        self.tcount = tcount
        self.tmax = tmax
        self.previous_output_folder = (
            Path(previous_output_folder).resolve() if previous_output_folder is not None else None
        )
        self.results_file = self.working_dir / RESULTS_FILE
        self.baseline_solution_key: str | None = None

    def tune(self) -> TemporalPruningRunResult:
        self.validate()
        self.working_dir.mkdir(parents=True, exist_ok=True)
        self.check_build_parameters()
        results: dict[int, TemporalPruningRunResult] = {}

        baseline = self.evaluate(0, results)
        if baseline.status != "ok" or baseline.solution_key is None:
            raise RuntimeError(f"Baseline run with pruning disabled failed. See {self.results_file}")
        self.baseline_solution_key = baseline.solution_key
        baseline = self.evaluate(0, results, force_reload=True)
        results[0] = baseline

        lower = min(self.initial_length_a, self.initial_length_b)
        upper = max(self.initial_length_a, self.initial_length_b)

        self.evaluate(lower, results)
        self.evaluate(upper, results)

        if lower == upper:
            self.evaluate(lower, results)
        else:
            optimization_result = minimize_scalar(
                lambda value: self.objective(value, lower, upper, results),
                bounds=(lower, upper),
                method="bounded",
                options={"xatol": 0.5},
            )
            rounded_minimum = self.to_length(optimization_result.x, lower, upper)
            for length in range(max(lower, rounded_minimum - 2), min(upper, rounded_minimum + 2) + 1):
                self.evaluate(length, results)

        self.write_results(results.values())
        candidate_results = [
            result for result in results.values()
            if lower <= result.length <= upper
        ]
        best = min(candidate_results, key=lambda result: result.sort_key())
        if best.status != "ok":
            raise RuntimeError(f"No successful pruning tuning runs. See {self.results_file}")

        logger.info(
            "Best temporal_pruning_min_plan_length=%s, average_total_time=%s over %s trials",
            best.length,
            best.average_total_time,
            best.performance_trials,
        )
        return best

    def objective(
        self,
        value: float,
        lower: int,
        upper: int,
        results: dict[int, TemporalPruningRunResult],
    ) -> float:
        length = self.to_length(value, lower, upper)
        return self.evaluate(length, results).sort_key()

    @staticmethod
    def to_length(value: float, lower: int, upper: int) -> int:
        return min(upper, max(lower, round(value)))

    @staticmethod
    def resolve_executable_path(executable_path: Path | str) -> Path:
        executable_path_string = str(executable_path)
        if executable_path_string in {"DARP-benchmark", "DARP-benchmark.exe"}:
            resolved_from_path = shutil.which(executable_path_string)
            if resolved_from_path is not None:
                return Path(resolved_from_path).resolve()
        return Path(executable_path).resolve()

    def validate(self) -> None:
        if not self.instance_config_file.is_file():
            raise FileNotFoundError(f"Instance config not found: {self.instance_config_file}")
        if not self.executable_path.is_file():
            raise FileNotFoundError(
                f"DARP-benchmark executable not found: {self.executable_path}. "
                "Pass --executable with an explicit binary path."
            )
        for length in (self.initial_length_a, self.initial_length_b):
            if length < 0:
                raise ValueError("Initial lengths must be non-negative. Use 0 to disable pruning.")
        if self.tcount <= 1:
            raise ValueError("tcount must be greater than 1 so the tuner can compare average runtime")
        if self.tmax < 0:
            raise ValueError("tmax must be non-negative")
        if self.previous_output_folder is not None and not self.previous_output_folder.is_dir():
            raise FileNotFoundError(f"Previous output folder not found: {self.previous_output_folder}")

    def check_build_parameters(self) -> None:
        try:
            current_build_parameters_path = build_parameters_path_for_binary(self.executable_path)
            working_build_parameters_path = self.working_dir / BUILD_PARAMETERS_FILENAME

            if working_build_parameters_path.exists():
                compare_build_parameters(current_build_parameters_path, self.working_dir)
                if self.previous_output_folder is not None:
                    compare_build_parameters(current_build_parameters_path, self.previous_output_folder)
                return

            check_and_copy_build_parameters(
                self.executable_path,
                self.working_dir,
                self.previous_output_folder,
            )
        except BenchmarkError as error:
            raise RuntimeError(f"Build-parameter validation failed: {error}") from error

    def evaluate(
        self,
        length: int,
        results: dict[int, TemporalPruningRunResult],
        force_reload: bool = False,
    ) -> TemporalPruningRunResult:
        if length in results and not force_reload:
            return results[length]

        length_dir = self.working_dir / f"length_{length}"
        output_dir = length_dir / "output"
        config_file = length_dir / "config.yaml"
        output_dir.mkdir(parents=True, exist_ok=True)
        self.write_experiment_config(config_file, output_dir, length)

        solution_files = self.expected_result_files(output_dir, "solution")
        performance_files = self.expected_result_files(output_dir, "performance")

        if not self.has_complete_results(solution_files, performance_files):
            logger.info("Running temporal pruning tuning length %s", length)
            return_code = self.run_experiment(config_file, length_dir)
        else:
            logger.info("Reusing existing temporal pruning tuning length %s", length)
            return_code = 0

        result = self.read_result(length, return_code, config_file, output_dir, solution_files, performance_files)
        results[length] = result
        self.write_results(results.values())
        return result

    def expected_result_files(self, output_dir: Path, result_type: str) -> list[Path]:
        files = []
        for trial_number in range(1, self.tcount + 1):
            suffix = f"-{result_type}.json" if trial_number == 1 else f"-{result_type}-{trial_number}.json"
            files.append(output_dir / f"{self.instance_config_file.name}{suffix}")
        return files

    @staticmethod
    def has_complete_results(solution_files: list[Path], performance_files: list[Path]) -> bool:
        return all(path.is_file() for path in solution_files) and all(path.is_file() for path in performance_files)

    def write_experiment_config(self, config_file: Path, output_dir: Path, length: int) -> None:
        config = {
            "instance": str(self.instance_config_file),
            "outdir": str(output_dir),
            "method": "ih",
            "tcount": self.tcount,
            "tmax": self.tmax,
            "ih": {
                "temporal_pruning_min_plan_length": length,
            },
            "simple_csv_export": False,
        }
        with open(config_file, "wt", encoding="utf-8") as f:
            yaml.safe_dump(config, f, sort_keys=False)

    def run_experiment(self, config_file: Path, length_dir: Path) -> int:
        command = [str(self.executable_path), str(config_file)]
        try:
            completed_process = subprocess.run(
                command,
                cwd=length_dir,
                text=True,
                capture_output=True,
                check=False,
            )
        except OSError as error:
            (length_dir / "stdout.log").write_text("", encoding="utf-8")
            (length_dir / "stderr.log").write_text(str(error), encoding="utf-8")
            logger.error("Failed to launch temporal pruning tuning command %s: %s", command, error)
            return -1

        (length_dir / "stdout.log").write_text(completed_process.stdout, encoding="utf-8")
        (length_dir / "stderr.log").write_text(completed_process.stderr, encoding="utf-8")
        if completed_process.returncode != 0:
            logger.error(
                "Temporal pruning tuning run failed for %s with return code %s. See %s",
                config_file,
                completed_process.returncode,
                length_dir,
            )
        return completed_process.returncode

    def read_result(
        self,
        length: int,
        return_code: int,
        config_file: Path,
        output_dir: Path,
        solution_files: list[Path],
        performance_files: list[Path],
    ) -> TemporalPruningRunResult:
        if return_code != 0:
            return self.make_invalid_result(length, "failed", return_code, config_file, output_dir)

        if not self.has_complete_results(solution_files, performance_files):
            return self.make_invalid_result(length, "missing_result", return_code, config_file, output_dir)

        performances = []
        for performance_file in performance_files:
            with open(performance_file, "rt", encoding="utf-8") as f:
                performances.append(json.load(f))

        solutions = []
        for solution_file in solution_files:
            with open(solution_file, "rt", encoding="utf-8") as f:
                solutions.append(json.load(f))

        total_times = [performance["total_time"] for performance in performances if "total_time" in performance]
        peak_memory_values = [
            performance["peak_memory_KiB"] for performance in performances if "peak_memory_KiB" in performance
        ]
        solution_keys = [self.solution_key(solution) for solution in solutions]
        first_solution = solutions[0]
        dropped_requests = len(first_solution.get("dropped_requests", []))
        cost = first_solution.get("cost")
        solution_matches_baseline = (
            None if self.baseline_solution_key is None else all(key == self.baseline_solution_key for key in solution_keys)
        )

        status = "ok"
        if len(total_times) != self.tcount:
            status = "missing_performance_time"
        elif any(key != solution_keys[0] for key in solution_keys):
            status = "inconsistent_solution_trials"
        elif dropped_requests != 0:
            status = "dropped_requests"
        elif solution_matches_baseline is False:
            status = "solution_mismatch"

        return TemporalPruningRunResult(
            length=length,
            status=status,
            return_code=return_code,
            average_total_time=sum(total_times) / len(total_times) if total_times else None,
            performance_trials=len(performances),
            peak_memory_kib=max(peak_memory_values) if peak_memory_values else None,
            cost=cost,
            dropped_requests=dropped_requests,
            solution_matches_baseline=solution_matches_baseline,
            experiment_config=config_file,
            output_dir=output_dir,
            solution_key=solution_keys[0],
        )

    @staticmethod
    def solution_key(solution: dict) -> str:
        return json.dumps(solution, sort_keys=True, separators=(",", ":"))

    @staticmethod
    def make_invalid_result(
        length: int,
        status: str,
        return_code: int | None,
        config_file: Path,
        output_dir: Path,
    ) -> TemporalPruningRunResult:
        return TemporalPruningRunResult(
            length=length,
            status=status,
            return_code=return_code,
            average_total_time=None,
            performance_trials=0,
            peak_memory_kib=None,
            cost=None,
            dropped_requests=None,
            solution_matches_baseline=None,
            experiment_config=config_file,
            output_dir=output_dir,
        )

    def write_results(self, results: Iterable[TemporalPruningRunResult]) -> None:
        sorted_results = sorted(results, key=lambda result: result.length)
        with open(self.results_file, "wt", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([
                "length",
                "status",
                "return_code",
                "average_total_time",
                "performance_trials",
                "peak_memory_KiB",
                "cost",
                "dropped_requests",
                "solution_matches_baseline",
                "experiment_config",
                "output_dir",
            ])
            for result in sorted_results:
                writer.writerow([
                    result.length,
                    result.status,
                    "" if result.return_code is None else result.return_code,
                    "" if result.average_total_time is None else result.average_total_time,
                    result.performance_trials,
                    "" if result.peak_memory_kib is None else result.peak_memory_kib,
                    "" if result.cost is None else result.cost,
                    "" if result.dropped_requests is None else result.dropped_requests,
                    "" if result.solution_matches_baseline is None else result.solution_matches_baseline,
                    result.experiment_config,
                    result.output_dir,
                ])

import csv
import json
import logging
import math
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import yaml

logger = logging.getLogger(__name__)

RESULTS_FILE = "temporal_pruning_min_plan_length_tuning.csv"


@dataclass(frozen=True)
class TemporalPruningRunResult:
    length: int
    status: str
    return_code: int | None
    total_time: int | None
    performance_trials: int
    peak_memory_kib: int | None
    cost: int | None
    dropped_requests: int | None
    experiment_config: Path
    output_dir: Path

    def sort_key(self) -> tuple[int, int, float]:
        if self.status != "ok":
            return math.inf, math.inf, math.inf
        return (
            self.dropped_requests if self.dropped_requests is not None else math.inf,
            self.cost if self.cost is not None else math.inf,
            self.total_time if self.total_time is not None else math.inf,
        )


class TemporalPruningMinPlanLengthTuner:
    def __init__(
        self,
        instance_config_file: Path,
        working_dir: Path,
        initial_length_a: int,
        initial_length_b: int,
        executable_path: Path | str = "DARP-benchmark",
        tcount: int = 1,
        tmax: int = 0,
    ):
        self.instance_config_file = instance_config_file.resolve()
        self.working_dir = working_dir.resolve()
        self.initial_length_a = initial_length_a
        self.initial_length_b = initial_length_b
        executable_path_string = str(executable_path)
        self.executable_path = (
            executable_path_string
            if executable_path_string in {"DARP-benchmark", "DARP-benchmark.exe"}
            else Path(executable_path).resolve()
        )
        self.tcount = tcount
        self.tmax = tmax
        self.results_file = self.working_dir / RESULTS_FILE

    def tune(self) -> TemporalPruningRunResult:
        self.validate()
        self.working_dir.mkdir(parents=True, exist_ok=True)
        results: dict[int, TemporalPruningRunResult] = {}

        lower = min(self.initial_length_a, self.initial_length_b)
        upper = max(self.initial_length_a, self.initial_length_b)

        self.evaluate(lower, results)
        self.evaluate(upper, results)

        while upper - lower > 3:
            left = lower + (upper - lower) // 3
            right = upper - (upper - lower) // 3

            left_result = self.evaluate(left, results)
            right_result = self.evaluate(right, results)

            if left_result.sort_key() <= right_result.sort_key():
                upper = right - 1
            else:
                lower = left + 1

        for length in range(lower, upper + 1):
            self.evaluate(length, results)

        best = min(results.values(), key=lambda result: result.sort_key())
        if best.status != "ok":
            raise RuntimeError(f"No successful pruning tuning runs. See {self.results_file}")

        logger.info(
            "Best temporal_pruning_min_plan_length=%s, total_time=%s, cost=%s, dropped_requests=%s",
            best.length,
            best.total_time,
            best.cost,
            best.dropped_requests,
        )
        return best

    def validate(self) -> None:
        if not self.instance_config_file.is_file():
            raise FileNotFoundError(f"Instance config not found: {self.instance_config_file}")
        for length in (self.initial_length_a, self.initial_length_b):
            if length < 0:
                raise ValueError("Initial lengths must be non-negative. Use 0 to disable pruning.")
        if self.tcount <= 0:
            raise ValueError("tcount must be positive")
        if self.tmax < 0:
            raise ValueError("tmax must be non-negative")

    def evaluate(
        self,
        length: int,
        results: dict[int, TemporalPruningRunResult],
    ) -> TemporalPruningRunResult:
        if length in results:
            return results[length]

        length_dir = self.working_dir / f"length_{length}"
        output_dir = length_dir / "output"
        config_file = length_dir / "config.yaml"
        output_dir.mkdir(parents=True, exist_ok=True)
        self.write_experiment_config(config_file, output_dir, length)

        solution_file = output_dir / f"{self.instance_config_file.name}-solution.json"
        performance_file = output_dir / f"{self.instance_config_file.name}-performance.json"

        if not solution_file.is_file() or not performance_file.is_file():
            logger.info("Running temporal pruning tuning length %s", length)
            return_code = self.run_experiment(config_file, length_dir)
        else:
            logger.info("Reusing existing temporal pruning tuning length %s", length)
            return_code = 0

        result = self.read_result(length, return_code, config_file, output_dir, solution_file, performance_file)
        results[length] = result
        self.write_results(results.values())
        return result

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
        solution_file: Path,
        performance_file: Path,
    ) -> TemporalPruningRunResult:
        if return_code != 0:
            return TemporalPruningRunResult(
                length,
                "failed",
                return_code,
                None,
                0,
                None,
                None,
                None,
                config_file,
                output_dir,
            )

        if not solution_file.is_file() or not performance_file.is_file():
            return TemporalPruningRunResult(
                length,
                "missing_result",
                return_code,
                None,
                0,
                None,
                None,
                None,
                config_file,
                output_dir,
            )

        performance_files = sorted(output_dir.glob(f"{self.instance_config_file.name}-performance*.json"))
        performances = []
        for current_performance_file in performance_files:
            with open(current_performance_file, "rt", encoding="utf-8") as f:
                performances.append(json.load(f))
        with open(solution_file, "rt", encoding="utf-8") as f:
            solution = json.load(f)

        total_times = [performance["total_time"] for performance in performances if "total_time" in performance]
        peak_memory_values = [
            performance["peak_memory_KiB"] for performance in performances if "peak_memory_KiB" in performance
        ]

        return TemporalPruningRunResult(
            length=length,
            status="ok",
            return_code=return_code,
            total_time=round(sum(total_times) / len(total_times)) if total_times else None,
            performance_trials=len(performances),
            peak_memory_kib=max(peak_memory_values) if peak_memory_values else None,
            cost=solution.get("cost"),
            dropped_requests=len(solution.get("dropped_requests", [])),
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
                "total_time",
                "performance_trials",
                "peak_memory_KiB",
                "cost",
                "dropped_requests",
                "experiment_config",
                "output_dir",
            ])
            for result in sorted_results:
                writer.writerow([
                    result.length,
                    result.status,
                    "" if result.return_code is None else result.return_code,
                    "" if result.total_time is None else result.total_time,
                    result.performance_trials,
                    "" if result.peak_memory_kib is None else result.peak_memory_kib,
                    "" if result.cost is None else result.cost,
                    "" if result.dropped_requests is None else result.dropped_requests,
                    result.experiment_config,
                    result.output_dir,
                ])

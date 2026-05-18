#!/usr/bin/env python3
"""Run CUTEst problems with the local NLopt/SLSQP package."""

from __future__ import annotations

import argparse
import csv
import json
import os
import pathlib
import signal
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone


PASS_STATUSES = {"SUCCESS", "STOPVAL_REACHED", "FTOL_REACHED", "XTOL_REACHED"}


def read_problem_file(path):
    problems = []
    with open(path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f, delimiter="\t")
        if reader.fieldnames and "problem" in reader.fieldnames:
            for row in reader:
                p = (row.get("problem") or "").strip()
                if p and not p.startswith("#"):
                    problems.append(p)
            return problems
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            p = line.strip().split()[0] if line.strip() else ""
            if p and not p.startswith("#"):
                problems.append(p)
    return problems


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[3]


def classify(row, timeout=False, returncode=0, gradient_tol=1e-5):
    if timeout:
        return "driver_timeout"
    if returncode != 0 and not row:
        return "driver_error"
    if not row:
        return "no_solver_status"
    status = row.get("status_name") or ""
    try:
        maxv = float(row.get("max_constraint_violation", "nan"))
    except Exception:
        maxv = float("nan")
    try:
        m = int(str(row.get("m", "0")).strip() or "0")
    except Exception:
        m = 0
    try:
        pg = float(row.get("projected_gradient_inf", "nan"))
    except Exception:
        pg = float("nan")
    if status in PASS_STATUSES and maxv <= 1e-6:
        if m == 0 and pg == pg and pg > gradient_tol:
            return "success_high_gradient"
        return "strict_success"
    if status == "MAXEVAL_REACHED":
        return "max_iter"
    if status == "MAXTIME_REACHED":
        return "max_time"
    if status == "ROUNDOFF_LIMITED":
        return "roundoff_limited"
    if status == "FORCED_STOP":
        return "forced_stop"
    if status.startswith("INVALID") or status.startswith("FAILURE"):
        return "solver_failure"
    return "other_solver_status"


def parse_json_line(output):
    for line in reversed(output.splitlines()):
        text = line.strip()
        if text.startswith("{") and text.endswith("}"):
            return json.loads(text)
    return None


def safe_path_part(value: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in value.strip())


def job_workdir(problem: str, args, job_index: int) -> str:
    base = pathlib.Path(args.workdir)
    if args.jobs <= 1:
        base.mkdir(parents=True, exist_ok=True)
        return str(base)
    path = base / f"slsqp_cutest_{job_index:04d}_{safe_path_part(problem)}"
    path.mkdir(parents=True, exist_ok=True)
    return str(path)


def install_cutest_package(root: pathlib.Path, cutest_root: pathlib.Path, env: dict[str, str]) -> None:
    install = root / "solvers/slsqp/cutest/install_cutest_package.sh"
    install_env = env.copy()
    install_env["CUTEST"] = str(cutest_root)
    subprocess.run([str(install)], check=True, cwd=root, env=install_env)


def prepare_worker_envs(args, root: pathlib.Path, base_env: dict[str, str]) -> list[dict[str, str]]:
    if args.jobs <= 1:
        return [base_env]

    source_cutest = pathlib.Path(args.cutest)
    worker_parent = pathlib.Path(args.workdir) / "cutest_worker_roots"
    if worker_parent.exists():
        shutil.rmtree(worker_parent)
    worker_parent.mkdir(parents=True, exist_ok=True)

    worker_envs: list[dict[str, str]] = []
    for worker_id in range(args.jobs):
        worker_cutest = worker_parent / f"cutest_{worker_id:02d}"
        shutil.copytree(source_cutest, worker_cutest, symlinks=True)
        worker_env = base_env.copy()
        worker_env["CUTEST"] = str(worker_cutest)
        install_cutest_package(root, worker_cutest, worker_env)
        worker_envs.append(worker_env)
    return worker_envs


def result_profile(args) -> str:
    return args.profile_name or "SLSQP_NLOPT"


def run_one(problem: str, args, env: dict[str, str], job_index: int = 0) -> dict[str, object]:
    cmd = [args.runcutest, "-p", "slsqp", "-D", problem]
    if args.rebuild:
        cmd.insert(1, "-r")

    def output_text(value: str | bytes | None) -> str:
        if value is None:
            return ""
        if isinstance(value, bytes):
            return value.decode(errors="replace")
        return value

    log_dir = pathlib.Path(args.log_dir)
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / f"{problem}.slsqp.log"

    timed_out = False
    stdout = ""
    returncode: int | None = None
    proc: subprocess.Popen[str] | None = None
    try:
        proc = subprocess.Popen(
            cmd,
            cwd=job_workdir(problem, args, job_index),
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        stdout, _ = proc.communicate(timeout=args.timeout_sec)
        returncode = proc.returncode
    except subprocess.TimeoutExpired as exc:
        timed_out = True
        if proc is not None:
            try:
                os.killpg(proc.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                tail, _ = proc.communicate(timeout=2)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                tail, _ = proc.communicate()
            stdout = output_text(exc.stdout) + output_text(tail)
        stdout += f"\nRUNNER_TIMEOUT: exceeded {args.timeout_sec} seconds\n"

    log_path.write_text(stdout, encoding="utf-8", errors="replace")
    row = parse_json_line(stdout) or {}
    row["problem_requested"] = problem
    row["package"] = "slsqp"
    row["driver_returncode"] = returncode
    row["timestamp_utc"] = datetime.now(timezone.utc).isoformat()
    row["log"] = str(log_path)
    if timed_out:
        row["driver_error"] = "timeout"
        row["timeout_sec"] = args.timeout_sec
    row["outcome_class"] = classify(
        row,
        timeout=timed_out,
        returncode=returncode or 0,
        gradient_tol=args.gradient_tol,
    )
    return row


def run_worker_jobs(worker_jobs: list[tuple[int, str]], args, env: dict[str, str]) -> dict[int, dict[str, object]]:
    results: dict[int, dict[str, object]] = {}
    for job_index, problem in worker_jobs:
        results[job_index] = run_one(problem, args, env, job_index)
    return results


def write_tsv_results(rows: list[dict[str, object]], args) -> None:
    if not args.tsv_out:
        return
    fields = [
        "profile",
        "solver",
        "hessian",
        "kkt_convergence",
        "acceptable_iter",
        "problem",
        "problem_requested",
        "classification",
        "n",
        "m",
        "status",
        "status_name",
        "outcome_class",
        "used_lsq",
        "objective",
        "max_constraint_violation",
        "objective_gradient_inf",
        "kkt_error",
        "projected_gradient_inf",
        "iterations",
        "obj_evals",
        "grad_evals",
        "con_evals",
        "jac_evals",
        "solve_time",
        "setup_time",
        "driver_error",
        "log",
    ]
    with open(args.tsv_out, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, delimiter="\t", fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        for row in rows:
            tsv_row = dict(row)
            tsv_row["profile"] = result_profile(args)
            tsv_row["solver"] = "slsqp"
            tsv_row["hessian"] = "n/a"
            tsv_row["kkt_convergence"] = "n/a"
            tsv_row["acceptable_iter"] = "n/a"
            tsv_row["problem"] = str(row.get("problem") or row.get("problem_requested") or "").strip()
            tsv_row["classification"] = str(row.get("classification") or "").strip()
            tsv_row["used_lsq"] = ""
            tsv_row["solve_time"] = row.get("cutest_solve_time", "")
            tsv_row["setup_time"] = row.get("cutest_setup_time", "")
            writer.writerow(tsv_row)


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("problems", nargs="*")
    parser.add_argument("--problem-file")
    parser.add_argument("--out", default="slsqp_cutest_results.jsonl")
    parser.add_argument("--tsv-out")
    parser.add_argument("--profile-name", default="SLSQP_NLOPT")
    parser.add_argument("--log-dir", default="slsqp_cutest_logs")
    parser.add_argument("--workdir", default="/tmp")
    parser.add_argument("--timeout-sec", type=float, default=30.0)
    parser.add_argument("--rebuild", action="store_true")
    parser.add_argument("--cutest", default=os.environ.get("CUTEST", "/home/john/CUTEst"))
    parser.add_argument("--runcutest", default=os.environ.get("RUNCUTEST", "runcutest"))
    parser.add_argument("--ascend-root", default=str(repo_root()))
    parser.add_argument("--max-iter", type=int, default=200)
    parser.add_argument("--constraint-tol", type=float, default=1e-8)
    parser.add_argument("--gradient-tol", type=float, default=1e-5)
    parser.add_argument("--jobs", type=int, default=1, help="Number of parallel isolated CUTEst workers")
    args = parser.parse_args(argv)

    problems = list(args.problems)
    if args.problem_file:
        problems.extend(read_problem_file(args.problem_file))
    if not problems:
        problems = ["HS21"]

    root = pathlib.Path(args.ascend_root).resolve()
    log_dir = pathlib.Path(args.log_dir)
    log_dir.mkdir(parents=True, exist_ok=True)
    out_path = pathlib.Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["CUTEST"] = str(pathlib.Path(args.cutest).resolve())
    env["ASCEND_ROOT"] = str(root)
    env["SLSQP_MAX_ITER"] = str(args.max_iter)
    env["SLSQP_CONSTRAINT_TOL"] = str(args.constraint_tol)
    ld = [str(root), str(root / "solvers/slsqp")]
    if env.get("LD_LIBRARY_PATH"):
        ld.append(env["LD_LIBRARY_PATH"])
    env["LD_LIBRARY_PATH"] = os.pathsep.join(ld)

    install_cutest_package(root, pathlib.Path(args.cutest).resolve(), env)
    worker_envs = prepare_worker_envs(args, root, env)

    rows: list[dict[str, object]] = []
    with out_path.open("w", encoding="utf-8") as out:
        if args.jobs <= 1:
            for job_index, problem in enumerate(problems):
                row = run_one(problem, args, env, job_index)
                out.write(json.dumps(row, sort_keys=True) + "\n")
                out.flush()
                rows.append(row)
                print(json.dumps(row, sort_keys=True))
        else:
            worker_jobs: list[list[tuple[int, str]]] = [[] for _ in range(args.jobs)]
            for job_index, problem in enumerate(problems):
                worker_jobs[job_index % args.jobs].append((job_index, problem))

            futures = {}
            results: dict[int, dict[str, object]] = {}
            with ThreadPoolExecutor(max_workers=args.jobs) as pool:
                for worker_id, jobs_for_worker in enumerate(worker_jobs):
                    future = pool.submit(run_worker_jobs, jobs_for_worker, args, worker_envs[worker_id])
                    futures[future] = worker_id
                for future in as_completed(futures):
                    worker_id = futures[future]
                    try:
                        worker_results = future.result()
                    except Exception as exc:  # pragma: no cover - defensive runner path
                        worker_results = {}
                        for job_index, problem in worker_jobs[worker_id]:
                            row = {
                                "solver": "SLSQP",
                                "problem": problem,
                                "problem_requested": problem,
                                "package": "slsqp",
                                "status": None,
                                "driver_error": f"runner_exception: {exc}",
                                "timestamp_utc": datetime.now(timezone.utc).isoformat(),
                            }
                            row["outcome_class"] = classify(row, returncode=1, gradient_tol=args.gradient_tol)
                            worker_results[job_index] = row
                    for job_index, row in sorted(worker_results.items()):
                        results[job_index] = row
                        print(json.dumps(row, sort_keys=True))
            for job_index in range(len(problems)):
                row = results[job_index]
                out.write(json.dumps(row, sort_keys=True) + "\n")
                rows.append(row)
            out.flush()

    write_tsv_results(rows, args)

    return 0


if __name__ == "__main__":
    sys.exit(main())

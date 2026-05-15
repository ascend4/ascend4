#!/usr/bin/env python3
"""Run A4SQP/IPOPTC CUTEst packages over a problem list and collect JSONL."""

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


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[3]


def load_problem_names(args: argparse.Namespace) -> list[str]:
    names: list[str] = []
    for item in args.problem:
        names.append(item.strip())
    if args.problem_file:
        for line in pathlib.Path(args.problem_file).read_text().splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name = line.split()[0]
            if name.lower() == "problem":
                continue
            names.append(name)
    return names


def packages_for_solver(solver: str) -> list[str]:
    if solver == "both":
        return ["a4sqp", "ipoptc"]
    return [solver]


def classify_result(result: dict[str, object], args: argparse.Namespace) -> str:
    status = result.get("status")
    if result.get("driver_error") == "timeout":
        return "driver_timeout"
    if result.get("driver_error"):
        return "driver_error"
    if status == 0:
        kkt = result.get("kkt_error")
        if kkt is not None and float(kkt) > max(args.acceptable_tol, args.tol):
            return "strict_success_high_kkt"
        return "strict_success"
    if status == 1:
        kkt = result.get("kkt_error")
        if kkt is not None and float(kkt) > max(args.acceptable_tol, args.tol):
            return "acceptable_success_high_kkt"
        return "acceptable_success"
    if status == -1:
        maxvio = float(result.get("max_constraint_violation") or 0.0)
        pg = float(result.get("projected_gradient_inf") or 0.0)
        kkt = result.get("kkt_error")
        kkt_value = float(kkt) if kkt is not None else None
        near_tol = max(args.acceptable_tol, args.tol)
        if kkt_value is not None and kkt_value <= near_tol:
            return "max_iter_near_solved"
        if kkt_value is None and maxvio <= near_tol and pg <= near_tol:
            return "max_iter_near_solved"
        if maxvio <= near_tol:
            return "max_iter_stationarity"
        return "max_iter_infeasible_or_stalled"
    if status == -3:
        if int(result.get("line_search_failures") or 0) > 0:
            return "line_search_error"
        if int(result.get("qp_failures") or 0) > 0:
            return "qp_failure"
        return "step_computation_error"
    if status is None:
        return "no_solver_status"
    return "other_solver_failure"


def run_one(
    problem: str,
    package: str,
    args: argparse.Namespace,
    env: dict[str, str],
    job_index: int = 0,
) -> dict[str, object]:
    cmd = [
        args.runcutest,
        "-p",
        package,
        "-D",
        problem,
    ]
    if args.rebuild:
        cmd.append("-r")
    if args.keep:
        cmd.append("-k")
    def output_text(value: str | bytes | None) -> str:
        if value is None:
            return ""
        if isinstance(value, bytes):
            return value.decode(errors="replace")
        return value

    timed_out = False
    stdout = ""
    returncode: int | None
    proc: subprocess.Popen[str] | None = None
    try:
        proc = subprocess.Popen(
            cmd,
            cwd=job_workdir(problem, package, args, job_index),
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
        returncode = None
    result: dict[str, object] | None = None
    for line in stdout.splitlines():
        line = line.strip()
        if line.startswith("{") and line.endswith("}"):
            try:
                result = json.loads(line)
            except json.JSONDecodeError:
                pass
    if result is None:
        result = {
            "solver": package.upper(),
            "problem": problem,
            "status": None,
            "driver_error": "timeout" if timed_out else "no_json_result",
        }
    if timed_out:
        result["driver_error"] = "timeout"
        result["timeout_sec"] = args.timeout_sec
    result["package"] = package
    result["problem_requested"] = problem
    result["driver_returncode"] = returncode
    result["timestamp_utc"] = datetime.now(timezone.utc).isoformat()
    result["outcome_class"] = classify_result(result, args)
    if args.log_dir:
        log_dir = pathlib.Path(args.log_dir)
        log_dir.mkdir(parents=True, exist_ok=True)
        log_path = log_dir / f"{problem}.{package}.log"
        log_path.write_text(stdout)
        result["log"] = str(log_path)
    return result


def safe_path_part(value: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in value.strip())


def job_workdir(problem: str, package: str, args: argparse.Namespace, job_index: int) -> str:
    base = pathlib.Path(args.workdir)
    if args.jobs <= 1:
        base.mkdir(parents=True, exist_ok=True)
        return str(base)
    path = base / f"a4sqp_cutest_{job_index:04d}_{safe_path_part(problem)}_{safe_path_part(package)}"
    path.mkdir(parents=True, exist_ok=True)
    return str(path)


def install_cutest_package(root: pathlib.Path, cutest_root: pathlib.Path, env: dict[str, str]) -> None:
    install = root / "solvers" / "a4sqp" / "cutest" / "install_cutest_package.sh"
    install_env = env.copy()
    install_env["CUTEST"] = str(cutest_root)
    subprocess.run([str(install)], cwd=root, env=install_env, check=True)


def prepare_worker_envs(
    args: argparse.Namespace,
    root: pathlib.Path,
    base_env: dict[str, str],
) -> list[dict[str, str]]:
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


def run_worker_jobs(
    worker_id: int,
    worker_jobs: list[tuple[int, str, str]],
    args: argparse.Namespace,
    env: dict[str, str],
) -> dict[int, dict[str, object]]:
    results: dict[int, dict[str, object]] = {}
    for job_index, problem, package in worker_jobs:
        results[job_index] = run_one(problem, package, args, env, job_index)
    return results


def result_profile(result: dict[str, object], args: argparse.Namespace) -> str:
    if args.profile_name:
        return args.profile_name
    package = str(result.get("package") or "").lower()
    if package == "ipoptc":
        return f"ipoptc_{args.ipopt_hessian}"
    if package == "a4sqp":
        suffix = "kkt" if args.kkt_convergence else "legacy"
        if args.acceptable_iter:
            suffix += "_acc"
        return f"a4sqp_{args.a4sqp_hessian.lower()}_{suffix}"
    return package or "profile"


def result_hessian(result: dict[str, object], args: argparse.Namespace) -> str:
    package = str(result.get("package") or "").lower()
    if package == "ipoptc":
        return args.ipopt_hessian
    if package == "a4sqp":
        return args.a4sqp_hessian
    return ""


def write_tsv_results(results: list[dict[str, object]], args: argparse.Namespace) -> None:
    if not args.tsv_out:
        return
    fields = [
        "profile",
        "solver",
        "hessian",
        "kkt_convergence",
        "acceptable_iter",
        "problem",
        "classification",
        "n",
        "m",
        "status",
        "outcome_class",
        "used_lsq",
        "lsq_residuals",
        "lsq_probe_status",
        "objective",
        "max_constraint_violation",
        "kkt_error",
        "projected_gradient_inf",
        "iterations",
        "qp_solves",
        "qp_failures",
        "line_search_failures",
        "solve_time",
        "setup_time",
        "driver_error",
        "log",
    ]
    out_path = pathlib.Path(args.tsv_out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, delimiter="\t", fieldnames=fields)
        writer.writeheader()
        for result in results:
            package = str(result.get("package") or "").lower()
            is_a4sqp = package == "a4sqp"
            writer.writerow(
                {
                    "profile": result_profile(result, args),
                    "solver": package or result.get("solver", ""),
                    "hessian": result_hessian(result, args),
                    "kkt_convergence": int(args.kkt_convergence) if is_a4sqp else "n/a",
                    "acceptable_iter": args.acceptable_iter if is_a4sqp else "n/a",
                    "problem": result.get("problem") or result.get("problem_requested", ""),
                    "classification": result.get("classification", ""),
                    "n": result.get("n", ""),
                    "m": result.get("m", ""),
                    "status": result.get("status", ""),
                    "outcome_class": result.get("outcome_class", ""),
                    "used_lsq": result.get("used_lsq", ""),
                    "lsq_residuals": result.get("lsq_residuals", ""),
                    "lsq_probe_status": result.get("lsq_probe_status", ""),
                    "objective": result.get("objective", ""),
                    "max_constraint_violation": result.get("max_constraint_violation", ""),
                    "kkt_error": result.get("kkt_error", ""),
                    "projected_gradient_inf": result.get("projected_gradient_inf", ""),
                    "iterations": result.get("iterations", ""),
                    "qp_solves": result.get("qp_solves", ""),
                    "qp_failures": result.get("qp_failures", ""),
                    "line_search_failures": result.get("line_search_failures", ""),
                    "solve_time": result.get("cutest_solve_time", ""),
                    "setup_time": result.get("cutest_setup_time", ""),
                    "driver_error": result.get("driver_error", ""),
                    "log": result.get("log", ""),
                }
            )


def main(argv: list[str]) -> int:
    root = repo_root()
    parser = argparse.ArgumentParser()
    parser.add_argument("problem", nargs="*", help="CUTEst problem names")
    parser.add_argument("--problem-file", help="File containing problem names")
    parser.add_argument("--solver", choices=["a4sqp", "ipoptc", "both"], default="a4sqp")
    parser.add_argument("--out", default="cutest_a4sqp_ipoptc_results.jsonl")
    parser.add_argument("--tsv-out", help="Optional per-problem TSV result file")
    parser.add_argument("--profile-name", help="Stable profile name for TSV output")
    parser.add_argument("--log-dir", default="cutest_a4sqp_ipoptc_logs")
    parser.add_argument("--workdir", default="/tmp")
    parser.add_argument("--cutest", default=os.environ.get("CUTEST", "/home/john/CUTEst"))
    parser.add_argument("--sifdecode", default=os.environ.get("SIFDECODE", "/home/john/sifdecode"))
    parser.add_argument("--archdefs", default=os.environ.get("ARCHDEFS", "/home/john/archdefs"))
    parser.add_argument("--runcutest", default=os.environ.get("RUNCUTEST", "runcutest"))
    parser.add_argument("--max-iter", type=int, default=200)
    parser.add_argument("--tol", type=float, default=1e-7)
    parser.add_argument("--acceptable-iter", type=int, default=int(os.environ.get("A4SQP_ACCEPTABLE_ITER", "0")))
    parser.add_argument("--acceptable-tol", type=float, default=float(os.environ.get("A4SQP_ACCEPTABLE_TOL", "1e-5")))
    parser.add_argument("--filter-accept", action="store_true", default=os.environ.get("A4SQP_FILTER_ACCEPT", "0") not in ("", "0", "false", "False"))
    parser.add_argument("--filter-margin", type=float, default=float(os.environ.get("A4SQP_FILTER_MARGIN", "1e-4")))
    parser.add_argument("--trust-unconstrained", action="store_true", default=os.environ.get("A4SQP_TRUST_UNCONSTRAINED", "0") not in ("", "0", "false", "False"))
    parser.add_argument("--restoration", action="store_true", default=os.environ.get("A4SQP_RESTORATION", "0") not in ("", "0", "false", "False"))
    parser.add_argument("--restoration-trigger-iter", type=int, default=int(os.environ.get("A4SQP_RESTORATION_TRIGGER_ITER", "3")))
    parser.add_argument("--restoration-max-iter", type=int, default=int(os.environ.get("A4SQP_RESTORATION_MAX_ITER", "0")))
    parser.add_argument("--restoration-improve", type=float, default=float(os.environ.get("A4SQP_RESTORATION_IMPROVE", "1e-3")))
    parser.add_argument("--restoration-margin", type=float, default=float(os.environ.get("A4SQP_RESTORATION_MARGIN", "1e-4")))
    parser.add_argument("--restoration-handoff-reduction", type=float, default=float(os.environ.get("A4SQP_RESTORATION_HANDOFF_REDUCTION", "0.5")))
    parser.add_argument("--restoration-reentry-factor", type=float, default=float(os.environ.get("A4SQP_RESTORATION_REENTRY_FACTOR", "1.0")))
    parser.set_defaults(kkt_convergence=os.environ.get("A4SQP_KKT_CONVERGENCE", "1") not in ("", "0", "false", "False"))
    parser.add_argument("--kkt-convergence", dest="kkt_convergence", action="store_true")
    parser.add_argument("--no-kkt-convergence", dest="kkt_convergence", action="store_false")
    parser.add_argument("--elastic-penalty", type=float, default=100.0)
    parser.add_argument("--elastic-penalty-growth", type=float, default=float(os.environ.get("A4SQP_ELASTIC_PENALTY_GROWTH", "10")))
    parser.add_argument("--elastic-penalty-max", type=float, default=float(os.environ.get("A4SQP_ELASTIC_PENALTY_MAX", "1e8")))
    parser.add_argument("--a4sqp-hessian", default=os.environ.get("A4SQP_HESSIAN", "BFGS"))
    parser.add_argument("--a4sqp-scaleopt", default=os.environ.get("A4SQP_SCALEOPT", "ROW_2NORM"))
    parser.add_argument("--try-lsq", default=os.environ.get("A4SQP_TRY_LSQ", "LM"))
    parser.add_argument("--a4sqp-hess-reg", type=float, default=float(os.environ.get("A4SQP_HESS_REG", "1e-8")))
    parser.add_argument("--a4sqp-bound-push", type=float, default=float(os.environ.get("A4SQP_BOUND_PUSH", "1e-8")))
    parser.add_argument("--a4sqp-qp-time-limit", type=float, default=float(os.environ.get("A4SQP_QP_TIME_LIMIT", "0")))
    parser.add_argument("--a4sqp-qp-iteration-limit", type=int, default=int(os.environ.get("A4SQP_QP_ITERATION_LIMIT", "0")))
    parser.add_argument("--ipopt-max-iter", type=int)
    parser.add_argument("--ipopt-tol", type=float)
    parser.add_argument("--ipopt-hessian", default="limited-memory")
    parser.add_argument("--ipopt-print-level", type=int, default=0)
    parser.add_argument("--rebuild", action="store_true")
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--timeout-sec", type=float, help="Per runcutest invocation timeout")
    parser.add_argument("--jobs", type=int, default=1, help="Number of parallel runcutest worker processes")
    args = parser.parse_args(argv)

    problems = load_problem_names(args)
    if not problems:
        parser.error("provide at least one problem or --problem-file")

    env = os.environ.copy()
    env["CUTEST"] = args.cutest
    env["SIFDECODE"] = args.sifdecode
    env["ARCHDEFS"] = args.archdefs
    env["ASCEND_ROOT"] = str(root)
    env["A4SQP_MAX_ITER"] = str(args.max_iter)
    env["A4SQP_TOL"] = str(args.tol)
    env["A4SQP_ACCEPTABLE_ITER"] = str(args.acceptable_iter)
    env["A4SQP_ACCEPTABLE_TOL"] = str(args.acceptable_tol)
    env["A4SQP_FILTER_ACCEPT"] = "1" if args.filter_accept else "0"
    env["A4SQP_FILTER_MARGIN"] = str(args.filter_margin)
    env["A4SQP_TRUST_UNCONSTRAINED"] = "1" if args.trust_unconstrained else "0"
    env["A4SQP_RESTORATION"] = "1" if args.restoration else "0"
    env["A4SQP_RESTORATION_TRIGGER_ITER"] = str(args.restoration_trigger_iter)
    env["A4SQP_RESTORATION_MAX_ITER"] = str(args.restoration_max_iter)
    env["A4SQP_RESTORATION_IMPROVE"] = str(args.restoration_improve)
    env["A4SQP_RESTORATION_MARGIN"] = str(args.restoration_margin)
    env["A4SQP_RESTORATION_HANDOFF_REDUCTION"] = str(args.restoration_handoff_reduction)
    env["A4SQP_RESTORATION_REENTRY_FACTOR"] = str(args.restoration_reentry_factor)
    env["A4SQP_KKT_CONVERGENCE"] = "1" if args.kkt_convergence else "0"
    env["A4SQP_ELASTIC_PENALTY"] = str(args.elastic_penalty)
    env["A4SQP_ELASTIC_PENALTY_GROWTH"] = str(args.elastic_penalty_growth)
    env["A4SQP_ELASTIC_PENALTY_MAX"] = str(args.elastic_penalty_max)
    env["A4SQP_HESSIAN"] = args.a4sqp_hessian
    env["A4SQP_SCALEOPT"] = args.a4sqp_scaleopt
    env["A4SQP_TRY_LSQ"] = args.try_lsq
    env["A4SQP_HESS_REG"] = str(args.a4sqp_hess_reg)
    env["A4SQP_BOUND_PUSH"] = str(args.a4sqp_bound_push)
    env["A4SQP_QP_TIME_LIMIT"] = str(args.a4sqp_qp_time_limit)
    env["A4SQP_QP_ITERATION_LIMIT"] = str(args.a4sqp_qp_iteration_limit)
    env["IPOPTC_MAX_ITER"] = str(args.ipopt_max_iter if args.ipopt_max_iter is not None else args.max_iter)
    env["IPOPTC_TOL"] = str(args.ipopt_tol if args.ipopt_tol is not None else args.tol)
    env["IPOPTC_HESSIAN"] = args.ipopt_hessian
    env["IPOPTC_PRINT_LEVEL"] = str(args.ipopt_print_level)
    lib_paths = [
        str(root / "solvers" / "a4sqp"),
        str(root),
        "/home/john/.local/lib",
    ]
    env["LD_LIBRARY_PATH"] = ":".join(lib_paths + [env.get("LD_LIBRARY_PATH", "")])

    install_cutest_package(root, pathlib.Path(args.cutest), env)
    worker_envs = prepare_worker_envs(args, root, env)

    jobs = [(problem, package) for problem in problems for package in packages_for_solver(args.solver)]
    out_path = pathlib.Path(args.out)
    ordered_results: list[dict[str, object]] = []
    with out_path.open("a", encoding="utf-8") as out:
        if args.jobs <= 1:
            for job_index, (problem, package) in enumerate(jobs):
                result = run_one(problem, package, args, env, job_index)
                ordered_results.append(result)
                out.write(json.dumps(result, sort_keys=True) + "\n")
                out.flush()
                print(json.dumps(result, sort_keys=True))
        else:
            workdir_root = pathlib.Path(args.workdir)
            workdir_root.mkdir(parents=True, exist_ok=True)
            worker_jobs: list[list[tuple[int, str, str]]] = [[] for _ in range(args.jobs)]
            for job_index, (problem, package) in enumerate(jobs):
                worker_jobs[job_index % args.jobs].append((job_index, problem, package))

            futures = {}
            results: dict[int, dict[str, object]] = {}
            with ThreadPoolExecutor(max_workers=args.jobs) as pool:
                for worker_id, jobs_for_worker in enumerate(worker_jobs):
                    future = pool.submit(
                        run_worker_jobs,
                        worker_id,
                        jobs_for_worker,
                        args,
                        worker_envs[worker_id],
                    )
                    futures[future] = worker_id
                for future in as_completed(futures):
                    worker_id = futures[future]
                    try:
                        worker_results = future.result()
                    except Exception as exc:  # pragma: no cover - defensive runner path
                        worker_results = {}
                        for job_index, problem, package in worker_jobs[worker_id]:
                            result = {
                                "solver": package.upper(),
                                "problem": problem,
                                "package": package,
                                "problem_requested": problem,
                                "status": None,
                                "driver_error": f"runner_exception: {exc}",
                                "timestamp_utc": datetime.now(timezone.utc).isoformat(),
                            }
                            result["outcome_class"] = classify_result(result, args)
                            worker_results[job_index] = result
                    for job_index, result in sorted(worker_results.items()):
                        results[job_index] = result
                        print(json.dumps(result, sort_keys=True))
            for job_index in range(len(jobs)):
                result = results[job_index]
                ordered_results.append(result)
                out.write(json.dumps(result, sort_keys=True) + "\n")
            out.flush()
    write_tsv_results(ordered_results, args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

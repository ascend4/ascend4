#!/usr/bin/env python3
"""Run standard CUTEst profiles and regenerate a progress Markdown report."""

from __future__ import annotations

import argparse
import datetime as _datetime
import pathlib
import subprocess
import sys


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[3]


def run(cmd: list[str], cwd: pathlib.Path, dry_run: bool) -> None:
    print("+ " + " ".join(cmd))
    if not dry_run:
        subprocess.run(cmd, cwd=cwd, check=True)


def default_run_dir(problem_set: pathlib.Path) -> pathlib.Path:
    stamp = _datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    return pathlib.Path("/tmp") / f"a4sqp_cutest_{problem_set.stem}_{stamp}"


def profile_run_command(
    runner: pathlib.Path,
    args: argparse.Namespace,
    run_dir: pathlib.Path,
    profile_name: str,
    solver_args: list[str],
) -> tuple[list[str], pathlib.Path]:
    jsonl = run_dir / f"{profile_name}.jsonl"
    tsv = run_dir / f"{profile_name}.tsv"
    log_dir = run_dir / "logs" / profile_name
    cmd = [
        sys.executable,
        str(runner),
        "--problem-file",
        str(args.problem_set),
        "--out",
        str(jsonl),
        "--tsv-out",
        str(tsv),
        "--profile-name",
        profile_name,
        "--log-dir",
        str(log_dir),
        "--workdir",
        str(run_dir / "work" / profile_name),
        "--max-iter",
        str(args.max_iter),
        "--tol",
        str(args.tol),
        "--acceptable-tol",
        str(args.acceptable_tol),
        "--timeout-sec",
        str(args.timeout_sec),
        "--jobs",
        str(args.jobs),
    ]
    if args.rebuild:
        cmd.append("--rebuild")
    else:
        cmd.append("--no-rebuild")
    cmd.extend(solver_args)
    return cmd, tsv


def slsqp_profile_run_command(
    runner: pathlib.Path,
    args: argparse.Namespace,
    run_dir: pathlib.Path,
    profile_name: str,
) -> tuple[list[str], pathlib.Path]:
    jsonl = run_dir / f"{profile_name}.jsonl"
    tsv = run_dir / f"{profile_name}.tsv"
    log_dir = run_dir / "logs" / profile_name
    cmd = [
        sys.executable,
        str(runner),
        "--problem-file",
        str(args.problem_set),
        "--out",
        str(jsonl),
        "--tsv-out",
        str(tsv),
        "--profile-name",
        profile_name,
        "--log-dir",
        str(log_dir),
        "--workdir",
        str(run_dir / "work" / profile_name),
        "--max-iter",
        str(args.max_iter),
        "--constraint-tol",
        str(args.tol),
        "--gradient-tol",
        str(args.acceptable_tol),
        "--timeout-sec",
        str(args.timeout_sec),
        "--jobs",
        str(args.jobs),
    ]
    if args.rebuild:
        cmd.append("--rebuild")
    return cmd, tsv


def main(argv: list[str]) -> int:
    root = repo_root()
    default_problem_set = root / "solvers" / "a4sqp" / "cutest" / "problem_sets" / "broad_stratified_89.tsv"
    parser = argparse.ArgumentParser()
    parser.add_argument("--problem-set", type=pathlib.Path, default=default_problem_set)
    parser.add_argument("--out", type=pathlib.Path, default=root / "solvers" / "a4sqp" / "CUTEST_PROGRESS.md")
    parser.add_argument("--run-dir", type=pathlib.Path)
    parser.add_argument("--suite", default="")
    parser.add_argument("--notes", default="")
    parser.add_argument("--max-iter", type=int, default=200)
    parser.add_argument("--timeout-sec", type=float, default=30)
    parser.add_argument("--tol", default="1e-7")
    parser.add_argument("--acceptable-tol", default="1e-5")
    parser.add_argument("--acceptable-iter", type=int, default=5)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--build-jobs", type=int, default=6)
    parser.add_argument("--input-tsv", action="append", default=[], help="Existing TSV to include before newly generated profiles; repeatable.")
    parser.add_argument("--include-ipopt", action="store_true", help="Also rerun IPOPT C API limited-memory and exact-Hessian profiles.")
    parser.add_argument("--include-conopt", action="store_true", help="Also rerun the CONOPT C API CUTEst profile.")
    parser.add_argument("--include-slsqp", action="store_true", help="Also rerun the NLopt/SLSQP CUTEst profile.")
    parser.add_argument("--no-a4sqp", dest="a4sqp", action="store_false", help="Do not rerun the three standard A4SQP profiles.")
    parser.set_defaults(a4sqp=True)
    parser.add_argument(
        "--a4sqp-reduced-gradient-polish-mode",
        choices=["OFF", "FALLBACK", "ON"],
        default="OFF",
        help="Pass an explicit reduced-gradient polish mode to all generated A4SQP profiles.",
    )
    parser.add_argument(
        "--a4sqp-lsq-linear-solver",
        choices=["NORMAL", "DENSE_QR"],
        default="DENSE_QR",
        help="Pass the requested LSQ linear solver to all generated A4SQP profiles.",
    )
    parser.add_argument("--build", dest="build", action="store_true", default=True)
    parser.add_argument("--no-build", dest="build", action="store_false")
    parser.add_argument("--rebuild", dest="rebuild", action="store_true", default=True, help="Force CUTEst/runcutest rebuilds for each problem/package.")
    parser.add_argument("--no-rebuild", dest="rebuild", action="store_false")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)

    args.problem_set = args.problem_set.resolve()
    run_dir = (args.run_dir or default_run_dir(args.problem_set)).resolve()
    runner = root / "solvers" / "a4sqp" / "cutest" / "run_a4sqp_cutest.py"
    slsqp_runner = root / "solvers" / "slsqp" / "cutest" / "run_slsqp_cutest.py"
    generator = root / "solvers" / "a4sqp" / "cutest" / "generate_cutest_progress.py"
    suite = args.suite or f"CUTEst {args.problem_set.stem.replace('_', ' ')} tracking subset"
    notes = args.notes or "Generated by update_cutest_progress.py from rebuilt CUTEst profile TSVs."

    if args.build:
        build_cmd = ["scons", f"-j{args.build_jobs}"]
        if args.include_slsqp:
            build_cmd.append("WITH_SLSQP=1")
        run(build_cmd, root, args.dry_run)

    run_dir.mkdir(parents=True, exist_ok=True)
    tsv_inputs = [pathlib.Path(path).resolve() for path in args.input_tsv]

    profile_commands: list[tuple[list[str], pathlib.Path]] = []
    if args.include_ipopt:
        profile_commands.append(profile_run_command(
            runner,
            args,
            run_dir,
            "ipoptc_limited",
            ["--solver", "ipoptc", "--ipopt-hessian", "limited-memory"],
        ))
        profile_commands.append(profile_run_command(
            runner,
            args,
            run_dir,
            "ipoptc_exact",
            ["--solver", "ipoptc", "--ipopt-hessian", "exact"],
        ))
    if args.include_conopt:
        profile_commands.append(profile_run_command(
            runner,
            args,
            run_dir,
            "CONOPT",
            ["--solver", "conoptc"],
        ))
    if args.a4sqp:
        for hessian, profile in [
            ("BFGS", "A4SQP_BFGS_AUTO"),
            ("EXACT_OBJ", "A4SQP_EXACT_OBJ_AUTO"),
            ("EXACT_LAGRANGIAN", "A4SQP_EXACT_LAGRANGIAN_AUTO"),
        ]:
            a4sqp_args = [
                "--solver",
                "a4sqp",
                "--a4sqp-hessian",
                hessian,
                "--a4sqp-scaleopt",
                "AUTO",
                "--acceptable-iter",
                str(args.acceptable_iter),
                "--lsq-linear-solver",
                args.a4sqp_lsq_linear_solver,
                "--restoration",
            ]
            if args.a4sqp_reduced_gradient_polish_mode != "OFF":
                a4sqp_args.extend([
                    "--reduced-gradient-polish-mode",
                    args.a4sqp_reduced_gradient_polish_mode,
                ])
            profile_commands.append(profile_run_command(
                runner,
                args,
                run_dir,
                profile,
                a4sqp_args,
            ))
    if args.include_slsqp:
        profile_commands.append(slsqp_profile_run_command(
            slsqp_runner,
            args,
            run_dir,
            "SLSQP_NLOPT",
        ))

    for cmd, tsv in profile_commands:
        run(cmd, root, args.dry_run)
        tsv_inputs.append(tsv.resolve())

    generate_cmd = [sys.executable, str(generator)]
    for tsv in tsv_inputs:
        generate_cmd.extend(["--input", str(tsv)])
    generate_cmd.extend([
        "--out",
        str(args.out),
        "--suite",
        suite,
        "--problem-set",
        str(args.problem_set),
        "--max-iter",
        str(args.max_iter),
        "--timeout-sec",
        str(args.timeout_sec),
        "--tol",
        str(args.tol),
        "--acceptable-tol",
        str(args.acceptable_tol),
        "--jobs",
        str(args.jobs),
        "--notes",
        notes,
    ])
    run(generate_cmd, root, args.dry_run)
    print(f"Run directory: {run_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

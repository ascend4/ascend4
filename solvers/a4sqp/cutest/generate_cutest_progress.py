#!/usr/bin/env python3
"""Generate a stable CUTEst progress report from per-problem TSV results."""

from __future__ import annotations

import argparse
import csv
import pathlib
import sys
from collections import Counter, OrderedDict, defaultdict


PASS_OUTCOMES = {"strict_success", "acceptable_success"}
SUSPECT_OUTCOMES = {"strict_success_high_kkt", "acceptable_success_high_kkt"}
NEAR_OUTCOMES = {"max_iter_near_solved"}
ERROR_OUTCOMES = {"driver_timeout", "driver_error", "no_solver_status"}

OUTCOME_CODES: dict[str, tuple[str, str, str]] = {
    "strict_success": ("1", "🟢", "strict success"),
    "acceptable_success": ("2", "🟢", "acceptable success"),
    "max_iter_near_solved": ("3", "🟠", "near solved at iteration limit"),
    "strict_success_high_kkt": ("4", "🟠", "strict success but high KKT residual"),
    "acceptable_success_high_kkt": ("5", "🟠", "acceptable success but high KKT residual"),
    "max_iter_stationarity": ("6", "🔴", "iteration limit; stationarity residual too high"),
    "max_iter_infeasible_or_stalled": ("7", "🔴", "iteration limit; infeasible or stalled"),
    "line_search_error": ("8", "🔴", "line-search failure"),
    "qp_failure": ("9", "🔴", "QP failure"),
    "step_computation_error": ("10", "🔴", "step computation failure"),
    "driver_timeout": ("11", "🔴", "driver timeout"),
    "driver_error": ("12", "🔴", "driver error"),
    "no_solver_status": ("13", "🔴", "missing solver status"),
    "other_solver_failure": ("14", "🔴", "other solver failure"),
}
UNKNOWN_OUTCOME = ("?", "🔴", "unclassified outcome")


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parents[3]


def read_rows(path: pathlib.Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle, delimiter="\t")
        if not reader.fieldnames:
            raise ValueError(f"{path} has no TSV header")
        rows = [{k: (v or "").strip() for k, v in row.items()} for row in reader]
    if not rows:
        raise ValueError(f"{path} contains no result rows")
    if not any("outcome_class" in row for row in rows):
        raise ValueError(f"{path} does not look like a CUTEst problem-results TSV")
    return rows


def row_profile(row: dict[str, str], fallback: str) -> str:
    if row.get("profile"):
        return row["profile"]
    solver = row.get("solver") or "solver"
    hessian = row.get("hessian")
    return "_".join(part for part in (fallback, solver, hessian) if part)


def row_problem(row: dict[str, str]) -> str:
    return row.get("problem") or row.get("problem_requested") or ""


def outcome_bucket(outcome: str) -> str:
    if outcome in PASS_OUTCOMES:
        return "pass"
    if outcome in SUSPECT_OUTCOMES:
        return "suspect"
    if outcome in NEAR_OUTCOMES:
        return "near"
    if outcome in ERROR_OUTCOMES:
        return "error"
    return "fail"


def markdown_cell(value: object) -> str:
    text = str(value)
    return text.replace("|", "\\|").replace("\n", " ").strip()


def markdown_table(headers: list[str], rows: list[list[object]]) -> str:
    lines = [
        "| " + " | ".join(markdown_cell(header) for header in headers) + " |",
        "| " + " | ".join("---" for _ in headers) + " |",
    ]
    for row in rows:
        lines.append("| " + " | ".join(markdown_cell(value) for value in row) + " |")
    return "\n".join(lines)


def profile_summary(rows: list[dict[str, str]], profile_order: list[str]) -> list[list[object]]:
    by_profile: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        by_profile[row["_profile"]].append(row)

    summary_rows: list[list[object]] = []
    for profile in profile_order:
        items = by_profile[profile]
        outcomes = Counter(row.get("outcome_class", "") for row in items)
        buckets = Counter(outcome_bucket(row.get("outcome_class", "")) for row in items)
        total = len(items)
        pass_count = buckets["pass"]
        summary_rows.append(
            [
                profile,
                first_nonempty(items, "solver"),
                first_nonempty(items, "hessian"),
                first_nonempty(items, "kkt_convergence"),
                first_nonempty(items, "acceptable_iter"),
                total,
                pass_count,
                f"{pass_count}/{total}" if total else "0/0",
                buckets["near"],
                buckets["suspect"],
                buckets["fail"],
                buckets["error"],
                format_outcomes(outcomes),
            ]
        )
    return summary_rows


def profile_label(rows: list[dict[str, str]], profile: str) -> str:
    """Return a compact label for the wide problem-outcome matrix.

    The detailed profile name remains in the summary table.  The matrix is
    intended for quick visual comparison, so keep these labels short enough
    that the table remains readable in a terminal or browser Markdown view.
    """
    items = [row for row in rows if row.get("_profile") == profile]
    solver = first_nonempty(items, "solver").lower()
    hessian = first_nonempty(items, "hessian")
    hessian_upper = hessian.upper()

    if solver in {"ipopt", "ipoptc"}:
        if hessian == "limited-memory":
            return "IPOPT L-BFGS"
        if hessian == "exact":
            return "IPOPT Exact"
        return f"IPOPT {hessian or profile}"

    if solver == "a4sqp":
        if hessian_upper == "BFGS":
            return "A4SQP BFGS"
        if hessian_upper == "EXACT_OBJ":
            return "A4SQP Obj"
        if hessian_upper == "EXACT_LAGRANGIAN":
            return "A4SQP Lagr"
        return f"A4SQP {hessian or profile}"

    return profile


def first_nonempty(rows: list[dict[str, str]], key: str) -> str:
    for row in rows:
        if row.get(key):
            return row[key]
    return ""


def format_outcomes(outcomes: Counter[str]) -> str:
    return "; ".join(f"{key}:{outcomes[key]}" for key in sorted(outcomes) if key)


def outcome_light_code(outcome: str) -> str:
    code, light, _description = OUTCOME_CODES.get(outcome, UNKNOWN_OUTCOME)
    return f"{light}{code}"


def outcome_cell(row: dict[str, str]) -> str:
    suffix = "L" if row.get("used_lsq") == "1" else ""
    return f"{outcome_light_code(row.get('outcome_class', ''))}{suffix}"


def outcome_key_rows(rows: list[dict[str, str]]) -> list[list[object]]:
    used = {row.get("outcome_class", "") for row in rows if row.get("outcome_class", "")}
    ordered = [
        outcome
        for outcome, (code, _light, _description) in sorted(
            OUTCOME_CODES.items(), key=lambda item: int(item[1][0])
        )
        if outcome in used
    ]
    ordered.extend(sorted(outcome for outcome in used if outcome not in OUTCOME_CODES))
    key_rows: list[list[object]] = []
    for outcome in ordered:
        code, light, description = OUTCOME_CODES.get(outcome, UNKNOWN_OUTCOME)
        key_rows.append([f"{light}{code}", outcome_bucket(outcome), outcome, description])
    return key_rows


def problem_matrix(
    rows: list[dict[str, str]], profile_order: list[str], problem_order: list[str]
) -> list[list[object]]:
    by_problem_profile: dict[tuple[str, str], dict[str, str]] = {}
    problem_meta: OrderedDict[str, dict[str, str]] = OrderedDict()
    for row in rows:
        problem = row_problem(row)
        profile = row["_profile"]
        by_problem_profile[(problem, profile)] = row
        problem_meta.setdefault(problem, row)

    matrix: list[list[object]] = []
    for problem in problem_order:
        meta = problem_meta[problem]
        values: list[object] = [
            problem,
            meta.get("classification", ""),
            meta.get("n", ""),
            meta.get("m", ""),
        ]
        for profile in profile_order:
            row = by_problem_profile.get((problem, profile))
            if row is None:
                values.append("")
                continue
            values.append(outcome_cell(row))
        matrix.append(values)
    return matrix


def build_report(args: argparse.Namespace, rows: list[dict[str, str]]) -> str:
    profile_order: list[str] = []
    problem_order: list[str] = []
    for row in rows:
        row["_profile"] = row_profile(row, args.default_profile)
        problem = row_problem(row)
        if not problem:
            raise ValueError("result row is missing problem/problem_requested")
        if row["_profile"] not in profile_order:
            profile_order.append(row["_profile"])
        if problem not in problem_order:
            problem_order.append(problem)

    contract_rows = [
        ["Suite", args.suite],
        ["Problem count", len(problem_order)],
        ["Max iterations", args.max_iter],
        ["Per-problem timeout", args.timeout_sec],
        ["Tolerance", args.tol],
        ["Acceptable tolerance", args.acceptable_tol],
        ["Parallel jobs", args.jobs],
    ]
    if args.problem_set:
        contract_rows.append(["Problem-set source", args.problem_set])
    if args.notes:
        contract_rows.append(["Notes", args.notes])

    summary_headers = [
        "Profile",
        "Solver",
        "Hessian",
        "KKT",
        "Acceptable iter",
        "Total",
        "Pass",
        "Pass rate",
        "Near",
        "Suspect",
        "Fail",
        "Error",
        "Outcomes",
    ]
    matrix_headers = ["Problem", "Class", "n", "m", *[profile_label(rows, profile) for profile in profile_order]]

    parts = [
        "# A4SQP CUTEst Progress",
        "",
        "Generated by `solvers/a4sqp/cutest/generate_cutest_progress.py`; do not edit this file by hand.",
        "",
        "This report intentionally excludes iteration counts, timings, objectives, and log paths so git diffs track pass/fail movement rather than run-to-run noise.",
        "",
        "## Benchmark Contract",
        "",
        markdown_table(["Field", "Value"], contract_rows),
        "",
        "## Profile Summary",
        "",
        markdown_table(summary_headers, profile_summary(rows, profile_order)),
        "",
        "## Problem Outcomes",
        "",
        "Outcome cells use compact light+number codes. The key below the table maps codes to outcome classes.",
        "",
        "Cells with an `L` suffix used the experimental least-squares solve path for that profile.",
        "",
        "Matrix profile headers are shortened to solver/Hessian labels; full profile settings are listed in the summary table.",
        "",
        markdown_table(matrix_headers, problem_matrix(rows, profile_order, problem_order)),
        "",
        "## Outcome Key",
        "",
        markdown_table(["Code", "Bucket", "Outcome", "Meaning"], outcome_key_rows(rows)),
        "",
    ]
    return "\n".join(parts)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--input",
        action="append",
        required=True,
        help="Per-problem CUTEst TSV result file; may be repeated for multiple profiles",
    )
    parser.add_argument(
        "--out",
        default=str(repo_root() / "solvers" / "a4sqp" / "CUTEST_PROGRESS.md"),
        help="Markdown progress file to write",
    )
    parser.add_argument("--suite", default="CUTEst NLP tracking subset")
    parser.add_argument("--problem-set", default="")
    parser.add_argument("--max-iter", default="200")
    parser.add_argument("--timeout-sec", default="30")
    parser.add_argument("--tol", default="1e-7")
    parser.add_argument("--acceptable-tol", default="1e-5")
    parser.add_argument("--jobs", default="6")
    parser.add_argument("--notes", default="")
    parser.add_argument("--default-profile", default="profile")
    args = parser.parse_args(argv)

    rows: list[dict[str, str]] = []
    for input_path in args.input:
        rows.extend(read_rows(pathlib.Path(input_path)))
    report = build_report(args, rows)
    out_path = pathlib.Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(report, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

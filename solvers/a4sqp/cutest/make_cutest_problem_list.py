#!/usr/bin/env python3
"""Create CUTEst problem-name lists from a CLASSF.DB classification file."""

from __future__ import annotations

import argparse
import pathlib
import sys
from dataclasses import dataclass


@dataclass(frozen=True)
class CutestProblem:
    name: str
    classification: str
    n: int | None
    m: int | None


def parse_size(value: str) -> int | None:
    return None if value == "V" else int(value)


def read_classification_db(path: pathlib.Path) -> list[CutestProblem]:
    problems: list[CutestProblem] = []
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if not line:
            continue
        fields = line.split()
        if len(fields) < 2:
            continue
        name = fields[0].strip()
        classification = "".join(fields[1:]).replace(" ", "")
        parts = classification.split("-")
        if len(parts) != 4 or len(parts[0]) < 4:
            continue
        try:
            n = parse_size(parts[2])
            m = parse_size(parts[3])
        except ValueError:
            continue
        problems.append(CutestProblem(name=name, classification=classification, n=n, m=m))
    return problems


def keep_problem(problem: CutestProblem, args: argparse.Namespace) -> bool:
    code = problem.classification
    objective = code[0]
    constraint = code[1]
    smoothness = code[2]
    degree = int(code[3])
    if objective not in args.objectives:
        return False
    if constraint not in args.constraints:
        return False
    if args.regular_only and smoothness != "R":
        return False
    if degree < args.min_derivative_degree:
        return False
    if args.fixed_size_only and (problem.n is None or problem.m is None):
        return False
    if args.max_n is not None and (problem.n is None or problem.n > args.max_n):
        return False
    if args.max_m is not None and (problem.m is None or problem.m > args.max_m):
        return False
    return True


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--classf-db",
        default="/home/john/MASTSIF/CLASSF.DB",
        help="CUTEst/SIFDecode CLASSF.DB path",
    )
    parser.add_argument("--out", help="Output problem-name file; stdout if omitted")
    parser.add_argument("--with-classification", action="store_true")
    parser.add_argument("--max-n", type=int, help="Exclude fixed-size problems with n greater than this")
    parser.add_argument("--max-m", type=int, help="Exclude fixed-size problems with m greater than this")
    parser.add_argument("--fixed-size-only", action="store_true", help="Exclude variable-size problems")
    parser.add_argument("--regular-only", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--min-derivative-degree", type=int, default=2, choices=[0, 1, 2])
    parser.add_argument("--objectives", default="LQSO", help="Allowed objective classes")
    parser.add_argument("--constraints", default="UXBNLQO", help="Allowed constraint classes")
    parser.add_argument("--limit", type=int, help="Write at most this many problems after filtering and sorting")
    args = parser.parse_args(argv)

    problems = [
        problem
        for problem in read_classification_db(pathlib.Path(args.classf_db))
        if keep_problem(problem, args)
    ]
    problems.sort(key=lambda problem: problem.name)
    if args.limit is not None:
        problems = problems[: args.limit]
    lines = [
        f"{problem.name} {problem.classification}" if args.with_classification else problem.name
        for problem in problems
    ]
    text = "\n".join(lines)
    if text:
        text += "\n"
    if args.out:
        pathlib.Path(args.out).write_text(text)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

#!/usr/bin/env python3
"""
Run multiple dynamic TGA experiments via ./a4 script and collect t_RD50 summaries.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
	ap = argparse.ArgumentParser(description="Batch runner for tgadyn.a4c experiments.")
	ap.add_argument("input_table", help="CSV or TSV table of experiments.")
	ap.add_argument(
		"--output-table",
		help="Optional CSV/TSV file to write summaries to. Defaults to stdout only.",
	)
	ap.add_argument(
		"--run-method",
		help="Optional prep method such as prep_sahar_873K or prep_sahar_1073K. Explicit row overrides are applied after the method.",
	)
	ap.add_argument(
		"--t-end-min",
		type=float,
		default=20.0,
		help="Integration horizon in minutes.",
	)
	ap.add_argument(
		"--max-step-s",
		type=float,
		default=1.0,
		help="Maximum IDA substep in seconds.",
	)
	ap.add_argument(
		"--nsteps",
		type=int,
		default=200,
		help="Number of reported output steps.",
	)
	return ap.parse_args()


def open_table(path: Path):
	with path.open(newline="", encoding="utf-8") as f:
		sample = f.read(2048)
		f.seek(0)
		try:
			dialect = csv.Sniffer().sniff(sample, delimiters=",\t")
		except csv.Error:
			dialect = csv.excel
		reader = csv.DictReader(f, dialect=dialect)
		rows = list(reader)
	return rows


def first_present(row: dict[str, str], *names: str) -> str | None:
	for name in names:
		if name in row and row[name] not in ("", None):
			return row[name]
	return None


def optional_float(row: dict[str, str], *names: str) -> float | None:
	value = first_present(row, *names)
	if value is None:
		return None
	return float(value)


def resolve_run_method(row: dict[str, str], default_method: str | None) -> str | None:
	return first_present(row, "run_method", "RunMethod", "prep_method") or default_method


def build_command(repo_root: Path, script_path: Path, row: dict[str, str], args: argparse.Namespace) -> list[str]:
	cmd = [
		str(repo_root / "a4"),
		"script",
		str(script_path),
		"--json-summary",
		"--t-end-min",
		str(args.t_end_min),
		"--nsteps",
		str(args.nsteps),
		"--max-step-s",
		str(args.max_step_s),
	]
	run_method = resolve_run_method(row, args.run_method)
	if run_method:
		cmd.extend(["--run-method", run_method])

	T_K = optional_float(row, "T_K")
	if T_K is None:
		T_C = optional_float(row, "T_C")
		if T_C is not None:
			T_K = T_C + 273.15
	if T_K is not None:
		cmd.extend(["--T-K", str(T_K)])

	y_h2 = optional_float(row, "y_H2", "y_list", "y_H2_bulk")
	if y_h2 is not None:
		cmd.extend(["--y-H2", str(y_h2)])

	H_bed_mm = optional_float(row, "H_bed_mm", "h_list_mm", "h_mm")
	if H_bed_mm is not None:
		cmd.extend(["--H-bed-mm", str(H_bed_mm)])

	L_freeboard_mm = optional_float(row, "L_freeboard_mm", "L_list_mm", "L_mm")
	if L_freeboard_mm is not None:
		cmd.extend(["--L-freeboard-mm", str(L_freeboard_mm)])

	m_sample_mg = optional_float(row, "m_sample_mg", "m_list_mg", "m_mg")
	if m_sample_mg is not None:
		cmd.extend(["--m-sample-mg", str(m_sample_mg)])

	return cmd


def parse_summary(stdout: str) -> dict[str, object]:
	for line in reversed(stdout.splitlines()):
		if line.startswith("json_summary="):
			return json.loads(line.split("=", 1)[1])
	raise RuntimeError("json_summary line not found in runner output")


def row_label(row: dict[str, str], fallback_index: int) -> str:
	value = first_present(row, "Test ID", "test_id", "Number_X", "id")
	if value is not None:
		return value
	return f"row_{fallback_index}"


def summarise_row(row: dict[str, str], summary: dict[str, object], returncode: int, stdout: str) -> dict[str, object]:
	result = dict(row)
	result["case_id"] = row_label(row, 0)
	result["integration_returncode"] = returncode
	result["integration_status"] = summary.get("integration_status")
	result["T_K_used"] = summary.get("T_K")
	result["y_H2_used"] = summary.get("y_H2_bulk")
	result["H_bed_mm_used"] = None if summary.get("H_bed_m") is None else 1e3 * float(summary["H_bed_m"])
	result["L_freeboard_mm_used"] = None if summary.get("L_freeboard_m") is None else 1e3 * float(summary["L_freeboard_m"])
	result["m_sample_init_mg_used"] = None if summary.get("m_sample_init_kg") is None else 1e6 * float(summary["m_sample_init_kg"])
	result["t_final_min"] = None if summary.get("t_final_s") is None else float(summary["t_final_s"]) / 60.0
	result["reduction_degree_final"] = summary.get("reduction_degree_final")
	result["t_RD50_min"] = summary.get("t_RD50_min")
	result["rd50_reached"] = summary.get("rd50_reached")
	result["n_Fe2O3_final_mol"] = summary.get("n_Fe2O3_final_mol")
	result["n_Fe3O4_final_mol"] = summary.get("n_Fe3O4_final_mol")
	result["n_FeO_final_mol"] = summary.get("n_FeO_final_mol")
	result["n_Fe_final_mol"] = summary.get("n_Fe_final_mol")
	if summary.get("integration_error") is not None:
		result["integration_error"] = summary.get("integration_error")
	elif returncode != 0:
		result["integration_error"] = stdout.strip().splitlines()[-1] if stdout.strip() else f"returncode={returncode}"
	return result


def write_table(path: Path, rows: list[dict[str, object]]) -> None:
	if not rows:
		return
	delimiter = "\t" if path.suffix.lower() == ".tsv" else ","
	fieldnames: list[str] = []
	for row in rows:
		for key in row.keys():
			if key not in fieldnames:
				fieldnames.append(key)
	with path.open("w", newline="", encoding="utf-8") as f:
		writer = csv.DictWriter(f, fieldnames=fieldnames, delimiter=delimiter)
		writer.writeheader()
		writer.writerows(rows)


def main() -> int:
	args = parse_args()
	repo_root = Path.cwd()
	script_path = repo_root / "models" / "johnpye" / "iron" / "simulate_tgadyn.py"
	input_rows = open_table(Path(args.input_table))
	results: list[dict[str, object]] = []

	for idx, row in enumerate(input_rows, start=1):
		case_id = row_label(row, idx)
		cmd = build_command(repo_root, script_path, row, args)
		print(f"running_case={case_id}", flush=True)
		proc = subprocess.run(
			cmd,
			stdout=subprocess.PIPE,
			stderr=subprocess.STDOUT,
			text=True,
			check=False,
			env=os.environ.copy(),
		)
		summary = parse_summary(proc.stdout)
		result = summarise_row(row, summary, proc.returncode, proc.stdout)
		result["case_id"] = case_id
		results.append(result)
		t_rd50_min = result.get("t_RD50_min")
		print(
			f"case_result={case_id}\tintegration_status={result.get('integration_status')}"
			f"\tt_RD50_min={t_rd50_min if t_rd50_min is not None else 'nan'}",
			flush=True,
		)

	if args.output_table:
		write_table(Path(args.output_table), results)

	json.dump(results, sys.stdout, indent=2, sort_keys=True)
	print()
	return 0


if __name__ == "__main__":
	raise SystemExit(main())

#!/usr/bin/env python3
"""
Extract original-UNIFAC source data from ../../components.a4l and emit a
native C-source-data scaffold.

This script is intentionally narrower than convcomp.py:

- it only targets immutable mixture/source data
- it does not prepare runtime caches
- it currently focuses on subgroup definitions, interaction parameters,
  and component-to-subgroup mappings required for gamma(T, x)

The current ASCEND-backed UNIFAC path remains the reference path while
this generator and the native C-data path are brought up.
"""

from __future__ import annotations

import argparse
import pathlib
import re
from dataclasses import dataclass

import yaml


ROOT = pathlib.Path(__file__).resolve().parents[4]
COMPONENTS_A4L = ROOT / "models" / "components.a4l"


@dataclass
class Subgroup:
	name: str
	subgroup_id: int
	main_group_id: int
	R: float
	Q: float


@dataclass
class ComponentSubgroup:
	name: str
	nu: int


@dataclass
class Component:
	name: str
	formula: str
	subgroups: list[ComponentSubgroup]
	r: float
	q: float
	Tc: float
	Pc: float
	vp_correlation: int
	vpa: float
	vpb: float
	vpc: float
	vpd: float
	T0: float
	P0: float
	H0: float
	G0: float
	cpvapa: float
	cpvapb: float
	cpvapc: float
	cpvapd: float
	omega: float
	Zc: float
	Vliq: float
	Tliq: float


def parse_quoted_list(text: str) -> list[str]:
	return re.findall(r"'([^']+)'", text)


def parse_float_token(token: str) -> float:
	return float(token)


def parse_case_expr(case: str, field: str) -> str:
	m = re.search(rf"(?ms)^\s*{re.escape(field)}\s*:==\s*(.*?);", case)
	if not m:
		raise RuntimeError(f"Missing scalar field '{field}'")
	expr = m.group(1)
	expr = re.sub(r"\(\*.*?\*\)", "", expr, flags=re.S)
	expr = re.sub(r"\{[^{}]*\}", "", expr)
	expr = " ".join(expr.split())
	return expr


def parse_case_scalar(case: str, field: str) -> float:
	expr = parse_case_expr(case, field)
	if not re.fullmatch(r"[0-9eE+\-*/(). ]+", expr):
		raise RuntimeError(f"Unsupported scalar expression for '{field}': {expr}")
	return float(eval(expr, {"__builtins__": {}}, {}))


def parse_case_int(case: str, field: str) -> int:
	return int(round(parse_case_scalar(case, field)))


def extract_block(text: str, start_pattern: str, end_pattern: str) -> str:
	start = re.search(start_pattern, text, re.M)
	if not start:
		raise RuntimeError(f"Unable to locate start pattern: {start_pattern}")
	end = re.search(end_pattern, text[start.end():], re.M)
	if not end:
		raise RuntimeError(f"Unable to locate end pattern: {end_pattern}")
	return text[start.end(): start.end() + end.start()]


def parse_unifac_constants(text: str) -> tuple[dict[str, Subgroup], list[list[float]]]:
	block = extract_block(
		text,
		r"^UNIVERSAL MODEL UNIFAC_constants\(\) REFINES compmodel;",
		r"^END UNIFAC_constants;"
	)

	main_group_subs: dict[int, list[str]] = {}
	for m in re.finditer(r"(?m)^\s*sub\[(\d+)\]\s*:==\s*\[(.*?)\];", block, re.S):
		main_group = int(m.group(1))
		main_group_subs[main_group] = parse_quoted_list(m.group(2))

	r_vals = {
		m.group(1): parse_float_token(m.group(2))
		for m in re.finditer(r"(?m)^\s*R\['([^']+)'\]\s*:==\s*([^;]+);", block)
	}
	q_vals = {
		m.group(1): parse_float_token(m.group(2))
		for m in re.finditer(r"(?m)^\s*Q\['([^']+)'\]\s*:==\s*([^;]+);", block)
	}

	subgroups: dict[str, Subgroup] = {}
	next_id = 1
	for main_group in sorted(main_group_subs):
		for name in main_group_subs[main_group]:
			if name not in r_vals or name not in q_vals:
				raise RuntimeError(f"Missing R/Q definition for subgroup '{name}'")
			subgroups[name] = Subgroup(
				name=name,
				subgroup_id=next_id,
				main_group_id=main_group,
				R=r_vals[name],
				Q=q_vals[name],
			)
			next_id += 1

	table_match = re.search(
		r"(?ms)^\s*TABLE a\[groups,groups\] UNITS \{K\};\s*(.*?)^\s*END TABLE;",
		block,
	)
	if not table_match:
		raise RuntimeError("Unable to locate UNIFAC interaction TABLE a[groups,groups]")

	lines = [ln.strip() for ln in table_match.group(1).splitlines() if ln.strip()]
	if not lines:
		raise RuntimeError("UNIFAC interaction table is empty")
	header = [int(tok) for tok in lines[0].split()]
	matrix = [[0.0 for _ in header] for _ in header]
	for row in lines[1:]:
		tokens = row.split()
		if len(tokens) != len(header) + 1:
			raise RuntimeError(f"Unexpected UNIFAC interaction row width: {row}")
		row_id = int(tokens[0])
		values = [parse_float_token(tok) for tok in tokens[1:]]
		if row_id not in header:
			raise RuntimeError(f"Unexpected UNIFAC interaction row id: {row_id}")
		matrix[row_id - 1] = values

	return subgroups, matrix


def parse_component_cases(text: str, subgroups: dict[str, Subgroup]) -> list[Component]:
	block = extract_block(
		text,
		r"(?m)^\s*SELECT\(component_name\)\s*$",
		r"(?m)^METHODS\s*$"
	)
	matches = list(re.finditer(r"(?m)^\s*CASE\s+'([^']+)':", block))
	components: list[Component] = []
	for i, m in enumerate(matches):
		name = m.group(1)
		start = m.end()
		end = matches[i + 1].start() if i + 1 < len(matches) else len(block)
		case = block[start:end]

		formula_match = re.search(r"(?m)^\s*formula\s*:==\s*'([^']+)';", case)
		subgroups_match = re.search(r"(?ms)^\s*subgroups\s*:==\s*\[(.*?)\];", case)
		if not formula_match or not subgroups_match:
			continue

		subgroup_names = parse_quoted_list(subgroups_match.group(1))
		if not subgroup_names:
			continue

		nu_map = {
			m2.group(1): int(m2.group(2))
			for m2 in re.finditer(r"(?m)^\s*nu\['([^']+)'\]\s*:==\s*([0-9]+)\s*;", case)
		}

		comp_subgroups: list[ComponentSubgroup] = []
		r = 0.0
		q = 0.0
		for sg in subgroup_names:
			if sg not in subgroups:
				raise RuntimeError(f"Component '{name}' references unknown subgroup '{sg}'")
			if sg not in nu_map:
				raise RuntimeError(f"Component '{name}' is missing nu['{sg}']")
			nu = nu_map[sg]
			comp_subgroups.append(ComponentSubgroup(name=sg, nu=nu))
			r += nu * subgroups[sg].R
			q += nu * subgroups[sg].Q

		components.append(
			Component(
				name=name,
				formula=formula_match.group(1),
				subgroups=comp_subgroups,
				r=r,
				q=q,
				Tc=parse_case_scalar(case, "Tc"),
				Pc=parse_case_scalar(case, "Pc") * 1e5,
				vp_correlation=parse_case_int(case, "vp_correlation"),
				vpa=parse_case_scalar(case, "vpa"),
				vpb=parse_case_scalar(case, "vpb"),
				vpc=parse_case_scalar(case, "vpc"),
				vpd=parse_case_scalar(case, "vpd"),
				T0=298.15,
				P0=101325.0,
				H0=parse_case_scalar(case, "Hf"),
				G0=parse_case_scalar(case, "Gf"),
				cpvapa=parse_case_scalar(case, "cpvapa"),
				cpvapb=parse_case_scalar(case, "cpvapb"),
				cpvapc=parse_case_scalar(case, "cpvapc"),
				cpvapd=parse_case_scalar(case, "cpvapd"),
				omega=parse_case_scalar(case, "omega"),
				Zc=parse_case_scalar(case, "Zc"),
				Vliq=(parse_case_scalar(case, "mw") / parse_case_scalar(case, "lden")) * 1e-6,
				Tliq=parse_case_scalar(case, "Tliq"),
			)
		)

	return components


def emit_header(outdir: pathlib.Path) -> None:
	header = outdir / "_unifac_generated_note.txt"
	header.write_text(
		"Generated by convunifac.py from models/components.a4l\n"
		"These files contain immutable source-data literals only.\n",
		encoding="ascii",
	)


def emit_groups(outdir: pathlib.Path, subgroups: dict[str, Subgroup], matrix: list[list[float]]) -> None:
	p = outdir / "_unifac_groups.c"
	lines: list[str] = []
	lines.append('#include "unifac_data.h"')
	lines.append("")
	lines.append("const FpropsUNIFACSubgroupSource fprops_unifac_orig_2003_subgroups[] = {")
	for sg in sorted(subgroups.values(), key=lambda s: s.subgroup_id):
		lines.append(
			f'\t{{"{sg.name}", {sg.subgroup_id}, {sg.main_group_id}, {sg.R:.12g}, {sg.Q:.12g}}},'
		)
	lines.append("};")
	lines.append("")
	lines.append("const double fprops_unifac_orig_2003_aij[] = {")
	for row in matrix:
		rowtxt = ", ".join(f"{v:.12g}" for v in row)
		lines.append(f"\t{rowtxt},")
	lines.append("};")
	lines.append("")
	lines.append("const FpropsUNIFACInteractionSource fprops_unifac_orig_2003_interactions = {")
	lines.append(f"\t47,")
	lines.append("\tfprops_unifac_orig_2003_aij")
	lines.append("};")
	p.write_text("\n".join(lines) + "\n", encoding="ascii")


def emit_components(outdir: pathlib.Path, subgroups: dict[str, Subgroup], components: list[Component]) -> None:
	p = outdir / "_unifac_components.c"
	lines: list[str] = []
	lines.append('#include "unifac_data.h"')
	lines.append("")
	for comp in components:
		arr = f"fprops_unifac_comp_{comp.name}_subgroups"
		lines.append(f"static const FpropsUNIFACComponentSubgroupSource {arr}[] = {{")
		for sg in comp.subgroups:
			lines.append(f"\t{{{subgroups[sg.name].subgroup_id - 1}, {sg.nu}}},")
		lines.append("};")
		lines.append("")
	lines.append("const FpropsUNIFACComponentSource fprops_unifac_orig_2003_components[] = {")
	for comp in components:
		arr = f"fprops_unifac_comp_{comp.name}_subgroups"
		lines.append(
			f'\t{{"{comp.name}", "{comp.formula}", {len(comp.subgroups)}, {arr},'
			f' {comp.r:.12g}, {comp.q:.12g},'
			f' {comp.Tc:.12g}, {comp.Pc:.12g}, {comp.vp_correlation},'
			f' {comp.vpa:.12g}, {comp.vpb:.12g}, {comp.vpc:.12g}, {comp.vpd:.12g},'
			f' {comp.T0:.12g}, {comp.P0:.12g}, {comp.H0:.12g}, {comp.G0:.12g},'
			f' {comp.cpvapa:.12g}, {comp.cpvapb:.12g}, {comp.cpvapc:.12g}, {comp.cpvapd:.12g},'
			f' {comp.omega:.12g}, {comp.Zc:.12g}, {comp.Vliq:.12g}, {comp.Tliq:.12g}}},'
		)
	lines.append("};")
	p.write_text("\n".join(lines) + "\n", encoding="ascii")


def emit_lookup(outdir: pathlib.Path, components: list[Component], subgroups: dict[str, Subgroup]) -> None:
	p = outdir / "_unifac_lookup.c"
	lines: list[str] = []
	lines.append('#include <string.h>')
	lines.append('#include "unifac_data.h"')
	lines.append("")
	lines.append("extern const FpropsUNIFACSubgroupSource fprops_unifac_orig_2003_subgroups[];")
	lines.append("extern const FpropsUNIFACComponentSource fprops_unifac_orig_2003_components[];")
	lines.append("extern const FpropsUNIFACInteractionSource fprops_unifac_orig_2003_interactions;")
	lines.append("")
	lines.append("const FpropsUNIFACSourceData fprops_unifac_orig_2003 = {")
	lines.append('\t"UNIFAC-orig-2003",')
	lines.append("\tfprops_unifac_orig_2003_subgroups,")
	lines.append(f"\t{len(subgroups)},")
	lines.append("\tfprops_unifac_orig_2003_components,")
	lines.append(f"\t{len(components)},")
	lines.append("\t&fprops_unifac_orig_2003_interactions,")
	lines.append("\tNULL,")
	lines.append("\t0")
	lines.append("};")
	lines.append("")
	lines.append("const FpropsUNIFACSourceData *fprops_unifac_source(const char *name){")
	lines.append('\tif(!name || strcmp(name, "UNIFAC-orig-2003") == 0 || strcmp(name, "orig-2003") == 0){')
	lines.append("\t\treturn &fprops_unifac_orig_2003;")
	lines.append("\t}")
	lines.append("\treturn NULL;")
	lines.append("}")
	lines.append("")
	lines.append("const FpropsUNIFACComponentSource *fprops_unifac_component(")
	lines.append("\t\tconst FpropsUNIFACSourceData *src, const char *name){")
	lines.append("\tint i;")
	lines.append("\tif(!src || !name){")
	lines.append("\t\treturn NULL;")
	lines.append("\t}")
	lines.append("\tfor(i = 0; i < src->ncomponents; ++i){")
	lines.append("\t\tif(strcmp(src->components[i].name, name) == 0){")
	lines.append("\t\t\treturn &src->components[i];")
	lines.append("\t\t}")
	lines.append("\t}")
	lines.append("\treturn NULL;")
	lines.append("}")
	lines.append("")
	lines.append("const FpropsUNIFACSubgroupSource *fprops_unifac_subgroup(")
	lines.append("\t\tconst FpropsUNIFACSourceData *src, const char *name){")
	lines.append("\tint i;")
	lines.append("\tif(!src || !name){")
	lines.append("\t\treturn NULL;")
	lines.append("\t}")
	lines.append("\tfor(i = 0; i < src->nsubgroups; ++i){")
	lines.append("\t\tif(strcmp(src->subgroups[i].name, name) == 0){")
	lines.append("\t\t\treturn &src->subgroups[i];")
	lines.append("\t\t}")
	lines.append("\t}")
	lines.append("\treturn NULL;")
	lines.append("}")
	p.write_text("\n".join(lines) + "\n", encoding="ascii")


def emit_generated(outdir: pathlib.Path, subgroups: dict[str, Subgroup], matrix: list[list[float]], components: list[Component]) -> None:
	outdir.mkdir(parents=True, exist_ok=True)
	emit_header(outdir)
	emit_groups(outdir, subgroups, matrix)
	emit_components(outdir, subgroups, components)
	emit_lookup(outdir, components, subgroups)


def emit_name_candidates(path: pathlib.Path, components: list[Component]) -> None:
	path.parent.mkdir(parents=True, exist_ok=True)
	entries: list[dict] = []
	for comp in components:
		entries.append(
			{
				"alias": comp.formula,
				"canonical": comp.name,
				"formula": comp.formula,
				"domains": ["mixture_component"],
				"sources": ["UNIFAC-orig-2003"],
				"flags": ["auto", "formula"],
				"priority": 10,
			}
		)
	data = {"version": 1, "entries": entries}
	path.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


def main() -> int:
	parser = argparse.ArgumentParser()
	parser.add_argument("--components", type=pathlib.Path, default=COMPONENTS_A4L)
	parser.add_argument("--summary", action="store_true", help="Print a summary of extracted UNIFAC source data")
	parser.add_argument("--emit-dir", type=pathlib.Path, default=None, help="Emit generated C-source-data scaffold into this directory")
	parser.add_argument("--emit-name-candidates", type=pathlib.Path, default=None,
		help="Emit auto-generated name candidates as YAML for the shared name registry")
	parser.add_argument("--limit-components", type=int, default=0, help="Limit emitted component count for debugging")
	args = parser.parse_args()

	text = args.components.read_text(encoding="utf-8")
	subgroups, matrix = parse_unifac_constants(text)
	components = parse_component_cases(text, subgroups)
	if args.limit_components > 0:
		components = components[: args.limit_components]

	if args.summary or args.emit_dir is None:
		print(f"components_a4l = {args.components}")
		print(f"unifac_subgroups = {len(subgroups)}")
		print(f"unifac_main_groups = 47")
		print(f"unifac_components_with_groups = {len(components)}")
		for comp in components[:10]:
			desc = ", ".join(f"{sg.name}:{sg.nu}" for sg in comp.subgroups)
			print(f"component {comp.name}: [{desc}] r={comp.r:.6f} q={comp.q:.6f}")

	if args.emit_dir is not None:
		emit_generated(args.emit_dir, subgroups, matrix, components)
		print(f"emitted = {args.emit_dir}")

	if args.emit_name_candidates is not None:
		emit_name_candidates(args.emit_name_candidates, components)
		print(f"name_candidates = {args.emit_name_candidates}")

	return 0


if __name__ == "__main__":
	raise SystemExit(main())

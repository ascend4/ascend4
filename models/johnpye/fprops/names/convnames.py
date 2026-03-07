#!/usr/bin/env python3
"""
Merge generated name candidates and manual YAML overrides into a single
generated C registry.
"""

from __future__ import annotations

import argparse
import pathlib
import sys
from collections import OrderedDict

import yaml


ROOT = pathlib.Path(__file__).resolve().parents[4]
NAME_DIR = pathlib.Path(__file__).resolve().parent
DEFAULT_INPUTS = [
	NAME_DIR / "generated" / "rpp_name_candidates.yml",
	NAME_DIR / "generated" / "unifac_name_candidates.yml",
]
DEFAULT_CANONICALS = NAME_DIR / "canonicals.yml"
DEFAULT_OVERRIDES = NAME_DIR / "aliases.yml"
DEFAULT_C_OUT = NAME_DIR / "_name_registry.c"

DOMAIN_BITS = {
	"pure_fluid": "FPROPS_NAME_DOMAIN_PURE_FLUID",
	"eqm_species": "FPROPS_NAME_DOMAIN_EQM_SPECIES",
	"mixture_component": "FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT",
}

FLAG_BITS = {
	"auto": "FPROPS_NAME_ALIAS_AUTO",
	"manual": "FPROPS_NAME_ALIAS_MANUAL",
	"formula": "FPROPS_NAME_ALIAS_FORMULA",
	"normalized": "FPROPS_NAME_ALIAS_NORM",
}


def load_yaml_entries(path: pathlib.Path) -> list[dict]:
	if not path.exists():
		return []
	data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
	return list(data.get("entries", []))


def normalize_token(token: str) -> str:
	return "".join(ch for ch in token.lower() if ch.isalnum())


def domains_expr(domains: list[str]) -> str:
	if not domains:
		return "FPROPS_NAME_DOMAIN_NONE"
	return " | ".join(DOMAIN_BITS[d] for d in domains)


def flags_expr(flags: list[str]) -> str:
	if not flags:
		return "0"
	return " | ".join(FLAG_BITS[f] for f in flags)


def domains_set(entry: dict) -> set[str]:
	return set(entry.get("domains", []))


def sources_set(entry: dict) -> set[str]:
	return set(entry.get("sources", []))


def canonical_matches(known: dict, alias_entry: dict) -> bool:
	if known["canonical"] != alias_entry["canonical"]:
		return False
	alias_sources = sources_set(alias_entry)
	known_sources = sources_set(known)
	if alias_sources and known_sources and known_sources.isdisjoint(alias_sources):
		return False
	alias_domains = domains_set(alias_entry)
	known_domains = domains_set(known)
	if alias_domains and known_domains and known_domains.isdisjoint(alias_domains):
		return False
	return True


def main() -> int:
	ap = argparse.ArgumentParser()
	ap.add_argument("--input", action="append", type=pathlib.Path, default=[])
	ap.add_argument("--canonicals", type=pathlib.Path, default=DEFAULT_CANONICALS)
	ap.add_argument("--overrides", type=pathlib.Path, default=DEFAULT_OVERRIDES)
	ap.add_argument("--emit-c", type=pathlib.Path, default=DEFAULT_C_OUT)
	ap.add_argument("--summary", action="store_true")
	args = ap.parse_args()

	inputs = args.input or DEFAULT_INPUTS
	auto_entries: list[dict] = []
	for p in inputs:
		auto_entries.extend(load_yaml_entries(p))
	manual_canonicals = load_yaml_entries(args.canonicals)
	override_entries = load_yaml_entries(args.overrides)

	canonicals: OrderedDict[tuple[str, str, tuple[str, ...]], dict] = OrderedDict()
	aliases: OrderedDict[tuple[str, str | None, tuple[str, ...]], dict] = OrderedDict()

	for entry in auto_entries:
		canonical = entry["canonical"]
		source_list = tuple(entry.get("sources", []))
		domains = list(entry.get("domains", []))
		formula = entry.get("formula")

		ckey = (canonical, formula or "", source_list)
		if ckey not in canonicals:
			canonicals[ckey] = {
				"canonical": canonical,
				"formula": formula,
				"source": source_list[0] if len(source_list) == 1 else None,
				"domains": domains,
			}

		akey = (entry["alias"], canonical, source_list)
		prev = aliases.get(akey)
		if prev is None or int(entry.get("priority", 0)) >= int(prev.get("priority", 0)):
			aliases[akey] = {
				"alias": entry["alias"],
				"canonical": canonical,
				"source": source_list[0] if len(source_list) == 1 else None,
				"domains": domains,
				"flags": list(entry.get("flags", [])),
				"priority": int(entry.get("priority", 0)),
			}

		norm_alias = normalize_token(entry["alias"])
		if norm_alias and norm_alias != entry["alias"].lower():
			nkey = (norm_alias, canonical, source_list)
			if nkey not in aliases:
				aliases[nkey] = {
					"alias": norm_alias,
					"canonical": canonical,
					"source": source_list[0] if len(source_list) == 1 else None,
					"domains": domains,
				"flags": sorted(set(list(entry.get("flags", [])) + ["normalized"])),
					"priority": int(entry.get("priority", 0)) - 1,
				}

	for entry in manual_canonicals:
		canonical = entry["canonical"]
		source_list = tuple(entry.get("sources", []))
		domains = list(entry.get("domains", []))
		formula = entry.get("formula")
		ckey = (canonical, formula or "", source_list)
		if ckey not in canonicals:
			canonicals[ckey] = {
				"canonical": canonical,
				"formula": formula,
				"source": source_list[0] if len(source_list) == 1 else None,
				"domains": domains,
			}

	canonical_index = list(canonicals.values())
	override_warnings: list[str] = []
	for entry in override_entries:
		if not any(canonical_matches(known, entry) for known in canonical_index):
			override_warnings.append(
				f'override alias "{entry["alias"]}" targets unknown or incompatible '
				f'canonical "{entry["canonical"]}"'
			)

		canonical = entry["canonical"]
		source_list = tuple(entry.get("sources", []))
		domains = list(entry.get("domains", []))
		akey = (entry["alias"], canonical, source_list)
		prev = aliases.get(akey)
		if prev is None or int(entry.get("priority", 0)) >= int(prev.get("priority", 0)):
			aliases[akey] = {
				"alias": entry["alias"],
				"canonical": canonical,
				"source": source_list[0] if len(source_list) == 1 else None,
				"domains": domains,
				"flags": list(entry.get("flags", [])),
				"priority": int(entry.get("priority", 0)),
			}

		norm_alias = normalize_token(entry["alias"])
		if norm_alias and norm_alias != entry["alias"].lower():
			nkey = (norm_alias, canonical, source_list)
			if nkey not in aliases:
				aliases[nkey] = {
					"alias": norm_alias,
					"canonical": canonical,
					"source": source_list[0] if len(source_list) == 1 else None,
					"domains": domains,
					"flags": sorted(set(list(entry.get("flags", [])) + ["normalized"])),
					"priority": int(entry.get("priority", 0)) - 1,
				}

	if args.summary:
		print(f"inputs = {', '.join(str(p) for p in inputs)}")
		print(f"manual_canonicals = {args.canonicals}")
		print(f"overrides = {args.overrides}")
		print(f"ncanonicals = {len(canonicals)}")
		print(f"aliases = {len(aliases)}")
		if override_warnings:
			for warning in override_warnings:
				print(f"warning: {warning}", file=sys.stderr)
			print(f"override_warnings = {len(override_warnings)}")

	out = args.emit_c
	out.parent.mkdir(parents=True, exist_ok=True)
	lines: list[str] = []
	lines.append("/* autogenerated by names/convnames.py -- do not edit */")
	lines.append('#include "../name_data.h"')
	lines.append("")
	lines.append("static const FpropsNameCanonical fprops_name_canonicals[] = {")
	for item in canonicals.values():
		formula = item["formula"] if item["formula"] is not None else ""
		source = item["source"] if item["source"] is not None else ""
		lines.append(
			f'\t{{"{item["canonical"]}", "{formula}", "{source}", {domains_expr(item["domains"])}}},'
		)
	lines.append("};")
	lines.append("")
	lines.append("static const FpropsNameAlias fprops_name_aliases[] = {")
	for item in aliases.values():
		source = item["source"] if item["source"] is not None else ""
		lines.append(
			f'\t{{"{item["alias"]}", "{item["canonical"]}", "{source}", '
			f'{domains_expr(item["domains"])}, {flags_expr(item["flags"])}, {item["priority"]}}},'
		)
	lines.append("};")
	lines.append("")
	lines.append("const FpropsNameRegistry fprops_name_registry = {")
	lines.append("\tfprops_name_canonicals,")
	lines.append(f"\t{len(canonicals)},")
	lines.append("\tfprops_name_aliases,")
	lines.append(f"\t{len(aliases)}")
	lines.append("};")
	out.write_text("\n".join(lines) + "\n", encoding="ascii")
	print(f"emitted = {out}")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())

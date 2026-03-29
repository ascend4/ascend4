#!/usr/bin/env python3
"""
Minimal IDA analyse() driver for LINK-based DER(...) test models.

Usage examples:
    ./a4 script models/test/ida/check_alias_der.py -- --model alias_der_direct_ok
    ./a4 script models/test/ida/check_alias_der.py -- --model alias_der_alias_fail --expect-fail
"""

from __future__ import annotations

import argparse
import sys

try:
	import os as _os
	sys.setdlopenflags(_os.RTLD_GLOBAL | _os.RTLD_NOW)
except Exception:
	pass

import ascpy


def main() -> int:
	ap = argparse.ArgumentParser()
	ap.add_argument(
		"--model-file",
		default="models/test/ida/alias_der_wLINK.a4c",
	)
	ap.add_argument(
		"--model",
		required=True,
	)
	ap.add_argument(
		"--expect-fail",
		action="store_true",
	)
	args = ap.parse_args()

	lib = ascpy.Library()
	lib.load(args.model_file)
	model_type = lib.findType(args.model)
	sim = model_type.getSimulation("sim", True)
	sim.setSolver(ascpy.Solver("QRSlv"))

	integrator = ascpy.Integrator(sim)
	integrator.setEngine("IDA")

	try:
		integrator.analyse()
		ok = True
		msg = "analyse_ok"
	except Exception as exc:
		ok = False
		msg = f"analyse_failed: {exc}"

	print(args.model)
	print(msg)

	if args.expect_fail:
		return 0 if not ok else 1
	return 0 if ok else 1


if __name__ == "__main__":
	raise SystemExit(main())

#!/usr/bin/env python3
"""
List solver and integrator engines registered by the current ASCEND build.
"""

import ascpy


def _name(obj):
	return obj.getName() if hasattr(obj, "getName") else str(obj)


def _version(obj):
	if hasattr(obj, "getVersion"):
		return obj.getVersion()
	return ""


def main():
	ascpy.Library()

	print("Solvers:")
	for solver in ascpy.getSolvers():
		name = _name(solver)
		version = _version(solver)
		if version:
			print(f"  {name}: {version}")
		else:
			print(f"  {name}")

	print("Integrators:")
	for name in ascpy.Integrator.getEngines():
		version = ""
		if hasattr(ascpy.Integrator, "getEngineVersion"):
			version = ascpy.Integrator.getEngineVersion(name)
		if version:
			print(f"  {name}: {version}")
		else:
			print(f"  {name}")


if __name__ == "__main__":
	main()

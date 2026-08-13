#!/usr/bin/env python3
"""
List solver and integrator engines available to the current ASCEND build.
"""

import ascpy


def _name(obj):
	return obj.getName() if hasattr(obj, "getName") else str(obj)


def _strip_name_prefix(name, value):
	if value == name:
		return ""
	prefix = name + " "
	if value.startswith(prefix):
		return value[len(prefix):]
	return value


def _version(obj, name):
	if hasattr(obj, "getVersion"):
		raw = obj.getVersion()
	else:
		raw = ""
	parts = []
	for part in raw.split(";"):
		part = _strip_name_prefix(name, part.strip())
		if not part:
			continue
		if ":" in part:
			dependency, value = part.split(":", 1)
			value = _strip_name_prefix(dependency, value.strip())
			part = f"{dependency} {value}" if value else dependency
		parts.append(part)
	return "; ".join(parts)


def _details(obj):
	if hasattr(obj, "getDetails"):
		return obj.getDetails()
	return ""


def main():
	ascpy.setAutoRegisterStandardSolvers(True)
	ascpy.Library()

	print("Solvers:")
	for solver in ascpy.getSolvers():
		name = _name(solver)
		version = _version(solver, name)
		details = _details(solver)
		label = f"  {name}"
		if version:
			label += f": {version}"
		if details:
			label += f" ({details})"
		print(label)

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

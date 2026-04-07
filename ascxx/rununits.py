import argparse
import configparser
import os
import pathlib
import re
import sys


_VAR_RE = re.compile(
	r"^[a-zA-Z_][a-zA-Z_0-9]*(\[[0-9]+|'[^']*'\])*(\.[a-zA-Z_][a-zA-Z_0-9]*(\[[0-9]+|'[^']*'\])*)*$"
)


def _note(message):
	print(message, file=sys.stderr)


def _load_ascpy():
	import platform

	if platform.system() == "Windows":
		os.add_dll_directory(pathlib.Path(__file__).parent.parent)
	import ascpy

	return ascpy


def _norm_path(path_value):
	return os.path.normcase(str(pathlib.Path(path_value).resolve()))


def _same_path(lhs, rhs):
	return _norm_path(lhs) == _norm_path(rhs)


def _types_in_file(library, filen):
	target = pathlib.Path(filen)
	types = []
	for module in library.getModules():
		module_filename = module.getFilename()
		if not module_filename:
			continue
		if _same_path(module_filename, target):
			for model_type in library.getModuleTypes(module):
				types.append((model_type, module_filename))
	return types


def _candidate_type_names(library, filen):
	return [t.getName().toString() for t, _module_filename in _types_in_file(library, filen)]


def _load_type(filen, model=None, allow_infer=False):
	ascpy = _load_ascpy()
	library = ascpy.Library()
	library.load(str(filen))
	inferred = False
	if model is None and allow_infer:
		model = pathlib.Path(filen).stem
		inferred = True
		_note(f"Assuming model '{model}' from file stem of {filen}")

	type_names = _candidate_type_names(library, filen)
	if model is None:
		raise RuntimeError("Model name required for scoped operation")
	if model not in type_names:
		lines = [f"Model '{model}' not found in '{filen}'."]
		if type_names:
			lines.append("Available models:")
			lines.extend(f"  {name}" for name in type_names)
		raise RuntimeError("\n".join(lines))
	for model_type, module_filename in _types_in_file(library, filen):
		if model_type.getName().toString() == model:
			return library, model_type, module_filename, inferred
	raise RuntimeError(f"Model '{model}' found in '{filen}' but could not be resolved")


def _canonical_scope(model_type, module_filename):
	model_name = model_type.getName().toString()
	if module_filename and model_name:
		return f"{module_filename}::{model_name}"
	if module_filename:
		return module_filename
	if model_name:
		return model_name
	raise RuntimeError("Unable to determine model scope")


def _prime_global_units_context():
	ascpy = _load_ascpy()
	library = ascpy.Library()
	library.load("atoms.a4l")
	return ascpy, library


def _validate_units_for_type(ascpy, model_type, units_text):
	units = ascpy.Units(units_text)
	if units.getDimensions() != model_type.getDimensions():
		raise RuntimeError(
			f"Units '{units_text}' do not match dimensions of type '{model_type.getName().toString()}'."
		)


def _resolve_instance(sim, varname):
	if not _VAR_RE.match(varname):
		raise RuntimeError(f"Variable name '{varname}' does not match allowable pattern.")
	try:
		return eval(f"sim.{varname}", {"sim": sim}, {})
	except Exception as exc:
		raise RuntimeError(f"Unable to resolve variable '{varname}': {exc}") from exc


def _reload_overrides(ascpy):
	rc = ascpy.reloadDisplayUnitsOverrides()
	if rc != 0:
		raise RuntimeError("Failed to reload units-overrides preferences")


def _save_overrides(ascpy):
	rc = ascpy.saveDisplayUnitsOverrides()
	if rc != 0:
		raise RuntimeError("Failed to save units-overrides preferences")


def _validate_scope_args(parser, args):
	if args.file is None and getattr(args, "model", None) is not None:
		parser.error("--model requires --file")
	if getattr(args, "varname", None) is not None and args.file is None:
		parser.error("--var requires --file")


def _parse_set_args(parser, args):
	if args.varname is None:
		if len(args.rest) != 2:
			parser.error("set requires TYPE and UNITS, or --var with UNITS")
		return args.rest[0], args.rest[1]
	if len(args.rest) != 1:
		parser.error("set with --var requires exactly one UNITS argument")
	return None, args.rest[0]


def _parse_clear_args(parser, args):
	if args.varname is None:
		if len(args.rest) != 1:
			parser.error("clear requires TYPE, or --var with no positional arguments")
		return args.rest[0]
	if args.rest:
		parser.error("clear with --var takes no positional arguments")
	return None


def _config_parser():
	parser = configparser.ConfigParser(interpolation=None)
	parser.optionxform = str
	return parser


def _read_overrides_file(ascpy):
	path = ascpy.getDisplayUnitsOverridesPath()
	parser = _config_parser()
	if path:
		parser.read(path)
	return path, parser


def _print_sections(parser, sections):
	printed = False
	for section in sections:
		if not parser.has_section(section):
			continue
		items = list(parser.items(section))
		if not items:
			continue
		if printed:
			print()
		print(f"[{section}]")
		for key, value in items:
			print(f"{key} = {value}")
		printed = True
	return printed


def _command_set(args):
	ascpy = _load_ascpy()
	type_name, units_text = _parse_set_args(args.parser, args)
	if args.file is None:
		_prime_global_units_context()
		_reload_overrides(ascpy)
		ascpy.setDisplayUnitsTypeOverride(type_name, units_text)
		_save_overrides(ascpy)
		return

	library, model_type, module_filename, _inferred = _load_type(args.file, args.model, allow_infer=True)
	_reload_overrides(ascpy)
	scope = _canonical_scope(model_type, module_filename)
	if args.varname is None:
		_validate_units_for_type(ascpy, library.findType(type_name), units_text)
		ascpy.setDisplayUnitsTypeOverride(type_name, units_text, scope)
	else:
		sim = model_type.getSimulation("sim", True)
		instance = _resolve_instance(sim, args.varname)
		if not instance.isReal():
			raise RuntimeError(f"Variable '{args.varname}' is not real-valued")
		units = ascpy.Units(units_text)
		if units.getDimensions() != instance.getDimensions():
			raise RuntimeError(
				f"Units '{units_text}' do not match dimensions of variable '{args.varname}'."
			)
		name_key = sim.getInstanceName(instance)
		ascpy.setDisplayUnitsNameOverride(name_key, units_text, scope)
	_save_overrides(ascpy)


def _command_clear(args):
	ascpy = _load_ascpy()
	type_name = _parse_clear_args(args.parser, args)
	if args.file is None:
		_prime_global_units_context()
		_reload_overrides(ascpy)
		ascpy.clearDisplayUnitsTypeOverride(type_name)
		_save_overrides(ascpy)
		return

	_library, model_type, module_filename, _inferred = _load_type(args.file, args.model, allow_infer=True)
	_reload_overrides(ascpy)
	scope = _canonical_scope(model_type, module_filename)
	if args.varname is None:
		ascpy.clearDisplayUnitsTypeOverride(type_name, scope)
	else:
		sim = model_type.getSimulation("sim", True)
		instance = _resolve_instance(sim, args.varname)
		name_key = sim.getInstanceName(instance)
		ascpy.clearDisplayUnitsNameOverride(name_key, scope)
	_save_overrides(ascpy)


def _command_list(args):
	ascpy = _load_ascpy()
	path, parser = _read_overrides_file(ascpy)
	if args.file is None:
		sections = []
		if parser.has_section("global"):
			sections.append("global")
		sections.extend(section for section in parser.sections() if section != "global")
		if not _print_sections(parser, sections):
			print("No units overrides set.")
		return

	_library, model_type, module_filename, _inferred = _load_type(args.file, args.model, allow_infer=True)
	scope = _canonical_scope(model_type, module_filename)
	sections = ["global", scope]
	if module_filename and module_filename != scope:
		sections.append(module_filename)
	if not _print_sections(parser, sections):
		if path:
			print(f"No units overrides found in {path}.")
		else:
			print("No units overrides set.")


def main(argv=None):
	parser = argparse.ArgumentParser(description="Manage ASCEND display-units overrides.")
	subparsers = parser.add_subparsers(dest="command", required=True)

	p_set = subparsers.add_parser("set", help="Set a display-units override")
	p_set.add_argument("-f", "--file", type=pathlib.Path, help="ASCEND model file")
	p_set.add_argument("-m", "--model", help="Model name (defaults to file stem when --file is used)")
	p_set.add_argument("-v", "--var", dest="varname", help="Variable name for a variable-specific override")
	p_set.add_argument("rest", nargs="+", help="TYPE UNITS, or just UNITS with --var")

	p_clear = subparsers.add_parser("clear", help="Clear a display-units override")
	p_clear.add_argument("-f", "--file", type=pathlib.Path, help="ASCEND model file")
	p_clear.add_argument("-m", "--model", help="Model name (defaults to file stem when --file is used)")
	p_clear.add_argument("-v", "--var", dest="varname", help="Variable name for a variable-specific override")
	p_clear.add_argument("rest", nargs="*", help="TYPE, or no positional arguments with --var")

	p_list = subparsers.add_parser("list", help="List saved display-units overrides")
	p_list.add_argument("-f", "--file", type=pathlib.Path, help="ASCEND model file")
	p_list.add_argument("-m", "--model", help="Model name (defaults to file stem when --file is used)")

	args = parser.parse_args(argv)
	args.parser = parser
	_validate_scope_args(parser, args)

	if args.command == "set":
		_command_set(args)
		return 0
	if args.command == "clear":
		_command_clear(args)
		return 0
	if args.command == "list":
		_command_list(args)
		return 0
	raise RuntimeError(f"Unsupported command '{args.command}'")


if __name__ == "__main__":
	try:
		sys.exit(main())
	except Exception as exc:
		sys.stderr.write(f"{pathlib.Path(sys.argv[0]).name}: {exc}\n")
		sys.exit(1)

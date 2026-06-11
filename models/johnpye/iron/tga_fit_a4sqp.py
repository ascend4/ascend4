#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ctypes
import importlib.util
import json
import math
import multiprocessing as mp
import os
import pathlib
import sys
import tempfile
import time
from dataclasses import dataclass
from typing import Any


ROOT = pathlib.Path(__file__).resolve().parents[3]
ASCXX = ROOT / "ascxx"
PYGTK = ROOT / "pygtk"
_ASCEND_LIBRARY = None
_LOADED_ASCEND_FILES: set[str] = set()
_LOG_HANDLE = None
_START_TIME = time.monotonic()


def _fflush_all() -> None:
	sys.stdout.flush()
	sys.stderr.flush()
	try:
		ctypes.CDLL(None).fflush(None)
	except Exception:
		pass


def _open_log(path: pathlib.Path | None) -> None:
	global _LOG_HANDLE
	if path is None:
		return
	path.parent.mkdir(parents=True, exist_ok=True)
	_LOG_HANDLE = path.open("a", encoding="utf-8")


def _close_log() -> None:
	global _LOG_HANDLE
	if _LOG_HANDLE is not None:
		_LOG_HANDLE.flush()
		_LOG_HANDLE.close()
		_LOG_HANDLE = None


def _log_event(event: str, **fields: Any) -> None:
	payload = {
		"elapsed_s": round(time.monotonic() - _START_TIME, 3),
		"event": event,
	}
	payload.update(fields)
	line = json.dumps(payload, sort_keys=True)
	print(line, file=sys.stderr, flush=True)
	if _LOG_HANDLE is not None:
		print(line, file=_LOG_HANDLE, flush=True)
	_fflush_all()


def _ensure_ascpy_path() -> None:
	for path in (ASCXX, PYGTK):
		text = str(path)
		if text not in sys.path:
			sys.path.insert(0, text)


def _load_ascpy():
	try:
		sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_NOW)
	except Exception:
		pass
	os.environ.setdefault("ASCENDLIBRARY", str(ROOT / "models"))
	if "ASCENDSOLVERS" not in os.environ:
		solver_dirs = [
			str(path) for path in sorted((ROOT / "solvers").iterdir())
			if path.is_dir()
		]
		os.environ["ASCENDSOLVERS"] = os.pathsep.join(solver_dirs)
	_ensure_ascpy_path()
	import ascpy  # type: ignore

	return ascpy


def _get_ascend_library(ascpy):
	global _ASCEND_LIBRARY
	if _ASCEND_LIBRARY is None:
		_ASCEND_LIBRARY = ascpy.Library()
	return _ASCEND_LIBRARY


def _find_method(model_type, name: str):
	for method in model_type.getMethods():
		if method.getName() == name:
			return method
	raise RuntimeError(f"method '{name}' not found on model '{model_type.getName()}'")


def _instance_by_path(sim, path: str):
	inst = sim
	for part in path.split("."):
		while "[" in part:
			name, rest = part.split("[", 1)
			if name:
				inst = getattr(inst, name)
			index_text, part = rest.split("]", 1)
			if not index_text.isdigit():
				raise ValueError(f"only integer array indices are supported in '{path}'")
			inst = inst[int(index_text)]
		if part:
			inst = getattr(inst, part)
	return inst


def _set_real(inst, value: float, units: str | None = None) -> None:
	if units:
		inst.setRealValueWithUnits(float(value), units)
	else:
		inst.setRealValue(float(value))


class MemoryIntegratorReporter:
	def __init__(self, ascpy, integrator, observed_paths: list[str]):
		class _Reporter(ascpy.IntegratorReporterCxx):
			def __init__(self, owner, wrapped):
				self._owner = owner
				ascpy.IntegratorReporterCxx.__init__(self, wrapped)

			def initOutput(self):
				self._owner.rows = []
				return 1

			def updateStatus(self):
				return 1

			def recordObservedValues(self):
				self._owner.capture_row()
				return 1

			def closeOutput(self):
				return 0

		self.integrator = integrator
		self.observed_paths = observed_paths
		self.rows: list[tuple[float, list[float]]] = []
		self.reporter = _Reporter(self, integrator)

	def capture_row(self) -> None:
		values = []
		for i in range(self.integrator.getNumObservedItems()):
			inst = self.integrator.getObservedInstance(i)
			if not inst.isReal():
				raise RuntimeError("only real-valued observed variables are supported")
			values.append(float(inst.getRealValue()))
		self.rows.append((float(self.integrator.getCurrentTime()), values))


@dataclass(frozen=True)
class ParameterSpec:
	name: str
	target: str
	initial: float
	lower: float
	upper: float
	units: str | None = None
	scale: float = 1.0


@dataclass(frozen=True)
class TrialSpec:
	trial_id: str
	times: tuple[float, ...]
	observed: tuple[float, ...]
	weights: tuple[float, ...]
	setup_methods: tuple[str, ...]
	overrides: dict[str, Any]
	observed_path: str
	time_units: str
	model_file: str
	model: str
	integrator: str
	run_base_methods: tuple[str, ...]
	parameters: tuple[ParameterSpec, ...]
	active_parameters: tuple[str, ...] = ()


def _parse_parameter(raw: dict[str, Any]) -> ParameterSpec:
	return ParameterSpec(
		name=str(raw["name"]),
		target=str(raw["target"]),
		initial=float(raw["initial"]),
		lower=float(raw.get("lower", -1.0e20)),
		upper=float(raw.get("upper", 1.0e20)),
		units=raw.get("units", raw.get("unit")),
		scale=float(raw.get("scale", 1.0)),
	)


def _select_parameter_names(params: list[ParameterSpec], selection: str | None) -> tuple[str, ...]:
	names = [param.name for param in params]
	if selection in (None, "", "all"):
		return tuple(names)
	requested = [item.strip() for item in selection.split(",") if item.strip()]
	if not requested:
		raise ValueError("parameter selection is empty")
	unknown = [name for name in requested if name not in names]
	if unknown:
		raise ValueError(
			"unknown fit parameter(s): "
			+ ", ".join(unknown)
			+ "; available: "
			+ ", ".join(names)
		)
	return tuple(dict.fromkeys(requested))


def _parse_manifest(
	path: pathlib.Path,
	fit_params: str | None = None,
	trial_selection: str | None = None,
) -> tuple[list[ParameterSpec], list[TrialSpec], tuple[str, ...]]:
	data = json.loads(path.read_text())
	asc = data.get("ascend", {})
	params = [_parse_parameter(item) for item in data["parameters"]]
	active_parameters = _select_parameter_names(
		params,
		fit_params if fit_params is not None else data.get("fit_parameters"),
	)
	selected_trials = None
	if trial_selection:
		selected_trials = set(_parse_trial_selection(trial_selection))
	base_methods = tuple(str(m) for m in asc.get("run_methods", []))
	default_observed = str(asc.get("observed", "reduction_degree"))
	default_units = str(asc.get("time_units", "s"))
	trials = []
	for raw in data["trials"]:
		trial_id = str(raw.get("id", len(trials)))
		if selected_trials is not None and trial_id not in selected_trials:
			continue
		times = tuple(float(v) for v in raw["times"])
		observed = tuple(float(v) for v in raw["observed"])
		if len(times) != len(observed):
			raise ValueError(f"trial '{raw.get('id')}' has mismatched times/observed lengths")
		weights = tuple(float(v) for v in raw.get("weights", [1.0] * len(times)))
		if len(weights) != len(times):
			raise ValueError(f"trial '{raw.get('id')}' has mismatched weights length")
		trials.append(
			TrialSpec(
				trial_id=trial_id,
				times=times,
				observed=observed,
				weights=weights,
				setup_methods=tuple(str(m) for m in raw.get("run_methods", [])),
				overrides=dict(raw.get("overrides", {})),
				observed_path=str(raw.get("observed", default_observed)),
				time_units=str(raw.get("time_units", default_units)),
				model_file=str(asc["model_file"]),
				model=str(asc["model"]),
				integrator=str(asc.get("integrator", "IDA")),
				run_base_methods=base_methods,
				parameters=tuple(params),
				active_parameters=active_parameters,
			)
		)
	if not trials:
		raise ValueError("no trials selected")
	return params, trials, active_parameters


def _parse_trial_selection(selection: str) -> list[str]:
	values: list[str] = []
	for chunk in selection.split(","):
		token = chunk.strip().upper()
		if not token:
			continue
		if ":" in token:
			start_text, end_text = [part.strip().upper() for part in token.split(":", 1)]
			start = _trial_number(start_text)
			end = _trial_number(end_text)
			prefix = _trial_prefix(start_text) or _trial_prefix(end_text) or "X"
			step = 1 if end >= start else -1
			values.extend(f"{prefix}{number}" for number in range(start, end + step, step))
		else:
			prefix = _trial_prefix(token) or "X"
			values.append(f"{prefix}{_trial_number(token)}")
	if not values:
		raise ValueError("no trials selected")
	return list(dict.fromkeys(values))


def _trial_number(value: str) -> int:
	text = value.strip().upper()
	if text.startswith("X"):
		text = text[1:]
	if not text.isdigit():
		raise ValueError(f"invalid trial identifier '{value}'")
	return int(text)


def _trial_prefix(value: str) -> str | None:
	text = value.strip().upper()
	for index, char in enumerate(text):
		if char.isdigit():
			return text[:index] or None
	return None


def _theta_values(parameters: tuple[ParameterSpec, ...], active: tuple[str, ...], theta: tuple[float, ...]) -> dict[str, float]:
	active_names = active or tuple(param.name for param in parameters)
	if len(active_names) != len(theta):
		raise ValueError(f"theta has {len(theta)} values for {len(active_names)} active parameters")
	values = {param.name: param.initial for param in parameters}
	values.update(zip(active_names, theta))
	return values


def _run_trial_ascend(args: tuple[TrialSpec, tuple[float, ...]]) -> tuple[str, list[float]]:
	trial, theta = args
	ascpy = _load_ascpy()
	lib = _get_ascend_library(ascpy)
	model_path = str((ROOT / trial.model_file).resolve() if not os.path.isabs(trial.model_file) else pathlib.Path(trial.model_file).resolve())
	if model_path not in _LOADED_ASCEND_FILES:
		lib.load(model_path)
		_LOADED_ASCEND_FILES.add(model_path)
	model_type = lib.findType(trial.model)
	sim = model_type.getSimulation(f"fit_{trial.trial_id}", False)
	sim.setSolver(ascpy.Solver("QRSlv"))

	for method_name in trial.run_base_methods + trial.setup_methods:
		sim.run(_find_method(model_type, method_name))

	for path, raw in trial.overrides.items():
		if isinstance(raw, dict):
			_set_real(_instance_by_path(sim, path), float(raw["value"]), raw.get("units"))
		else:
			_set_real(_instance_by_path(sim, path), float(raw))

	parameter_values = _theta_values(trial.parameters, trial.active_parameters, theta)
	for spec in trial.parameters:
		value = parameter_values[spec.name]
		_set_real(_instance_by_path(sim, spec.target), float(value) * spec.scale, spec.units)

	integrator = ascpy.Integrator(sim)
	integrator.setEngine(trial.integrator)
	integrator.findIndependentVar()
	integrator.setTimesteps(ascpy.Units(trial.time_units), list(trial.times))
	integrator.clearObservedInstances()
	integrator.addObservedInstance(_instance_by_path(sim, trial.observed_path))
	reporter = MemoryIntegratorReporter(ascpy, integrator, [trial.observed_path])
	integrator.setReporter(reporter.reporter)
	integrator.analyse()
	integrator.solve()

	if len(reporter.rows) != len(trial.times):
		raise RuntimeError(
			f"trial '{trial.trial_id}' returned {len(reporter.rows)} rows, expected {len(trial.times)}"
		)
	residuals = []
	for (_, values), observed, weight in zip(reporter.rows, trial.observed, trial.weights):
		residuals.append(math.sqrt(weight) * (values[0] - observed))
	return trial.trial_id, residuals


@dataclass(frozen=True)
class SyntheticTrial:
	trial_id: str
	times: tuple[float, ...]
	observed: tuple[float, ...]
	weights: tuple[float, ...]


def _run_trial_synthetic(args: tuple[SyntheticTrial, tuple[float, ...]]) -> tuple[str, list[float]]:
	trial, theta = args
	a, b = theta
	residuals = []
	for t, y, w in zip(trial.times, trial.observed, trial.weights):
		model = a * math.exp(-b * t)
		residuals.append(math.sqrt(w) * (model - y))
	return trial.trial_id, residuals


class ParallelResidualEvaluator:
	def __init__(self, trials, worker, processes: int):
		self.trials = list(trials)
		self.eval_count = 0
		self.worker = worker
		self.processes = processes
		self.pool = mp.Pool(processes=processes) if processes != 1 else None
		self._last_x: tuple[float, ...] | None = None
		self._last_residuals: list[float] | None = None

	def close(self) -> None:
		if self.pool is not None:
			self.pool.close()
			self.pool.join()
			self.pool = None

	def residuals(self, x: tuple[float, ...]) -> list[float]:
		if self._last_x == x and self._last_residuals is not None:
			return list(self._last_residuals)
		self.eval_count += 1
		_log_event("residual_eval_start", eval=self.eval_count, trials=len(self.trials))
		jobs = [(trial, x) for trial in self.trials]
		if self.pool is None:
			results = [self.worker(job) for job in jobs]
		else:
			results = self.pool.map(self.worker, jobs)
		by_id = {trial_id: residuals for trial_id, residuals in results}
		flat: list[float] = []
		for trial in self.trials:
			flat.extend(by_id[trial.trial_id])
		self._last_x = x
		self._last_residuals = list(flat)
		_log_event("residual_eval_done", eval=self.eval_count, residuals=len(flat))
		return flat


class FiniteDifferenceObjective:
	def __init__(self, evaluator: ParallelResidualEvaluator, rel_step: float = 1.0e-5):
		self.evaluator = evaluator
		self.rel_step = rel_step
		self.value_count = 0
		self.gradient_count = 0

	def value(self, x: tuple[float, ...]) -> float:
		self.value_count += 1
		r = self.evaluator.residuals(x)
		value = 0.5 * sum(v * v for v in r)
		_log_event("objective_value", eval=self.value_count, value=value)
		return value

	def gradient(self, x: tuple[float, ...]) -> list[float]:
		self.gradient_count += 1
		_log_event("gradient_start", eval=self.gradient_count, parameters=len(x))
		base = self.evaluator.residuals(x)
		grad = []
		for j, xj in enumerate(x):
			step = self.rel_step * max(1.0, abs(xj))
			xp = list(x)
			xp[j] += step
			rp = self.evaluator.residuals(tuple(xp))
			component = sum(b * (p - b) / step for b, p in zip(base, rp))
			grad.append(component)
			_log_event("gradient_component", eval=self.gradient_count, index=j, step=step, value=component)
		_log_event("gradient_done", eval=self.gradient_count)
		return grad


class A4SqpPythonProblem:
	def __init__(self, lib_path: pathlib.Path, objective: FiniteDifferenceObjective):
		self.lib = ctypes.CDLL(str(lib_path))
		self.objective = objective
		self._callbacks: list[Any] = []
		self._configure_api()

	def _configure_api(self) -> None:
		c_int = ctypes.c_int
		c_double_p = ctypes.POINTER(ctypes.c_double)
		self.EvalF = ctypes.CFUNCTYPE(
			c_int, c_int, c_double_p, c_int, c_double_p, ctypes.c_void_p
		)
		self.EvalGradF = ctypes.CFUNCTYPE(
			c_int, c_int, c_double_p, c_int, c_double_p, ctypes.c_void_p
		)
		self.lib.CreateA4SqpProblem.restype = ctypes.c_void_p
		self.lib.CreateA4SqpProblem.argtypes = [
			c_int, c_double_p, c_double_p, c_int, c_double_p, c_double_p,
			c_int, c_int, c_int, self.EvalF, ctypes.c_void_p, self.EvalGradF,
			ctypes.c_void_p, ctypes.c_void_p,
		]
		self.lib.FreeA4SqpProblem.argtypes = [ctypes.c_void_p]
		self.lib.AddA4SqpIntOption.argtypes = [ctypes.c_void_p, ctypes.c_char_p, c_int]
		self.lib.AddA4SqpNumOption.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_double]
		self.lib.AddA4SqpStrOption.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
		self.lib.A4SqpSolve.restype = c_int
		self.lib.A4SqpSolve.argtypes = [
			ctypes.c_void_p, c_double_p, ctypes.c_void_p, c_double_p,
			ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
		]

	def solve(
		self,
		x0: list[float],
		lower: list[float],
		upper: list[float],
		options: dict[str, Any],
	) -> tuple[int, float, list[float]]:
		n = len(x0)
		x_arr = (ctypes.c_double * n)(*x0)
		l_arr = (ctypes.c_double * n)(*lower)
		u_arr = (ctypes.c_double * n)(*upper)

		@self.EvalF
		def eval_f(n_, x, new_x, obj_value, user_data):
			try:
				xv = tuple(float(x[i]) for i in range(n_))
				obj_value[0] = self.objective.value(xv)
				return 1
			except Exception as exc:
				print(f"A4SQP eval_f failed: {exc}", file=sys.stderr)
				return 0

		@self.EvalGradF
		def eval_grad_f(n_, x, new_x, grad_f, user_data):
			try:
				xv = tuple(float(x[i]) for i in range(n_))
				grad = self.objective.gradient(xv)
				for i, value in enumerate(grad):
					grad_f[i] = value
				return 1
			except Exception as exc:
				print(f"A4SQP eval_grad_f failed: {exc}", file=sys.stderr)
				return 0

		self._callbacks = [eval_f, eval_grad_f]
		problem = self.lib.CreateA4SqpProblem(
			n, l_arr, u_arr, 0, None, None, 0, 0, 0,
			eval_f, None, eval_grad_f, None, None,
		)
		if not problem:
			raise RuntimeError("CreateA4SqpProblem failed")
		try:
			for key, value in options.items():
				key_b = str(key).encode()
				if isinstance(value, bool):
					ok = self.lib.AddA4SqpIntOption(problem, key_b, int(value))
				elif isinstance(value, int):
					ok = self.lib.AddA4SqpIntOption(problem, key_b, value)
				elif isinstance(value, float):
					ok = self.lib.AddA4SqpNumOption(problem, key_b, value)
				else:
					ok = self.lib.AddA4SqpStrOption(problem, key_b, str(value).encode())
				if not ok:
					raise RuntimeError(f"A4SQP rejected option '{key}'")
			obj = ctypes.c_double(0.0)
			status = self.lib.A4SqpSolve(problem, x_arr, None, ctypes.byref(obj), None, None, None, None)
			return int(status), float(obj.value), [float(x_arr[i]) for i in range(n)]
		finally:
			self.lib.FreeA4SqpProblem(problem)


def _import_module_from_path(name: str, path: pathlib.Path):
	spec = importlib.util.spec_from_file_location(name, path)
	if spec is None or spec.loader is None:
		raise ImportError(f"could not load {name} from {path}")
	module = importlib.util.module_from_spec(spec)
	sys.modules[name] = module
	spec.loader.exec_module(module)
	return module


def _load_kinetics_module(kinetics_root: pathlib.Path, module_name: str):
	path = kinetics_root / f"{module_name}.py"
	if not path.exists():
		raise FileNotFoundError(f"could not find {path}")
	root_text = str(kinetics_root)
	if root_text not in sys.path:
		sys.path.insert(0, root_text)
	return _import_module_from_path(module_name, path)


def _default_parameter_specs(base_model: str, fit_contract: dict[str, Any] | None, target_prefix: str) -> list[ParameterSpec]:
	if fit_contract is not None:
		specs = []
		for raw in fit_contract.get("parameters", []):
			item = dict(raw)
			name = str(item["name"])
			lower = float(item.get("low", item.get("lower", -1.0e20)))
			upper = float(item.get("high", item.get("upper", 1.0e20)))
			if "initial" in item:
				initial = float(item["initial"])
			elif "fixed" in item:
				initial = float(item["fixed"])
			elif str(item.get("scale", "linear")).lower() == "log" and lower > 0 and upper > 0:
				initial = math.sqrt(lower * upper)
			else:
				initial = 0.5 * (lower + upper)
			specs.append(
				ParameterSpec(
					name=name,
					target=target_prefix + str(item.get("target", name)),
					initial=initial,
					lower=lower,
					upper=upper,
					units=item.get("unit", item.get("units")),
				)
			)
		return specs

	use_msbed = "msbed" in base_model
	prefix = target_prefix
	specs: list[ParameterSpec] = [
		ParameterSpec("k_fb_multiplier", prefix + "k_fb_multiplier", 1.0, 0.1, 20.0),
		ParameterSpec("k_bed_multiplier", prefix + "k_bed_multiplier", 1.0, 0.1, 20.0),
		ParameterSpec("eps_bed", prefix + "eps_bed", 0.45, 0.2, 0.8),
		ParameterSpec("tau_bed", prefix + "tau_bed", 3.0, 1.2, 8.0),
	]
	if use_msbed:
		for name, initial in (
			("etak0_1", 3.0e-4),
			("etak0_2", 1.5e-4),
			("etak0_3", 7.5e-4),
			("etak0_4", 1.5e-4),
		):
			step = name.rsplit("_", 1)[1]
			specs.append(ParameterSpec(name, prefix + f"etak0[{step}]", initial, initial * 0.05, initial * 20.0, "1/s/Pa"))
	else:
		kinetic_steps = ((1, 3.0e-4), (2, 1.5e-4), (3, 7.5e-4))
		if base_model != "tga_reduction_screen_dyn":
			kinetic_steps += ((4, 1.5e-4),)
		for step, initial in kinetic_steps:
			specs.append(ParameterSpec(f"eta_{step}", prefix + f"eta[{step}]", 1.0, 0.05, 2.0))
			specs.append(ParameterSpec(f"k0_{step}", prefix + f"k0[{step}]", initial, initial * 0.1, initial * 10.0, "1/s/Pa"))
	arrhenius_steps = ((1, 55.0), (2, 60.0), (3, 62.0))
	if use_msbed or base_model != "tga_reduction_screen_dyn":
		arrhenius_steps += ((4, 60.0),)
	for step, initial in arrhenius_steps:
		specs.append(ParameterSpec(f"E_a_{step}", prefix + f"E_a[{step}]", initial, initial - 20.0, initial + 20.0, "kJ/mol"))
	if use_msbed:
		specs.extend(
			[
				ParameterSpec("Sh_particle", prefix + "Sh_particle", 2.0, 0.1, 10.0),
				ParameterSpec("a_open", prefix + "a_open", 0.0, 0.0, 5.0),
				ParameterSpec("a_close", prefix + "a_close", 0.0, 0.0, 8.0),
				ParameterSpec("beta_T", prefix + "beta_T", 0.0, -8.0, 8.0),
			]
		)
	return specs


def _row_from_tga_run(run, base_model: str) -> dict[str, str]:
	conditions = run.conditions
	row = {
		"Number_X": run.test_id,
		"T_C": f"{conditions.temperature_c:.12g}",
		"y_list": f"{conditions.inlet_h2_fraction:.12g}",
		"h_list_mm": f"{conditions.bed_height_mm:.12g}",
		"L_list_mm": f"{conditions.freeboard_height_mm:.12g}",
		"m_list_mg": f"{conditions.initial_sample_mass_mg:.12g}",
		"D_C_mm": f"{conditions.crucible_internal_diameter_mm:.12g}",
	}
	if base_model != "tga_reduction_screen_dyn":
		row["d_p"] = f"{conditions.particle_diameter_m:.12g}"
	return row


def _normalise_observed_rd(values: list[float]) -> tuple[float, ...]:
	if values and max(values) > 1.5:
		return tuple(value / 100.0 for value in values)
	return tuple(values)


def _build_tga_trials(args, temp_dir: pathlib.Path) -> tuple[list[ParameterSpec], list[TrialSpec], tuple[str, ...]]:
	kinetics_root = args.kinetics_root.expanduser().resolve()
	tga_runs = _load_kinetics_module(kinetics_root, "tga_runs")
	batch_tgadyn = _load_kinetics_module(kinetics_root, "batch_tgadyn")
	runs = tga_runs.load_test_runs(
		args.tga_selection,
		base_dir=kinetics_root,
		conditions_file=args.conditions_file,
		x_data_file=args.x_data_file,
	)
	if not runs:
		raise ValueError("no TGA runs selected")

	fit_contract = batch_tgadyn.load_fit_contract(ROOT, args.ascend_model_require, args.ascend_base_model)
	core_path = batch_tgadyn.resolve_core_instance_path(args.ascend_base_model)
	target_prefix = "core." if core_path is not None else ""
	params = _default_parameter_specs(args.ascend_base_model, fit_contract, target_prefix)
	active_parameters = _select_parameter_names(params, args.fit_params)
	post_methods = tuple(args.ascend_post_method or ())
	if not post_methods and fit_contract is None and args.ascend_base_model == "tga_reduction_screen_dyn":
		post_methods = ("ode_init",)
	run_method = args.ascend_run_method
	if run_method is None and fit_contract is None and args.ascend_base_model == "tga_reduction_screen_dyn":
		run_method = "algebraic_base"

	trials = []
	for index, run in enumerate(runs, start=1):
		model_name = f"a4sqp_tga_{run.test_id}"
		setup_name = f"setup_{run.test_id}"
		row = _row_from_tga_run(run, args.ascend_base_model)
		model_text = batch_tgadyn.build_generated_model_text(
			model_name,
			setup_name,
			row,
			run_method,
			args.ascend_observe_method,
			args.ascend_model_require,
			args.ascend_base_model,
			fit_contract,
		)
		model_path = temp_dir / f"{model_name}.a4c"
		model_path.write_text(model_text, encoding="utf-8")
		times = tuple(float(value) for value in run.time_min)
		observed = _normalise_observed_rd([float(value) for value in run.reduction_degree])
		if len(times) != len(observed):
			raise ValueError(f"{run.test_id} has mismatched time/RD lengths")
		trials.append(
			TrialSpec(
				trial_id=run.test_id,
				times=times,
				observed=observed,
				weights=tuple(1.0 for _ in times),
				setup_methods=(setup_name,) + post_methods,
				overrides={},
				observed_path=target_prefix + "reduction_degree" if target_prefix else "reduction_degree",
				time_units="min",
				model_file=str(model_path),
				model=model_name,
				integrator=args.integrator,
				run_base_methods=(),
				parameters=tuple(params),
				active_parameters=active_parameters,
			)
		)
	return params, trials, active_parameters


def _emit_result(payload: dict[str, Any], args) -> None:
	text = json.dumps(payload, indent=2)
	print(text, flush=True)
	result_file = getattr(args, "result_file", None)
	if result_file is not None:
		result_file.parent.mkdir(parents=True, exist_ok=True)
		result_file.write_text(text + "\n", encoding="utf-8")
	_log_event("result", status=payload.get("status"), objective=payload.get("objective"), result_file=str(result_file) if result_file is not None else None)
	_fflush_all()


def _run_self_test(args) -> int:
	trials = []
	true_a = 2.5
	true_b = 0.35
	for trial_index, scale in enumerate((1.0, 1.0, 1.0)):
		times = tuple(0.4 * i for i in range(1, 12 + trial_index))
		observed = tuple(scale * true_a * math.exp(-true_b * t) for t in times)
		trials.append(SyntheticTrial(str(trial_index), times, observed, tuple(1.0 for _ in times)))
	evaluator = ParallelResidualEvaluator(trials, _run_trial_synthetic, args.jobs)
	try:
		objective = FiniteDifferenceObjective(evaluator, rel_step=args.fd_rel_step)
		solver = A4SqpPythonProblem(args.a4sqp_lib, objective)
		status, obj, x = solver.solve(
			x0=[1.0, 0.1],
			lower=[0.0, 0.0],
			upper=[10.0, 10.0],
			options={"max_iter": args.max_iter, "hessian": "BFGS"},
		)
	finally:
		evaluator.close()
	payload = {"status": status, "objective": obj, "x": x}
	_emit_result(payload, args)
	return 0 if status >= 0 else 1


def _run_manifest(args) -> int:
	params, trials, active_parameters = _parse_manifest(args.manifest, args.fit_params, args.trials)
	evaluator = ParallelResidualEvaluator(trials, _run_trial_ascend, args.jobs)
	try:
		objective = FiniteDifferenceObjective(evaluator, rel_step=args.fd_rel_step)
		solver = A4SqpPythonProblem(args.a4sqp_lib, objective)
		active_specs = [param for param in params if param.name in active_parameters]
		status, obj, x = solver.solve(
			x0=[p.initial for p in active_specs],
			lower=[p.lower for p in active_specs],
			upper=[p.upper for p in active_specs],
			options={"max_iter": args.max_iter, "hessian": "BFGS"},
		)
	finally:
		evaluator.close()
	all_values = {param.name: param.initial for param in params}
	all_values.update({param.name: value for param, value in zip(active_specs, x)})
	payload = {
		"status": status,
		"objective": obj,
		"active_parameters": list(active_parameters),
		"parameters": all_values,
	}
	_emit_result(payload, args)
	return 0 if status >= 0 else 1


def _run_tga_selection(args) -> int:
	with tempfile.TemporaryDirectory(prefix="tga_fit_a4sqp_") as temp_name:
		params, trials, active_parameters = _build_tga_trials(args, pathlib.Path(temp_name))
		active_specs = [param for param in params if param.name in active_parameters]
		evaluator = ParallelResidualEvaluator(trials, _run_trial_ascend, args.jobs)
		try:
			objective = FiniteDifferenceObjective(evaluator, rel_step=args.fd_rel_step)
			solver = A4SqpPythonProblem(args.a4sqp_lib, objective)
			status, obj, x = solver.solve(
				x0=[p.initial for p in active_specs],
				lower=[p.lower for p in active_specs],
				upper=[p.upper for p in active_specs],
				options={"max_iter": args.max_iter, "hessian": "BFGS"},
			)
		finally:
			evaluator.close()
	all_values = {param.name: param.initial for param in params}
	all_values.update({param.name: value for param, value in zip(active_specs, x)})
	payload = {
		"status": status,
		"objective": obj,
		"trials": [trial.trial_id for trial in trials],
		"active_parameters": list(active_parameters),
		"parameters": all_values,
	}
	_emit_result(payload, args)
	return 0 if status >= 0 else 1


def main(argv: list[str] | None = None) -> int:
	ap = argparse.ArgumentParser(description="Multiprocessing A4SQP driver for ASCEND TGA fitting.")
	ap.add_argument("--manifest", type=pathlib.Path, help="JSON manifest describing ASCEND trials.")
	ap.add_argument("--tga-selection", help="TGA trial selection, e.g. X1:X6 or X1,X3,X5.")
	ap.add_argument("--trials", help="Trial selection for --manifest, e.g. X1:X6 or X1,X3.")
	ap.add_argument("--fit-params", default="all", help="Comma-separated parameter names to optimize, or 'all'.")
	ap.add_argument("--kinetics-root", type=pathlib.Path, default=pathlib.Path("/home/john/kinetics"))
	ap.add_argument("--conditions-file", help="Optional explicit path to 'Test_Conditions - Total.csv'.")
	ap.add_argument("--x-data-file", help="Optional explicit path to 'X_data.csv'.")
	ap.add_argument("--ascend-model-require", default="johnpye/iron/tgadyn.a4c")
	ap.add_argument("--ascend-base-model", default="tga_reduction_screen_dyn")
	ap.add_argument("--ascend-run-method", help="ASCEND setup method run before metadata assignments in generated TGA cases.")
	ap.add_argument("--ascend-post-method", action="append", help="ASCEND setup method run after generated TGA case setup; can be repeated.")
	ap.add_argument("--ascend-observe-method", default="observe_cli_default")
	ap.add_argument("--integrator", default="IDA")
	ap.add_argument("--self-test", action="store_true", help="Run a synthetic multiprocessing/A4SQP smoke test.")
	ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
	ap.add_argument("--fd-rel-step", type=float, default=1.0e-5)
	ap.add_argument("--max-iter", type=int, default=100)
	ap.add_argument("--log-file", type=pathlib.Path, help="Append JSON-lines progress log to this file.")
	ap.add_argument("--result-file", type=pathlib.Path, help="Write final JSON result to this file.")
	ap.add_argument(
		"--a4sqp-lib",
		type=pathlib.Path,
		default=ROOT / "solvers" / "a4sqp" / "liba4sqp.so",
	)
	args = ap.parse_args(argv)
	_open_log(args.log_file)
	try:
		_log_event("start", argv=sys.argv[1:] if argv is None else argv)
		if args.self_test:
			return _run_self_test(args)
		if args.tga_selection:
			return _run_tga_selection(args)
		if args.manifest is None:
			ap.error("provide --manifest, --tga-selection, or --self-test")
		return _run_manifest(args)
	except Exception as exc:
		_log_event("fatal", error=repr(exc))
		raise
	finally:
		_close_log()


if __name__ == "__main__":
	raise SystemExit(main())

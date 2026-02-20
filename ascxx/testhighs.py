#!/usr/bin/env python3
import argparse
import pathlib
import sys

import ascpy

try:
	import tqdm
	_HAVE_TQDM = True
except Exception:
	_HAVE_TQDM = False


class TdlmReporter(ascpy.SolverReporter):
	def __init__(self, sim, tdlm=True, time_limit=None, max_postfix=80, progress_mode="auto", milestone_lines=False):
		super().__init__()
		self.sim = sim
		self.tdlm = tdlm
		self.last_len = 0
		self.use_tqdm = tdlm and _HAVE_TQDM
		self.pbar = None
		self.gap_mode = False
		self.pulse_total = 100
		self.time_limit = time_limit
		self.max_postfix = max_postfix
		self.progress_mode = progress_mode
		self.milestone_lines = milestone_lines
		self.have_primal = False
		self.last_primal = None
		if self.use_tqdm:
			self.pbar = tqdm.tqdm(
				total=self.pulse_total,
				dynamic_ncols=True,
				leave=True,
				file=sys.stdout,
				unit="cb",
				bar_format="{l_bar}{bar}| {postfix}"
			)

	def reportProgress(self, solver_name, message):
		if self.sim is None:
			return
		status = self.sim.getStatus()
		line = f"{solver_name}: " if solver_name else ""
		mip_gap = None
		runtime = None
		msg_parts = []
		if hasattr(status, "getCpuElapsed"):
			runtime = float(status.getCpuElapsed())
		if hasattr(status, "getIterationNum"):
			msg_parts.append(f"iter={status.getIterationNum()}")
		if runtime is not None:
			msg_parts.append(f"cpu={runtime:.2f}s")
		is_mip = hasattr(status, "isMIP") and status.isMIP()
		is_lp = hasattr(status, "isLP") and status.isLP()
		milestone = False
		if is_mip:
			if hasattr(status, "hasMipNodeCount") and status.hasMipNodeCount():
				msg_parts.append(f"mip_nodes={status.getMipNodeCount()}")
			if hasattr(status, "hasMipTotalLpIterations") and status.hasMipTotalLpIterations():
				msg_parts.append(f"mip_lp_iter={status.getMipTotalLpIterations()}")
			if hasattr(status, "hasMipPrimalBound") and status.hasMipPrimalBound():
				primal = float(status.getMipPrimalBound())
				msg_parts.append(f"mip_primal={primal:.8g}")
				if (not self.have_primal) or (self.last_primal != primal):
					milestone = True
					self.have_primal = True
					self.last_primal = primal
			if hasattr(status, "hasMipDualBound") and status.hasMipDualBound():
				msg_parts.append(f"mip_dual={float(status.getMipDualBound()):.8g}")
			if hasattr(status, "hasMipGap") and status.hasMipGap():
				mip_gap = float(status.getMipGap())
				msg_parts.append(f"mip_gap={mip_gap:.6g}")
			elif hasattr(status, "hasMipAbsGap") and status.hasMipAbsGap():
				msg_parts.append(f"mip_abs_gap={float(status.getMipAbsGap()):.8g}")
		elif is_lp:
			if hasattr(status, "hasLpObjective") and status.hasLpObjective():
				msg_parts.append(f"obj={float(status.getLpObjective()):.8g}")

		msg = ", ".join(msg_parts)
		if self.max_postfix and len(msg) > self.max_postfix:
			msg = msg[: self.max_postfix - 3] + "..."
		line += msg if msg else ""
		if self.use_tqdm:
			if milestone and self.milestone_lines:
				line_short = line
				if self.max_postfix and len(line_short) > self.max_postfix:
					line_short = line_short[: self.max_postfix - 3] + "..."
				tqdm.tqdm.write(line_short)
				return
			if self.pbar is not None:
				if solver_name:
					self.pbar.set_description_str(str(solver_name))
				self.pbar.set_postfix_str(str(msg))
				use_gap = (self.progress_mode in ("auto","gap")) and (mip_gap is not None)
				use_time = (self.progress_mode in ("auto","time")) and (runtime is not None)
				use_pulse = (self.progress_mode in ("auto","pulse"))
				if use_gap:
					if not self.gap_mode:
						self.pbar.total = 1.0
						self.pbar.n = 0.0
						self.gap_mode = True
					progress = 1.0 - max(0.0, min(1.0, mip_gap))
					self.pbar.n = progress
				elif use_time and (self.time_limit is not None) and not self.gap_mode:
					if self.time_limit > 0 and self.time_limit < 1e19:
						self.pbar.total = float(self.time_limit)
						self.pbar.n = min(runtime, self.time_limit)
					else:
						self.pbar.update(1)
						if self.pbar.n >= self.pbar.total:
							self.pbar.reset()
				elif use_pulse and not self.gap_mode:
					self.pbar.update(1)
					if self.pbar.n >= self.pbar.total:
						self.pbar.reset()
				self.pbar.refresh()
			return
		if self.tdlm:
			if milestone:
				if self.last_len:
					sys.stderr.write("\r" + (" " * self.last_len) + "\r")
					self.last_len = 0
				sys.stderr.write(line + "\n")
				sys.stderr.flush()
				return
			sys.stderr.write("\r" + line)
			if self.last_len > len(line):
				sys.stderr.write(" " * (self.last_len - len(line)))
			sys.stderr.flush()
			self.last_len = len(line)
		else:
			sys.stderr.write(line + "\n")
			sys.stderr.flush()

	def finalise(self, status):
		if self.use_tqdm and self.pbar is not None:
			self.pbar.close()
			self.pbar = None
		if self.tdlm and self.last_len:
			sys.stderr.write("\n")
			sys.stderr.flush()
			self.last_len = 0
		ascpy.SolverReporter.finalise(self, status)


def set_bool_param(sim, name, value):
	try:
		pp = sim.getParameters()
		for p in pp:
			if p.getName() == name:
				p.setBoolValue(bool(value))
				sim.setParameters(pp)
				return True
	except Exception:
		return False
	return False


def get_bool_param(sim, name):
	try:
		pp = sim.getParameters()
		for p in pp:
			if p.getName() == name:
				return True, bool(p.getBoolValue())
	except Exception:
		return False, False
	return False, False


def get_real_param(sim, name):
	try:
		pp = sim.getParameters()
		for p in pp:
			if p.getName() == name:
				return True, float(p.getRealValue())
	except Exception:
		return False, None
	return False, None


def main():
	parser = argparse.ArgumentParser(description="ASCEND HiGHS progress test driver (Python).")
	parser.add_argument("file", nargs="?", default="models/johnpye/orienteer.a4c")
	parser.add_argument("--model", help="Model name (defaults to file stem)")
	parser.add_argument("--solver", default="HiGHS", help="Solver name (default: HiGHS)")
	parser.add_argument("--no-progress", action="store_true", help="Disable progress callbacks")
	parser.add_argument("--progress-log", action="store_true", help="Enable solver progress logging to console")
	parser.add_argument("--progress", choices=["auto","gap","time","pulse"], default="auto",
		help="Progress bar mode (default: auto)")
	parser.add_argument("--milestone-lines", action="store_true",
		help="Print milestone lines (incumbent/solution) in addition to the bar")
	parser.add_argument("--no-tdlm", action="store_true", help="Print each progress line")
	parser.add_argument("--no-solve", action="store_true", help="Instantiate only, do not solve")
	args = parser.parse_args()

	filepath = pathlib.Path(args.file)
	model = args.model or filepath.stem

	L = ascpy.Library()
	L.load(str(filepath))
	T = L.findType(model)
	M = T.getSimulation("sim", True)  # run on_load

	if args.solver:
		M.setSolver(ascpy.Solver(args.solver))

	if not args.no_solve:
		ok_t, time_limit = get_real_param(M, "time_limit")
		reporter = TdlmReporter(
			M,
			tdlm=not args.no_tdlm,
			time_limit=time_limit if ok_t else None,
			progress_mode=args.progress,
			milestone_lines=args.milestone_lines,
		)
		if reporter.tdlm and not reporter.use_tqdm:
			print("Note: tqdm not available; falling back to stderr progress lines", file=sys.stderr)
		if reporter.use_tqdm:
			print("tqdm enabled", file=sys.stderr)

		M.presolve(M.getSolver())
		if not set_bool_param(M, "progress_callbacks", not args.no_progress):
			if args.no_progress:
				print("Note: progress_callbacks param not available for solver", file=sys.stderr)
		set_bool_param(M, "progress_log", bool(args.progress_log))
		ok, cur = get_bool_param(M, "progress_callbacks")
		if ok:
			print(f"progress_callbacks = {'TRUE' if cur else 'FALSE'}", file=sys.stderr)
		else:
			print("progress_callbacks param unavailable", file=sys.stderr)
		ok, cur = get_bool_param(M, "progress_log")
		if ok:
			print(f"progress_log = {'TRUE' if cur else 'FALSE'}", file=sys.stderr)
		else:
			print("progress_log param unavailable", file=sys.stderr)

		ascpy.setSolverProgressReporter(reporter)
		print("progress reporter enabled", file=sys.stderr)
		try:
			status = M.getStatus()
			res = 0
			while status.isReadyToSolve():
				res = M.iterate()
				status.getSimulationStatus(M)
				if res != 0:
					break
			M.postsolve(status)
			if res != 0:
				raise RuntimeError("Error in slv_iterate")
		finally:
			ascpy.setSolverProgressReporter(None)


if __name__ == "__main__":
	main()

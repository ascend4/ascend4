#!/usr/bin/env python3
"""Solve the ASCEND printing schedule and plot it, or provide an extpy method.

    ./a4 script models/job_shop.py --solver HiGHS --output job_shop.png

The .a4c file is the single source of operation data and MIP equations. No Pyomo,
pandas, or vendor Python API is required. Plotting imports are deliberately lazy
so command-line use can select Agg without changing the ASCEND GUI's backend.
"""

import argparse
from dataclasses import dataclass
from itertools import combinations
import math
from pathlib import Path
import sys


@dataclass(frozen=True)
class Operation:
    """One solved operation, with all times converted to minutes."""

    number: int
    job: str
    machine: str
    predecessor: int
    start: float
    duration: float
    finish: float


def read_schedule(model):
    """Read either an ascpy Simulation's model or the SELF passed by extpy."""
    operations = [
        Operation(i, str(model.job[i].getSymbolValue()),
                  str(model.press[i].getSymbolValue()), model.pred[i].getIntValue(),
                  model.start[i].getRealValue() / 60,
                  model.dur[i].getRealValue() / 60,
                  model.finish[i].getRealValue() / 60)
        for i in sorted(model.op.getSetValue())
    ]
    makespan = model.makespan.getRealValue() / 60
    validate_schedule(operations, makespan)
    return operations, makespan


def validate_schedule(operations, makespan, tol=1e-6):
    """Reject unsolved/invalid values before presenting them as a schedule."""
    if not operations or not math.isfinite(makespan) or makespan <= 0:
        raise ValueError("Missing or invalid schedule; solve the model first.")
    by_number = {op.number: op for op in operations}
    if len(by_number) != len(operations):
        raise ValueError("Duplicate operation numbers.")
    for op in operations:
        if (not all(math.isfinite(v) for v in (op.start, op.duration, op.finish))
                or op.start < -tol or op.duration <= 0
                or abs(op.finish - op.start - op.duration) > tol
                or op.finish > makespan + tol):
            raise ValueError(f"Invalid timing for operation {op.number}.")
        if op.predecessor:
            pred = by_number.get(op.predecessor)
            if pred is None or pred.finish > op.start + tol:
                raise ValueError(f"Precedence violation for operation {op.number}.")
    for a, b in combinations(operations, 2):
        if (a.machine == b.machine
                and min(a.finish, b.finish) > max(a.start, b.start) + tol):
            raise ValueError(f"Overlapping operations on the {a.machine} press.")
    if abs(max(op.finish for op in operations) - makespan) > tol:
        raise ValueError("Makespan does not match the final operation's finish.")


def time_label(value):
    """Suppress numerical noise near zero in display only, not in validation."""
    return f"{0 if abs(value) < 1e-6 else value:.6g}"


def plot_schedule(operations, makespan, solver=None):
    """Return a two-panel Matplotlib figure, without saving or showing it."""
    validate_schedule(operations, makespan)
    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch
    from matplotlib.ticker import MultipleLocator

    jobs = sorted({op.job for op in operations})
    machines = sorted({op.machine for op in operations})
    # Press determines hue; Paper 1/2/3 determine light/medium-light/medium.
    # Keep each operation's colour identical in both views.
    machine_colors = {
        "Blue": ("#c6def1", "#92bddc", "#5c9fcb"),
        "Green": ("#c6e8c2", "#98d18d", "#69b95b"),
        "Yellow": ("#fff1b3", "#ffe17a", "#ffd04b"),
    }
    job_shades = {job: i for i, job in enumerate(jobs)}
    fig, axes = plt.subplots(2, 1, figsize=(12, 7.5), sharex=True,
                             constrained_layout=True)
    title = "Printing job shop | 3 jobs / 3 presses / 8 operations"
    fig.suptitle(title + f"\nMakespan: {makespan:g} min"
                 + (f" | {solver}" if solver else "")
                 + "\nShade: Paper 1 = light | Paper 2 = medium-light | Paper 3 = medium",
                 fontsize=13)
    panels = (
        (axes[0], jobs, "job", "machine", "Schedule by job", "Paper job"),
        (axes[1], machines, "machine", "job", "Schedule by press", "Printing press"),
    )
    for ax, lanes, lane_key, label_key, heading, ylabel in panels:
        for op in operations:
            lane = lanes.index(getattr(op, lane_key))
            label = getattr(op, label_key).replace("_", " ")
            ax.barh(lane, op.duration, left=op.start, height=0.62,
                    color=machine_colors[op.machine][job_shades[op.job]],
                    edgecolor="#424242", linewidth=0.8)
            ax.text(op.start + op.duration / 2, lane,
                    f"{label}\n{time_label(op.start)}–{time_label(op.finish)}",
                    ha="center", va="center", fontsize=8, color="#202020")
        ax.set_yticks(range(len(lanes)), [s.replace("_", " ") for s in lanes])
        ax.set_ylim(len(lanes) - 0.4, -0.8)
        ax.set_ylabel(ylabel)
        ax.set_title(heading, loc="left", fontsize=12)
        ax.set_xlim(0, makespan * 1.07)
        ax.xaxis.set_major_locator(MultipleLocator(10))
        ax.xaxis.set_minor_locator(MultipleLocator(5))
        ax.grid(axis="x", alpha=0.25)
        ax.set_axisbelow(True)
        ax.axvline(makespan, color="#b22222", linestyle="--", linewidth=1.3)
        ax.text(makespan + 0.8, -0.55, f"{makespan:g} min", color="#b22222", fontsize=9)
        ax.spines[["top", "right"]].set_visible(False)
        ax.legend(handles=[Patch(facecolor=shades[1], edgecolor="#424242",
                                 label=name) for name, shades in machine_colors.items()],
                  loc="lower right", bbox_to_anchor=(1, 1.01), ncol=3, frameon=False)
    axes[0].tick_params(labelbottom=True)
    axes[1].set_xlabel("Elapsed time / min (bar labels show start–finish)")
    return fig


def solve_schedule(solver):
    """Optimise using the ASCEND adapter, then run the model's self-test."""
    import ascpy

    library = ascpy.Library()
    library.load(str(Path(__file__).resolve().with_suffix(".a4c")))
    model_type = library.findType("job_shop_highs" if solver == "HiGHS" else "job_shop")
    sim = model_type.getSimulation("schedule", True)
    sim.setSolver(ascpy.Solver(solver))
    params = sim.getParameters()
    for param in params:
        if param.getName() == "threads":
            param.setIntValue(1)
        elif param.getName() in ("mip_rel_gap", "mip_abs_gap"):
            param.setRealValue(0.0)
    sim.setParameters(params)
    sim.solve(ascpy.Solver(solver), ascpy.SolverReporter())
    if not sim.getStatus().isConverged():
        raise RuntimeError(f"{solver} did not converge; no optimal chart has been produced.")
    sim.run(next(m for m in model_type.getMethods() if str(m.getName()) == "self_test"))
    return read_schedule(sim.getModel())


def job_shop_gantt(self):
    """Plot the solved printing schedule by job and by press (run Solve first)."""
    import extpy

    browser = extpy.getbrowser()
    if browser is None:
        raise RuntimeError("Use the job_shop.py command-line driver outside the GUI.")
    if browser.sim.isSolveDirty() or not browser.sim.getStatus().isConverged():
        raise RuntimeError("Solve the job shop model successfully before running gantt.")
    operations, makespan = read_schedule(self)
    # Use ASCEND's GTK backend loader, not the command-line Agg backend.
    import loading
    loading.load_matplotlib(throw=True)
    import matplotlib.pyplot as plt

    fig = plot_schedule(operations, makespan, str(browser.sim.getSolver().getName()))
    plt.show(block=False)
    browser.reporter.reportNote(f"Job shop makespan: {makespan:g} min. Use the plot toolbar to save.")
    return fig


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--solver", choices=("HiGHS", "Gurobi"), default="HiGHS")
    parser.add_argument("--output", type=Path, default=Path("job_shop.png"),
                        help="Chart filename, e.g. .png, .svg or .pdf (default: job_shop.png)")
    parser.add_argument("--show", action="store_true", help="Also open an interactive plot window")
    args = parser.parse_args(argv)
    try:
        import matplotlib
        if not args.show:
            matplotlib.use("Agg")
        import matplotlib.pyplot as plt

        operations, makespan = solve_schedule(args.solver)
        print(f"\n{args.solver}: optimal makespan = {makespan:g} min")
        print("Op  Job      Press     Start / min  Duration / min  Finish / min")
        for op in operations:
            print(f"{op.number:2}  {op.job:7}  {op.machine:6}  {time_label(op.start):>11}"
                  f"  {time_label(op.duration):>14}  {time_label(op.finish):>12}")
        fig = plot_schedule(operations, makespan, args.solver)
        try:
            fig.savefig(args.output, dpi=160)
            print(f"Gantt chart: {args.output.resolve()}")
            if args.show:
                plt.show()
        finally:
            plt.close(fig)
    except (ImportError, RuntimeError, ValueError, OSError) as exc:
        print(f"job_shop: {exc}", file=sys.stderr)
        return 1
    return 0


# extpy executes imported scripts in __main__, so test for its module first.
# Loading the GUI wrapper must register the method, not run the CLI driver.
if "extpy" in sys.modules:
    sys.modules["extpy"].registermethod(job_shop_gantt)
elif __name__ == "__main__":
    sys.exit(main())

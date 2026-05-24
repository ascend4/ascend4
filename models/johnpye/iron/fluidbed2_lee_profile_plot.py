#!/usr/bin/env python3
"""Solve the Lee varying-bubble fluidised-bed model and plot axial profiles.

Run from the ASCEND source tree with, for example:

    ./a4 script models/johnpye/iron/fluidbed2_lee_profile_plot.py

The script uses ASCEND's Python bindings, so direct `python3` execution only works
if the ASCEND runtime environment has already been configured.
"""
from __future__ import annotations

import argparse
import csv
from pathlib import Path

try:
    import ascpy
except ImportError as exc:  # pragma: no cover - useful when run outside ./a4 script
    raise SystemExit(
        "Could not import ascpy. Run this via './a4 script /usr/bin/python3 "
        "models/johnpye/iron/fluidbed2_lee_profile_plot.py' or from an "
        "ASCEND-configured environment."
    ) from exc


THIS_DIR = Path(__file__).resolve().parent
ASCEND_ROOT = THIS_DIR.parents[2]
DEFAULT_MODEL_FILE = ASCEND_ROOT / "models" / "johnpye" / "fluidbed2.a4c"
DEFAULT_MODEL = "fluidbed2_lee_kl_example"
DEFAULT_PLOT_OUT = THIS_DIR / "fluidbed2_lee_kl_example_profile.png"
DEFAULT_CSV_OUT = THIS_DIR / "fluidbed2_lee_kl_example_profile.csv"


def find_method(model_type, name: str):
    """Return an ASCEND method by name, with a helpful error if absent."""
    try:
        return model_type.getMethod(name)
    except Exception:
        pass

    available = []
    for method in model_type.getMethods():
        method_name = str(method.getName())
        available.append(method_name)
        if method_name == name:
            return method
    raise RuntimeError(
        "Method '%s' not found on model '%s'. Available methods: %s"
        % (name, model_type.getName(), ", ".join(available))
    )


def solve_model(model_file: Path, model_name: str, run_method: str, solver_name: str):
    lib = ascpy.Library()
    lib.load(str(model_file))
    model_type = lib.findType(model_name)
    sim = model_type.getSimulation("sim_%s" % model_name, True)
    sim.setSolver(ascpy.Solver(solver_name))
    sim.run(find_method(model_type, run_method))
    sim.solve(sim.getSolver(), ascpy.SolverReporter())
    return sim


def value(instance, units: str | None = None) -> float:
    if units is not None:
        return float(instance.to(units))
    return float(instance)


def extract_profile(sim) -> list[dict[str, float]]:
    n = int(sim.n_lee_profile.getIntValue())
    rows = []
    for i in range(n + 1):
        rows.append(
            {
                "node": i,
                "z_m": value(sim.z_lee[i], "m"),
                "d_b_m": value(sim.d_b_lee[i], "m"),
                "d_b_mm": value(sim.d_b_lee[i], "mm"),
                "u_br_m_s": value(sim.u_br_lee[i], "m/s"),
                "u_b_m_s": value(sim.u_b_lee[i], "m/s"),
                "u_b_gas_m_s": value(sim.u_b_gas_lee[i], "m/s"),
                "delta": value(sim.delta_lee[i]),
                "K_be_1_s": value(sim.K_be_lee[i], "1/s"),
                "delta_K_be_1_s": value(sim.delta_K_be_lee[i], "1/s"),
            }
        )
    return rows


def extract_summary(sim) -> dict[str, float]:
    return {
        "u_m_s": value(sim.u, "m/s"),
        "u_mf_m_s": value(sim.u_mf, "m/s"),
        "bed_L_m": value(sim.bed.L, "m"),
        "d_b_avg_m": value(sim.d_b_lee_avg, "m"),
        "u_b_avg_m_s": value(sim.u_b_lee_avg, "m/s"),
        "delta_avg": value(sim.delta_lee_avg),
        "K_be_avg_1_s": value(sim.K_be_lee_avg, "1/s"),
        "delta_K_be_avg_1_s": value(sim.delta_K_be_lee_avg, "1/s"),
    }


def write_csv(path: Path, rows: list[dict[str, float]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="ascii") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def plot_profile(
    path: Path,
    rows: list[dict[str, float]],
    summary: dict[str, float],
    model_name: str,
    save_plot: bool,
) -> None:
    if save_plot:
        import matplotlib

        matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    z = [r["z_m"] for r in rows]
    db = [r["d_b_mm"] for r in rows]
    u_br = [r["u_br_m_s"] for r in rows]
    u_b = [r["u_b_m_s"] for r in rows]
    u_b_gas = [r["u_b_gas_m_s"] for r in rows]
    delta = [r["delta"] for r in rows]
    k_be = [r["K_be_1_s"] for r in rows]
    avg_db_mm = 1e3 * summary["d_b_avg_m"]
    delta_k = [r["delta_K_be_1_s"] for r in rows]

    fig, axes = plt.subplots(
        1,
        4,
        figsize=(13.2, 5.8),
        sharey=True,
        constrained_layout=True,
    )
    ax_db, ax_u, ax_delta, ax_k = axes
    fig.supylabel(r"Height above distributor, $z$ [m]")

    ax_db.plot(db, z, color="tab:blue", marker="o", linewidth=1.8, markersize=3.8)
    ax_db.axvline(avg_db_mm, color="tab:blue", linestyle=":", linewidth=1.7, label=r"$\bar{d}_b$")
    ax_db.set_xlabel(r"Bubble diameter, $d_b$ [mm]")
    ax_db.legend(loc="best", fontsize=9)

    ax_u.plot(u_b, z, color="tab:orange", marker="o", linewidth=1.7, markersize=3.5, label=r"$u_b$")
    ax_u.plot(u_b_gas, z, color="tab:red", linestyle="--", linewidth=1.6, label=r"$u_b + u_{mf}$")
    ax_u.plot(u_br, z, color="tab:gray", linestyle=":", linewidth=1.6, label=r"$u_{br}$")
    ax_u.axvline(summary["u_b_avg_m_s"], color="tab:orange", linestyle=":", linewidth=1.7, label=r"$\bar{u}_b$")
    ax_u.set_xlabel(r"Bubble speed [m/s]")
    ax_u.legend(loc="best", fontsize=9)

    ax_delta.plot(delta, z, color="tab:green", marker="o", linewidth=1.8, markersize=3.8)
    ax_delta.axvline(summary["delta_avg"], color="tab:green", linestyle=":", linewidth=1.7, label=r"$\bar{\delta}$")
    ax_delta.set_xlabel(r"Bubble fraction, $\delta$ [-]")
    ax_delta.set_xlim(left=0)
    ax_delta.legend(loc="best", fontsize=9)

    ax_k.plot(k_be, z, color="tab:purple", marker="o", linewidth=1.7, markersize=3.5, label=r"$K_{be}$")
    ax_k.plot(delta_k, z, color="tab:brown", marker="s", linewidth=1.7, markersize=3.5, label=r"$\delta K_{be}$")
    ax_k.axvline(summary["K_be_avg_1_s"], color="tab:purple", linestyle=":", linewidth=1.7, label=r"$\bar{K}_{be}$")
    ax_k.axvline(summary["delta_K_be_avg_1_s"], color="tab:brown", linestyle=":", linewidth=1.7, label=r"$\overline{\delta K}_{be}$")
    ax_k.set_xlabel(r"Mass-transfer rate [s$^{-1}$]")
    ax_k.legend(loc="best", fontsize=9)

    for ax in axes:
        ax.grid(True, alpha=0.32)
        ax.set_ylim(min(z), max(z))

    fig.suptitle(
        r"%s Lee varying-bubble profile; $u$ = %.3g m/s, $u_{mf}$ = %.3g m/s"
        % (model_name, summary["u_m_s"], summary["u_mf_m_s"]),
        fontsize=12,
    )

    if save_plot:
        path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(path, dpi=180)
    else:
        plt.show()
    plt.close(fig)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Solve a Lee varying-bubble ASCEND model and plot bed-height profiles."
    )
    ap.add_argument("--model-file", type=Path, default=DEFAULT_MODEL_FILE)
    ap.add_argument("--model", default=DEFAULT_MODEL)
    ap.add_argument("--run-method", default="on_load")
    ap.add_argument("--solver", default="QRSlv")
    ap.add_argument("--plot-out", type=Path, default=DEFAULT_PLOT_OUT)
    ap.add_argument("--csv-out", type=Path, default=DEFAULT_CSV_OUT)
    ap.add_argument(
        "--save-plot",
        action="store_true",
        help="Write the plot to --plot-out instead of showing it interactively.",
    )
    args = ap.parse_args(argv)

    sim = solve_model(args.model_file, args.model, args.run_method, args.solver)
    rows = extract_profile(sim)
    summary = extract_summary(sim)
    write_csv(args.csv_out, rows)
    plot_profile(args.plot_out, rows, summary, args.model, args.save_plot)

    print("wrote csv: %s" % args.csv_out)
    if args.save_plot:
        print("wrote plot: %s" % args.plot_out)
    print(
        "summary: bed.L = %.6g m, d_b_avg = %.6g m, u_b_avg = %.6g m/s, "
        "delta_avg = %.6g, K_be_avg = %.6g 1/s"
        % (
            summary["bed_L_m"],
            summary["d_b_avg_m"],
            summary["u_b_avg_m_s"],
            summary["delta_avg"],
            summary["K_be_avg_1_s"],
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

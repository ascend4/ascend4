#!/usr/bin/env python3
"""
Compare the current pure-Fe oxide package against hand-traced Hidayat
2015 oxide-side boundaries.

Inputs:
  - Fig. 11 `wustite|spinel` and `spinel|Fe2O3` potential boundaries
  - Fig. 10 `spinel|Fe2O3` composition boundary

The traced files are expected as two-column plain-text data:

  Fig. 11:
    x = 1000 / T[K]
    y = log10(pO2 / atm)

  Fig. 10:
    x = mass ratio Fe2O3 / (FeO + Fe2O3)
    y = temperature in C
"""

from __future__ import annotations

import argparse
import math
import os
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

from feo_hidayat_validation import R, spinel_phase_g
from feoh_hydrogen_boundary import (
    default_runner,
    normalize_gas_source,
    oxygen_potential_at_wustite_spinel_boundary_state,
    query_mu0,
)
from feoxide_potential_boundaries import oxygen_potential_at_spinel_hematite_boundary

THIS_DIR = Path(__file__).resolve().parent
P0 = 1e5
M_FE = 55.845
M_O = 15.999
M_FEO = M_FE + M_O
M_FE3O4 = 3.0 * M_FE + 4.0 * M_O
M_FE2O3 = 2.0 * M_FE + 3.0 * M_O


@dataclass(frozen=True)
class PointSet:
    name: str
    path: Path
    points: tuple[tuple[float, float], ...]


def load_points(path: Path) -> PointSet:
    pts: list[tuple[float, float]] = []
    with path.open("r", encoding="ascii") as fp:
        for line in fp:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 2:
                raise ValueError(f"bad line in {path}: {line!r}")
            pts.append((float(parts[0]), float(parts[1])))
    if not pts:
        raise ValueError(f"no points loaded from {path}")
    return PointSet(path.stem, path, tuple(pts))


def log10_po2_from_lambda(lam_o: float, mu_o2: float, T: float) -> float:
    return (2.0 * lam_o - mu_o2) / (R * T * math.log(10.0))


def spinel_fe2o3_mass_ratio_vs_feo(a_t_fe2: float, b_o_fe2: float, T: float) -> float:
    _g, n_fe, _yo_fe3, _yo_va = spinel_phase_g(T, a_t_fe2, b_o_fe2)
    alpha_feo = max(0.0, 3.0 * n_fe - 8.0)
    beta_fe2o3 = max(0.0, 4.0 - n_fe)
    denom = alpha_feo * M_FEO + beta_fe2o3 * M_FE2O3
    if denom <= 0.0:
        raise RuntimeError("invalid spinel|hematite composition state")
    return beta_fe2o3 * M_FE2O3 / denom


def mass_ratio_from_atpct_o(atpct_o: float) -> float:
    y = atpct_o / 100.0
    n_fe = 4.0 * (1.0 - y) / y
    alpha_feo = max(0.0, 3.0 * n_fe - 8.0)
    beta_fe2o3 = max(0.0, 4.0 - n_fe)
    denom = alpha_feo * M_FEO + beta_fe2o3 * M_FE2O3
    if denom <= 0.0:
        raise RuntimeError("invalid at%O -> mass ratio conversion")
    return beta_fe2o3 * M_FE2O3 / denom


@lru_cache(maxsize=None)
def oxygen_mu0(runner_str: str, gas_source: str, T: float) -> float:
    vals = query_mu0(Path(runner_str), gas_source, T, ["oxygen"])
    return vals["oxygen"]


def summarize_residuals(resids: list[float]) -> tuple[float, float]:
    if not resids:
        return math.nan, math.nan
    rms = math.sqrt(sum(r * r for r in resids) / len(resids))
    maxabs = max(abs(r) for r in resids)
    return rms, maxabs


def compare_fig11_wustite_spinel(runner: Path, gas_source: str, data: PointSet) -> list[tuple[float, float, float, float]]:
    rows: list[tuple[float, float, float, float]] = []
    x_hint = None
    a_hint = None
    b_hint = None
    ordered = sorted(data.points, key=lambda p: 1000.0 / p[0])
    for invtemp, y_ref in ordered:
        tk = 1000.0 / invtemp
        tc = tk - 273.15
        _x_best, lam_o, a_hint, b_hint, _v, _resid = oxygen_potential_at_wustite_spinel_boundary_state(
            tk, x_hint=x_hint, a_hint=a_hint, b_hint=b_hint
        )
        mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
        y_model = log10_po2_from_lambda(lam_o, mu_o2, tk)
        rows.append((tc, invtemp, y_ref, y_model))
        x_hint = _x_best
    return rows


def compare_fig11_spinel_fe2o3(runner: Path, gas_source: str, data: PointSet) -> list[tuple[float, float, float, float]]:
    rows: list[tuple[float, float, float, float]] = []
    for invtemp, y_ref in sorted(data.points, key=lambda p: 1000.0 / p[0]):
        tk = 1000.0 / invtemp
        tc = tk - 273.15
        lam_o, _a_best, _b_best = oxygen_potential_at_spinel_hematite_boundary(tk)
        mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
        y_model = log10_po2_from_lambda(lam_o, mu_o2, tk)
        rows.append((tc, invtemp, y_ref, y_model))
    return rows


def compare_fig10_spinel_fe2o3(data: PointSet) -> list[tuple[float, float, float]]:
    rows: list[tuple[float, float, float]] = []
    for x_ref, tc in sorted(data.points, key=lambda p: p[1]):
        tk = tc + 273.15
        _lam_o, a_best, b_best = oxygen_potential_at_spinel_hematite_boundary(tk)
        x_model = spinel_fe2o3_mass_ratio_vs_feo(a_best, b_best, tk)
        rows.append((tc, x_ref, x_model))
    return rows


def plot_fig11(
    plot_file: Path,
    rows_ws: list[tuple[float, float, float, float]],
    rows_sh: list[tuple[float, float, float, float]],
    gas_source: str,
) -> None:
    fig, ax = plt.subplots(figsize=(8.0, 5.5))
    ax.plot([inv for _tc, inv, y_ref, _y_model in rows_ws], [y_ref for _tc, _inv, y_ref, _y_model in rows_ws],
            color="#b55200", lw=2.0, label="Hidayat Fig. 11 wustite|spinel")
    ax.plot([inv for _tc, inv, _y_ref, y_model in rows_ws], [y_model for _tc, _inv, _y_ref, y_model in rows_ws],
            color="#b55200", lw=1.5, ls="--", label="Model wustite|spinel")
    ax.plot([inv for _tc, inv, y_ref, _y_model in rows_sh], [y_ref for _tc, _inv, y_ref, _y_model in rows_sh],
            color="#7a1f5c", lw=2.0, label="Hidayat Fig. 11 spinel|Fe2O3")
    ax.plot([inv for _tc, inv, _y_ref, y_model in rows_sh], [y_model for _tc, _inv, _y_ref, y_model in rows_sh],
            color="#7a1f5c", lw=1.5, ls="--", label="Model spinel|Fe2O3")
    ax.set_xlabel("1000 / T [K]")
    ax.set_ylabel(r"log$_{10}$(pO$_2$ / atm)")
    ax.set_title(f"Hidayat 2015 Fig. 11 Oxide-Side Comparison ({gas_source})")
    ax.grid(True, alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()
    fig.savefig(plot_file, dpi=160)
    plt.close(fig)


def plot_fig10(plot_file: Path, rows: list[tuple[float, float, float]]) -> None:
    fig, ax = plt.subplots(figsize=(7.0, 5.5))
    ax.plot([x_ref for _tc, x_ref, _x_model in rows], [tc for tc, _x_ref, _x_model in rows],
            color="#7a1f5c", lw=2.0, label="Hidayat Fig. 10 spinel|Fe2O3")
    ax.plot([x_model for _tc, _x_ref, x_model in rows], [tc for tc, _x_ref, _x_model in rows],
            color="#7a1f5c", lw=1.5, ls="--", label="Model spinel|Fe2O3")
    ax.set_xlabel(r"Mass ratio Fe$_2$O$_3$ / (FeO + Fe$_2$O$_3$)")
    ax.set_ylabel("Temperature [C]")
    ax.set_title("Hidayat 2015 Fig. 10 Oxide-Side Comparison")
    ax.grid(True, alpha=0.3)
    ax.legend(loc="best")
    fig.tight_layout()
    fig.savefig(plot_file, dpi=160)
    plt.close(fig)


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare current oxide-side model against Hidayat 2015 traced boundaries.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="O2 mu0 source for Fig. 11 conversion.")
    ap.add_argument("--no-plots", action="store_true", help="Skip writing PNG plots.")
    args = ap.parse_args()
    args.gas_source = normalize_gas_source(args.gas_source)

    fig11_ws = load_points(THIS_DIR / "hidayat-2015-fig11-wust-spin.dat")
    fig11_sh = load_points(THIS_DIR / "hidayat-2015-fig11-spin-Fe2O3.dat")
    fig10_sh = load_points(THIS_DIR / "hidayat-2015-fig10-spin-Fe2O3.dat")

    rows_ws = compare_fig11_wustite_spinel(args.runner, args.gas_source, fig11_ws)
    rows_sh = compare_fig11_spinel_fe2o3(args.runner, args.gas_source, fig11_sh)
    rows_fig10 = compare_fig10_spinel_fe2o3(fig10_sh)

    ws_resids = [y_model - y_ref for _tc, _inv, y_ref, y_model in rows_ws]
    sh_resids = [y_model - y_ref for _tc, _inv, y_ref, y_model in rows_sh]
    fig10_resids = [x_model - x_ref for _tc, x_ref, x_model in rows_fig10]
    ws_rms, ws_max = summarize_residuals(ws_resids)
    sh_rms, sh_max = summarize_residuals(sh_resids)
    f10_rms, f10_max = summarize_residuals(fig10_resids)

    print("Hidayat oxide-side comparison")
    print(f"Gas source for Fig. 11 oxygen conversion: {args.gas_source}")
    print()
    print("Fig. 11 wustite|spinel")
    print(f"- points: {len(rows_ws)}")
    print(f"- RMS delta log10(pO2): {ws_rms:.6f}")
    print(f"- max |delta log10(pO2)|: {ws_max:.6f}")
    print(f"- first/last T range: {rows_ws[0][0]:.1f} C to {rows_ws[-1][0]:.1f} C")
    print()
    print("Fig. 11 spinel|Fe2O3")
    print(f"- points: {len(rows_sh)}")
    print(f"- RMS delta log10(pO2): {sh_rms:.6f}")
    print(f"- max |delta log10(pO2)|: {sh_max:.6f}")
    print(f"- first/last T range: {rows_sh[0][0]:.1f} C to {rows_sh[-1][0]:.1f} C")
    print()
    print("Fig. 10 spinel|Fe2O3")
    print(f"- points: {len(rows_fig10)}")
    print(f"- RMS delta mass ratio: {f10_rms:.6f}")
    print(f"- max |delta mass ratio|: {f10_max:.6f}")
    print(f"- first/last T range: {rows_fig10[0][0]:.1f} C to {rows_fig10[-1][0]:.1f} C")
    print("- Table 2 consistency check:")
    print(
        f"  1459 C invariant implies x≈{mass_ratio_from_atpct_o(58.0):.6f}, "
        f"while the traced curve is near x≈{min(rows_fig10, key=lambda r: abs(r[0]-1459.0))[1]:.6f}"
    )
    print(
        f"  1552 C invariant implies x≈{mass_ratio_from_atpct_o(58.1):.6f}, "
        f"while the traced curve is near x≈{min(rows_fig10, key=lambda r: abs(r[0]-1552.0))[1]:.6f}"
    )
    print(
        "  1620 C invariant implies x->1 as FeO disappears, "
        f"while the traced curve ends near x≈{max(rows_fig10, key=lambda r: r[0])[1]:.6f}"
    )
    print()
    print("Representative offsets")
    mid_ws = rows_ws[len(rows_ws) // 2]
    mid_sh = rows_sh[len(rows_sh) // 2]
    mid_f10 = rows_fig10[len(rows_fig10) // 2]
    print(
        f"- Fig. 11 wustite|spinel near {mid_ws[0]:.1f} C: "
        f"ref={mid_ws[2]:.4f}, model={mid_ws[3]:.4f}, delta={mid_ws[3] - mid_ws[2]:+.4f}"
    )
    print(
        f"- Fig. 11 spinel|Fe2O3 near {mid_sh[0]:.1f} C: "
        f"ref={mid_sh[2]:.4f}, model={mid_sh[3]:.4f}, delta={mid_sh[3] - mid_sh[2]:+.4f}"
    )
    print(
        f"- Fig. 10 spinel|Fe2O3 near {mid_f10[0]:.1f} C: "
        f"ref={mid_f10[1]:.6f}, model={mid_f10[2]:.6f}, delta={mid_f10[2] - mid_f10[1]:+.6f}"
    )

    if not args.no_plots:
        gas = args.gas_source.lower().replace(" ", "_").replace("+", "_plus_").replace(":", "")
        fig11_plot = THIS_DIR / f"hidayat_fig11_oxide_compare_{gas}.png"
        fig10_plot = THIS_DIR / "hidayat_fig10_spinel_fe2o3_compare.png"
        plot_fig11(fig11_plot, rows_ws, rows_sh, args.gas_source)
        plot_fig10(fig10_plot, rows_fig10)
        print()
        print("Plots")
        print(f"- {fig11_plot}")
        print(f"- {fig10_plot}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

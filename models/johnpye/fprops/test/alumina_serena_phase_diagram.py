#!/usr/bin/env python3
"""Serena et al. (2009) Al2O3-H2O phase-boundary check.

This script is deliberately a direct boundary calculator, not a general
eqm_phase liquid/vapor implementation. The solid hydrates use the FPROPS
`serena_2009` Shomate source. Water vapor is treated as an ideal gas with a
Helmholtz/ref0 standard potential. The optional saturation diagnostics use the
FPROPS Helmholtz/IAPWS saturation curve, which is intentionally reported
separately from the Serena vapor-branch boundary check.

Metastable gamma-Al2O3 boundaries use the public USGS Bulletin 1452 gamma
alumina table rather than Serena's commercial SSUB-3 source, so these curves
are a check of available public thermodynamics rather than an exact Fig. 7
reproduction.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path
import sys

try:
    import fprops
except ImportError:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
    import fprops


P_REF = 100000.0
P_ATM = 100000.0
P_MAX_SERENA = 30.0e6
SOURCE_SOLIDS = "serena_2009"
SOURCE_GAMMA = "usgs_bull_1452_1978"
SOURCE_WATER = "helmholtz+ref0:"


def solid_mu(name: str, T: float, P: float = P_REF,
        source: str = SOURCE_SOLIDS) -> float:
    return fprops.mu0_source(name, source, T, P)


def gamma_mu(T: float, P: float = P_REF) -> float:
    return solid_mu("gamma-Al2O3", T, P, SOURCE_GAMMA)


def water_vapor_mu_ideal(T: float, P: float) -> float:
    mu0 = fprops.mu0_source("water", SOURCE_WATER, T, P_REF)
    return mu0 + fprops.FPROPS_R * T * math.log(P / P_REF)


class WaterFluid:
    def __init__(self) -> None:
        self.fluid = fprops.fluid("water", "helmholtz")

    def psat(self, T: float) -> tuple[float, float, float]:
        psat, rhof, rhog = self.fluid.sat_T(T)
        return psat, rhof, rhog

    def mu(self, T: float, P: float, branch: str = "auto") -> float:
        if branch == "vapor":
            return water_vapor_mu_ideal(T, P)
        if branch == "helmholtz":
            return fprops.mu0_source("water", SOURCE_WATER, T, P)
        if branch != "auto":
            raise ValueError(f"unknown water branch '{branch}'")

        # Below the critical point, Serena's stable diagram switches from
        # vapor to liquid water at saturation. At saturation, liquid and vapor
        # chemical potentials are equal, so an ideal-vapor saturation value plus
        # a compressed-liquid v dP correction is sufficient for this plotting
        # calculation.
        if T < self.fluid.T_c:
            psat, rhof, _rhog = self.psat(T)
            if P > psat:
                molar_volume = self.fluid.M * 1e-3 / rhof
                return water_vapor_mu_ideal(T, psat) + molar_volume * (P - psat)
        return water_vapor_mu_ideal(T, P)


def dg_gibbsite_boehmite(T: float, P: float, water: WaterFluid,
        branch: str = "auto") -> float:
    """Return Delta G for gibbsite -> boehmite + 2 H2O, J/mol."""
    return (
        solid_mu("boehmite", T, P)
        + 2.0 * water.mu(T, P, branch)
        - solid_mu("gibbsite", T, P)
    )


def dg_boehmite_corundum(T: float, P: float, water: WaterFluid,
        branch: str = "auto") -> float:
    """Return Delta G for boehmite -> alpha-Al2O3 + H2O, J/mol."""
    return (
        solid_mu("Al2O3", T, P)
        + water.mu(T, P, branch)
        - solid_mu("boehmite", T, P)
    )


def dg_gibbsite_gamma(T: float, P: float, water: WaterFluid,
        branch: str = "auto") -> float:
    """Return Delta G for gibbsite -> gamma-Al2O3 + 3 H2O, J/mol."""
    return (
        gamma_mu(T, P)
        + 3.0 * water.mu(T, P, branch)
        - solid_mu("gibbsite", T, P)
    )


def dg_boehmite_gamma(T: float, P: float, water: WaterFluid,
        branch: str = "auto") -> float:
    """Return Delta G for boehmite -> gamma-Al2O3 + H2O, J/mol."""
    return (
        gamma_mu(T, P)
        + water.mu(T, P, branch)
        - solid_mu("boehmite", T, P)
    )


BOUNDARIES = {
    "gibbsite-boehmite": dg_gibbsite_boehmite,
    "boehmite-corundum": dg_boehmite_corundum,
    "gibbsite-gamma": dg_gibbsite_gamma,
    "boehmite-gamma": dg_boehmite_gamma,
}


def eval_or_nan(fn, x: float) -> float:
    try:
        return fn(x)
    except Exception:
        return math.nan


def bisect(fn, lo: float, hi: float, tol: float = 1e-8,
        maxit: int = 100) -> float:
    flo = eval_or_nan(fn, lo)
    fhi = eval_or_nan(fn, hi)
    if not (math.isfinite(flo) and math.isfinite(fhi)) or flo * fhi > 0:
        return math.nan
    for _ in range(maxit):
        mid = 0.5 * (lo + hi)
        fmid = eval_or_nan(fn, mid)
        if not math.isfinite(fmid):
            return math.nan
        if abs(fmid) < tol or abs(hi - lo) < tol * max(1.0, abs(mid)):
            return mid
        if flo * fmid <= 0.0:
            hi = mid
            fhi = fmid
        else:
            lo = mid
            flo = fmid
    return 0.5 * (lo + hi)


def solve_T_at_P(boundary: str, P: float, water: WaterFluid,
        branch: str = "auto", lo: float | None = None,
        hi: float | None = None) -> float:
    if lo is None:
        lo = 350.0 if boundary in ("gibbsite-boehmite", "gibbsite-gamma") else 400.0
    if hi is None:
        hi = 590.0 if boundary in ("gibbsite-boehmite", "gibbsite-gamma") else 900.0
    dg = BOUNDARIES[boundary]
    return bisect(lambda T: dg(T, P, water, branch), lo, hi, tol=1e-7)


def solve_saturation_intersection(boundary: str, water: WaterFluid,
        lo: float | None = None, hi: float | None = None) -> tuple[float, float]:
    if lo is None:
        lo = 350.0 if boundary == "gibbsite-boehmite" else 500.0
    if hi is None:
        hi = 590.0 if boundary == "gibbsite-boehmite" else 640.0
    dg = BOUNDARIES[boundary]

    def f(T: float) -> float:
        psat, _rhof, _rhog = water.psat(T)
        return dg(T, psat, water, "vapor")

    T = bisect(f, lo, hi, tol=1e-7)
    if not math.isfinite(T):
        return math.nan, math.nan
    psat, _rhof, _rhog = water.psat(T)
    return T, psat


def solve_P_at_T(boundary: str, T: float, water: WaterFluid,
        branch: str = "auto", pmin: float = 1.0e2,
        pmax: float = P_MAX_SERENA) -> float:
    dg = BOUNDARIES[boundary]
    log_lo = math.log(pmin)
    log_hi = math.log(pmax)
    logP = bisect(lambda x: dg(T, math.exp(x), water, branch), log_lo, log_hi)
    return math.exp(logP) if math.isfinite(logP) else math.nan


def make_plot(path: Path, water: WaterFluid) -> None:
    import matplotlib.pyplot as plt

    T_sat_hi = water.fluid.T_c - 1.0e-5
    temps_by_boundary = {
        "gibbsite-boehmite": [298.16 + i * (590.0 - 298.16) / 200 for i in range(201)],
        "boehmite-corundum": [298.16 + i * (T_sat_hi - 298.16) / 260 for i in range(261)],
        "gibbsite-gamma": [298.16 + i * (900.0 - 298.16) / 260 for i in range(261)],
        "boehmite-gamma": [298.16 + i * (900.0 - 298.16) / 260 for i in range(261)],
    }
    fig, ax = plt.subplots(figsize=(7.0, 4.8))

    for boundary, label, style in [
        ("gibbsite-boehmite", "gibbsite | boehmite + 2H2O", "-"),
        ("boehmite-corundum", "boehmite | corundum + H2O", "-"),
        ("gibbsite-gamma", "gibbsite | gamma-Al2O3 + 3H2O", ":"),
        ("boehmite-gamma", "boehmite | gamma-Al2O3 + H2O", ":"),
    ]:
        pts = []
        for T in temps_by_boundary[boundary]:
            P = solve_P_at_T(boundary, T, water, "auto")
            if math.isfinite(P):
                pts.append((T, math.log10(P)))
        if pts:
            ax.plot([p[0] for p in pts], [p[1] for p in pts], style, label=label)

    sat = []
    temps = [298.16 + i * (T_sat_hi - 298.16) / 260 for i in range(261)]
    for T in temps:
        if T < water.fluid.T_c:
            P, _rhof, _rhog = water.psat(T)
            if P <= P_MAX_SERENA:
                sat.append((T, math.log10(P)))
    ax.plot([p[0] for p in sat], [p[1] for p in sat], "k--", label="H2O sat.")
    ax.set_xlabel("T / K")
    ax.set_ylabel("log10(P / Pa)")
    ax.set_xlim(300.0, 900.0)
    ax.set_ylim(5.0, 7.5)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(path, dpi=180)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check FPROPS Serena 2009 Al2O3-H2O boundaries."
    )
    parser.add_argument("--plot-file", type=Path)
    args = parser.parse_args()

    water = WaterFluid()
    print("Serena 2009 Al2O3-H2O boundary check")
    print("solid source:", SOURCE_SOLIDS)
    print("gamma-Al2O3 source:", SOURCE_GAMMA)
    print("water vapor source:", SOURCE_WATER)
    print()

    for boundary, paper_T, paper_P in [
        ("gibbsite-boehmite", 396.8, P_ATM),
        ("boehmite-corundum", 438.6, P_ATM),
    ]:
        T = solve_T_at_P(boundary, P_ATM, water, "vapor")
        print(
            f"{boundary} at 0.100 MPa vapor: "
            f"T = {T:.3f} K ({T - 273.15:.3f} C), "
            f"paper T ~= {paper_T:.1f} K"
        )
        print(f"  Delta = {T - paper_T:+.3f} K")

    print()
    print("Serena-reported vaporization intersections, checked on the vapor branch")
    for boundary, paper_T, paper_P in [
        ("gibbsite-boehmite", 413.2, 0.347936e6),
        ("boehmite-corundum", 603.4, 8.794274e6),
    ]:
        P = solve_P_at_T(boundary, paper_T, water, "vapor")
        dg_at_paper = BOUNDARIES[boundary](paper_T, paper_P, water, "vapor")
        psat, _rhof, _rhog = water.psat(paper_T)
        print(
            f"{boundary} at paper T = {paper_T:.1f} K: "
            f"P(vapor branch) = {P / 1e6:.6f} MPa, "
            f"paper P ~= {paper_P / 1e6:.6f} MPa"
        )
        print(
            f"  Delta P = {(P - paper_P) / 1e6:+.6f} MPa; "
            f"Delta G at paper point = {dg_at_paper:+.3f} J/mol"
        )
        print(f"  FPROPS/IAPWS psat at this T = {psat / 1e6:.6f} MPa")

    print()
    print("Intersections with the FPROPS/IAPWS saturation curve")
    for boundary, paper_T, paper_P in [
        ("gibbsite-boehmite", 413.2, 0.347936e6),
        ("boehmite-corundum", 603.4, 8.794274e6),
    ]:
        T, P = solve_saturation_intersection(boundary, water)
        if math.isfinite(T):
            print(
                f"{boundary}: "
                f"T = {T:.3f} K ({T - 273.15:.3f} C), P = {P / 1e6:.6f} MPa"
            )
            print(
                f"  paper T ~= {paper_T:.1f} K, paper P ~= {paper_P / 1e6:.6f} MPa; "
            f"Delta T = {T - paper_T:+.3f} K, Delta P = {(P - paper_P) / 1e6:+.6f} MPa"
        )
        else:
            print(
                f"{boundary}: no crossing below the FPROPS water critical point; "
                "the vapor-branch check above is the relevant Serena comparison."
            )

    print()
    print("Metastable gamma-Al2O3 boundaries using public USGS gamma data")
    for boundary, paper_T in [
        ("gibbsite-gamma", math.nan),
        ("boehmite-gamma", 567.2),
    ]:
        T = solve_T_at_P(boundary, P_ATM, water, "vapor")
        paper_note = (
            f", paper T ~= {paper_T:.1f} K, Delta = {T - paper_T:+.3f} K"
            if math.isfinite(paper_T) and math.isfinite(T)
            else ""
        )
        print(
            f"{boundary} at 0.100 MPa vapor: "
            f"T = {T:.3f} K ({T - 273.15:.3f} C){paper_note}"
        )
    P_gamma_830 = solve_P_at_T("boehmite-gamma", 830.0, water, "vapor", pmax=100.0e6)
    print(
        "boehmite-gamma at paper T = 830.0 K: "
        f"P(vapor branch) = {P_gamma_830 / 1e6:.6f} MPa, "
        "paper P ~= 27.470000 MPa"
    )

    if args.plot_file:
        make_plot(args.plot_file, water)
        print()
        print(f"wrote {args.plot_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

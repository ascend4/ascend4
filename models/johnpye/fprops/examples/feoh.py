#!/usr/bin/env python3
"""Fe-O-H phase-equilibrium example using the Python FPROPS wrapper."""

from __future__ import annotations

import math
from pathlib import Path
import sys

try:
    import fprops
except ImportError:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
    import fprops


P_ATM = 101325.0
P_REF = 100000.0
GAS_SOURCE = "helmholtz+ref0:"


def interp(x: float, y0: float, x0: float, y1: float, x1: float) -> float:
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0)


def log10_from_god(god: float) -> float:
    return math.log10(god / (1.0 - god))


def interp_log10_god(tc: float, god0: float, tc0: float, god1: float,
        tc1: float) -> float:
    return log10_from_god(interp(tc, god0, tc0, god1, tc1))


def bg_fe_wustite_log10_h2o_h2_700c() -> float:
    return interp_log10_god(
        700.0,
        0.28587264008, 682.801879077,
        0.295194336274, 704.894663517,
    )


def bg_wustite_spinel_log10_h2o_h2_700c() -> float:
    return interp_log10_god(
        700.0,
        0.5333888167, 698.63992637,
        0.553100275071, 707.819009833,
    )


def build_feoh_problem() -> fprops.Eqm:
    # The problem object stores the phase package, element list, feed totals,
    # temperature, pressure, and solver settings. These compact strings are the
    # same phase specifications used by the C example.
    eqm = fprops.Eqm()
    eqm.add_phases([
        "Fe_bcc=hidayat_2015",
        "wustite=hidayat_2015",
        "spinel=degterov_2001",
        "Fe2O3=hidayat_2015",
        "gas:ideal(hydrogen,water)=helmholtz+ref0:",
    ])
    return eqm


def add_h2_h2o_feed(eqm: fprops.Eqm, gas_total: float,
        log10_h2o_h2: float) -> None:
    # A gas buffer is most naturally specified by its H2O/H2 ratio. The solver
    # still receives ordinary element totals after add_comps parses H2 and H2O.
    ratio = 10.0 ** log10_h2o_h2
    n_h2o = gas_total * ratio / (1.0 + ratio)
    n_h2 = gas_total - n_h2o
    eqm.add_comps([("H2", n_h2), ("H2O", n_h2o)])


def find_lambda_fe_for_entry(phase: fprops.EqmPhase, T: float, P: float,
        lambda_o: float) -> tuple[float, list[float]]:
    # Lambda is the vector of element chemical potentials used as Lagrange
    # multipliers for element balance. A phase sits on an entry boundary when
    # its minimized residual g_phase - sum(lambda_i a_i) is zero.
    elements = phase.element_names()
    i_fe = elements.index("Fe")
    i_o = elements.index("O")
    lam = [0.0] * len(elements)
    lam[i_o] = lambda_o

    def residual(lambda_fe: float) -> tuple[float, list[float]]:
        lam[i_fe] = lambda_fe
        return phase.entry_residual(T, P, lam)

    lo = -1.0e6
    hi = 1.0e6
    r_lo, y_lo = residual(lo)
    r_hi, _ = residual(hi)
    if r_lo * r_hi > 0.0:
        raise RuntimeError("failed to bracket wustite entry residual")

    y_mid = y_lo
    for _ in range(80):
        mid = 0.5 * (lo + hi)
        r_mid, y_mid = residual(mid)
        if abs(r_mid) < 1.0e-8:
            return mid, y_mid
        if r_lo * r_mid <= 0.0:
            hi = mid
            r_hi = r_mid
        else:
            lo = mid
            r_lo = r_mid
    return 0.5 * (lo + hi), y_mid


def print_phase_summary(result: fprops.EqmPhaseResult) -> None:
    print("  summary:")
    print(f"    metallic Fe = {result.phase_amount('Fe_bcc'):.8e} mol")
    print(f"    unreduced hematite = {result.phase_amount('Fe2O3'):.8e} mol")
    print(
        "    wustite = "
        f"{result.phase_amount('wustite'):.8e} mol, "
        f"x_FeO1p5 = {result.phase_coord('wustite', 'x_FeO1p5'):.8e}"
    )
    print(
        "    spinel = "
        f"{result.phase_amount('spinel'):.8e} mol, "
        f"y_tet_fe2 = {result.phase_coord('spinel', 'y_tet_fe2'):.8e}, "
        f"y_oct_fe2 = {result.phase_coord('spinel', 'y_oct_fe2'):.8e}"
    )
    print(
        "    H2 = "
        f"{result.phase_member_amount('gas:ideal', 'hydrogen'):.8e} mol, "
        f"H2O = {result.phase_member_amount('gas:ideal', 'water'):.8e} mol"
    )


def print_package_order(result: fprops.EqmPhaseResult) -> None:
    print("  phases in package order:")
    for phase in result.phase_names():
        amount = result.phase_amount(phase)
        active = result.phase_active(phase)
        print(f"    {phase}: amount={amount:.8e}, active={active}")

        coord_names = result.phase_coord_names(phase)
        if coord_names:
            coords = result.phase_coords(phase)
            values = ", ".join(
                f"{name}={value:.8e}"
                for name, value in zip(coord_names, coords)
            )
            print(f"      coords: {values}")

        member_names = result.phase_member_names(phase)
        if member_names:
            amounts = result.phase_member_amounts(phase)
            values = ", ".join(
                f"{name}={value:.8e}"
                for name, value in zip(member_names, amounts)
            )
            print(f"      members: {values}")


def solve_and_print_case(label: str, eqm: fprops.Eqm, T: float) -> None:
    result = eqm.solve_TP(T, P_ATM)
    print(f"\n{label}")
    print(f"  T = {T:.2f} K, P = {P_ATM:.0f} Pa")
    print(
        f"  status = {result.status()}, "
        f"solver_status = {result.solver_status()} ({result.status_text()})"
    )
    print_phase_summary(result)
    print_package_order(result)


def main() -> int:
    eqm = build_feoh_problem()

    # Reducing feed: one mole of hematite plus 100 mole H2. Written as element
    # totals, this is [Fe, O, H] = [2, 3, 200], but add_comps lets the user
    # enter the feed using familiar chemical formulae.
    eqm.clear_feed()
    eqm.add_comps([("Fe2O3", 1.0), ("H2", 100.0)])
    solve_and_print_case("reducing: 1 mol Fe2O3 + 100 mol H2", eqm, 1173.15)

    # Intermediate feed: choose the midpoint between nearby Baur-Glaessner
    # Fe/wustite and wustite/spinel H2O/H2 buffer ratios at 700 C.
    log10_mid = 0.5 * (
        bg_fe_wustite_log10_h2o_h2_700c()
        + bg_wustite_spinel_log10_h2o_h2_700c()
    )

    # Convert the gas ratio into an oxygen element potential using
    # H2 + O = H2O. This constructs a convenient wustite feed near a known
    # Fe-O-H buffer; the equilibrium solve below remains a general Gibbs
    # minimization subject to element balance.
    T_mid = 973.15
    mu_h2 = fprops.mu0_source("hydrogen", GAS_SOURCE, T_mid, P_REF)
    mu_h2o = fprops.mu0_source("water", GAS_SOURCE, T_mid, P_REF)
    lambda_o = (
        fprops.FPROPS_R * T_mid * math.log(10.0) * log10_mid
        + (mu_h2o - mu_h2)
    )

    wustite = fprops.EqmPhase("wustite=hidayat_2015")
    lambda_fe, y_wus = find_lambda_fe_for_entry(
        wustite, T_mid, P_ATM, lambda_o
    )
    _ = lambda_fe

    eqm.clear_feed()
    eqm.add_phase_feed("wustite", 1.0, y_wus)
    add_h2_h2o_feed(eqm, 10.0, log10_mid)
    solve_and_print_case("intermediate: wustite-buffered feed", eqm, T_mid)

    # Oxidizing feed: start from one mole of spinel at a simple site
    # composition, then add 10 mole gas at a high H2O/H2 ratio.
    eqm.clear_feed()
    eqm.add_phase_feed_vars(
        "spinel", 1.0,
        {"y_tet_fe2": 0.40, "y_oct_fe2": 0.20},
    )
    add_h2_h2o_feed(eqm, 10.0, 0.80)
    solve_and_print_case("oxidizing: spinel-buffered feed", eqm, T_mid)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""
Compare simple one-parameter residual trends for both H2 and CO BG branches
under different abscissae:

  - T_C
  - 1 / T_K
  - 1 / T_K^2

for two response variables:

  - delta_GOD
  - delta_lambda_O

This extends the CO-only diagnostic to H2 so the branch behavior can be
compared side by side on the same basis.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np

from feoc_baur_glaessner_compare import (
    FIT_SPECS as CO_FIT_SPECS,
    cached_model_god_at_temp as cached_model_god_at_temp_co,
    fit_god_at_temp as fit_co_god,
)
from feoh_baur_glaessner_compare import (
    FIT_SPECS as H2_FIT_SPECS,
    cached_model_god_at_temp as cached_model_god_at_temp_h2,
    fit_god_at_temp as fit_h2_god,
)
from feoh_hydrogen_boundary import (
    default_runner,
    oxygen_potential_at_fe_spinel_boundary,
    oxygen_potential_at_fe_wustite_boundary,
    oxygen_potential_at_wustite_spinel_boundary,
    query_mu0,
)

R = 8.31446261815324
GAS_SOURCE = "helmholtz+ref0:"


@dataclass(frozen=True)
class BranchSpec:
    label: str
    gas_family: str
    spec_name: str
    boundary: str
    temps_c: tuple[float, ...]
    boundary_fn: object


BRANCHES = (
    BranchSpec(
        "H2 Fe|wustite",
        "h2",
        "h2-fe-wustite",
        "fe-wustite",
        (600.0, 650.0, 700.0, 750.0, 800.0, 850.0, 900.0, 950.0),
        oxygen_potential_at_fe_wustite_boundary,
    ),
    BranchSpec(
        "H2 wustite|spinel",
        "h2",
        "h2-wustite-spinel",
        "wustite-spinel",
        (600.0, 650.0, 700.0, 750.0, 800.0, 850.0, 900.0),
        oxygen_potential_at_wustite_spinel_boundary,
    ),
    BranchSpec(
        "H2 Fe|spinel",
        "h2",
        "h2-fe-spinel",
        "fe-spinel",
        (350.0, 400.0, 450.0, 500.0, 550.0),
        oxygen_potential_at_fe_spinel_boundary,
    ),
    BranchSpec(
        "CO Fe|wustite",
        "co",
        "co-fe-wustite",
        "fe-wustite",
        (650.0, 700.0, 750.0, 800.0, 850.0, 900.0, 950.0),
        oxygen_potential_at_fe_wustite_boundary,
    ),
    BranchSpec(
        "CO wustite|spinel",
        "co",
        "co-wustite-spinel",
        "wustite-spinel",
        (600.0, 650.0, 700.0, 750.0, 800.0, 850.0, 900.0, 950.0),
        oxygen_potential_at_wustite_spinel_boundary,
    ),
    BranchSpec(
        "CO Fe|spinel",
        "co",
        "co-fe-spinel",
        "fe-spinel",
        (350.0, 400.0, 450.0, 500.0, 550.0),
        oxygen_potential_at_fe_spinel_boundary,
    ),
)


def lambda_from_h2_god(tc: float, god: float) -> float:
    tk = tc + 273.15
    mu = query_mu0(default_runner(), GAS_SOURCE, tk, ["hydrogen", "water"])
    log10_ratio = math.log10(god / (1.0 - god))
    return (mu["water"] - mu["hydrogen"]) + R * tk * math.log(10.0) * log10_ratio


def lambda_from_co_god(tc: float, god: float) -> float:
    tk = tc + 273.15
    mu = query_mu0(default_runner(), GAS_SOURCE, tk, ["carbonmonoxide", "carbondioxide"])
    log10_ratio = math.log10(god / (1.0 - god))
    return (mu["carbondioxide"] - mu["carbonmonoxide"]) + R * tk * math.log(10.0) * log10_ratio


def fit_r2(xs: list[float], ys: list[float], basis: str) -> tuple[np.ndarray, float]:
    rows = []
    for x in xs:
        tk = x + 273.15
        if basis == "T":
            rows.append([1.0, x])
        elif basis == "invT":
            rows.append([1.0, 1.0 / tk])
        elif basis == "invT2":
            rows.append([1.0, 1.0 / (tk * tk)])
        else:
            raise KeyError(basis)
    X = np.array(rows, dtype=float)
    y = np.array(ys, dtype=float)
    beta = np.linalg.lstsq(X, y, rcond=None)[0]
    yhat = X.dot(beta)
    ss_res = float(((y - yhat) ** 2).sum())
    ss_tot = float(((y - y.mean()) ** 2).sum())
    r2 = 1.0 - ss_res / ss_tot if ss_tot else 1.0
    return beta, r2


def main() -> int:
    runner = default_runner()
    for branch in BRANCHES:
        dgod: list[float] = []
        dlam: list[float] = []
        xs: list[float] = []
        for tc in branch.temps_c:
            if branch.gas_family == "h2":
                fit = fit_h2_god(H2_FIT_SPECS[branch.spec_name], tc)
                if fit is None:
                    continue
                model_god, _ = cached_model_god_at_temp_h2(branch.boundary, str(runner), GAS_SOURCE, tc, "current")
                lam_fit = lambda_from_h2_god(tc, fit)
            else:
                fit = fit_co_god(CO_FIT_SPECS[branch.spec_name], tc)
                if fit is None:
                    continue
                model_god, _ = cached_model_god_at_temp_co(branch.boundary, str(runner), GAS_SOURCE, tc, "current")
                lam_fit = lambda_from_co_god(tc, fit)
            _phase, lam_model = branch.boundary_fn(tc + 273.15)
            xs.append(tc)
            dgod.append(model_god - fit)
            dlam.append((lam_model - lam_fit) / 1000.0)

        print(branch.label)
        for label, ys in (("delta_GOD", dgod), ("delta_lambda_O [kJ/mol O]", dlam)):
            print(f"  {label}")
            for basis in ("T", "invT", "invT2"):
                beta, r2 = fit_r2(xs, ys, basis)
                print(f"    {basis:5s} R2={r2:.5f} beta={beta.tolist()}")
        print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

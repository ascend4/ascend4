#!/usr/bin/env python3
"""
Freeze the current Fe|wustite vs Baur-Glaessner comparison as an explicit
regression.

This is intentionally not a "good agreement" test. The current Hidayat-based
Fe|wustite hydrogen boundary sits well away from the traced Baur-Glaessner
line under the present GOD-axis interpretation. The purpose here is to keep
that comparison numerically explicit, so later thermo changes can be compared
against a known baseline rather than only against ourselves.
"""

from __future__ import annotations

import argparse
import math
import os
import sys
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp")

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from feoh_baur_glaessner_compare import (  # noqa: E402
    FIT_SPECS,
    cached_model_god_at_temp,
    fit_god_at_temp,
    log10_ratio_from_god,
)

DEFAULT_TEMPS_C = (600.0, 700.0, 800.0, 900.0, 990.0)
BASELINE_RMS_DLOG = 1.71443112384
BASELINE_MAX_DLOG = 1.92555823019
RMS_TOL = 0.05
MAX_TOL = 0.05


def main() -> int:
    ap = argparse.ArgumentParser(description="Regression for current Fe|wustite vs Baur-Glaessner comparison.")
    ap.add_argument(
        "--runner",
        type=Path,
        default=THIS_DIR / "eqm_mu0_runner",
        help="Path to eqm_mu0_runner.",
    )
    ap.add_argument(
        "--gas-source",
        default="reaktoro_clone_supcrt98",
        help="Gas mu0 source for hydrogen and water.",
    )
    args = ap.parse_args()

    if not args.runner.exists():
        raise SystemExit(f"Runner not found: {args.runner}")

    spec = FIT_SPECS["h2-fe-wustite"]
    temps_c = DEFAULT_TEMPS_C
    sum_sq = 0.0
    max_abs_dlog = 0.0

    print("Fe|wustite Baur-Glaessner regression")
    print(f"Runner: {args.runner}")
    print(f"Gas source: {args.gas_source}")
    print()
    print(
        f"{'T[C]':>6} {'GOD_fit':>12} {'GOD_model':>12} "
        f"{'log10 fit':>12} {'log10 model':>12} {'dlog10':>12}"
    )

    for tc in temps_c:
        fit_god = fit_god_at_temp(spec, tc)
        model_god, log10_model = cached_model_god_at_temp(spec.boundary, str(args.runner), args.gas_source, tc)
        if fit_god is None:
            raise SystemExit(f"Temperature {tc} C is outside the fit range for {spec.name}")
        log10_fit = log10_ratio_from_god(fit_god)
        dlog = log10_model - log10_fit
        sum_sq += dlog * dlog
        max_abs_dlog = max(max_abs_dlog, abs(dlog))
        print(
            f"{tc:6.0f} {fit_god:12.6f} {model_god:12.6f} "
            f"{log10_fit:12.6f} {log10_model:12.6f} {dlog:12.6f}"
        )

    rms_dlog = math.sqrt(sum_sq / len(temps_c))
    print()
    print(f"RMS delta log10(H2O/H2): {rms_dlog:.12f}")
    print(f"Max abs delta log10(H2O/H2): {max_abs_dlog:.12f}")

    if abs(rms_dlog - BASELINE_RMS_DLOG) > RMS_TOL:
        print(
            f"Regression failure: RMS dlog moved from baseline {BASELINE_RMS_DLOG:.12f} "
            f"by more than {RMS_TOL:.3f}",
            file=sys.stderr,
        )
        return 1
    if abs(max_abs_dlog - BASELINE_MAX_DLOG) > MAX_TOL:
        print(
            f"Regression failure: max |dlog| moved from baseline {BASELINE_MAX_DLOG:.12f} "
            f"by more than {MAX_TOL:.3f}",
            file=sys.stderr,
        )
        return 1

    print("Regression OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

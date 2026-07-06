#!/usr/bin/env python3
"""Compare current Fe-O-Si boundary calculations against Hidayat 2017 Fig. 15 digitised curves."""

from __future__ import annotations

import argparse
import json
import math
import statistics
import subprocess
from pathlib import Path

import matplotlib.pyplot as plt

from feosial_source_audit import R, P0, qfi_log10fo2, qfm_log10fo2, query_mu0

THIS_DIR = Path(__file__).resolve().parent

QFI_DATA = THIS_DIR / "hidayat-2017-fig15-fe2sio4-sio2-fe.dat"
QFM_DATA = THIS_DIR / "hidayat-2017-fig15-fe2sio4-spinel-sio2.dat"

QFI_FAYALITE = "hidayat_2017_feo_fe2o3_sio2"
QFI_O2 = "helmholtz+ref0:"
QFM_SPINEL = "hidayat_adj1"
QFM_O2 = "helmholtz+ref0:"
SIO2_SOURCE = "slag_pragmatic_2026"


def load_points(path: Path) -> list[tuple[float, float]]:
    pts: list[tuple[float, float]] = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        x, y = map(float, line.split()[:2])
        pts.append((x, y))
    return pts


def qfi_fit(x: float) -> float:
    return 7.78776 - 29.8308 * x


def qfm_fit(x: float) -> float:
    return 8.1539 - 24.5305 * x


def entry_phi(entry_runner: Path, spinel_source: str, T: float, lambda_fe: float, lambda_o: float) -> tuple[float, list[float]]:
    out = subprocess.check_output(
        [
            str(entry_runner),
            spinel_source,
            f"{T:.12g}",
            f"{P0:.12g}",
            f"{lambda_fe:.12g}",
            f"{lambda_o:.12g}",
        ],
        text=True,
    )
    data = json.loads(out)
    return float(data["phi"]), [float(v) for v in data["y"]]


class BoundaryContext:
    def __init__(self, mu_runner: Path, entry_runner: Path, o2_source: str):
        self.mu_runner = mu_runner
        self.entry_runner = entry_runner
        self.o2_source = o2_source
        self._mu_cache: dict[float, dict[str, float]] = {}

    def mu_base(self, T: float) -> dict[str, float]:
        key = round(T, 9)
        if key not in self._mu_cache:
            self._mu_cache[key] = query_mu0(
                self.mu_runner,
                (
                    f"SiO2={SIO2_SOURCE};"
                    f"Fe2SiO4={QFI_FAYALITE};"
                    f"oxygen={self.o2_source}"
                ),
                T,
                ["SiO2", "Fe2SiO4", "oxygen"],
            )
        return self._mu_cache[key]

    def spinel_phi_at_logfo2(self, T: float, logfo2: float, spinel_source: str) -> tuple[float, list[float]]:
        mu = self.mu_base(T)
        mu_o2 = mu["oxygen"] + R * T * math.log(10.0) * logfo2
        lambda_o = 0.5 * mu_o2
        lambda_fe = (mu["Fe2SiO4"] - mu["SiO2"] - mu_o2) / 2.0
        return entry_phi(self.entry_runner, spinel_source, T, lambda_fe, lambda_o)

    def qfm_spinel_logfo2(self, T: float, spinel_source: str, guess: float) -> tuple[float, list[float]]:
        lo = guess - 2.0
        hi = guess + 2.0
        flo, _ = self.spinel_phi_at_logfo2(T, lo, spinel_source)
        fhi, _ = self.spinel_phi_at_logfo2(T, hi, spinel_source)
        expand = 0
        while flo * fhi > 0.0 and expand < 8:
            lo -= 2.0
            hi += 2.0
            flo, _ = self.spinel_phi_at_logfo2(T, lo, spinel_source)
            fhi, _ = self.spinel_phi_at_logfo2(T, hi, spinel_source)
            expand += 1
        if flo * fhi > 0.0:
            raise RuntimeError(f"could not bracket spinel boundary at T={T:.3f}: phi({lo})={flo}, phi({hi})={fhi}")
        y = [math.nan, math.nan]
        for _ in range(70):
            mid = 0.5 * (lo + hi)
            fm, y = self.spinel_phi_at_logfo2(T, mid, spinel_source)
            if abs(fm) < 1e-5:
                return mid, y
            if flo * fm <= 0.0:
                hi = mid
                fhi = fm
            else:
                lo = mid
                flo = fm
        return 0.5 * (lo + hi), y


def residual_stats(model: list[float], ref: list[float]) -> tuple[float, float, float]:
    res = [m - r for m, r in zip(model, ref)]
    return statistics.mean(res), math.sqrt(statistics.mean([r * r for r in res])), max(abs(r) for r in res)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--mu-runner", type=Path, default=THIS_DIR / "eqm_mu0_runner")
    ap.add_argument("--entry-runner", type=Path, default=THIS_DIR / "eqm_spinel_entry_runner")
    ap.add_argument("--out", type=Path, default=THIS_DIR.parent / "res" / "hidayat-2017-fig15-feosial-compare.png")
    args = ap.parse_args()

    qfi_pts = load_points(QFI_DATA)
    qfm_pts = load_points(QFM_DATA)
    qfi_x = [x for x, _ in qfi_pts]
    qfi_y = [y for _, y in qfi_pts]
    qfm_x = [x for x, _ in qfm_pts]
    qfm_y = [y for _, y in qfm_pts]

    qfi_model = [qfi_log10fo2(args.mu_runner, 1000.0 / x, QFI_O2, QFI_FAYALITE)[0] for x in qfi_x]
    qfm_pure_re = [qfm_log10fo2(args.mu_runner, 1000.0 / x, "reaktoro_clone_supcrt98", QFI_FAYALITE) for x in qfm_x]
    qfm_pure_h = [qfm_log10fo2(args.mu_runner, 1000.0 / x, QFM_O2, QFI_FAYALITE) for x in qfm_x]

    ctx = BoundaryContext(args.mu_runner, args.entry_runner, QFM_O2)
    qfm_spinel = []
    qfm_ycoord = []
    for x in qfm_x:
        y_model, y_coord = ctx.qfm_spinel_logfo2(1000.0 / x, QFM_SPINEL, qfm_fit(x))
        qfm_spinel.append(y_model)
        qfm_ycoord.append(y_coord)

    print("QFI model vs digitised points", residual_stats(qfi_model, qfi_y))
    print("QFI model vs fit", residual_stats(qfi_model, [qfi_fit(x) for x in qfi_x]))
    print("QFM spinel-entry vs digitised points", residual_stats(qfm_spinel, qfm_y))
    print("QFM spinel-entry vs fit", residual_stats(qfm_spinel, [qfm_fit(x) for x in qfm_x]))
    print("QFM pure Fe3O4 reaktoro O2 vs fit", residual_stats(qfm_pure_re, [qfm_fit(x) for x in qfm_x]))
    print("QFM pure Fe3O4 helmholtz O2 vs fit", residual_stats(qfm_pure_h, [qfm_fit(x) for x in qfm_x]))
    print("QFM spinel y endpoints", qfm_ycoord[0], qfm_ycoord[-1])

    fig, ax = plt.subplots(figsize=(8.0, 5.4), dpi=160)
    ax.scatter(qfi_x, qfi_y, s=14, color="#c93f3f", alpha=0.75, label="Fig. 15 QFI points")
    ax.plot(qfi_x, [qfi_fit(x) for x in qfi_x], color="#8f1f1f", lw=1.5, label="Fig. 15 QFI fit")
    ax.plot(qfi_x, qfi_model, color="#111111", lw=1.4, ls="--", label="FPROPS QFI")

    ax.scatter(qfm_x, qfm_y, s=14, color="#286db6", alpha=0.65, label="Fig. 15 spinel-SiO2 points")
    ax.plot(qfm_x, [qfm_fit(x) for x in qfm_x], color="#174a86", lw=1.5, label="Fig. 15 spinel-SiO2 fit")
    ax.plot(qfm_x, qfm_spinel, color="#16835f", lw=1.6, ls="--", label=f"FPROPS spinel entry ({QFM_SPINEL})")
    ax.plot(qfm_x, qfm_pure_re, color="#9367bc", lw=1.0, ls=":", label="FPROPS pure Fe3O4 proxy, Reaktoro O2")
    ax.plot(qfm_x, qfm_pure_h, color="#b78d2d", lw=1.0, ls=":", label="FPROPS pure Fe3O4 proxy, Helmholtz O2")

    ax.axvline(0.8775, color="#888888", lw=0.8, ls="-.", label="tridymite/quartz split")
    ax.set_xlabel("1000 K / T")
    ax.set_ylabel("log10(P(O2) / 1 atm)")
    ax.set_title("Fe-O-Si buffers against Hidayat et al. 2017 Fig. 15")
    ax.grid(True, lw=0.3, alpha=0.45)
    ax.legend(fontsize=7.2, loc="lower left", ncol=1)
    fig.tight_layout()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.out)
    print(f"Wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

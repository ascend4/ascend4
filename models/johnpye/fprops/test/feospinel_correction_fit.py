#!/usr/bin/env python3
"""
Search low-order thermo-style Gibbs corrections on top of the mmc1_guess
spinel baseline.

This is the disciplined follow-on to the earlier ad hoc lambda-fit and tapered
fit experiments. It evaluates simple correction families against:

- CO Fe|spinel BG points
- CO wustite|spinel BG points
- Hidayat Fig. 11 spinel|Fe2O3 oxygen-potential trace

The goal is not to silently "find numbers", but to produce a logged,
repeatable, reviewable search with explicit objective components.
"""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import optuna

from feo_hidayat_validation import (
    g_fe2o3,
    g_fe_bcc,
    g_fe_fcc,
    minimize_spinel_grand_residual_with_phase_g,
    mu_a_wustite,
    mu_b_wustite,
    spinel_phase_g_mmc1_guess,
)
from feoh_hydrogen_boundary import default_runner, query_mu0
from feoc_baur_glaessner_compare import (
    FIT_SPECS as CO_SPECS,
    fit_god_at_temp as fit_co_god,
    gas_ratio_logs as co_ratio_logs,
)
from feoxide_hidayat_compare import (
    THIS_DIR,
    load_points,
    log10_po2_from_lambda,
    oxygen_mu0,
    summarize_residuals,
)


@dataclass(frozen=True)
class FitResult:
    objective: float
    rms_fe_spinel: float
    rms_wustite_spinel: float
    rms_spinel_fe2o3: float
    a: float
    b: float
    c: float


def stable_fe_g(T: float) -> float:
    return g_fe_bcc(T) if g_fe_bcc(T) <= g_fe_fcc(T) else g_fe_fcc(T)


def phase_g_corr_factory(kind: str, a: float, b: float, c: float):
    def fn(T: float, y_t_fe2: float, y_o_fe2: float):
        g, n_fe, y_o_fe3, y_o_va = spinel_phase_g_mmc1_guess(T, y_t_fe2, y_o_fe2)
        if not math.isfinite(g):
            return g, n_fe, y_o_fe3, y_o_va
        if kind == "affine":
            dg = a + b * T
        elif kind == "affine_invT":
            dg = a + b * T + c / T
        else:
            raise KeyError(kind)
        return g + dg, n_fe, y_o_fe3, y_o_va

    return fn


def seeded_trials(kind: str) -> list[dict[str, float]]:
    # Diagnostic lambda-fit implied phase-level correction:
    #   d(lambda_O)[kJ/mol O] ~= 4.4760957447 - 0.00713793374 T_C
    # For an additive correction on G_spinel(T[K]) this maps approximately to:
    #   dG[J/mol formula] ~= -25700 + 28.55 T
    seeds = [
        {"A": -25700.0, "B": 28.55},
        {"A": -12850.0, "B": 14.275},
        {"A": -5000.0, "B": 5.0},
    ]
    if kind == "affine":
        return seeds
    return [dict(seed, C=0.0) for seed in seeds]


def fe_spinel_lambda(T: float, phase_g_fn) -> float:
    gfe = stable_fe_g(T)
    resid, _a, _b, _v = minimize_spinel_grand_residual_with_phase_g(T, gfe, 0.0, phase_g_fn)
    return resid / 4.0


def wustite_spinel_lambda(
    T: float, phase_g_fn, x_hint: float | None = None, a_hint: float | None = None, b_hint: float | None = None
) -> tuple[float, float, float, float, float, float]:
    best_abs = math.inf
    best = None
    if x_hint is None:
        x_lo = 1e-4
        x_hi = 0.95
        steps_x = 80
    else:
        x_lo = max(1e-4, x_hint - 0.04)
        x_hi = min(0.95, x_hint + 0.04)
        steps_x = 28

    best_a = a_hint
    best_b = b_hint
    for _ in range(4):
        span = x_hi - x_lo
        a_seed = best_a
        b_seed = best_b
        for ix in range(steps_x + 1):
            x = x_lo + span * ix / steps_x
            mu_a = mu_a_wustite(T, x)
            mu_b = mu_b_wustite(T, x)
            lam_o = 2.0 * (mu_b - mu_a)
            lam_fe = 3.0 * mu_a - 2.0 * mu_b
            resid, a_cur, b_cur, v_cur = minimize_spinel_grand_residual_with_phase_g(
                T, lam_fe, lam_o, phase_g_fn, a_hint=a_seed, b_hint=b_seed
            )
            a_seed = a_cur
            b_seed = b_cur
            if abs(resid) < best_abs:
                best_abs = abs(resid)
                best = (x, lam_o, a_cur, b_cur, v_cur, resid)
                best_a = a_cur
                best_b = b_cur
        dx = max(5e-4, 0.15 * span)
        x_lo = max(1e-4, best[0] - dx)
        x_hi = min(0.95, best[0] + dx)
    return best


def spinel_hematite_lambda(T: float, phase_g_fn) -> tuple[float, float, float]:
    lam_lo = -350000.0
    lam_hi = -120000.0

    def signed(lam_o: float, a_hint: float | None = None, b_hint: float | None = None):
        lam_fe = (g_fe2o3(T) - 3.0 * lam_o) / 2.0
        resid, a_best, b_best, _v = minimize_spinel_grand_residual_with_phase_g(
            T, lam_fe, lam_o, phase_g_fn, a_hint=a_hint, b_hint=b_hint
        )
        return resid, a_best, b_best

    r_lo, a_lo, b_lo = signed(lam_lo)
    r_hi, a_hi, b_hi = signed(lam_hi)
    if r_lo * r_hi > 0.0:
        return math.nan, math.nan, math.nan

    lo = lam_lo
    hi = lam_hi
    a_hint = a_lo
    b_hint = b_lo
    for _ in range(80):
        mid = 0.5 * (lo + hi)
        r_mid, a_mid, b_mid = signed(mid, a_hint, b_hint)
        if abs(r_mid) < 1e-6 or abs(hi - lo) < 1e-6:
            return mid, a_mid, b_mid
        if r_lo * r_mid <= 0.0:
            hi = mid
        else:
            lo = mid
            r_lo = r_mid
        a_hint = a_mid
        b_hint = b_mid
    return 0.5 * (lo + hi), a_hint, b_hint


def evaluate_candidate(
    runner: Path,
    gas_source: str,
    kind: str,
    a: float,
    b: float,
    c: float,
    co_fe_spinel_temps_c: list[float],
    co_wustite_spinel_temps_c: list[float],
    fig11_spinel_fe2o3_points,
    w_fe_spinel: float,
    w_wustite_spinel: float,
    w_spinel_fe2o3: float,
) -> FitResult:
    phase_g_fn = phase_g_corr_factory(kind, a, b, c)

    errs = []
    for tc in co_fe_spinel_temps_c:
        tk = tc + 273.15
        lam_o = fe_spinel_lambda(tk, phase_g_fn)
        mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_ratio, _ = co_ratio_logs(mu["carbonmonoxide"], mu["carbondioxide"], lam_o, tk)
        god_model = (10.0**log10_ratio) / (1.0 + 10.0**log10_ratio)
        god_fit = fit_co_god(CO_SPECS["co-fe-spinel"], tc)
        errs.append(god_model - god_fit)
    rms_fe_spinel = math.sqrt(sum(e * e for e in errs) / len(errs))

    errs = []
    x_hint = None
    a_hint = None
    b_hint = None
    for tc in co_wustite_spinel_temps_c:
        tk = tc + 273.15
        x_hint, lam_o, a_hint, b_hint, _v, _resid = wustite_spinel_lambda(
            tk, phase_g_fn, x_hint, a_hint, b_hint
        )
        mu = query_mu0(runner, gas_source, tk, ["carbonmonoxide", "carbondioxide"])
        log10_ratio, _ = co_ratio_logs(mu["carbonmonoxide"], mu["carbondioxide"], lam_o, tk)
        god_model = (10.0**log10_ratio) / (1.0 + 10.0**log10_ratio)
        god_fit = fit_co_god(CO_SPECS["co-wustite-spinel"], tc)
        errs.append(god_model - god_fit)
    rms_wustite_spinel = math.sqrt(sum(e * e for e in errs) / len(errs))

    errs = []
    for invtemp, y_ref in sorted(fig11_spinel_fe2o3_points.points, key=lambda p: 1000.0 / p[0]):
        tk = 1000.0 / invtemp
        lam_o, _a_best, _b_best = spinel_hematite_lambda(tk, phase_g_fn)
        mu_o2 = oxygen_mu0(str(runner), gas_source, tk)
        y_model = log10_po2_from_lambda(lam_o, mu_o2, tk)
        errs.append(y_model - y_ref)
    rms_spinel_fe2o3 = math.sqrt(sum(e * e for e in errs) / len(errs))

    objective = (
        w_fe_spinel * rms_fe_spinel
        + w_wustite_spinel * rms_wustite_spinel
        + w_spinel_fe2o3 * rms_spinel_fe2o3
    )
    return FitResult(objective, rms_fe_spinel, rms_wustite_spinel, rms_spinel_fe2o3, a, b, c)


def maybe_report(label: str, result: FitResult, report_best: list[FitResult], top_n: int) -> list[FitResult]:
    report_best.append(result)
    report_best.sort(key=lambda r: r.objective)
    del report_best[top_n:]
    print(
        f"{label}: obj={result.objective:.6f} "
        f"rms_fs={result.rms_fe_spinel:.6f} "
        f"rms_ws={result.rms_wustite_spinel:.6f} "
        f"rms_sh={result.rms_spinel_fe2o3:.6f} "
        f"A={result.a:.6f} B={result.b:.6f} C={result.c:.6f}"
    , flush=True)
    return report_best


def main() -> int:
    ap = argparse.ArgumentParser(description="Logged fitter for simple spinel Gibbs corrections on top of mmc1_guess.")
    ap.add_argument("--kind", choices=("affine", "affine_invT"), default="affine", help="Correction family.")
    ap.add_argument("--engine", choices=("grid", "optuna"), default="optuna", help="Search engine.")
    ap.add_argument("--runner", type=Path, default=default_runner(), help="Path to eqm_mu0_runner.")
    ap.add_argument("--gas-source", default="helmholtz+ref0:", help="Gas mu0 source.")
    ap.add_argument("--report-every", type=int, default=25, help="Print progress every N candidates.")
    ap.add_argument("--top-n", type=int, default=8, help="Keep and print the best N candidates.")
    ap.add_argument("--w-fe-spinel", type=float, default=1.0, help="Objective weight for CO Fe|spinel.")
    ap.add_argument("--w-wustite-spinel", type=float, default=1.0, help="Objective weight for CO wustite|spinel.")
    ap.add_argument("--w-spinel-fe2o3", type=float, default=1.0, help="Objective weight for Hidayat Fig. 11 spinel|Fe2O3.")
    ap.add_argument("--trials", type=int, default=80, help="Optuna trial count.")
    ap.add_argument("--jobs", type=int, default=2, help="Optuna parallel jobs.")
    ap.add_argument("--study-name", default=None, help="Optional Optuna study name.")
    ap.add_argument(
        "--storage",
        type=Path,
        default=None,
        help="Optional Optuna SQLite file path. Default is a file beside this script.",
    )
    ap.add_argument("--heartbeat-sec", type=int, default=60, help="Optuna heartbeat interval in seconds.")
    ap.add_argument("--grace-sec", type=int, default=180, help="Optuna stale-trial grace period in seconds.")
    ap.add_argument(
        "--summary-csv",
        type=Path,
        default=None,
        help="Optional CSV file for completed-trial summary export.",
    )
    args = ap.parse_args()

    fig11_spinel_fe2o3 = load_points(THIS_DIR / "hidayat-2015-fig11-spin-Fe2O3.dat")
    co_fe_spinel_temps_c = [350.0, 400.0, 450.0, 500.0, 550.0]
    co_wustite_spinel_temps_c = [600.0, 700.0, 800.0, 900.0]

    baseline = evaluate_candidate(
        args.runner,
        args.gas_source,
        args.kind,
        0.0,
        0.0,
        0.0,
        co_fe_spinel_temps_c,
        co_wustite_spinel_temps_c,
        fig11_spinel_fe2o3,
        args.w_fe_spinel,
        args.w_wustite_spinel,
        args.w_spinel_fe2o3,
    )
    print("Baseline")
    print(
        f"  obj={baseline.objective:.6f} "
        f"rms_fs={baseline.rms_fe_spinel:.6f} "
        f"rms_ws={baseline.rms_wustite_spinel:.6f} "
        f"rms_sh={baseline.rms_spinel_fe2o3:.6f}"
    , flush=True)
    print(flush=True)

    counter = 0
    best: list[FitResult] = []

    def run_candidate(label: str, a: float, b: float, c: float) -> None:
        nonlocal counter, best
        counter += 1
        result = evaluate_candidate(
            args.runner,
            args.gas_source,
            args.kind,
            a,
            b,
            c,
            co_fe_spinel_temps_c,
            co_wustite_spinel_temps_c,
            fig11_spinel_fe2o3,
            args.w_fe_spinel,
            args.w_wustite_spinel,
            args.w_spinel_fe2o3,
        )
        if (not best) or result.objective < best[0].objective or counter % args.report_every == 0:
            best = maybe_report(label, result, best, args.top_n)

    if args.engine == "grid":
        if args.kind == "affine":
            coarse_A = np.linspace(-40000.0, 10000.0, 15)
            coarse_B = np.linspace(-20.0, 40.0, 14)
            for A in coarse_A:
                for B in coarse_B:
                    run_candidate("coarse", float(A), float(B), 0.0)

            seed = best[0]
            fine_A = np.linspace(seed.a - 4000.0, seed.a + 4000.0, 17)
            fine_B = np.linspace(seed.b - 4.0, seed.b + 4.0, 17)
            for A in fine_A:
                for B in fine_B:
                    run_candidate("refine", float(A), float(B), 0.0)
        else:
            coarse_A = np.linspace(-40000.0, 10000.0, 11)
            coarse_B = np.linspace(-20.0, 40.0, 11)
            coarse_C = np.linspace(-4.0e6, 4.0e6, 9)
            for A in coarse_A:
                for B in coarse_B:
                    for C in coarse_C:
                        run_candidate("coarse", float(A), float(B), float(C))
    else:
        storage_path = args.storage if args.storage is not None else (Path(__file__).resolve().parent / "feospinel_correction_fit_optuna.db")
        storage_url = f"sqlite:///{storage_path}"
        study_name = args.study_name or f"feospinel_{args.kind}"
        sampler = optuna.samplers.TPESampler(seed=42, multivariate=True)
        storage = optuna.storages.RDBStorage(
            url=storage_url,
            heartbeat_interval=args.heartbeat_sec,
            grace_period=args.grace_sec,
        )
        study = optuna.create_study(
            study_name=study_name,
            storage=storage,
            load_if_exists=True,
            direction="minimize",
            sampler=sampler,
        )
        optuna.storages.fail_stale_trials(study)
        queued = {(tuple(sorted(t.params.items()))) for t in study.trials if t.params}
        new_seed_count = 0
        for params in seeded_trials(args.kind):
            key = tuple(sorted(params.items()))
            if key not in queued:
                study.enqueue_trial(params)
                new_seed_count += 1

        print(f"Optuna study: {study.study_name}", flush=True)
        print(f"Storage: {storage_url}", flush=True)
        print(f"Parallel jobs: {args.jobs}", flush=True)
        print(f"Requested trials: {args.trials}", flush=True)
        print(f"Heartbeat / grace [s]: {args.heartbeat_sec} / {args.grace_sec}", flush=True)
        print(flush=True)

        def objective(trial: optuna.trial.Trial) -> float:
            if args.kind == "affine":
                a = trial.suggest_float("A", -40000.0, 10000.0)
                b = trial.suggest_float("B", -20.0, 40.0)
                c = 0.0
            else:
                a = trial.suggest_float("A", -40000.0, 10000.0)
                b = trial.suggest_float("B", -20.0, 40.0)
                c = trial.suggest_float("C", -4.0e6, 4.0e6)

            result = evaluate_candidate(
                args.runner,
                args.gas_source,
                args.kind,
                a,
                b,
                c,
                co_fe_spinel_temps_c,
                co_wustite_spinel_temps_c,
                fig11_spinel_fe2o3,
                args.w_fe_spinel,
                args.w_wustite_spinel,
                args.w_spinel_fe2o3,
            )
            if not (
                math.isfinite(result.objective)
                and math.isfinite(result.rms_fe_spinel)
                and math.isfinite(result.rms_wustite_spinel)
                and math.isfinite(result.rms_spinel_fe2o3)
            ):
                trial.set_user_attr("rms_fe_spinel", math.inf)
                trial.set_user_attr("rms_wustite_spinel", math.inf)
                trial.set_user_attr("rms_spinel_fe2o3", math.inf)
                return 1.0e9
            trial.set_user_attr("rms_fe_spinel", result.rms_fe_spinel)
            trial.set_user_attr("rms_wustite_spinel", result.rms_wustite_spinel)
            trial.set_user_attr("rms_spinel_fe2o3", result.rms_spinel_fe2o3)
            return result.objective

        def callback(study: optuna.study.Study, trial: optuna.trial.FrozenTrial) -> None:
            optuna.storages.fail_stale_trials(study)
            value = trial.value if trial.value is not None else math.inf
            print(
                f"trial={trial.number} state={trial.state.name} value={value:.6f} "
                f"A={trial.params.get('A', 0.0):.6f} "
                f"B={trial.params.get('B', 0.0):.6f} "
                f"C={trial.params.get('C', 0.0):.6f} "
                f"rms_fs={trial.user_attrs.get('rms_fe_spinel', math.nan):.6f} "
                f"rms_ws={trial.user_attrs.get('rms_wustite_spinel', math.nan):.6f} "
                f"rms_sh={trial.user_attrs.get('rms_spinel_fe2o3', math.nan):.6f} "
                f"best={study.best_value:.6f}",
                flush=True,
            )

        remaining_trials = args.trials
        if new_seed_count:
            seed_trials = min(new_seed_count, remaining_trials)
            print(f"Queued seeds: {new_seed_count} (running first {seed_trials} sequentially)", flush=True)
            study.optimize(objective, n_trials=seed_trials, n_jobs=1, callbacks=[callback])
            remaining_trials -= seed_trials
        if remaining_trials > 0:
            study.optimize(objective, n_trials=remaining_trials, n_jobs=args.jobs, callbacks=[callback])

        best = []
        completed = [t for t in study.trials if t.state == optuna.trial.TrialState.COMPLETE]
        completed.sort(key=lambda t: t.value)
        for trial in completed[: args.top_n]:
            best.append(
                FitResult(
                    trial.value,
                    float(trial.user_attrs["rms_fe_spinel"]),
                    float(trial.user_attrs["rms_wustite_spinel"]),
                    float(trial.user_attrs["rms_spinel_fe2o3"]),
                    float(trial.params.get("A", 0.0)),
                    float(trial.params.get("B", 0.0)),
                    float(trial.params.get("C", 0.0)),
                )
            )

        summary_csv = args.summary_csv if args.summary_csv is not None else (
            Path(__file__).resolve().parent / f"feospinel_correction_fit_{args.kind}_summary.csv"
        )
        try:
            with summary_csv.open("w", newline="") as fp:
                writer = csv.writer(fp)
                writer.writerow(
                    [
                        "number",
                        "state",
                        "value",
                        "A",
                        "B",
                        "C",
                        "rms_fe_spinel",
                        "rms_wustite_spinel",
                        "rms_spinel_fe2o3",
                    ]
                )
                for trial in sorted(study.trials, key=lambda t: t.number):
                    writer.writerow(
                        [
                            trial.number,
                            trial.state.name,
                            "" if trial.value is None else trial.value,
                            trial.params.get("A", ""),
                            trial.params.get("B", ""),
                            trial.params.get("C", ""),
                            trial.user_attrs.get("rms_fe_spinel", ""),
                            trial.user_attrs.get("rms_wustite_spinel", ""),
                            trial.user_attrs.get("rms_spinel_fe2o3", ""),
                        ]
                    )
            print(f"Summary CSV: {summary_csv}", flush=True)
            print(flush=True)
        except Exception as e:
            print(f"WARNING: failed to write summary CSV: {e}", flush=True)

    print()
    print("Best candidates")
    for i, result in enumerate(best, start=1):
        print(
            f"{i}. obj={result.objective:.6f} "
            f"rms_fs={result.rms_fe_spinel:.6f} "
            f"rms_ws={result.rms_wustite_spinel:.6f} "
            f"rms_sh={result.rms_spinel_fe2o3:.6f} "
            f"A={result.a:.6f} B={result.b:.6f} C={result.c:.6f}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

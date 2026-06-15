#!/usr/bin/env python3
"""Closed-system Fe-O-H-Si equilibrium sweep for a simple hydrogen DRI ore basis.

Default basis: 100 g ore assaying 60 wt% elemental Fe, with Fe present
initially as Fe2O3 and the remaining mass as SiO2.  GOD means
H2O/(H2+H2O).
"""

import argparse
import csv
import json
import math
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
RUNNER = ROOT / "models/johnpye/fprops/test/feohsi_phase_runner"
MU0_RUNNER = ROOT / "models/johnpye/fprops/test/eqm_mu0_runner"
R = 8.31446261815324
P0 = 100000.0
M_FE = 55.845
M_O = 15.999
M_SI = 28.0855
M_SIO2 = M_SI + 2.0 * M_O
M_FE2O3 = 2.0 * M_FE + 3.0 * M_O
M_FE2SIO4 = 2.0 * M_FE + M_SI + 4.0 * M_O


def ore_basis(fe_wt_pct=60.0, ore_g=100.0, gas_mol=100.0):
    fe_mass = ore_g * fe_wt_pct / 100.0
    fe_mol = fe_mass / 55.845
    fe2o3_mol = fe_mol / 2.0
    fe2o3_mass = fe2o3_mol * M_FE2O3
    sio2_mass = ore_g - fe2o3_mass
    if sio2_mass < 0.0:
        raise ValueError("Fe assay is too high for a Fe2O3 + SiO2 ore basis")
    sio2_mol = sio2_mass / M_SIO2
    solid_o = 3.0 * fe2o3_mol + 2.0 * sio2_mol
    return {
        "Fe": fe_mol,
        "Si": sio2_mol,
        "solid_O": solid_o,
        "H": 2.0 * gas_mol,
        "gas_mol": gas_mol,
        "ore_g": ore_g,
        "fe_mass": fe_mass,
        "fe2o3_mass": fe2o3_mass,
        "sio2_mass": sio2_mass,
    }


def run_json(args):
    cp = subprocess.run([str(a) for a in args], text=True, capture_output=True)
    if cp.returncode != 0:
        raise RuntimeError(cp.stderr.strip() or cp.stdout.strip())
    return json.loads(cp.stdout)


def mu0_gas(T):
    data = run_json([MU0_RUNNER, "helmholtz+ref0:", T, P0, "hydrogen", "water", "oxygen"])
    return dict(zip(data["species"], data["mu0"]))


def log10_po2_from_god(T, god, mu0):
    if not (0.0 < god < 1.0):
        return math.nan
    ratio = god / (1.0 - god)
    mu_o2_eff = 2.0 * (mu0["water"] - mu0["hydrogen"] + R * T * math.log(ratio))
    return (mu_o2_eff - mu0["oxygen"]) / (R * T * math.log(10.0))


def qfi_fit(T):
    return 7.78776 - 29.8308 * (1000.0 / T)


def qfm_fit(T):
    return 8.1539 - 24.5305 * (1000.0 / T)

def solid_phase_masses(phases, members):
    fe_metal_mol = phases["Fe_bcc"] + phases["Fe_fcc"]
    w_feo = members["wustite:Wus_FeO"]
    w_feo15 = members["wustite:Wus_FeO1p5"]
    sp_t_fe2 = members["spinel:Sp_Fe2_tet"]
    sp_t_fe3 = members["spinel:Sp_Fe3_tet"]
    sp_o_fe2 = members["spinel:Sp_Fe2_oct"]
    sp_o_fe3 = members["spinel:Sp_Fe3_oct"]
    spinel_fe_mol = sp_t_fe2 + sp_t_fe3 + sp_o_fe2 + sp_o_fe3
    spinel_o_mol = 4.0 * (sp_t_fe2 + sp_t_fe3)
    masses = {
        "metal": fe_metal_mol * M_FE,
        "fayalite": phases["Fe2SiO4"] * M_FE2SIO4,
        "silica": phases["SiO2"] * M_SIO2,
        "wustite": w_feo * (M_FE + M_O) + w_feo15 * (M_FE + 1.5 * M_O),
        "spinel": spinel_fe_mol * M_FE + spinel_o_mol * M_O,
        "hematite": phases["Fe2O3"] * M_FE2O3,
    }
    total = sum(masses.values())
    fractions = {f"mass_frac_{k}": (v / total if total > 0.0 else math.nan) for k, v in masses.items()}
    masses.update({f"solid_mass_frac_{k}": fractions[f"mass_frac_{k}"] for k in masses})
    masses["solid_mass_total_g"] = total
    return masses


def sweep(temps_c, gods, basis, solver="slsqp"):
    rows = []
    for tc in temps_c:
        T = tc + 273.15
        mu0 = mu0_gas(T)
        for god in gods:
            b_o = basis["solid_O"] + basis["gas_mol"] * god
            args = [RUNNER, T, P0, basis["Fe"], b_o, basis["Si"], basis["H"], solver, "auto"]
            try:
                data = run_json(args)
                used_solver = solver
            except RuntimeError:
                data = run_json([RUNNER, T, P0, basis["Fe"], b_o, basis["Si"], basis["H"], "ipopt", "auto"])
                used_solver = "ipopt"
            summary = data["summary"]
            phases = data["phase_amounts"]
            members = data["member_amounts"]
            god_out = summary["gas_GOD_out"]
            fe_wustite = members["wustite:Wus_FeO"] + members["wustite:Wus_FeO1p5"]
            fe_spinel = (
                members["spinel:Sp_Fe2_tet"]
                + members["spinel:Sp_Fe3_tet"]
                + members["spinel:Sp_Fe2_oct"]
                + members["spinel:Sp_Fe3_oct"]
            )
            fe_hematite = 2.0 * phases["Fe2O3"]
            fe_fayalite_capacity = min(1.0, 2.0 * basis["Si"] / basis["Fe"])
            masses = solid_phase_masses(phases, members)
            row = {
                "T_C": tc,
                "T_K": T,
                "feed_GOD": god,
                "gas_GOD_out": god_out,
                "log10_pO2_out": log10_po2_from_god(T, god_out, mu0),
                "qfi_log10_pO2_fit": qfi_fit(T),
                "qfm_log10_pO2_fit": qfm_fit(T),
                "Fe_metal_frac": summary["Fe_metal_frac"],
                "Fe_fayalite_frac": summary["Fe_fayalite_frac"],
                "Fe_fayalite_capacity_frac": fe_fayalite_capacity,
                "Fe_wustite_frac": fe_wustite / basis["Fe"],
                "Fe_spinel_frac": fe_spinel / basis["Fe"],
                "Fe_hematite_frac": fe_hematite / basis["Fe"],
                "Si_fayalite_frac": summary["Si_fayalite_frac"],
                "Fe_bcc_mol": phases["Fe_bcc"],
                "Fe_fcc_mol": phases["Fe_fcc"],
                "wustite_mol": phases["wustite"],
                "spinel_mol": phases["spinel"],
                "hematite_mol": phases["Fe2O3"],
                "SiO2_mol": phases["SiO2"],
                "fayalite_mol": phases["Fe2SiO4"],
                "solid_mass_total_g": masses["solid_mass_total_g"],
                "mass_metal_g": masses["metal"],
                "mass_fayalite_g": masses["fayalite"],
                "mass_silica_g": masses["silica"],
                "mass_wustite_g": masses["wustite"],
                "mass_spinel_g": masses["spinel"],
                "mass_hematite_g": masses["hematite"],
                "solid_mass_frac_metal": masses["solid_mass_frac_metal"],
                "solid_mass_frac_fayalite": masses["solid_mass_frac_fayalite"],
                "solid_mass_frac_silica": masses["solid_mass_frac_silica"],
                "solid_mass_frac_wustite": masses["solid_mass_frac_wustite"],
                "solid_mass_frac_spinel": masses["solid_mass_frac_spinel"],
                "solid_mass_frac_hematite": masses["solid_mass_frac_hematite"],
                "solver": used_solver,
                "status": data["status"],
                "solver_status": data["solver_status"],
            }
            rows.append(row)
    return rows


def write_csv(rows, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_plot(rows, path):
    import matplotlib.pyplot as plt

    path.parent.mkdir(parents=True, exist_ok=True)
    temps = sorted({r["T_C"] for r in rows})
    cmap = plt.get_cmap("viridis", len(temps))
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.0, 4.4), constrained_layout=True)
    fe_cap = rows[0]["Fe_fayalite_capacity_frac"] if rows else 1.0
    for i, tc in enumerate(temps):
        rr = [r for r in rows if r["T_C"] == tc]
        rr.sort(key=lambda r: r["feed_GOD"])
        color = cmap(i)
        ax1.plot([r["feed_GOD"] for r in rr], [r["Fe_fayalite_frac"] for r in rr], color=color, label=f"{tc:.0f} C")
        ax2.plot([r["gas_GOD_out"] for r in rr], [r["log10_pO2_out"] for r in rr], color=color, label=f"{tc:.0f} C")
        ax2.scatter([r["gas_GOD_out"] for r in rr if 0.05 < r["Fe_fayalite_frac"] < 0.95],
                    [r["log10_pO2_out"] for r in rr if 0.05 < r["Fe_fayalite_frac"] < 0.95],
                    color=color, s=18)
    ax1.set_xlabel("Feed GOD = H2O/(H2+H2O)")
    ax1.set_ylabel("Fraction of Fe in fayalite")
    ax1.axhline(fe_cap, color="0.35", lw=0.9, ls="--", label="Si-limited cap")
    ax1.set_ylim(-0.02, min(1.02, fe_cap + 0.18))
    ax1.grid(True, alpha=0.25)
    ax1.legend(title="Temperature", fontsize=8)
    ax2.set_xlabel("Equilibrium gas GOD")
    ax2.set_ylabel("log10(pO2 / 1 bar), from H2/H2O")
    ax2.grid(True, alpha=0.25)
    ax2.legend(title="Temperature", fontsize=8)
    fig.suptitle("Fe-O-H-Si equilibrium sweep: 60 wt% Fe assay as Fe2O3, balance SiO2")
    fig.savefig(path, dpi=160)




def write_mass_stack_plot(rows, path, temps_c=(600.0, 800.0)):
    import matplotlib.pyplot as plt

    path.parent.mkdir(parents=True, exist_ok=True)
    phases = [
        ("metal", "Fe metal", "#737373"),
        ("fayalite", "Fayalite", "#3b7f5f"),
        ("silica", "SiO2", "#d8c45f"),
        ("wustite", "Wustite", "#9b5a3c"),
        ("spinel", "Spinel", "#4b6fb5"),
        ("hematite", "Hematite", "#b24a4a"),
    ]
    fig, axes = plt.subplots(1, len(temps_c), figsize=(11.0, 4.2), sharey=True, constrained_layout=True)
    if len(temps_c) == 1:
        axes = [axes]
    for ax, tc in zip(axes, temps_c):
        rr = sorted([r for r in rows if abs(r["T_C"] - tc) < 1e-9], key=lambda r: r["feed_GOD"])
        x = [r["feed_GOD"] for r in rr]
        y = [[r[f"solid_mass_frac_{key}"] for r in rr] for key, _, _ in phases]
        ax.stackplot(x, y, labels=[label for _, label, _ in phases], colors=[color for _, _, color in phases], alpha=0.92)
        ax.set_title(f"{tc:.0f} C")
        ax.set_xlabel("Feed GOD = H2O/(H2+H2O)")
        ax.set_xlim(min(x), max(x))
        ax.set_ylim(0.0, 1.0)
        ax.grid(True, axis="x", alpha=0.2)
    axes[0].set_ylabel("Solid phase mass fraction")
    axes[-1].legend(loc="center left", bbox_to_anchor=(1.02, 0.5), frameon=False)
    fig.suptitle("Fe-O-H-Si solid phase mass fractions, 60 wt% Fe assay ore")
    fig.savefig(path, dpi=160)


def transition_summary(rows):
    out = []
    for tc in sorted({r["T_C"] for r in rows}):
        rr = sorted([r for r in rows if r["T_C"] == tc], key=lambda r: r["feed_GOD"])
        cap = rr[0]["Fe_fayalite_capacity_frac"]
        candidates = [r for r in rr if 0.05 * cap <= r["Fe_fayalite_frac"] <= 0.95 * cap]
        if candidates:
            r = min(candidates, key=lambda r: abs(r["Fe_fayalite_frac"] - 0.5 * cap))
        else:
            richer = [r for r in rr if r["Fe_fayalite_frac"] >= 0.95 * cap]
            r = richer[0] if richer else min(rr, key=lambda r: abs(r["Fe_fayalite_frac"] - 0.5 * cap))
        out.append(r)
    return out


def main():
    ap = argparse.ArgumentParser(description="Fe-O-H-Si DRI equilibrium GOD sweep.")
    ap.add_argument("--output", default=str(ROOT / "models/johnpye/fprops/test/feohsi_dri_sweep_60Fe_assay.csv"))
    ap.add_argument("--plot", default=str(ROOT / "models/johnpye/fprops/res/feohsi_dri_sweep_60Fe_assay.png"))
    ap.add_argument("--mass-plot", default=str(ROOT / "models/johnpye/fprops/res/feohsi_dri_mass_stack_600C_800C.png"))
    ap.add_argument("--no-plot", action="store_true")
    args = ap.parse_args()

    temps_c = [500.0, 600.0, 700.0, 800.0, 900.0, 1000.0]
    gods = [i / 200.0 for i in range(0, 81)] + [0.5, 0.6, 0.8]
    basis = ore_basis()
    rows = sweep(temps_c, gods, basis)
    write_csv(rows, Path(args.output))
    if not args.no_plot:
        write_plot(rows, Path(args.plot))
        write_mass_stack_plot(rows, Path(args.mass_plot))
    print(
        "Fe-O-H-Si sweep basis: "
        f"{basis['ore_g']:.0f} g ore, {basis['fe_mass']:.3g} g Fe as "
        f"{basis['fe2o3_mass']:.3g} g Fe2O3, {basis['sio2_mass']:.3g} g SiO2, "
        f"{basis['gas_mol']:.3g} mol H2/H2O gas"
    )
    print(f"Wrote {args.output}")
    if not args.no_plot:
        print(f"Wrote {args.plot}")
        print(f"Wrote {args.mass_plot}")
    print(
        f"{'T[C]':>6} {'feed GOD*':>10} {'out GOD':>10} {'logpO2':>10} "
        f"{'QFI fit':>10} {'Fe fay':>8} {'Fe met':>8} {'Fe wus':>8} {'Fe sp':>8}"
    )
    for r in transition_summary(rows):
        print(
            f"{r['T_C']:6.0f} {r['feed_GOD']:10.3f} {r['gas_GOD_out']:10.4f} "
            f"{r['log10_pO2_out']:10.3f} {r['qfi_log10_pO2_fit']:10.3f} "
            f"{r['Fe_fayalite_frac']:8.3f} {r['Fe_metal_frac']:8.3f} "
            f"{r['Fe_wustite_frac']:8.3f} {r['Fe_spinel_frac']:8.3f}"
        )
    print("*Nearest sampled point to the silica-limited fayalite onset/saturation switch.")


if __name__ == "__main__":
    main()

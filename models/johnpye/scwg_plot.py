#!/usr/bin/env python3
"""Plot a native ASCEND scwg_study TSV; all model calculations stay in ASCEND.

Usage from the source directory:
    ./a4 run models/johnpye/scwg_study.a4c -r study
    python3 models/johnpye/scwg_plot.py scwg_study.tsv

Writes PNG and SVG beside the input by default. Use --output PATH to change
the output basename, or --show to also open the figure interactively.
"""

import argparse
import csv
import math
from pathlib import Path


def read_study(path):
    # ASCEND appends display-unit annotations to the TSV column names.
    # These particular model outputs already contain numerical values in
    # explicitly chosen units, independently of ASCEND display preferences.
    required = (
        "T_degC", "P_blank_bar", "P_low_bar", "P_high_bar",
        "CO2_low_g", "CO2_high_g", "elapsed_min",
    )
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        headers = {name.split(" [", 1)[0]: name for name in reader.fieldnames or []}
        missing = set(required) - headers.keys()
        if missing:
            raise ValueError("Missing study columns: " + ", ".join(sorted(missing)))
        rows = [
            {name: float(row[headers[name]]) for name in required}
            for row in reader
        ]
    if len(rows) < 2:
        raise ValueError("A plot requires at least two study points")
    if any(not math.isfinite(value) for row in rows for value in row.values()):
        raise ValueError("Study contains non-finite values")
    if any(b["T_degC"] <= a["T_degC"] for a, b in zip(rows, rows[1:])):
        raise ValueError("Expected an increasing-temperature study")
    return {name: [row[name] for row in rows] for name in required}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path, default=Path("scwg_study.tsv"))
    parser.add_argument("--output", type=Path, help="Output basename (without extension)")
    parser.add_argument("--show", action="store_true")
    args = parser.parse_args()
    try:
        data = read_study(args.input)
    except (OSError, ValueError, TypeError) as error:
        parser.error(str(error))

    import matplotlib
    if not args.show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.size": 11, "axes.spines.top": False, "axes.spines.right": False})
    fig, (pressure, release) = plt.subplots(
        2, 1, figsize=(9, 8), sharex=True, gridspec_kw={"height_ratios": [2, 1]}
    )
    temperature = data["T_degC"]
    colours = ("#277da1", "#bc4749")
    pressure.plot(temperature, data["P_blank_bar"], color="#444444", ls="--", lw=2,
                  label="Water + inert gas (blank)")
    for case, (pressure_key, release_key, colour) in enumerate(zip(
        ("P_low_bar", "P_high_bar"), ("CO2_low_g", "CO2_high_g"), colours
    ), start=1):
        label = f"Case {case}: {data[release_key][-1]:.2f} g CO₂ at {temperature[-1]:g}°C"
        pressure.plot(temperature, data[pressure_key], color=colour, lw=2.2, label=label)
        release.plot(temperature, data[release_key], color=colour, lw=2.2, label=f"Case {case}")

    pressure.set_title("Fixed-volume heating with prescribed gas release", loc="left", pad=14)
    pressure.set_ylabel("Total pressure / bar absolute")
    pressure.legend(loc="upper left", frameon=False, fontsize=10)
    release.set_ylabel("Cumulative CO₂ / g")
    release.set_xlabel("Temperature / °C")
    for axis in (pressure, release):
        axis.grid(alpha=0.2)
        axis.set_ylim(bottom=0)
        axis.set_xlim(temperature[0], temperature[-1])
    fig.text(0.12, 0.035,
             "Prescribed release scenarios, not fitted kinetics. Char–water chemistry includes H₂.\n"
             "Liquid water present; ideal gases; shift equilibrium; gas dissolution omitted.",
             fontsize=9, color="#444444")
    fig.tight_layout(rect=(0, 0.095, 1, 1))
    base = args.output if args.output is not None else args.input.with_suffix("")
    for suffix in (".png", ".svg"):
        target = Path(str(base) + suffix)
        fig.savefig(target, dpi=180)
        print(target)
    if args.show:
        plt.show()
    plt.close(fig)


if __name__ == "__main__":
    main()

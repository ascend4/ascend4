#!/usr/bin/env python3
import json
import math
import subprocess
import sys
from pathlib import Path

R = 8.3145
T = 340.0
P0 = 100000.0
P = 202600.0
FOGLER_KC_DM3 = 0.1
FOGLER_XEF = 0.51
DATABASE_XEF = 0.3998285657818691


def runner_path():
	return Path(__file__).resolve().parent / "eqm_mu0_runner"


def query_mu0(source, species):
	cmd = [str(runner_path()), source, f"{T}", f"{P0}"] + species
	proc = subprocess.run(cmd, check=True, capture_output=True, text=True)
	data = json.loads(proc.stdout)
	return data["mu0"]


def main():
	mu_n2o4, mu_no2 = query_mu0("ideal+ref0:RPP", ["nitrogen_tetroxide", "nitrogen_dioxide"])
	dg = 2.0 * mu_no2 - mu_n2o4
	k_activity = math.exp(-dg / (R * T))
	kc_m3 = k_activity * P0 / (R * T)
	kc_dm3 = kc_m3 / 1000.0
	ca0_dm3 = P / (R * T) / 1000.0
	kc_from_x = 4.0 * ca0_dm3 * DATABASE_XEF * DATABASE_XEF / ((1.0 - DATABASE_XEF) * (1.0 + DATABASE_XEF))

	print(f"Fogler KC [mol/dm^3] = {FOGLER_KC_DM3:.12g}")
	print(f"Database ideal-RPP KC [mol/dm^3] = {kc_dm3:.12g}")
	print(f"Database ideal-RPP KC from Xeq [mol/dm^3] = {kc_from_x:.12g}")
	print(f"DeltaG0 [J/mol] = {dg:.12g}")
	print(f"Dimensionless K(activity basis) = {k_activity:.12g}")
	print(f"Fogler Xef = {FOGLER_XEF:.12g}")
	print(f"Database Xef = {DATABASE_XEF:.12g}")

	assert abs(kc_dm3 - kc_from_x) < 1e-6, "thermo-derived KC and equilibrium-conversion KC should agree"
	assert abs(kc_dm3 - FOGLER_KC_DM3) > 0.03, "database KC should remain visibly different from Fogler's textbook KC"
	assert abs(DATABASE_XEF - FOGLER_XEF) > 0.05, "database Xeq should remain visibly different from Fogler's textbook Xef"


if __name__ == "__main__":
	main()

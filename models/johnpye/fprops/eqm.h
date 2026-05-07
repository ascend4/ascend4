#ifndef FPROPS_EQM_H
#define FPROPS_EQM_H

#include "common.h"

/*
 * Public API for chemical-equilibrium calculations in FPROPS.
 *
 * The basic problem solved is:
 *
 *   minimize G(T, P, n)
 *
 * over species mole amounts n, subject to:
 *
 *   A n = b    (element balances)
 *   n >= 0
 *
 * The public flat-species solvers accept species names and element totals;
 * FPROPS builds the element matrix from species formula data internally.
 *
 * Thermodynamic data are selected with the `source` string, which may be:
 *
 * - a plain source name, eg "Moran and Shapiro", "hidayat_2015"
 * - an explicit model selector, eg "ideal:Moran and Shapiro",
 *   "helmholtz+ref0:", "shomate:reaktoro_clone_supcrt98"
 * - a per-species source map, eg
 *     "Ni=oecd_nea_tdb_vol6_nickel;NiO=oecd_nea_tdb_vol6_nickel;*=Moran and Shapiro"
 */

typedef struct FpropsRxnPackage_struct FpropsRxnPackage;

typedef struct FpropsRxnTPN{
	double T;
	double P;
	const double *n;
} FpropsRxnTPN;

typedef struct FpropsRxnResult{
	int status;
	double H;
	double G;
	double *n_out;
} FpropsRxnResult;

typedef enum FpropsEqmNlpSolver{
	FPROPS_EQM_NLP_DEFAULT = 0,
	FPROPS_EQM_NLP_SLSQP,
	FPROPS_EQM_NLP_IPOPT,
	FPROPS_EQM_NLP_IPOPT_SCALED_N,
	FPROPS_EQM_NLP_IPOPT_LOGN,
	FPROPS_EQM_NLP_IPOPT_N
} FpropsEqmNlpSolver;

/**
 * Return a short human-readable description for an equilibrium status code.
 *
 * The codes are intentionally kept numeric for ABI compatibility with IPOPT
 * and the existing FPROPS wrappers, but examples and diagnostics should use
 * this function rather than carrying local status decoders.
 */
const char *fprops_eqm_status_text(int status);

/**
 * Return non-zero if an equilibrium status code represents a usable result.
 *
 * This keeps examples and callers from encoding solver-specific accepted
 * positive statuses such as IPOPT's "acceptable level" or "feasible point".
 */
int fprops_eqm_status_ok(int status);

/**
 * Return the algorithm-selector string corresponding to an NLP solver
 * preference. `FPROPS_EQM_NLP_DEFAULT` maps to `"auto"`; the current default
 * full-space NLP backend is SLSQP when NLOPT is available.
 */
const char *fprops_eqm_nlp_solver_name(FpropsEqmNlpSolver solver);

/**
 * Parse an NLP solver preference name.
 *
 * Accepted names include `default`, `auto`, `slsqp`, `ipopt`,
 * `ipopt_scaled_n`, `ipopt_logn`, and `ipopt_n`. On success, stores the enum
 * value in `solver_out` and returns non-zero.
 */
int fprops_eqm_nlp_solver_from_name(const char *name, FpropsEqmNlpSolver *solver_out);

/**
 * Build a compiled reactive-package runtime object from a species basis
 * and source specification.
 *
 * The returned package caches species/element metadata and prepared
 * thermo handles suitable for repeated evaluation from ASCEND blackboxes
 * or other C callers.
 *
 * @param names Species names, length ns.
 * @param ns Number of species.
 * @param source Thermodynamic source specification or source map.
 * @return Newly allocated package on success, or NULL on failure.
 */
FpropsRxnPackage *fprops_rxn_package_build(const char **names, int ns, const char *source);

/**
 * Destroy a package created by fprops_rxn_package_build(...).
 */
void fprops_rxn_package_free(FpropsRxnPackage *pkg);

/**
 * Return the number of species in a compiled reactive package.
 */
int fprops_rxn_package_num_species(const FpropsRxnPackage *pkg);

/**
 * Return the number of conserved elements in a compiled reactive package.
 */
int fprops_rxn_package_num_elements(const FpropsRxnPackage *pkg);

/**
 * Return the package element matrix A in row-major order with shape [ne x ns].
 *
 * The returned pointer is owned by the package and remains valid until the
 * package is freed.
 */
const double *fprops_rxn_package_element_matrix(const FpropsRxnPackage *pkg);

/**
 * Solve equilibrium using a compiled reactive package.
 *
 * This is the package-oriented counterpart of fprops_eqm_tpb(...).
 *
 * @param pkg Compiled reactive package.
 * @param state Input T/P state.
 * @param b Element totals, length equal to the package element count.
 * @param algorithm Algorithm selector string.
 * @param n_init Optional initial guess, length equal to package species count.
 * @param out Output/result structure. `out->n_out` must point to a caller-owned
 *        array of length equal to package species count.
 * @return Solver status code.
 */
int fprops_rxn_eqm_tpb(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state,
		const double *b, const char *algorithm, const double *n_init, FpropsRxnResult *out);

/**
 * Solve equilibrium using a compiled reactive package and an inlet
 * species-amount vector.
 *
 * This is the package-oriented counterpart of fprops_eqm_tpy(...): it
 * infers conserved element totals from `state->n` using the package's
 * cached element matrix, then calls fprops_rxn_eqm_tpb(...).
 *
 * @param pkg Compiled reactive package.
 * @param state Input T/P/species-amount state.
 * @param algorithm Algorithm selector string.
 * @param n_init Optional initial guess, length equal to package species count.
 * @param out Output/result structure. `out->n_out` must point to a caller-owned
 *        array of length equal to package species count.
 * @return Solver status code.
 */
int fprops_rxn_eqm_tpy(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state,
		const char *algorithm, const double *n_init, FpropsRxnResult *out);

/**
 * Compute first sensitivities of an already-solved package equilibrium state.
 *
 * This routine currently targets gas-only package equilibria with no
 * solution-phase members. It linearizes the interior equilibrium KKT
 * system at the supplied equilibrium composition and returns local
 * sensitivities with respect to temperature, pressure, and conserved
 * element totals.
 *
 * @param pkg Compiled reactive package.
 * @param state Input T/P/species-amount state whose conserved element totals
 *        define the equilibrium problem.
 * @param n_eq Accepted equilibrium species amounts, length equal to package
 *        species count.
 * @param dn_dT Optional output vector, length equal to package species count.
 * @param dn_dP Optional output vector, length equal to package species count.
 * @param dn_db Optional output matrix, row-major with shape [ns x ne], where
 *        `dn_db[i * ne + e] = d n_eq[i] / d b[e]`.
 * @return 0 on success, negative code on failure or unsupported phase model.
 */
int fprops_rxn_eqm_sensitivities(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state,
		const double *n_eq, double *dn_dT, double *dn_dP, double *dn_db);

/**
 * Compute total mixture enthalpy using a compiled reactive package.
 *
 * @param pkg Compiled reactive package.
 * @param state Input T/P/species-amount state.
 * @param H_out Output total enthalpy in J on the same basis as `state->n`.
 * @return 0 on success, negative code on failure.
 */
int fprops_rxn_mix_h(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state, double *H_out);

/**
 * Compute total mixture volume using a compiled reactive package.
 *
 * This is currently a best-effort extensive property path intended for
 * packages whose member species have meaningful per-species volume data
 * at the requested `T`, `P`. It is suitable for pure-fluid and selected
 * condensed-species packages, but may return a negative status for
 * unsupported solution/species models.
 *
 * @param pkg Compiled reactive package.
 * @param state Input T/P/species-amount state.
 * @param V_out Output total volume in m^3 on the same extensive basis as
 *        `state->n`.
 * @return 0 on success, negative code on failure.
 */
int fprops_rxn_mix_v(const FpropsRxnPackage *pkg, const FpropsRxnTPN *state, double *V_out);

/**
 * Evaluate the standard chemical potential mu0(T, P0) for one species.
 *
 * This routine is source-aware and understands:
 *
 * - plain source names
 * - explicit model selectors
 * - per-species source maps
 *
 * Examples:
 *
 * - "Moran and Shapiro"
 * - "ideal:Moran and Shapiro"
 * - "helmholtz+ref0:"
 * - "Fe_bcc=hidayat_2015;hydrogen=helmholtz+ref0:;water=helmholtz+ref0:"
 *
 * @param name Species name.
 * @param source Thermodynamic source specification or source map.
 * @param T Temperature in K.
 * @param P0 Standard/reference pressure in Pa.
 * @param mu0 Output molar standard chemical potential in J/mol.
 * @return Nonzero on success, zero on failure.
 */
int eqm_mu0_source(const char *name, const char *source, double T, double P0,
		double *mu0);

/**
 * Solve a chemical-equilibrium problem at specified T, P, and element
 * totals, and optionally evaluate the total equilibrium enthalpy of the
 * returned state.
 *
 * It keeps the natural equilibrium primitive in terms of T, P, and b while
 * hiding the internal element-matrix setup.
 *
 * @param names Species names, length ns.
 * @param ns Number of species.
 * @param elements Element symbols/names, length ne.
 * @param ne Number of conserved elements.
 * @param b Element totals, length ne.
 * @param source Thermodynamic source specification or source map.
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param algorithm Algorithm selector string.
 * @param n_init Optional initial guess for species mole amounts, length ns,
 *        or NULL.
 * @param n_out Output equilibrium species mole amounts, length ns.
 * @param H_out Optional output total equilibrium enthalpy in J for the
 *        returned species amounts, or NULL to skip enthalpy evaluation.
 * @return Solver status code. Zero is the normal success code; some
 *         positive codes may also indicate successful convergence.
 *         If equilibrium converges but enthalpy evaluation is unsupported
 *         for the requested species/models, a negative code is returned.
 */
int fprops_eqm_tpb(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out, double *H_out);

/**
 * Solve a chemical-equilibrium problem from inlet mole fractions.
 *
 * This convenience wrapper infers the element set from species data,
 * computes elemental totals `b` from `y_in`, and then
 * calls fprops_eqm_tpb(...).
 *
 * The inlet fractions are interpreted on a 1-mol feed basis after
 * normalization, so `sum(y_in)` does not need to be exactly 1.
 *
 * @param names Species names, length ns.
 * @param ns Number of species.
 * @param y_in Inlet mole fractions (or proportional nonnegative values),
 *        length ns.
 * @param source Thermodynamic source specification or source map.
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param algorithm Algorithm selector string.
 * @param n_init Optional initial guess for species mole amounts, length ns,
 *        or NULL.
 * @param n_out Output equilibrium species mole amounts on the same 1-mol
 *        feed element basis, length ns.
 * @return Solver status code, matching fprops_eqm_tpb(...).
 */
int fprops_eqm_tpy(const char **names, int ns, const double *y_in, const char *source,
		double T, double P, const char *algorithm, const double *n_init, double *n_out);

/**
 * Compute total stream enthalpy for a specified species-amount state.
 *
 * This routine evaluates:
 *   H = sum_i n_i * h_i(T, P, source)
 * for the provided species list and mole amounts.
 *
 * The function does not perform reaction or phase equilibrium.
 *
 * @param names Species names, length ns.
 * @param ns Number of species.
 * @param n Species mole amounts (or molar flowrates on any consistent basis),
 *        length ns.
 * @param source Thermodynamic source specification or source map.
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param H_out Output total enthalpy in J on the same basis as `n`.
 * @return 0 on success, negative code on failure.
 *         -11 invalid input.
 *         -14 unsupported/missing enthalpy model for one or more species.
 *         -15 solution-phase species currently unsupported in this API.
 */
int fprops_mix_h_tpn(const char **names, int ns, const double *n, const char *source,
		double T, double P, double *H_out);

#endif

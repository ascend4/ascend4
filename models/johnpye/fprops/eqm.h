#ifndef FPROPS_EQM_H
#define FPROPS_EQM_H

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
 * Two usage styles are supported:
 *
 * 1. Provide the element matrix A explicitly with eqm_solve(...).
 * 2. Provide species names and element names, and let FPROPS build A
 *    from the species formulae with eqm_solve_elements(...).
 *
 * Thermodynamic data are selected with the `source` string, which may be:
 *
 * - a plain source name, eg "Moran and Shapiro", "hidayat_2015"
 * - an explicit model selector, eg "ideal:Moran and Shapiro",
 *   "helmholtz+ref0:", "shomate:reaktoro_clone_supcrt98"
 * - a per-species source map, eg
 *     "Ni=oecd_nea_tdb_vol6_nickel;NiO=oecd_nea_tdb_vol6_nickel;*=Moran and Shapiro"
 */

typedef enum EqmAlgorithm{
	EQM_ALG_AUTO = 0,
	EQM_ALG_IPOPT,
	EQM_ALG_IPOPT_N,
	EQM_ALG_IPOPT_LOGN,
	EQM_ALG_SLSQP
} EqmAlgorithm;

/**
 * Solve a chemical-equilibrium problem with an explicitly supplied
 * element matrix.
 *
 * @param names Species names, length ns.
 * @param ns Number of species.
 * @param ne Number of conserved elements.
 * @param A Element matrix of size ne x ns, row-major.
 * @param b Element totals, length ne.
 * @param source Thermodynamic source specification or source map.
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param algorithm Algorithm selector string, eg "auto", "ipopt",
 *        "ipopt_n", "ipopt_logn", "slsqp", "reduced", "nullspace".
 * @param n_init Optional initial guess for species mole amounts, length ns,
 *        or NULL.
 * @param n_out Output equilibrium species mole amounts, length ns.
 * @return Solver status code. Zero is the normal success code; some
 *         positive codes are also accepted by the test suite as converged.
 */
int eqm_solve(const char **names, int ns, int ne, const double *A, const double *b,
		const char *source, double T, double P, const char *algorithm, const double *n_init,
		double *n_out);

/* `algorithm` keywords for eqm_solve_elements:
   auto (default), ipopt*, slsqp, reduced, auto_reduced,
   auto_nullspace, auto_no_nullspace, nullspace
   Note: nullspace prepass is used for auto and ipopt* unless disabled. */
/**
 * Solve a chemical-equilibrium problem from species and element names.
 *
 * FPROPS constructs the element matrix internally from the species
 * formula data and then solves the Gibbs minimization problem.
 *
 * This is the usual entry point for end users.
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
 * @return Solver status code.
 */
int eqm_solve_elements(const char **names, int ns, const char **elements, int ne,
		const double *b, const char *source, double T, double P, const char *algorithm,
		const double *n_init, double *n_out);

/**
 * Evaluate the standard chemical potential mu0(T, P0) for one species
 * using the ideal-gas route.
 *
 * This is a narrower convenience entry point. In most user-facing code,
 * eqm_mu0_source(...) is preferred because it supports the full model
 * selector syntax.
 *
 * @param name Species name.
 * @param source Thermodynamic source specification.
 * @param T Temperature in K.
 * @param P0 Standard/reference pressure in Pa.
 * @param mu0 Output molar standard chemical potential in J/mol.
 * @return Nonzero on success, zero on failure.
 */
int eqm_mu0_ideal_source(const char *name, const char *source, double T, double P0,
		double *mu0);
/* `source` supports explicit model prefixes:
   auto:<source>, ideal:<source>, constcp:<source>, shomate:<source>,
   helmholtz:<source>, pengrob:<source>.
   This also works per species when used inside source maps, eg
   "Ni=shomate:oecd_nea_tdb_vol6_nickel;water=ideal:Moran and Shapiro". */
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

#endif

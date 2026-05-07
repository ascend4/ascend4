#ifndef FPROPS_EQM_PHASE_H
#define FPROPS_EQM_PHASE_H

#include <stdio.h>
#include "eqm.h"

/** Maximum number of phases currently supported in one phase package. */
#define FPROPS_EQM_PHASE_MAX_PHASES 20
/** Maximum number of expanded members/components reported for one phase. */
#define FPROPS_EQM_PHASE_MAX_MEMBERS 8
/** Maximum number of conserved elements currently supported in one package. */
#define FPROPS_EQM_PHASE_MAX_ELEMS 8
/** Maximum number of internal composition coordinates for one phase. */
#define FPROPS_EQM_PHASE_MAX_VARS 8

/**
 * Broad category of a phase model.
 *
 * This is mostly diagnostic metadata for callers. The concrete phase model
 * also carries name/source strings, members, elements, coordinate names, and
 * composition bounds.
 */
typedef enum FpropsEqmPhaseKind{
	FPROPS_EQM_PHASE_STOICHIOMETRIC = 0,
	FPROPS_EQM_PHASE_IDEAL_GAS,
	FPROPS_EQM_PHASE_BINARY_SOLUTION,
	FPROPS_EQM_PHASE_SITE_SOLUTION,
	FPROPS_EQM_PHASE_GENERIC
} FpropsEqmPhaseKind;

/**
 * Resolved phase-model descriptor.
 *
 * The record is caller-owned when returned by fprops_eqm_phase_resolve(...),
 * and owned by FpropsEqm when stored in a problem object. String pointers
 * refer to storage inside the same struct, so callers may copy the struct by
 * value but must not free individual strings.
 *
 * Public fields:
 * - `kind`: phase category.
 * - `name`: canonical phase name, eg "wustite" or "gas:ideal".
 * - `source`: thermodynamic data source/selector.
 * - `basis`: text description of the natural phase unit used for `g` and
 *   element contents.
 * - `nmember`: number of expanded members/components in `members`.
 * - `nelem`: number of conserved elements in `elements`.
 * - `nvar`: number of internal composition coordinates in `var_names`.
 * - `members`: member/component names, length `nmember`.
 * - `elements`: element names, length `nelem`.
 * - `var_names`: coordinate names, length `nvar`.
 * - `lower`, `upper`: coordinate bounds, length `nvar`.
 * - `data`: implementation-private model data; do not dereference in public
 *   callers.
 */
typedef struct FpropsEqmPhaseModel{
	FpropsEqmPhaseKind kind;
	const char *name;
	const char *source;
	const char *basis;
	int nmember;
	int nelem;
	int nvar;
	const char *members[FPROPS_EQM_PHASE_MAX_MEMBERS];
	const char *elements[FPROPS_EQM_PHASE_MAX_ELEMS];
	const char *var_names[FPROPS_EQM_PHASE_MAX_VARS];
	double lower[FPROPS_EQM_PHASE_MAX_VARS];
	double upper[FPROPS_EQM_PHASE_MAX_VARS];
	const void *data;
	char name_storage[128];
	char source_storage[256];
	char basis_storage[64];
	char member_storage[FPROPS_EQM_PHASE_MAX_MEMBERS][64];
	char element_storage[FPROPS_EQM_PHASE_MAX_ELEMS][16];
	char var_storage[FPROPS_EQM_PHASE_MAX_VARS][64];
} FpropsEqmPhaseModel;

typedef struct FpropsEqmProblem FpropsEqmProblem;
/**
 * Phase-equilibrium problem object.
 *
 * Initialize with fprops_eqm_init(...), then add phases and feed material.
 * The current API uses caller-owned storage, so stack allocation is valid.
 */
typedef FpropsEqmProblem FpropsEqm;

/**
 * Result from a phase-aware equilibrium solve.
 *
 * Public fields are provided for tests and simple callers; prefer accessor
 * functions for normal use because they handle phase/member name lookup and
 * future layout changes.
 *
 * Units:
 * - `phase_amounts`: mol or formula-unit amount on each phase model's natural
 *   basis, indexed by phase package order.
 * - `phase_y`: dimensionless internal coordinates, laid out as
 *   `phase_y[iphase * FPROPS_EQM_PHASE_MAX_VARS + ivar]`.
 * - `member_amounts`: expanded member/component amounts on the phase model's
 *   member basis, laid out as
 *   `member_amounts[iphase * FPROPS_EQM_PHASE_MAX_MEMBERS + imember]`.
 *
 * Status:
 * - `status == 0` means the result fields are usable.
 * - `solver_status` keeps the raw solver detail for diagnostics.
 * - `eqm` points back to the source problem and is used by name-based
 *   accessors. Keep the source FpropsEqm alive while using the result.
 */
typedef struct FpropsEqmPhaseResult{
	const FpropsEqm *eqm;
	/* Public result status: zero means the composition fields are usable. */
	int status;
	/* Raw solver status retained for diagnostics and solver-specific reports. */
	int solver_status;
	int nphase;
	int nmember;
	double phase_amounts[FPROPS_EQM_PHASE_MAX_PHASES];
	double phase_y[FPROPS_EQM_PHASE_MAX_PHASES * FPROPS_EQM_PHASE_MAX_VARS];
	int phase_active[FPROPS_EQM_PHASE_MAX_PHASES];
	double member_amounts[FPROPS_EQM_PHASE_MAX_PHASES * FPROPS_EQM_PHASE_MAX_MEMBERS];
} FpropsEqmPhaseResult;

/**
 * Concrete storage for a phase-equilibrium problem.
 *
 * Callers may inspect fields for debugging, but should use API functions for
 * setup. Element totals `b` are in mol of conserved elements on the feed basis.
 * `T` is in K and `P` is in Pa.
 */
struct FpropsEqmProblem{
	int nphase;
	int nelem;
	FpropsEqmPhaseModel phases[FPROPS_EQM_PHASE_MAX_PHASES];
	const char *elements[FPROPS_EQM_PHASE_MAX_ELEMS];
	char element_storage[FPROPS_EQM_PHASE_MAX_ELEMS][16];
	double b[FPROPS_EQM_PHASE_MAX_ELEMS];
	double T;
	double P;
	char algorithm[32];
	FpropsEqmNlpSolver nlp_solver;
};

/**
 * Name/amount pair used by fprops_eqm_add_comp_items(...).
 *
 * `name` may be an element name, species/formula, or known phase member name.
 * `amount` is a molar amount on the natural basis of that name.
 */
typedef struct FpropsEqmComp{
	const char *name;
	double amount;
} FpropsEqmComp;

/**
 * Coordinate name/value pair used by fprops_eqm_add_phase_feed_var_items(...).
 *
 * Coordinate values are dimensionless composition/site variables in the phase
 * model's declared coordinate system.
 */
typedef struct FpropsEqmCoord{
	const char *name;
	double value;
} FpropsEqmCoord;

#define FPROPS_EQM_CAT_(A, B) A##B
#define FPROPS_EQM_CAT(A, B) FPROPS_EQM_CAT_(A, B)
#define FPROPS_EQM_NARGS_( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define FPROPS_EQM_NARGS(...) \
	FPROPS_EQM_NARGS_(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define FPROPS_EQM_REQUIRE_PAIRS(N) \
	((void)sizeof(char[((N) % 2 == 0) ? 1 : -1]))
#define FPROPS_EQM_PAIR_ITEMS_1(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_2(N1, A1) {N1, (double)(A1)}
#define FPROPS_EQM_PAIR_ITEMS_3(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_4(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_2(__VA_ARGS__)
#define FPROPS_EQM_PAIR_ITEMS_5(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_6(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_4(__VA_ARGS__)
#define FPROPS_EQM_PAIR_ITEMS_7(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_8(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_6(__VA_ARGS__)
#define FPROPS_EQM_PAIR_ITEMS_9(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_10(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_8(__VA_ARGS__)
#define FPROPS_EQM_PAIR_ITEMS_11(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_12(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_10(__VA_ARGS__)
#define FPROPS_EQM_PAIR_ITEMS_13(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_14(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_12(__VA_ARGS__)
#define FPROPS_EQM_PAIR_ITEMS_15(...) {NULL, 0.0}
#define FPROPS_EQM_PAIR_ITEMS_16(N1, A1, ...) {N1, (double)(A1)}, FPROPS_EQM_PAIR_ITEMS_14(__VA_ARGS__)
/**
 * Add component/formula amounts to the feed using name/amount pairs.
 *
 * Example: `fprops_eqm_add_comps(&eqm, "Fe2O3", 1.0, "H2", 100.0)`.
 * Names may be elements, formulae, species, or known phase members. Amounts
 * are molar amounts on each name's natural basis.
 */
#define fprops_eqm_add_comps(EQM, ...) \
	(FPROPS_EQM_REQUIRE_PAIRS(FPROPS_EQM_NARGS(__VA_ARGS__)), \
	fprops_eqm_add_comp_items((EQM), FPROPS_EQM_NARGS(__VA_ARGS__) / 2, \
		(const FpropsEqmComp[]){ \
			FPROPS_EQM_CAT(FPROPS_EQM_PAIR_ITEMS_, FPROPS_EQM_NARGS(__VA_ARGS__))(__VA_ARGS__) \
		}))
/**
 * Add phase specifications to a problem package.
 *
 * Example: `fprops_eqm_add_phases(&eqm, "wustite=hidayat_2015",
 * "gas:ideal(hydrogen,water)=helmholtz+ref0:")`.
 */
#define fprops_eqm_add_phases(EQM, ...) \
	fprops_eqm_add_phase_list((EQM), FPROPS_EQM_NARGS(__VA_ARGS__), \
		(const char *[]){__VA_ARGS__})

/**
 * Add a phase amount to the feed using coordinates in declared order.
 *
 * Example: `fprops_eqm_add_phase_feed(&eqm, "wustite", 1.0, x_FeO1p5)`.
 */
#define fprops_eqm_add_phase_feed(EQM, PHASE, AMOUNT, ...) \
	fprops_eqm_add_phase_feed_values((EQM), (PHASE), (AMOUNT), \
		(const double[]){__VA_ARGS__}, FPROPS_EQM_NARGS(__VA_ARGS__))

/**
 * Add a phase amount to the feed using coordinate name/value pairs.
 *
 * Example: `fprops_eqm_add_phase_feed_vars(&eqm, "spinel", 1.0,
 * "y_tet_fe2", 0.40, "y_oct_fe2", 0.20)`.
 */
#define fprops_eqm_add_phase_feed_vars(EQM, PHASE, AMOUNT, ...) \
	(FPROPS_EQM_REQUIRE_PAIRS(FPROPS_EQM_NARGS(__VA_ARGS__)), \
	fprops_eqm_add_phase_feed_var_items((EQM), (PHASE), (AMOUNT), \
		FPROPS_EQM_NARGS(__VA_ARGS__) / 2, \
		(const FpropsEqmCoord[]){ \
			FPROPS_EQM_CAT(FPROPS_EQM_PAIR_ITEMS_, FPROPS_EQM_NARGS(__VA_ARGS__))(__VA_ARGS__) \
		}))

/*
 * Phase-aware equilibrium API.
 *
 * Use the flat species API in eqm.h when every species can be treated as an
 * independent amount in a single Gibbs minimization. Use this phase-aware API
 * when whole phases must enter or leave the assemblage and when solution
 * phases such as wustite or spinel need internal composition variables.
 *
 * Phase specifications follow the existing resolver syntax, for example:
 *
 *   Fe_bcc=hidayat_2015
 *   wustite=hidayat_2015
 *   spinel=degterov_2001
 *   gas:ideal(hydrogen,water)=helmholtz+ref0:
 *
 * A separate source argument may also be supplied to
 * fprops_eqm_phase_resolve(...); an inline `=source` in the spec takes the
 * same path as the tests and examples.
 */

/**
 * Return a short name for a phase kind enum.
 *
 * @param kind Phase kind value.
 * @return Static string; never free.
 */
const char *fprops_eqm_phase_kind_name(FpropsEqmPhaseKind kind);

/**
 * Return the index of an element in a phase model, or -1 if absent.
 *
 * @param phase Input phase model.
 * @param name Element symbol/name to find.
 * @return Zero-based element index, or -1 if not present.
 */
int fprops_eqm_phase_find_element(const FpropsEqmPhaseModel *phase, const char *name);

/**
 * Resolve one phase specification into a caller-owned phase model record.
 *
 * @param spec Phase/species string, optionally with an inline `=source`.
 * @param source Optional source string used when `spec` has no inline source.
 * @param phase Output phase record.
 * @return Non-zero on success, zero on failure.
 */
int fprops_eqm_phase_resolve(const char *spec, const char *source,
		FpropsEqmPhaseModel *phase);

/**
 * Evaluate element contents of one natural phase unit.
 *
 * @param phase Input phase model.
 * @param y Input coordinate vector, length `phase->nvar`, or NULL for phases
 *        with no coordinates.
 * @param a_out Output element contents, length `phase->nelem`, ordered like
 *        `phase->elements`. Units are mol element per natural phase unit.
 * @return Equilibrium status code; use fprops_eqm_status_ok(...) to test
 *         usability.
 */
int fprops_eqm_phase_elements(const FpropsEqmPhaseModel *phase,
		const double *y, double *a_out);

/**
 * Evaluate Gibbs energy of one natural phase unit.
 *
 * @param phase Input phase model.
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param y Input coordinate vector, length `phase->nvar`, or NULL for phases
 *        with no coordinates.
 * @param g_out Output Gibbs energy in J per natural phase unit.
 * @return Equilibrium status code; use fprops_eqm_status_ok(...) to test
 *         usability.
 */
int fprops_eqm_phase_gibbs(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *y, double *g_out);

/**
 * Evaluate the phase-entry residual at supplied element potentials.
 *
 * `lambda` is ordered like `phase->elements`. Each entry is the chemical
 * potential contribution of one mole of that element, so the residual is
 * minimized over phase composition as:
 *
 *   phi = min_y(g_phase(T,P,y) - sum_i lambda_i a_i(y)) / RT
 *
 * A phase on its entry boundary has phi ~= 0. A negative phi means the phase
 * can lower the total Gibbs energy and should be considered for activation.
 *
 * @param phase Input phase model.
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param lambda Input element potentials in J/mol element, length
 *        `phase->nelem`, ordered like `phase->elements`.
 * @param phi_out Output dimensionless entry residual.
 * @param y_out Optional output minimizing coordinate vector, length
 *        `phase->nvar`; may be NULL.
 * @return Equilibrium status code; use fprops_eqm_status_ok(...) to test
 *         usability.
 */
int fprops_eqm_phase_entry_residual(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out);

/**
 * Initialize a caller-owned phase-equilibrium problem object.
 *
 * @param eqm Output problem object to initialize.
 */
void fprops_eqm_init(FpropsEqm *eqm);

/**
 * Add one phase to a problem package.
 *
 * @param eqm Input/output problem object.
 * @param spec Phase specification string, optionally including `=source`.
 * @param source Optional source string used when `spec` has no inline source.
 * @return 0 on success, negative status on failure.
 */
int fprops_eqm_add_phase(FpropsEqm *eqm, const char *spec, const char *source);

/**
 * Add a counted list of phase specifications to a problem package.
 *
 * The convenience macro fprops_eqm_add_phases(&eqm, ...) builds this counted
 * list automatically for C callers.
 *
 * @param eqm Input/output problem object.
 * @param nphase Number of entries in `specs`.
 * @param specs Input array of phase specification strings.
 * @return 0 on success, negative status on failure.
 */
int fprops_eqm_add_phase_list(FpropsEqm *eqm, int nphase, const char **specs);

/**
 * Return the number of phases currently in the package.
 *
 * @param eqm Input problem object.
 * @return Phase count, or negative status on invalid input.
 */
int fprops_eqm_phase_count(const FpropsEqm *eqm);

/**
 * Return the canonical phase name at package index `iphase`.
 *
 * @return Static pointer owned by `eqm`, or NULL if `iphase` is invalid.
 */
const char *fprops_eqm_phase_name(const FpropsEqm *eqm, int iphase);

/**
 * Find a phase by canonical name.
 *
 * @return Zero-based phase index, or -1 if absent.
 */
int fprops_eqm_find_phase(const FpropsEqm *eqm, const char *phase);

/**
 * Return the resolved model for a named phase in a problem package.
 *
 * @return Pointer owned by `eqm`, or NULL if absent.
 */
const FpropsEqmPhaseModel *fprops_eqm_phase_model(const FpropsEqm *eqm,
		const char *phase);

/**
 * Return the number of internal coordinates for a named phase.
 *
 * @param eqm Input problem object.
 * @param phase Phase name.
 * @return Coordinate count, or -1 if the phase is unknown.
 */
int fprops_eqm_phase_coord_count(const FpropsEqm *eqm, const char *phase);

/**
 * Return the coordinate name at index `icoord` for a named phase.
 *
 * @return Pointer owned by `eqm`, or NULL if the phase/index is invalid.
 */
const char *fprops_eqm_phase_coord_name(const FpropsEqm *eqm, const char *phase,
		int icoord);

/**
 * Fill a caller-supplied array with coordinate names for a phase.
 *
 * @param names Output array of at least fprops_eqm_phase_coord_count(...)
 *        pointers. The strings remain owned by `eqm`.
 * @return Number of names written, or negative status on failure.
 */
int fprops_eqm_phase_coord_names(const FpropsEqm *eqm, const char *phase,
		const char **names);

/**
 * Return the number of expanded members/components for a named phase.
 *
 * @param eqm Input problem object.
 * @param phase Phase name.
 * @return Member count, or -1 if the phase is unknown.
 */
int fprops_eqm_phase_member_count(const FpropsEqm *eqm, const char *phase);

/**
 * Return the expanded member/component name at index `imember`.
 *
 * @return Pointer owned by `eqm`, or NULL if the phase/index is invalid.
 */
const char *fprops_eqm_phase_member_name(const FpropsEqm *eqm, const char *phase,
		int imember);

/**
 * Fill a caller-supplied array with member/component names for a phase.
 *
 * @param names Output array of at least fprops_eqm_phase_member_count(...)
 *        pointers. The strings remain owned by `eqm`.
 * @return Number of names written, or negative status on failure.
 */
int fprops_eqm_phase_member_names(const FpropsEqm *eqm, const char *phase,
		const char **names);

/**
 * Set the problem temperature and pressure.
 *
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @return 0 on success, negative status on invalid input.
 */
int fprops_eqm_set_TP(FpropsEqm *eqm, double T, double P);

/**
 * Set the phase-selection algorithm by name.
 *
 * This selector controls the outer phase strategy. It is separate from the NLP
 * backend selector. Public callers normally leave this as `"auto"`.
 *
 * @param algorithm Selector string such as "auto".
 * @return 0 on success, negative status on invalid input.
 */
int fprops_eqm_set_algorithm(FpropsEqm *eqm, const char *algorithm);

/**
 * Set the preferred NLP backend.
 *
 * @param solver Enum value such as FPROPS_EQM_NLP_DEFAULT,
 *        FPROPS_EQM_NLP_SLSQP, or FPROPS_EQM_NLP_IPOPT.
 * @return 0 on success, negative status on invalid input.
 */
int fprops_eqm_set_nlp_solver(FpropsEqm *eqm, FpropsEqmNlpSolver solver);

/**
 * Set the preferred NLP backend by name.
 *
 * Accepted names are parsed by fprops_eqm_nlp_solver_from_name(...). A solve
 * uses the selected backend only; it does not silently fall back to another
 * backend.
 *
 * @return 0 on success, negative status on invalid input.
 */
int fprops_eqm_set_nlp_solver_name(FpropsEqm *eqm, const char *solver);

/**
 * Return the currently selected NLP backend enum.
 *
 * @param eqm Input problem object.
 * @return NLP solver enum value.
 */
FpropsEqmNlpSolver fprops_eqm_nlp_solver(const FpropsEqm *eqm);

/**
 * Clear all stored feed element totals from a problem.
 *
 * @param eqm Input/output problem object.
 */
void fprops_eqm_clear_feed(FpropsEqm *eqm);

/**
 * Add an amount of one conserved element to the feed.
 *
 * @param amount Element amount in mol on the feed basis.
 * @return 0 on success, negative status on invalid/unknown element.
 */
int fprops_eqm_add_element(FpropsEqm *eqm, const char *element, double amount);

/**
 * Set an absolute amount for one conserved element in the feed.
 *
 * @param amount Element amount in mol on the feed basis.
 * @return 0 on success, negative status on invalid/unknown element.
 */
int fprops_eqm_set_element(FpropsEqm *eqm, const char *element, double amount);

/**
 * Return the currently stored feed amount for one element.
 *
 * @return Element amount in mol, or NaN if the element is absent.
 */
double fprops_eqm_element_amount(const FpropsEqm *eqm, const char *element);

/**
 * Add a formula/species amount to the feed by converting it to element totals.
 *
 * @param formula Formula or known species/member name.
 * @param amount Amount in mol on the formula/species basis.
 * @return 0 on success, negative status on parse/resolution failure.
 */
int fprops_eqm_add_formula(FpropsEqm *eqm, const char *formula, double amount);

/**
 * Add a counted list of component/formula amounts to the feed.
 *
 * @param names Input names, length `nitem`.
 * @param amounts Input amounts in mol, length `nitem`.
 * @return 0 on success, negative status on failure.
 */
int fprops_eqm_add_comp_list(FpropsEqm *eqm, int nitem, const char **names,
		const double *amounts);

/**
 * Add a counted list of component/formula amounts to the feed.
 *
 * The convenience macro fprops_eqm_add_comps(&eqm, "Fe2O3", 1, "H2", 100)
 * builds this typed array automatically.
 *
 * @param items Input name/amount pairs, length `nitem`.
 * @return 0 on success, negative status on failure.
 */
int fprops_eqm_add_comp_items(FpropsEqm *eqm, int nitem, const FpropsEqmComp *items);

/**
 * Add a phase amount at coordinates in the phase's declared coordinate order.
 *
 * @param phase Phase name, eg "wustite".
 * @param amount Phase amount in mol/formula units on the phase basis.
 * @param coords Input coordinate values, length `ncoord`.
 * @param ncoord Number of coordinate values; must match the phase coordinate
 *        count.
 * @return 0 on success, negative status on failure.
 */
int fprops_eqm_add_phase_feed_values(FpropsEqm *eqm, const char *phase,
		double amount, const double *coords, int ncoord);

/**
 * Add a phase amount using named coordinate values.
 *
 * Omitted coordinates are treated as zero by the current implementation.
 *
 * @param phase Phase name, eg "spinel".
 * @param amount Phase amount in mol/formula units on the phase basis.
 * @param nitem Number of coordinate name/value pairs.
 * @param items Input coordinate name/value pairs.
 * @return 0 on success, negative status on failure.
 */
int fprops_eqm_add_phase_feed_var_items(FpropsEqm *eqm, const char *phase,
		double amount, int nitem, const FpropsEqmCoord *items);

/**
 * Solve the current phase-equilibrium problem.
 *
 * Uses the problem's stored `T`, `P`, feed totals, phase package, outer
 * algorithm selector, and NLP backend selector.
 *
 * @param result Output result object. The caller owns the storage.
 * @return Public result status; zero means result fields are usable.
 */
int fprops_eqm_solve(const FpropsEqm *eqm, FpropsEqmPhaseResult *result);

/**
 * Set `T` and `P`, then solve the phase-equilibrium problem.
 *
 * @param T Temperature in K.
 * @param P Pressure in Pa.
 * @param result Output result object. The caller owns the storage.
 * @return Public result status; zero means result fields are usable.
 */
int fprops_eqm_solve_TP(FpropsEqm *eqm, double T, double P,
		FpropsEqmPhaseResult *result);

/**
 * Return the solved amount of a named phase.
 *
 * @return Phase amount in mol/formula units, or NaN if the phase is unknown.
 */
double fprops_eqm_phase_amount(const FpropsEqmPhaseResult *result,
		const char *phase);

/**
 * Fill a caller-supplied array with solved coordinate values for a phase.
 *
 * @param values Output array of at least the phase coordinate count.
 * @return Number of values written, or negative status on failure.
 */
int fprops_eqm_phase_coord_values(const FpropsEqmPhaseResult *result,
		const char *phase, double *values);

/**
 * Return one solved coordinate value for a phase.
 *
 * @return Dimensionless coordinate value, or NaN if unknown/inactive.
 */
double fprops_eqm_phase_coord(const FpropsEqmPhaseResult *result,
		const char *phase, const char *coord);

/**
 * Fill a caller-supplied array with expanded member amounts for a phase.
 *
 * @param amounts Output array of at least the phase member count.
 * @return Number of amounts written, or negative status on failure.
 */
int fprops_eqm_phase_member_amounts(const FpropsEqmPhaseResult *result,
		const char *phase, double *amounts);

/**
 * Return the amount of one expanded member within a solved phase.
 *
 * For stoichiometric phases this is the phase amount. For ideal gases and
 * binary solutions, member amounts sum to the phase amount. For site-basis
 * solutions such as spinel, member amounts are on the phase model's site
 * basis; one mole of spinel formula units has one tetrahedral and two
 * octahedral cation sites, so the site member amounts sum to three moles of
 * cation sites, not one mole of spinel formula units.
 *
 * @return Member amount on the phase model's member basis, or NaN if unknown.
 */
double fprops_eqm_phase_member_amount(const FpropsEqmPhaseResult *result,
		const char *phase, const char *member);

/**
 * Return the fraction of one expanded member within a solved phase.
 *
 * For ideal gases and binary solutions this is an ordinary mole fraction. For
 * site-basis phases it is a member amount divided by the phase model's member
 * basis total, not necessarily divided by the phase formula-unit amount.
 *
 * @return Dimensionless fraction, or NaN if unknown.
 */
double fprops_eqm_phase_member_fraction(const FpropsEqmPhaseResult *result,
		const char *phase, const char *member);

/**
 * Write a formatted phase-equilibrium result.
 *
 * @param out Output stream.
 * @param result Input solved result.
 * @param format Format selector. Currently `"text"` is supported; future
 *        values may include machine-readable formats.
 * @return 0 on success, negative status on failure/unsupported format.
 */
int fprops_eqm_write(FILE *out, const FpropsEqmPhaseResult *result,
		const char *format);

#endif /* FPROPS_EQM_PHASE_H */

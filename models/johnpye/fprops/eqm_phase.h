#ifndef FPROPS_EQM_PHASE_H
#define FPROPS_EQM_PHASE_H

#include <stdio.h>
#include "eqm.h"

#define FPROPS_EQM_PHASE_MAX_PHASES 20
#define FPROPS_EQM_PHASE_MAX_MEMBERS 8
#define FPROPS_EQM_PHASE_MAX_ELEMS 8
#define FPROPS_EQM_PHASE_MAX_VARS 8

typedef enum FpropsEqmPhaseKind{
	FPROPS_EQM_PHASE_STOICHIOMETRIC = 0,
	FPROPS_EQM_PHASE_IDEAL_GAS,
	FPROPS_EQM_PHASE_BINARY_SOLUTION,
	FPROPS_EQM_PHASE_SITE_SOLUTION,
	FPROPS_EQM_PHASE_GENERIC
} FpropsEqmPhaseKind;

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
typedef FpropsEqmProblem FpropsEqm;

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

typedef struct FpropsEqmComp{
	const char *name;
	double amount;
} FpropsEqmComp;

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
#define fprops_eqm_add_comps(EQM, ...) \
	(FPROPS_EQM_REQUIRE_PAIRS(FPROPS_EQM_NARGS(__VA_ARGS__)), \
	fprops_eqm_add_comp_items((EQM), FPROPS_EQM_NARGS(__VA_ARGS__) / 2, \
		(const FpropsEqmComp[]){ \
			FPROPS_EQM_CAT(FPROPS_EQM_PAIR_ITEMS_, FPROPS_EQM_NARGS(__VA_ARGS__))(__VA_ARGS__) \
		}))
#define fprops_eqm_add_phases(EQM, ...) \
	fprops_eqm_add_phase_list((EQM), FPROPS_EQM_NARGS(__VA_ARGS__), \
		(const char *[]){__VA_ARGS__})

#define fprops_eqm_add_phase_feed(EQM, PHASE, AMOUNT, ...) \
	fprops_eqm_add_phase_feed_values((EQM), (PHASE), (AMOUNT), \
		(const double[]){__VA_ARGS__}, FPROPS_EQM_NARGS(__VA_ARGS__))

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

const char *fprops_eqm_phase_kind_name(FpropsEqmPhaseKind kind);

/**
 * Return the index of an element in a phase model, or -1 if absent.
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

int fprops_eqm_phase_elements(const FpropsEqmPhaseModel *phase,
		const double *y, double *a_out);

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
 */
int fprops_eqm_phase_entry_residual(const FpropsEqmPhaseModel *phase,
		double T, double P, const double *lambda, double *phi_out, double *y_out);

void fprops_eqm_init(FpropsEqm *eqm);

int fprops_eqm_add_phase(FpropsEqm *eqm, const char *spec, const char *source);

int fprops_eqm_add_phase_list(FpropsEqm *eqm, int nphase, const char **specs);

int fprops_eqm_phase_count(const FpropsEqm *eqm);

const char *fprops_eqm_phase_name(const FpropsEqm *eqm, int iphase);

int fprops_eqm_find_phase(const FpropsEqm *eqm, const char *phase);

const FpropsEqmPhaseModel *fprops_eqm_phase_model(const FpropsEqm *eqm,
		const char *phase);

int fprops_eqm_phase_coord_count(const FpropsEqm *eqm, const char *phase);

const char *fprops_eqm_phase_coord_name(const FpropsEqm *eqm, const char *phase,
		int icoord);

int fprops_eqm_phase_coord_names(const FpropsEqm *eqm, const char *phase,
		const char **names);

int fprops_eqm_phase_member_count(const FpropsEqm *eqm, const char *phase);

const char *fprops_eqm_phase_member_name(const FpropsEqm *eqm, const char *phase,
		int imember);

int fprops_eqm_phase_member_names(const FpropsEqm *eqm, const char *phase,
		const char **names);

int fprops_eqm_set_TP(FpropsEqm *eqm, double T, double P);

int fprops_eqm_set_algorithm(FpropsEqm *eqm, const char *algorithm);

int fprops_eqm_set_nlp_solver(FpropsEqm *eqm, FpropsEqmNlpSolver solver);

int fprops_eqm_set_nlp_solver_name(FpropsEqm *eqm, const char *solver);

FpropsEqmNlpSolver fprops_eqm_nlp_solver(const FpropsEqm *eqm);

void fprops_eqm_clear_feed(FpropsEqm *eqm);

int fprops_eqm_add_element(FpropsEqm *eqm, const char *element, double amount);

int fprops_eqm_set_element(FpropsEqm *eqm, const char *element, double amount);

double fprops_eqm_element_amount(const FpropsEqm *eqm, const char *element);

int fprops_eqm_add_formula(FpropsEqm *eqm, const char *formula, double amount);

int fprops_eqm_add_comp_list(FpropsEqm *eqm, int nitem, const char **names,
		const double *amounts);

int fprops_eqm_add_comp_items(FpropsEqm *eqm, int nitem, const FpropsEqmComp *items);

int fprops_eqm_add_phase_feed_values(FpropsEqm *eqm, const char *phase,
		double amount, const double *coords, int ncoord);

int fprops_eqm_add_phase_feed_var_items(FpropsEqm *eqm, const char *phase,
		double amount, int nitem, const FpropsEqmCoord *items);

int fprops_eqm_solve(const FpropsEqm *eqm, FpropsEqmPhaseResult *result);

int fprops_eqm_solve_TP(FpropsEqm *eqm, double T, double P,
		FpropsEqmPhaseResult *result);

double fprops_eqm_phase_amount(const FpropsEqmPhaseResult *result,
		const char *phase);

int fprops_eqm_phase_coord_values(const FpropsEqmPhaseResult *result,
		const char *phase, double *values);

double fprops_eqm_phase_coord(const FpropsEqmPhaseResult *result,
		const char *phase, const char *coord);

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
 */
double fprops_eqm_phase_member_amount(const FpropsEqmPhaseResult *result,
		const char *phase, const char *member);

double fprops_eqm_phase_member_fraction(const FpropsEqmPhaseResult *result,
		const char *phase, const char *member);

int fprops_eqm_write(FILE *out, const FpropsEqmPhaseResult *result,
		const char *format);

#endif /* FPROPS_EQM_PHASE_H */

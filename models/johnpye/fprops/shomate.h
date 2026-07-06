#ifndef FPROPS_SHOMATE_H
#define FPROPS_SHOMATE_H

#include "rundata.h"

typedef enum {
	SHOMATE_TERM_UNSET = 0,
	SHOMATE_TERM_CONST,
	SHOMATE_TERM_T,
	SHOMATE_TERM_T2,
	SHOMATE_TERM_T3,
	SHOMATE_TERM_INV_SQRT_T,
	SHOMATE_TERM_INV_T,
	SHOMATE_TERM_INV_T2,
	SHOMATE_TERM_LOG_T,
	SHOMATE_TERM_POW
} ShomateTermKind;

typedef struct {
	double coeff;
	double exponent;
	unsigned char kind; /* ShomateTermKind; assigned during data-prepare */
} ShomateTerm;

typedef struct {
	double T_min; /* K */
	double T_max; /* K */
	unsigned n_terms;
	ShomateTerm *terms;
	unsigned prepared;
} ShomateRange;

int shomate_range_contains(const ShomateRange *R, double T);
int shomate_prepare_range(ShomateRange *R);

double shomate_cp_molar(const ShomateRange *R, double T, FpropsError *err);

double shomate_delta_h_molar(const ShomateRange *R, double T0, double T1, FpropsError *err);

double shomate_delta_s_molar(const ShomateRange *R, double T0, double T1, FpropsError *err);

#endif /* FPROPS_SHOMATE_H */

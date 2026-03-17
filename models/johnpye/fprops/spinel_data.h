#ifndef FPROPS_SPINEL_DATA_H
#define FPROPS_SPINEL_DATA_H

#include "rundata.h"

typedef struct FeSpinelPhaseDef_struct{
	const char *name;
	const char *source;
	const char *member_names[5];
	unsigned member_nelem[5];
	const char **member_elements[5];
	const double *member_stoich[5];
	int (*eval)(const double *n_members, double T, double p, double *g_out, double *mu_out);
} FeSpinelPhaseDef;

const FeSpinelPhaseDef *spinel_phase_lookup(const char *name, const char *source);
int spinel_phase_lookup_member(const char *name, const char *source,
		const FeSpinelPhaseDef **phase, unsigned *member_index);
unsigned spinel_phase_member_count(const FeSpinelPhaseDef *phase);
unsigned spinel_phase_member_nelem(const FeSpinelPhaseDef *phase, unsigned member_index);
const char **spinel_phase_member_elements(const FeSpinelPhaseDef *phase, unsigned member_index);
const double *spinel_phase_member_stoich(const FeSpinelPhaseDef *phase, unsigned member_index);
int spinel_phase_eval(const FeSpinelPhaseDef *phase, const double *n_members,
		double T, double p, double *g_out, double *mu_out);

#endif /* FPROPS_SPINEL_DATA_H */

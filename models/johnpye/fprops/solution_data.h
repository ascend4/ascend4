#ifndef FPROPS_SOLUTION_DATA_H
#define FPROPS_SOLUTION_DATA_H

#include "solution.h"

typedef struct {
	const char *name;
	const char *source;
	const char *member_a_name;
	const char *member_b_name;
	unsigned nelem_a;
	const char **elements_a;
	const double *stoich_a;
	unsigned nelem_b;
	const char **elements_b;
	const double *stoich_b;
	const BinarySolutionModel *model;
} BinarySolutionPhaseDef;

const BinarySolutionPhaseDef *solution_phase_lookup(const char *name, const char *source);
int solution_phase_lookup_member(const char *name, const char *source,
		const BinarySolutionPhaseDef **phase, unsigned *member_index);

#endif /* FPROPS_SOLUTION_DATA_H */

#ifndef ASC_RANKI2_H
#define ASC_RANKI2_H

#include "linsolqr_impl.h"

int ranki2_solve(linsolqr_system_t sys, struct rhs_list *rl);
int ranki2_entry(linsolqr_system_t sys, mtx_region_t *region);
void calc_dependent_rows_ranki2(linsolqr_system_t sys);
void calc_dependent_cols_ranki2(linsolqr_system_t sys);

void forward_substitute2(linsolqr_system_t sys,
		real64 *arr,
		boolean transpose
);

void backward_substitute2(linsolqr_system_t sys,
		real64 *arr,
		boolean transpose
);


#endif

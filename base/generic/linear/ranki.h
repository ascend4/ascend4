#ifndef ASC_RANKI_H
#define ASC_RANKI_H

#include "linsolqr.h"

int ranki_solve(linsolqr_system_t sys, struct rhs_list *rl);
int ranki_entry(linsolqr_system_t sys,mtx_region_t *region);

void forward_substitute(linsolqr_system_t sys,
                               real64 *arr,
                               boolean transpose);

void backward_substitute(linsolqr_system_t sys,
                                real64 *arr,
                                boolean transpose);

#endif

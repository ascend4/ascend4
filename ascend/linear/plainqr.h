#ifndef ASC_PLAINQR_H
#define ASC_PLAINQR_H

#include "linsolqr.h"

int cpqr_entry(linsolqr_system_t sys,mtx_region_t *region);
int cpqr_solve(linsolqr_system_t sys,struct rhs_list *rl);

#endif

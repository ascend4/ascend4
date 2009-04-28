#ifndef ASC_RANKIBA2_H
#define ASC_RANKIBA2_H

#include "linsolqr.h"

void rankiba2_factor(linsolqr_system_t sys);
int reset_elimination_data(int32 size, int init);

#endif

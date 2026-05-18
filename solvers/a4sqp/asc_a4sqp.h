/*
 * A4SQP solver for ASCEND.
 *
 * Phase 1 is intentionally limited to solver registration and construction of
 * an inspectable problem view from slv_system_t.
 */

#ifndef ASC_A4SQP_H
#define ASC_A4SQP_H

#include <ascend/solver/solver.h>
#include <ascend/system/slv_client.h>

#define A4SQP_SOLVER_NAME "A4SQP"
#define A4SQP_SOLVER_NUMBER 70

int a4sqp_register(void);

#endif

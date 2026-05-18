/*
 * NLopt SLSQP solver adapter for ASCEND.
 */

#ifndef ASC_SLSQP_H
#define ASC_SLSQP_H

#include <ascend/solver/solver.h>
#include <ascend/system/slv_client.h>

#define SLSQP_SOLVER_NAME "SLSQP"
#define SLSQP_SOLVER_NUMBER 71

int slsqp_register(void);

#endif

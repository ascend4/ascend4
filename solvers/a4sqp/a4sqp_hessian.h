/*
 * Shared Hessian utilities for A4SQP frontends.
 */

#ifndef ASC_A4SQP_HESSIAN_H
#define ASC_A4SQP_HESSIAN_H

#include <ascend/general/platform.h>

real64 a4sqp_hessian_dense_regularize_psd(real64 *hess, int32 n, real64 min_diag);
void a4sqp_hessian_dense_mul(const real64 *hess, int32 n, const real64 *x, real64 *y);

#endif

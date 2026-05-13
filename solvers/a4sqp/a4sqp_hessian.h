/*
 * Shared Hessian utilities for A4SQP frontends.
 */

#ifndef ASC_A4SQP_HESSIAN_H
#define ASC_A4SQP_HESSIAN_H

#include "a4sqp_types.h"

A4SQP_CORE_EXPORT real64 a4sqp_hessian_dense_regularize_psd(real64 *hess, int32 n, real64 min_diag);
A4SQP_CORE_EXPORT void a4sqp_hessian_dense_mul(const real64 *hess, int32 n, const real64 *x, real64 *y);

#endif

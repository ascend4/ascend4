/*
 * A4SQP scaling placeholder.
 */

#ifndef ASC_A4SQP_SCALE_H
#define ASC_A4SQP_SCALE_H

#include "a4sqp_view.h"

A4SQP_CORE_EXPORT const char *a4sqp_scale_mode_name(const char *mode);
A4SQP_CORE_EXPORT int a4sqp_view_apply_scaling(struct A4SqpView *view, const char *mode);

#endif

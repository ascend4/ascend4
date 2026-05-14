/*
 * Shared trust-region policy for A4SQP frontends.
 */

#ifndef ASC_A4SQP_TRUST_H
#define ASC_A4SQP_TRUST_H

#include "a4sqp_types.h"

struct A4SqpTrustOptions {
	real64 radius_init;
	real64 radius_min;
	real64 radius_max;
	real64 shrink;
	real64 grow;
	real64 good_ratio;
};

real64 a4sqp_trust_clamp_value(const struct A4SqpTrustOptions *opt, real64 radius);
A4SQP_CORE_EXPORT real64 a4sqp_trust_initial_radius(const struct A4SqpTrustOptions *opt);
A4SQP_CORE_EXPORT int a4sqp_trust_shrink_radius(real64 *radius, const struct A4SqpTrustOptions *opt);
A4SQP_CORE_EXPORT void a4sqp_trust_grow_if_good(
	real64 *radius,
	const struct A4SqpTrustOptions *opt,
	real64 alpha,
	real64 scaled_step_inf,
	real64 trust_ratio
);
A4SQP_CORE_EXPORT int a4sqp_trust_update_after_accept(
	real64 *radius,
	const struct A4SqpTrustOptions *opt,
	real64 alpha,
	real64 scaled_step_inf,
	real64 trust_ratio,
	real64 tiny_alpha,
	real64 tiny_radius_factor
);

#endif

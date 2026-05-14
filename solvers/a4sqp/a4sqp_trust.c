/*
 * Shared trust-region policy for A4SQP frontends.
 */

#define ASC_BUILDING_INTERFACE

#include "a4sqp_trust.h"

#include <math.h>

real64 a4sqp_trust_clamp_value(const struct A4SqpTrustOptions *opt, real64 radius){
	real64 radius_min;
	real64 radius_max;
	if(opt == NULL){
		return radius;
	}
	radius_min = opt->radius_min;
	radius_max = opt->radius_max;
	if(!isfinite(radius_min) || radius_min <= 0.0){
		radius_min = 1e-12;
	}
	if(!isfinite(radius_max) || radius_max < radius_min){
		radius_max = radius_min;
	}
	if(!isfinite(radius) || radius < radius_min){
		radius = radius_min;
	}
	if(radius > radius_max){
		radius = radius_max;
	}
	return radius;
}

real64 a4sqp_trust_initial_radius(const struct A4SqpTrustOptions *opt){
	if(opt == NULL){
		return 1.0;
	}
	return a4sqp_trust_clamp_value(opt,opt->radius_init);
}

int a4sqp_trust_shrink_radius(real64 *radius, const struct A4SqpTrustOptions *opt){
	real64 shrink;
	real64 radius_min;
	real64 new_radius;
	if(radius == NULL || opt == NULL){
		return 0;
	}
	shrink = opt->shrink;
	radius_min = opt->radius_min;
	if(!isfinite(shrink) || shrink <= 0.0 || shrink >= 1.0){
		shrink = 0.25;
	}
	if(!isfinite(radius_min) || radius_min <= 0.0){
		radius_min = 1e-12;
	}
	new_radius = a4sqp_trust_clamp_value(opt,*radius * shrink);
	if(new_radius >= *radius){
		return 0;
	}
	*radius = new_radius;
	return *radius > radius_min * (1.0 + 1e-12);
}

void a4sqp_trust_grow_if_good(
	real64 *radius,
	const struct A4SqpTrustOptions *opt,
	real64 alpha,
	real64 scaled_step_inf,
	real64 trust_ratio
){
	real64 grow;
	real64 good_ratio;
	if(radius == NULL || opt == NULL){
		return;
	}
	grow = opt->grow;
	good_ratio = opt->good_ratio;
	if(!isfinite(grow) || grow <= 1.0){
		grow = 2.0;
	}
	if(
		alpha >= 0.999
		&& *radius > 0.0
		&& scaled_step_inf >= 0.95 * *radius
		&& trust_ratio >= good_ratio
	){
		*radius = a4sqp_trust_clamp_value(opt,*radius * grow);
	}
}

int a4sqp_trust_update_after_accept(
	real64 *radius,
	const struct A4SqpTrustOptions *opt,
	real64 alpha,
	real64 scaled_step_inf,
	real64 trust_ratio,
	real64 tiny_alpha,
	real64 tiny_radius_factor
){
	real64 old_radius;
	real64 target;
	if(radius == NULL || opt == NULL || !isfinite(*radius) || *radius <= 0.0){
		return 0;
	}
	old_radius = *radius;
	if(
		isfinite(tiny_alpha)
		&& tiny_alpha > 0.0
		&& alpha > 0.0
		&& alpha <= tiny_alpha
		&& isfinite(scaled_step_inf)
		&& scaled_step_inf > 0.0
	){
		if(!isfinite(tiny_radius_factor) || tiny_radius_factor < 1.0){
			tiny_radius_factor = 2.0;
		}
		target = scaled_step_inf * tiny_radius_factor;
		if(target < *radius){
			*radius = a4sqp_trust_clamp_value(opt,target);
			return *radius < old_radius;
		}
	}
	a4sqp_trust_grow_if_good(radius,opt,alpha,scaled_step_inf,trust_ratio);
	return *radius != old_radius;
}

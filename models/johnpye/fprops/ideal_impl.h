#ifndef FPROPS_IDEAL_IMPL_H
#define FPROPS_IDEAL_IMPL_H

#include "ideal.h"

/*
	This file contains the headers for the private code definedin 'ideal.c'.
	You shouldn't include this file in your programs, because the implementation
	of the ideal gas curves is 'secret business' of the fprops code.
*/

double helm_ideal(double tau, double delta, const IdealData *data);
double helm_ideal_tau(double tau, double delta, const IdealData *data);

#endif


/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Implementation of the reduced molar Helmholtz free energy equation of state.

	For nomenclature see Tillner-Roth, Harms-Watzenberg and Baehr, Eine neue
	Fundamentalgleichung für Ammoniak.

	John Pye, 29 Jul 2008.
*/

#include <math.h>

#include "helmholtz.h"

/* forward decls */

static double helm_resid(double tau, double delta, HelmholtzData *data);
static double helm_resid_del(double tau, double delta, HelmholtzData *data);

/**
	Function to calculate pressure from Helmholtz free energy EOS, given temperature
	and molar density.

	@param T temperature in K
	@param rhob molar density in mol/L
	@return pressure in Pa
*/
double helmholtz_p(double T, double rho, HelmholtzData *data){
	
	double tau = data->T_star / T;
	double delta = rho / data->rho_star;

	return data->R * T * rho * (1. + delta * helm_resid_del(tau,delta,data));
}

double helm_ideal(double tau, double delta, HelmholtzData *data){
	double *a0;

	double tau13 = pow(tau,1./3.);
	double taum32 = pow(tau,-3./2.);
	double taum74 = pow(tau,-7./4.);

	a0 = &(data->a0[0]);
	return log(delta) + a0[1] + a0[2] * tau - log(tau) + a0[3] * tau13 + a0[4] * taum32 + a0[5] * taum74;
}

/**
	Residual part of helmholtz function. Note: we have NOT prematurely
	optimised here ;-)
*/
double helm_resid(double tau, double delta, HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;

	HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		phir += atd->a * pow(tau, atd->t) * pow(delta, atd->d);
		++atd;
	}

	sum = 0;
	for(i=5; i<10; ++i){
		sum += atd->a * pow(tau, atd->t) * pow(delta, atd->d);
		++atd;
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	for(i=10; i<17; ++i){
		sum += atd->a * pow(tau, atd->t) * pow(delta, atd->d);
		++atd;
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	for(i=17; i<21; ++i){
		sum += atd->a * pow(tau, atd->t) * pow(delta, atd->d);
		++atd;
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}

/**
	Derivative of the helmholtz residual function with respect to
	delta.
*/	
double helm_resid_del(double tau,double delta,HelmholtzData *data){
	
	double sum;
	double phir = 0;
	unsigned i;
	double XdelX;

	HelmholtzATD *atd = &(data->atd[0]);
	
	for(i=0; i<5; ++i){
		phir += atd->a * pow(tau, atd->t) * pow(delta, atd->d - 1) * atd->d;
		++atd;
	}

	sum = 0;
	XdelX = delta;
	for(i=5; i<10; ++i){
		sum += atd->a * pow(tau, atd->t) * pow(delta, atd->d) * (atd->d - XdelX);
	}
	phir += exp(-delta) * sum;

	sum = 0; 
	XdelX = 2*delta*delta;
	for(i=10; i<17; ++i){
		sum += atd->a * pow(tau, atd->t) * pow(delta, atd->d) * (atd->d - XdelX);
	}
	phir += exp(-delta*delta) * sum;

	sum = 0;
	XdelX = 3*delta*delta*delta;
	for(i=17; i<21; ++i){
		sum += atd->a * pow(tau, atd->t) * pow(delta, atd->d) * (atd->d - XdelX);
	}
	phir += exp(-delta*delta*delta) * sum;

	return phir;
}
		







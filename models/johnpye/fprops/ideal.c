/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

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
*/

#include <math.h>

#include "ideal.h"

#ifdef TEST
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#endif

/*---------------------------------------------
  IDEAL COMPONENT RELATIONS
*/

/**
	Zero-pressure specific heat function (ideal gas limit)

	This is returned in the units of data->cp0star.
*/
double helm_cp0(double T, const IdealData *data){
	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double sum = 0;
	double term;

	/* power terms */
	pt = &(data->pt[0]);
#if 0
	fprintf(stderr,"np = %d\n",data->np);
#endif
	for(i = 0; i<data->np; ++i, ++pt){
		term = pt->a0 * pow(T, pt->t0);
#if 0
		fprintf(stderr,"i = %d: ",i);
		fprintf(stderr,"power term, a = %f, t = %f, val = %f\n",pt->a0, pt->t0, term);
#endif
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
#if 1
		fprintf(stderr,"exp term\n");
#endif
		double x = et->beta / T;
		double e = exp(-x);
		double d = (1-e)*(1-e);
		term = et->b * x*x * e / d;
#if 0
		fprintf(stderr,"exp term, b = %f, beta = %f, val = %f\n",et->b, et->beta, term);
#endif
		sum += term;
	}

#if 0
	fprintf(stderr,"Mult by cp0* = %f\n",data->cp0star);
#endif
	return data->cp0star * sum;
}

/*
	Hypothesis:
	in calculating the ideal component relations, we have some integration
	constants that ultimately allow the arbitrary scales of h and s to
	be specified. Hence we can ignore components of helm_ideal that
	are either constant or linear in tau, and work them out later when we
	stick in the values of data->c and data->m.
*/

/**
	Ideal component of helmholtz function
*/	
double helm_ideal(double tau, double delta, const IdealData *data){

	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double sum = log(delta) - log(tau) + data->c + data->m * tau;
	double term;
	double Tstar_on_tau = data->Tstar / tau;

	/* power terms */
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double a = pt->a0;
		double t = pt->t0;
		if(t == 0){
			term = a*log(tau);
		}else{
#ifdef TEST
			assert(t!=-1);
#endif
			term = -a / (t*(t+1)) * pow(Tstar_on_tau,t);
		}
		sum += pt->a0 * pow(tau, pt->t0);
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * log(1 - exp(-et->beta / Tstar_on_tau));
	}

	return sum;
}

/**
	Partial dervivative of ideal component of helmholtz residual function with 
	respect to tau.
*/	
double helm_ideal_tau(double tau, double delta, const IdealData *data){
	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double term;
	double sum = -1./tau + data->m;
	double Tstar_on_tau = data->Tstar / tau;

	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double a = pt->a0;
		double t = pt->t0;
		if(t==0){
			term = a / tau;
		}else{
			// term = -a / (t*(t+1)) * pow(Tstar_on_tau,t);
			term = a/(t+1)*pow(Tstar_on_tau,t)/tau;
		}
#ifdef TEST
		if(isinf(term)){
			fprintf(stderr,"Error with infinite-valued term with i = %d, a = %f, t = %f\n", i,a ,t);
			abort();
		}
#endif
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		term = et->b * et->beta / data->Tstar / (1 - exp(-et->beta/Tstar_on_tau));
#ifdef TEST
		assert(!isinf(term));
#endif
		sum += term;
	}

#ifdef TEST
	assert(!isinf(sum));
#endif
	return sum;
}

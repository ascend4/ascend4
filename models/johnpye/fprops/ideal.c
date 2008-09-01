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

	//fprintf(stderr,"constant = %f, linear = %f", data->c, data->m);
	//fprintf(stderr,"initial terms = %f\n",sum);
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double a = pt->a0;
		double t = pt->t0;
		if(t == 0){
			term = a*log(tau) /* + a */; /* term ignored, see above */
		}else if(t == 1){
			term = /* a * tau */ - a * tau * log(tau); /* term ignored, see above */
		}else{
			term = a * pow(tau,t) / ((t - 1) * t);
		}
		//fprintf(stderr,"i = %d: a0 = %f, t0 = %f, term = %f\n",i,pt->a0, pt->t0, term);
		sum += pt->a0 * pow(tau, pt->t0);
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * log( 1 - exp(-et->B * tau));
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

	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double a = pt->a0;
		double t = pt->t0;
		if(t==0){
			term = a / tau - a*t*pow(tau,t-1)/(t-1);
		}else if(t==1){
			term = -a*log(tau);
		}else{
			term = a*t*pow(tau,t-1)/(t-1);
		}
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * et->B  / (1 - exp(-tau*et->B));
	}
	return sum;
}

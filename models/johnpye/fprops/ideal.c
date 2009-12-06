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

#define SQ(X) ((X)*(X))

/*---------------------------------------------
  IDEAL COMPONENT RELATIONS
*/

//#define CP0_DEBUG

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
#ifdef CP0_DEBUG
	fprintf(stderr,"np = %d\n",data->np);
#endif
	for(i = 0; i<data->np; ++i, ++pt){
		term = pt->c * pow(T, pt->t);
#ifdef CP0_DEBUG
		fprintf(stderr,"i = %d: ",i);
		fprintf(stderr,"power term, c = %f, t = %f, val = %f\n",pt->c, pt->t, term);
#endif
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
#ifdef CP0_DEBUG
	fprintf(stderr,"ne = %d\n",data->ne);
#endif
	for(i=0; i<data->ne; ++i, ++et){
#ifdef xCP0_DEBUG
		fprintf(stderr,"exp term\n");
#endif
		double x = et->beta / T;
		double e = exp(-x);
		double d = (1-e)*(1-e);
		term = et->b * x*x * e / d;
#ifdef CP0_DEBUG
		fprintf(stderr,"i = %d: ",i);
		fprintf(stderr,"exp term, b = %f, beta = %f, val = %f\n",et->b, et->beta, term);
#endif
		sum += term;
	}

#ifdef CP0_DEBUG
	fprintf(stderr,"Sum = %f. Mult by cp0* = %f => %f\n",sum,data->cp0star,sum*data->cp0star);
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

//#define IDEAL_DEBUG
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

#ifdef IDEAL_DEBUG
	fprintf(stderr,"\tlog(delta) - log(tau) + c + m tau = %f (c=%f,m=%f)\n",sum,data->c, data->m);
	fprintf(stderr,"sum = %f\n",sum);
#endif

	/* power terms */
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double c = pt->c;
		double t = pt->t;
		if(t == 0){
			term = c*log(tau);
#ifdef IDEAL_DEBUG
			fprintf(stderr,"\tc log(tau) = %f (c=%f)\n",term,c);
#endif
		}else{
#ifdef IDEAL_DEBUG
			assert(t!=-1);
#endif
			term = -c / (t*(t+1)) * pow(Tstar_on_tau,t);
#ifdef IDEAL_DEBUG
			fprintf(stderr,"\t-c/t/(t+1)*pow(T*/tau) = %f (c=%f, t=%f)\n",term,c,t);
#endif
		}
		sum += term;
#ifdef IDEAL_DEBUG
		fprintf(stderr,"sum = %f\n",sum);
#endif
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		term = et->b * log(1 - exp(-et->beta / Tstar_on_tau));
#ifdef IDEAL_DEBUG
		fprintf(stderr,"\tb log(1-exp(-beta tau/Tstar)) = %f, (b=%f, beta=%f)\n",term,et->b,et->beta);
#endif
		sum += term;
	}

#ifdef IDEAL_DEBUG
	fprintf(stderr,"phi0 = %f\n",sum);
#endif

	return sum;
}

/**
	Partial dervivative of ideal component (phi0) of normalised helmholtz 
	residual function (phi), with respect to tau.
*/	
double helm_ideal_tau(double tau, double delta, const IdealData *data){
	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double term;
	double sum = -1./tau + data->m;
	//fprintf(stderr,"\t-1./tau + data->m = %f\n",sum);
	double Tstar_on_tau = data->Tstar / tau;

	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double c = pt->c;
		double t = pt->t;
		if(t==0){
			term = c / tau;
			//fprintf(stderr,"\tc/tau = %f\n",term);
		}else{
			// term = -c / (t*(t+1)) * pow(Tstar_on_tau,t);
			term = c/(t+1)*pow(Tstar_on_tau,t)/tau;
			double coeff = c/(t+1)*pow(data->Tstar,t);
			double powtau = pow(tau,-t-1.);
			//fprintf(stderr,"\tc / tau^p = %f (t=%f, c=%.3e, p=%f)\n",term,t,coeff,-t-1.);
		}
#ifdef TEST
		if(isinf(term)){
			fprintf(stderr,"Error with infinite-valued term with i = %d, c = %f, t = %f\n", i,c ,t);
			abort();
		}
#endif
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		double expo = exp(-et->beta/Tstar_on_tau);
		term = et->b * et->beta / data->Tstar * expo / (1-expo);
		//fprintf(stderr,"\tet->b * et->beta / data->Tstar / (1 - exp(-et->beta/Tstar_on_tau)) = %f\n",term);
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



/**
	Second partial dervivative of ideal component (phi0) of normalised helmholtz 
	residual function (phi), with respect to tau. This one is easy! It's just
	cp0 divided by tau^2, and it's not a function of delta.

	FIXME although this one is easy, we want to pull Tstar out of the
	ideal properties stuff, if that's possible.
*/	
double helm_ideal_tautau(double tau, const IdealData *data){
	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double sum = 0;
	double term;

	double T = data->Tstar / tau;

	/* power terms */
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		term = pt->c * pow(T, pt->t);
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		double x = et->beta / T;
		double e = exp(-x);
		double d = (1-e)*(1-e);
		term = et->b * x*x * e / d;
		sum += term;
	}

	return (1 - sum)/ SQ(tau);
}


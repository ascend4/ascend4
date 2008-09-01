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
	Ideal component of helmholtz function
*/	
double helm_ideal(double tau, double delta, const IdealData *data){

	const IdealPowTerm *pt;
	const IdealExpTerm *et;

	unsigned i;
	double sum = log(delta) + data->c + data->m * tau - log(tau);
	double term;

	//fprintf(stderr,"constant = %f, linear = %f", data->c, data->m);
	//fprintf(stderr,"initial terms = %f\n",sum);
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		term = pt->a0 * pow(tau, pt->t0);
		//fprintf(stderr,"i = %d: a0 = %f, t0 = %f, term = %f\n",i,pt->a0, pt->t0, term);
		sum += pt->a0 * pow(tau, pt->t0);
	}

#if 1
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * log( 1 - exp(- et->B * tau));
	}
#endif

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
	double sum = -1./tau + data->m;

	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		sum += pt->a0 * pt->t0 * pow(tau, pt->t0 - 1);
	}

#if 1
	/* FIXME we're missing the '2.5' in the alpha0 expression for Nitrogen... */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		sum += et->b * log( 1 - exp(- et->B * tau));
	}
#endif
	return sum;
}	

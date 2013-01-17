/*	ASCEND modelling environment
	Copyright (C) 2008-2009 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Ideal-gas components of helmholtz fundamental functions, calculated using
	terms in cp0 in a standard power series form. For details see the
	publications cited in the various fluid *.c files.

	John Pye, Jul 2008.
*/

#include <math.h>

#include "cp0.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define SQ(X) ((X)*(X))

//#define CP0_DEBUG
#ifdef CP0_DEBUG
# include "color.h"
# define MSG(FMT, ...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s: ",__func__);\
	color_off(stderr);\
	fprintf(stderr,FMT "\n",##__VA_ARGS__)
#else
# define MSG(ARGS...) ((void)0)
#endif

/*--------------------------------------------
  PREPARATION OF IDEAL RUNDATA from FILEDATA
*/

/*
	FIXME

	we have changed the definition of the cp0 expression to cp/R = sum( c *T/T*)^t )

	but now we need to fix the re-derive the conversion from the cp0 to alpha0
	expressions

	currently there is an error!
*/
Phi0RunData *cp0_prepare(const IdealData *I, double R, double Tstar){
	Phi0RunData *N = FPROPS_NEW(Phi0RunData);
	int i, add_const_term=1;
	double Tred, cp0red, p;
	N->c = 0;
	N->m = 0;
	switch(I->type){
	case IDEAL_CP0:
		N->np = I->data.cp0.np;
		N->ne = I->data.cp0.ne;
		Tred = I->data.cp0.Tstar;
		cp0red = I->data.cp0.cp0star;
		MSG("Preparing PHI0 data for ideal fluid (np = %d, ne = %d)",N->np, N->ne);
		MSG("Tred = %f, Tstar = %f", Tred, Tstar);
		MSG("cp0red = %f, R = %f", cp0red, R);

		for(i=0; i < N->np; ++i)if(I->data.cp0.pt[i].t == 0)add_const_term = 0;
		MSG("add_const_term = %d",add_const_term);

		N->pt = FPROPS_NEW_ARRAY(Phi0RunPowTerm,N->np + add_const_term);
		N->et = FPROPS_NEW_ARRAY(Phi0RunExpTerm, N->ne);
		add_const_term = 1;

		for(i=0; i < N->np; ++i){
			p = - I->data.cp0.pt[i].t;
			N->pt[i].p = p;
			if(p == 0){
				//if(add_const_term)fprintf(stderr,"Addding offset here\n");
				N->pt[i].a = (I->data.cp0.pt[i].c - add_const_term);
				add_const_term = 0;
			}else{
				N->pt[i].a = -I->data.cp0.pt[i].c / p / (p - 1) * pow(Tred / Tstar, p);
			}
		}

		if(add_const_term){
			fprintf(stderr,"WARNING: adding constant term %d in cp0, is that what you really want?\n",i);
			N->pt[i].a = -1;
			N->pt[i].p = 0;
			N->np++;
		}

		for(i=0; i < N->ne; ++i){
			N->et[i].n = I->data.cp0.et[i].b;
			N->et[i].gamma = I->data.cp0.et[i].beta * Tred / Tstar;
		}

		if(cp0red != R){
			fprintf(stderr,"WARNING: adjusting for R (=%f) != cp0red (=%f)...\n",R,cp0red);
			double X = cp0red / R;
			// scale for any differences in R and cpstar */
			for(i=0; i < N->np; ++i)N->pt[i].a *= X;
			for(i=0; i < N->ne; ++i)N->et[i].n *= X;
		}

		break;
	case IDEAL_PHI0:
		// TODO add checks for disallowed terms, eg p = 0 or p = 1?
		N->np = I->data.phi0.np;
		N->ne = I->data.phi0.ne;
		MSG("Preparing PHI0 data for ideal fluid (np = %d, ne = %d)",N->np, N->ne);

		// power terms
		N->pt = FPROPS_NEW_ARRAY(Phi0RunPowTerm,N->np);
		for(i=0; i< N->np; ++i){
			double a = I->data.phi0.pt[i].a0;
			double p = I->data.phi0.pt[i].p0;
			N->pt[i].a = a;
			N->pt[i].p = p;
		}

		// exponential terms
		N->et = FPROPS_NEW_ARRAY(Phi0RunExpTerm,N->ne);
		for(i=0; i < N->ne; ++i){
			double n = I->data.phi0.et[i].n;
			double gamma = I->data.phi0.et[i].gamma;
			N->et[i].gamma = gamma;
			N->et[i].n = n;
		}
		break;
	}
	return N;
}

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

//#define IDEAL_DEBUG
/**
	Ideal component of helmholtz function
*/
double ideal_phi(double tau, double delta, const Phi0RunData *data){

	const Phi0RunPowTerm *pt;
	const Phi0RunExpTerm *et;

	unsigned i;
	// FIXME what if rhostar != rhoc??
	double sum = log(delta) + data->c + data->m * tau;
	double term;

#ifdef IDEAL_DEBUG
	fprintf(stderr,"\ttau = %f, delta = %f\n",tau,delta);
	fprintf(stderr,"\tT ~ %f\n\n",Tstar_on_tau);
	fprintf(stderr,"\tlog(delta) + c + m tau = %f (c=%f,m=%f)\n",sum,data->c, data->m);
	fprintf(stderr,"sum = %f\n",sum);
#endif

	/* power terms */
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double a = pt->a;
		double p = pt->p;
		if(p == 0){
			term = a*log(tau);
#ifdef IDEAL_DEBUG
			fprintf(stderr,"\ta log(tau) = %f (a=%f, t=%f)\n",term,a,p);
#endif
		}else{
#ifdef IDEAL_DEBUG
			assert(p!=-1);
#endif
			term = a * pow(tau, p);
#ifdef IDEAL_DEBUG
			fprintf(stderr,"\ta tau^p = %f (a=%f, p=%f)\n",term,a,p);
#endif
		}
		sum += term;
#ifdef IDEAL_DEBUG
		fprintf(stderr,"sum = %f\n",sum);
#endif
	}

	/* Planck-Einstein terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		term = et->n * log(1 - exp(-et->gamma * tau));
#ifdef IDEAL_DEBUG
		fprintf(stderr,"\tn log(1-exp(-gamma*tau)) = %f, (n=%f, gamma=%f)\n",term,et->n,et->gamma);
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
double ideal_phi_tau(double tau, double delta, const Phi0RunData *data){
	const Phi0RunPowTerm *pt;
	const Phi0RunExpTerm *et;

	unsigned i;
	double term;
	double sum = data->m;

	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		double a = pt->a;
		double p = pt->p;
		if(p == 0){
			term = a / tau;
			//fprintf(stderr,"\tc/tau = %f\n",term);
		}else{
			term = a*p*pow(tau,p - 1);
			//fprintf(stderr,"\tc / tau^p = %f (t=%f, c=%.3e, p=%f)\n",term,t,coeff,-t-1.);
		}
#ifdef TEST
		if(isinf(term)){
			fprintf(stderr,"Error with infinite-valued term with i = %d, a = %f, p = %f\n", i,a ,p);
			abort();
		}
#endif
		assert(!isnan(term));
		sum += term;
	}

	/* Planck-Einstein terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		double expo = exp(-et->gamma * tau);
		term = et->n * et->gamma * expo / (1 - expo);
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
	residual function (phi), with respect to tau. This one is easy!
	It's not a function of delta.

	FIXME: although this one is easy, we want to pull Tstar out of the
	ideal properties stuff, if that's possible.
*/
double ideal_phi_tautau(double tau, const Phi0RunData *data){
	const Phi0RunPowTerm *pt;
	const Phi0RunExpTerm *et;

	unsigned i;

	double sum = 0;
	double term;

#ifdef CP0_DEBUG
	fprintf(stderr,"\ttau = %f\n",tau);
#endif

	/* power terms */
	pt = &(data->pt[0]);
	for(i = 0; i<data->np; ++i, ++pt){
		if(pt->p == 0){
			term = pt->a;
		}else{
			term = -pt->a * pt->p * (pt->p - 1) * pow(tau, pt->p);
		}
#ifdef CP0_DEBUG
		fprintf(stderr,"\tpt[%d] = ap(p-1)*tau^p (a = %e, p = %e) = %f\n",i,pt->a, pt->p, term);
#endif
		sum += term;
	}

	/* 'exponential' terms */
	et = &(data->et[0]);
	for(i=0; i<data->ne; ++i, ++et){
		double x = et->gamma * tau;
		double e = exp(-x);
		double d = (1-e)*(1-e);
		term = et->n * x*x * e / d;
#ifdef CP0_DEBUG
		fprintf(stderr,"\tet[%d] = n x^2 exp(-x)/(1 - exp(-x))^2  (n = %e, x=gamma*tau, gamma = %e) = %f\n",i,et->n, et->gamma, term);
#endif
		sum += term;
	}
	/* note, at this point, sum == cp0/R - 1 */
	MSG("sum = %f, phi_tautau = %f",sum, -sum/SQ(tau));
	return -sum/SQ(tau);
}

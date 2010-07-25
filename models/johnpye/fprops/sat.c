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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Routines to calculate saturation properties using Helmholtz correlation
	data. We first include some 'generic' saturation equations that make use
	of the acentric factor and critical point properties to predict saturation
	properties (pressure, vapour density, liquid density). These correlations
	seem to be only very rough in some cases, but it is hoped that they will
	be suitable as first-guess values that can then be fed into an iterative
	solver to converge to an accurate result.
*/

#include "sat.h"

#include "helmholtz_impl.h"
# include <assert.h>

#include <math.h>
#ifdef TEST
# include <stdio.h>
# include <stdlib.h>
#endif

#include <gsl/gsl_multiroots.h>

#define SQ(X) ((X)*(X))

/**
	Estimate of saturation pressure using H W Xiang ''The new simple extended
	corresponding-states principle: vapor pressure and second virial
	coefficient'', Chemical Engineering Science,
	57 (2002) pp 1439-1449.
*/
double fprops_psat_T_xiang(double T, const HelmholtzData *d){

	double Zc = d->p_c / (8314. * d->rho_c * d->T_c);

#ifdef TEST
	fprintf(stderr,"Zc = %f\n",Zc);
#endif

	double theta = SQ(Zc - 0.29);

#ifdef TEST
	fprintf(stderr,"theta = %f\n",theta);
#endif

	double aa[] = {5.790206, 4.888195, 33.91196
	            , 6.251894, 15.08591, -315.0248
	            , 11.65859, 46.78273, -1672.179
	};

	double a0 = aa[0] + aa[1]*d->omega + aa[2]*theta;
	double a1 = aa[3] + aa[4]*d->omega + aa[5]*theta;
	double a2 = aa[6] + aa[7]*d->omega + aa[8]*theta;

	double Tr = T / d->T_c;
	double tau = 1 - Tr;

	double taupow = pow(tau, 1.89);
	double temp = a0 + a1 * taupow + a2 * taupow * taupow * taupow;

	double logpr = temp * log(Tr);
	double p_r = exp(logpr);

#ifdef TEST
	fprintf(stderr,"a0 = %f\n", a0);
	fprintf(stderr,"a1 = %f\n", a1);
	fprintf(stderr,"a2 = %f\n", a2);
	fprintf(stderr,"temp = %f\n", temp);
	fprintf(stderr,"T_r = %f\n", Tr);
	fprintf(stderr,"p_r = %f\n", p_r);
#endif

	return p_r * d->p_c;
}

/**
	Saturated liquid density correlation of Rackett, Spencer & Danner (1972)
	see http://dx.doi.org/10.1002/aic.690250412
*/
double fprops_rhof_T_rackett(double T, const HelmholtzData *D){

	double Zc = D->rho_c * D->R * D->T_c / D->p_c;
	double Tau = 1. - T/D->T_c;
	double vf = (D->R * D->T_c / D->p_c) * pow(Zc, -1 - pow(Tau, 2./7));

	return 1./vf;
}


/**
	Saturated vapour density correlation of Chouaieb, Ghazouani, Bellagi
	see http://dx.doi.org/10.1016/j.tca.2004.05.017
*/
double fprops_rhog_T_chouaieb(double T, const HelmholtzData *D){
	double Zc = D->rho_c * D->R * D->T_c / D->p_c;
	double Tau = 1. - T/D->T_c;
#if 0
# define N1 -0.1497547
# define N2 0.6006565 
# define P1 -19.348354
# define P2 -41.060325
# define P3 1.1878726
	double MMM = 2.6; /* guess, reading from Chouaieb, Fig 8 */
	//MMM = 2.4686277;
	double PPP = Zc / (P1 + P2*Zc*log(Zc) + P3/Zc);
	fprintf(stderr,"PPP = %f\n",PPP);
	//PPP = -0.6240188;
	double NNN = PPP + 1./(N1*D->omega + N2);	
#else
/* exact values from Chouaieb for CO2 */
# define MMM 2.4686277
# define NNN 1.1345838
# define PPP -0.6240188
#endif

	double alpha = exp(pow(Tau,1./3) + sqrt(Tau) + Tau + pow(Tau, MMM));
	return D->rho_c * exp(PPP * (pow(alpha,NNN) - exp(1-alpha)));
}


/**
	Maxwell phase criterion, first principles
*/
void phase_criterion(double T, double rho_f, double rho_g, double p_sat, double *eq1, double *eq2, double *eq3, const HelmholtzData *D){
#ifdef TEST
	fprintf(stderr,"PHASE CRITERION: T = %f, rho_f = %f, rho_g = %f, p_sat = %f\n", T, rho_f, rho_g, p_sat);
#endif
	double delta_f, delta_g, tau;
    tau = D->T_c / T;

	delta_f = rho_f / D->rho_c;
	delta_g = rho_g/ D->rho_c;

#ifdef TEST
	assert(!isnan(delta_f));
	assert(!isnan(delta_g));
	assert(!isnan(p_sat));		
	assert(!isinf(delta_f));
	assert(!isinf(delta_g));
	assert(!isinf(p_sat));		
#endif

	*eq1 = (p_sat - helmholtz_p(T, rho_f, D));
	*eq2 = (p_sat - helmholtz_p(T, rho_g, D));
	*eq3 = helmholtz_g(T, rho_f, D) - helmholtz_g(T,rho_g, D);

#ifdef TEST
	fprintf(stderr,"eq1 = %e\t\teq2 = %e\t\teq3 = %e\n", *eq1, *eq2, *eq3);
#endif
}


typedef struct PhaseSolve_struct{
	double tau;
	const HelmholtzData *D;
	const double *scaling;
} PhaseSolve;

/**
	Maxwell phase criterion, from equations from IAPWS-95.

	We define here pi = p_sat / (R T rho_c)
*/
static int phase_resid(const gsl_vector *x, void *params, gsl_vector *f){
	const double tau = ((PhaseSolve *)params)->tau;
    const double pi = gsl_vector_get(x,0);
    double delta_f = gsl_vector_get(x,1);
	double delta_g = exp(gsl_vector_get(x,2));
	const HelmholtzData *D = ((PhaseSolve *)params)->D;
	const double *scaling = ((PhaseSolve *)params)->scaling;

	assert(!isinf(pi));
	assert(!isinf(delta_f));
	assert(!isinf(delta_g));

	gsl_vector_set(f, 0, scaling[0] * (delta_f + delta_f * delta_f * helm_resid_del(tau, delta_f, D) - pi));
	gsl_vector_set(f, 1, scaling[1] * (1 + delta_g * helm_resid_del(tau, delta_g, D) - pi/delta_g));
	gsl_vector_set(f, 2, scaling[2] * (pi * (delta_f - delta_g) - delta_f * delta_g * (log(delta_f / delta_g)  + helm_resid(delta_g,tau,D) - helm_resid(delta_f,tau,D))));


	if(isnan(gsl_vector_get(f,2)) || isnan(gsl_vector_get(f,2)) || isnan(gsl_vector_get(f,2))){
		fprintf(stderr,"NaN encountered... ");
	}

	//gsl_vector_set(f, 2, pi * (delta_f - delta_g) * delta_f*delta_g* (log(delta_f/delta_g) + helm_resid(delta_g,tau,D) - helm_resid(delta_f,tau,D)));

	return GSL_SUCCESS;
}

static int print_state(size_t iter, gsl_multiroot_fsolver * s){
	fprintf(stderr,"iter = %3u: pi = %.3f delf = %.3f delg = %.3f  "
		"E = % .3e % .3e %.3e\n",
		iter,
		gsl_vector_get(s->x, 0),
		gsl_vector_get(s->x, 1),
		gsl_vector_get(s->x, 2),
		gsl_vector_get (s->f, 0),
		gsl_vector_get (s->f, 1),
		gsl_vector_get (s->f, 2));
}


int phase_solve(double T, double *p_sat, double *rho_f, double *rho_g, const HelmholtzData *D){
	gsl_multiroot_fsolver *s;
	int status;
	const size_t n = 3;
	double tau = D->T_c / T;
	size_t i, iter = 0;
	const gsl_multiroot_fsolver_type *ftype;

	const double scaling[3] = {1., 1., 0.1};

	PhaseSolve p = {
		tau
		, D
		, scaling
	};

	gsl_multiroot_function f = {
		&phase_resid,
		n, &p
	};

	/* TODO use the first guess 'provided' if psat, rho_f, rho_g less than zero? */

	double x_init[3] = {
		/* first guesses, such as they are... */
		fprops_psat_T_xiang(T, D) / D->R / T / D->rho_c
		,fprops_rhof_T_rackett(T, D) / D->rho_c
		,log(fprops_rhog_T_chouaieb(T, D) / D->rho_c)
	};

	gsl_vector *x = gsl_vector_alloc (n);
	for(i=0; i<3; ++i)gsl_vector_set(x, i, x_init[i]);
	ftype = gsl_multiroot_fsolver_broyden;
	s = gsl_multiroot_fsolver_alloc(ftype, n);
	gsl_multiroot_fsolver_set(s, &f, x);

	//print_state(iter, s);

	double pi_xiang = fprops_psat_T_xiang(T, D) / D->R / T / D->rho_c;
	double delg_chouaieb = fprops_rhog_T_chouaieb(T, D) / D->rho_c;
	double delf_rackett = fprops_rhof_T_rackett(T, D) / D->rho_c;

	assert(!isnan(pi_xiang));
	assert(!isnan(delg_chouaieb));
	assert(!isnan(delf_rackett));

	do{
		iter++;

		status = gsl_multiroot_fsolver_iterate(s);

		print_state(iter, s);
		if(status)break;

		status = gsl_multiroot_test_residual(s->f, 1e-7);

		double pi = gsl_vector_get(s->x,0);
		double delf = gsl_vector_get(s->x,1);
		double delg = exp(gsl_vector_get(s->x,2));


#define VAR_PI 0
#define VAR_DELF 1
#define VAR_DELG 2
#define CHECK_RESET(COND, VAR, NEWVAL)\
	if(COND){\
		gsl_vector_set(s->x,VAR,NEWVAL);\
		fprintf(stderr,"RESET %s to %s = %f (FAILED %s)\n", #VAR, #NEWVAL, NEWVAL, #COND);\
	}

#define CHECK_RESET_CONTINUE(COND, VAR, NEWVAL)\
	if(COND){\
		gsl_vector_set(s->x,VAR,NEWVAL);\
		fprintf(stderr,"RESET %s to %s = %f (FAILED %s)\n", #VAR, #NEWVAL, NEWVAL, #COND);\
		continue;\
	}

		//CHECK_RESET_CONTINUE(pi < 0, VAR_PI, pi_xiang);
		//CHECK_RESET_CONTINUE(delg < 0, VAR_DELG, delg_chouaieb);
		//CHECK_RESET_CONTINUE(delf < 0, VAR_DELF, delf_rackett);

#if 0
		CHECK_RESET_CONTINUE(delf < 1, VAR_DELF, delf_rackett);
		CHECK_RESET_CONTINUE(delg > 1, VAR_DELG, delg_chouaieb);

		CHECK_RESET_CONTINUE(pi > 2. * pi_xiang, VAR_PI, pi_xiang)
		else CHECK_RESET_CONTINUE(pi < 0.5 * pi_xiang, VAR_PI, pi_xiang)

		CHECK_RESET_CONTINUE(delg < 0.8 * delg_chouaieb, VAR_DELG, delg_chouaieb)
		else CHECK_RESET_CONTINUE(delg > 1.5 * delg_chouaieb, VAR_DELG, delg_chouaieb);

		//if(gsl_vector_get(s->x,1) < D->rho_c)gsl_vector_set(s->x,1,2 * D->rho_c);
		//if(gsl_vector_get(s->x,2) > D->rho_c)gsl_vector_set(s->x,2,0.5 * D->rho_c);

#endif
	}while(status == GSL_CONTINUE && iter < 40);

	if(status!=GSL_SUCCESS)fprintf(stderr,"%s:%d: warning: (%d) status = %s\n", __FILE__,__LINE__, status, gsl_strerror (status));

	//fprintf(stderr,"SOLUTION: pi = %f\n", gsl_vector_get(s->x, 0));


	//fprintf(stderr,"          p  = %f\n", p_sat);

	*p_sat = gsl_vector_get(s->x, 0) * T * D->R * D->rho_c;
	*rho_f = gsl_vector_get(s->x, 1) * D->rho_c;
	*rho_g = gsl_vector_get(s->x, 2) * D->rho_c;

	gsl_multiroot_fsolver_free(s);
	gsl_vector_free(x);

	return status;
}


#if 0
	

#ifdef TEST
	fprintf(stderr,"PHASE CRITERION: T = %f, rho_f = %f, rho_g = %f, p_sat = %f\n", T, rho_f, rho_g, p_sat);
#endif
	double delta_f, delta_g, tau;
    tau = D->T_c / T;

	delta_f = rho_f / D->rho_c;
	delta_g = rho_g/ D->rho_c;

#ifdef TEST
	assert(!isnan(delta_f));
	assert(!isnan(delta_g));
	assert(!isnan(p_sat));		
	assert(!isinf(delta_f));
	assert(!isinf(delta_g));
	assert(!isinf(p_sat));		
#endif

	*eq1 = (p_sat - helmholtz_p(T, rho_f, D));
	*eq2 = (p_sat - helmholtz_p(T, rho_g, D));
	*eq3 = helmholtz_g(T, rho_f, D) - helmholtz_g(T,rho_g, D);

#ifdef TEST
	fprintf(stderr,"eq1 = %e\t\teq2 = %e\t\teq3 = %e\n", *eq1, *eq2, *eq3);
#endif
}



#if 0
static int phase_deriv(const gsl_vector * x, void *params, gsl_matrix * J){
	const double tau = ((PhaseSolve *)params)->tau;
    const double pi = gsl_vector_get(x,0);
    const double delta_f = gsl_vector_get(x,1);
	const double delta_g = gsl_vector_get(x,2);
	const HelmholtzData *D = ((PhaseSolve *)params)->D;

	double dE1ddelf = helm_resid_del(tau, delta_f, D) + delta_f * helm_resid_deldel(tau, delta_f, D) - pi ;
	double dE1ddelg = 0;
	double dE1dpi = - 1./ delta_f;

	double dE2ddelf = 0;
	double dE2ddelg = helm_resid_del(tau, delta_g, D) + delta_g * helm_resid_deldel(tau, delta_g, D) - pi ;
	double dE2dpi = - 1./ delta_g;

	double dE3ddelf = pi - delta_g * (log(delta_f/delta_g) + helm_resid(delta_g, tau, D) - helm_resid(delta_f, tau, D) + 1 - delta_f * helm_resid_del(delta_f, tau, D));
	double dE3ddelg = -pi - delta_f * (log(delta_f/delta_g) + helm_resid(delta_g, tau, D) - helm_resid(delta_f, tau, D) - 1 + delta_g * helm_resid_del(delta_g, tau, D));
	double dE3dpi = delta_f - delta_g;

	gsl_matrix_set (J, 0, 0, dE1ddelf);
	gsl_matrix_set (J, 0, 1, dE1ddelg);
	gsl_matrix_set (J, 0, 2, dE1dpi);
	gsl_matrix_set (J, 1, 0, dE2ddelf);
	gsl_matrix_set (J, 1, 1, dE2ddelg);
	gsl_matrix_set (J, 1, 2, dE2dpi);
	gsl_matrix_set (J, 2, 0, dE3ddelf);
	gsl_matrix_set (J, 2, 1, dE3ddelg);
	gsl_matrix_set (J, 2, 2, dE3dpi);

	return GSL_SUCCESS;
}

static int phase_residderiv(
	const gsl_vector * x, void *params, gsl_vector * f, gsl_matrix * J
){
	phase_resid(x, params, f);
	phase_deriv(x, params, J);
	return GSL_SUCCESS;
}

static int print_state_fdf(size_t iter, gsl_multiroot_fdfsolver * s){
	fprintf(stderr,"iter = %3u: delf = %.3f delg = %.3f pi = %.3f "
		"E = % .3e % .3e %.3e\n",
		iter,
		gsl_vector_get (s->x, 0),
		gsl_vector_get (s->x, 1),
		gsl_vector_get (s->x, 2),
		gsl_vector_get (s->f, 0),
		gsl_vector_get (s->f, 1),
		gsl_vector_get (s->f, 2));
}


double phase_solve_fdf(double T, const HelmholtzData *D){
	gsl_multiroot_fdfsolver *s;
	int status;
	const size_t n = 3;
	double tau = D->T_c / T;
	size_t i, iter = 0;
	const gsl_multiroot_fdfsolver_type *fdftype;

	PhaseSolve p = {tau, D};
	gsl_multiroot_function_fdf f = {
		&phase_resid,
		&phase_deriv,
		&phase_residderiv,
		n, &p
	};

	double x_init[3] = {
		/* first guesses, such as they are... */
		fprops_psat_T_xiang(T, D) / D->R / T / D->rho_c
		,fprops_rhof_T_rackett(T, D) / D->rho_c
		,fprops_rhog_T_chouaieb(T, D) / D->rho_c
	};

	gsl_vector *x = gsl_vector_alloc (n);
	for(i=0; i<3; ++i)gsl_vector_set(x, i, x_init[i]);
	fdftype = gsl_multiroot_fdfsolver_hybridsj;
	s = gsl_multiroot_fdfsolver_alloc (fdftype, n);
	gsl_multiroot_fdfsolver_set (s, &f, x);

	print_state_fdf(iter, s);

	do{
		iter++;
		status = gsl_multiroot_fdfsolver_iterate (s);
		print_state_fdf(iter, s);
		if(status)break;
		if(gsl_vector_get(s->x,2) > D->rho_c)gsl_vector_set(s->x,2,D->rho_c);
		status = gsl_multiroot_test_residual(s->f, 1e-7);
	}while(status == GSL_CONTINUE && iter < 1000);

	fprintf(stderr,"status = %s\n", gsl_strerror (status));

	fprintf(stderr,"SOLUTION: pi = %f\n", gsl_vector_get(s->f, 0));

	double p_sat = gsl_vector_get(s->f, 0) * T * D->R * D->rho_c;

	fprintf(stderr,"          p  = %f\n", p_sat);
	gsl_multiroot_fdfsolver_free (s);
	gsl_vector_free (x);

	return 0;
}
#endif


void solve_saturation(double T, const HelmholtzData *D){
	double rho_f, rho_g, p_sat;

	/* first guesses, such as they are... */
	p_sat = fprops_psat_T_xiang(T, D);
	rho_f = fprops_rhof_T_rackett(T, D);
	rho_g = fprops_rhog_T_chouaieb(T, D);

	

#ifdef TEST
	fprintf(stderr,"Rackett liquid density: %f\n", rho_f);	
	fprintf(stderr,"Chouaieb vapour density: %f\n", rho_g);
#endif

	double delta_f, delta_g, eq1, eq2, eq3, tau;
		
	int i = 40;
	while(--i > 0){
		delta_f = rho_f / D->rho_c;
		delta_g = rho_g/ D->rho_c;

#ifdef TEST
		assert(!isnan(delta_f));
		assert(!isnan(delta_g));
		assert(!isnan(p_sat));		
		assert(!isinf(delta_f));
		assert(!isinf(delta_g));
		assert(!isinf(p_sat));		
#endif
		
		phase_criterion(T, rho_f, rho_g, p_sat, &eq1, &eq2, &eq3, D);
			
#ifdef TEST    
		fprintf(stderr,"p_sat = %f\trho_f = %f\trho_g = %f\teq1 = %e\t\teq2 = %e\t\teq3 = %e\n",p_sat, rho_f, rho_g, eq1, eq2, eq3);
		assert(!isnan(eq1));
		assert(!isnan(eq2));
		assert(!isnan(eq3));
		assert(!isinf(eq1));		
		assert(!isinf(eq2));		
		assert(!isinf(eq3));		
#endif

		double p1 = D->R * T * D->rho_c * delta_f * delta_g / (delta_f - delta_g) * (helm_resid(delta_f,tau,D) - helm_resid(delta_g,tau,D) + log(delta_f/delta_g));
		if(p1/p_sat > 1.5){
			p1 = p_sat * 1.5;
		}else if(p1/p_sat < 0.7){
			p1 = p_sat * 0.7;
		}
		p_sat = p1;
		if(p_sat < D->p_t) p_sat = D->p_t;
		if(p_sat > D->p_c) p_sat = D->p_c;

		double rho_f1 = rho_f - (helmholtz_p(T, rho_f, D) - p_sat) / helmholtz_dpdrho_T(T, rho_f, D);
		double rho_g1 = rho_g - (helmholtz_p(T, rho_g, D) - p_sat) / helmholtz_dpdrho_T(T, rho_g, D);

		if(rho_g1 > 0){
			rho_g = rho_g1;
		}
		if(rho_f1 > 0){
			rho_f = rho_f1;
		}
		if(fabs(rho_f - rho_g) < 1e-5){
#ifdef TEST
			fprintf(stderr,"%s:%d: rho_f = rho_g\n", __FILE__, __LINE__);
			exit(1);
#else
			break;
#endif
		}



	}

	//return eq3;
}


#endif





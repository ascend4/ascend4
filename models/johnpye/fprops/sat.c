/*
	Estimate of saturation pressure using 

	H W Xiang "The new simple extended corresponding-states principle: vapor
	pressure and second virial coefficient", Chemical Engineering Science,
	57 (2002) pp 1439-1449.
*/

#include "sat.h"

#include "helmholtz_impl.h"

#include <math.h>
#ifdef TEST
# include <stdio.h>
# include <assert.h>
#endif

#define SQ(X) ((X)*(X))

double fprops_psat_T_xiang(double T, const HelmholtzData *d){

	double Zc = d->p_c / (8314. * d->rho_c * d->T_c);

	fprintf(stderr,"Zc = %f\n",Zc);

	double theta = SQ(Zc - 0.29);

	fprintf(stderr,"theta = %f\n",theta);

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
	Maxwell phase criterion as described in the IAPWS95 release.
*/
void phase_criterion(double T, double rho_f, double rho_g, double p_sat, const HelmholtzData *D){
	fprintf(stderr,"PHASE CRITERION: T = %f, rho_f = %f, rho_g = %f, p_sat = %f\n", T, rho_f, rho_g, p_sat);
	double delta_f, delta_g, tau;
	double eq1, eq2, eq3;
    tau = D->T_c / T;
	
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
		eq1 = (p_sat - helmholtz_p(T, rho_f, D));
		eq2 = (p_sat - helmholtz_p(T, rho_g, D));
		eq3 = (p_sat * (delta_f - delta_g) / D->rho_c - delta_f*delta_g/(D->R * T)*(helm_resid(delta_f,tau,D) - helm_resid(delta_g,tau,D) + log(delta_f/delta_g)));

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
			fprintf(stderr,"%s:%d: rho_f = rho_g\n", __FILE__, __LINE__);
			exit(1);
		}



	}

	//return eq3;
}




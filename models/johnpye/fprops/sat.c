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
void phase_criterion(double rho_f, double rho_g, double T, double p_sat, const HelmholtzData *D){
	fprintf(stderr,"PHASE CRITERION\n");
	double delta_f, delta_g, tau;
	double eq1, eq2, eq3;

    delta_f = rho_f / D->rho_c;
    delta_g = rho_g/ D->rho_c;
    tau = D->T_c / T;

    eq1 = p_sat / (D->R * T)- (1.0 + delta_f*helm_resid_del(delta_f,tau,D)) * rho_f;

	fprintf(stderr,"PHASE CRITERION\n");

    eq2 = p_sat / (D->R * T) - (1.0 + delta_g*helm_resid_del(delta_g,tau,D)) * rho_g;

	fprintf(stderr,"PHASE CRITERION\n");

    eq3 = (p_sat / (D->R*T*D->rho_c))*((delta_f - delta_g)/(delta_f*delta_g)) - log(delta_f/delta_g) - helm_resid(delta_f,tau,D) + helm_resid(delta_g,tau,D);

#ifdef TEST    
	fprintf(stderr,"eq1 = %f\neq2 = %f\neq3 = %f\n",eq1, eq2, eq3);
#endif

	//return eq3;
}




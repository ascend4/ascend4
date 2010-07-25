#include "sat2.h"

#include "helmholtz_impl.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

/**
	solve Maxwell phase criterion using successive substitution

	@return p
	@param rhof output, liquid density
	@param rhof output, vapour density
	@param err error code, returns non-zero if unable to solve.
*/
double fprops_sat_succsubs(double T, double *rhof_out, double *rhog_out, const HelmholtzData *d, int *err){
	double p, delta_p, delta_p_old;

	/* first guess using acentric factor */
	p = d->p_c * pow(10, -7./3 * (1.+d->omega) * (d->T_c / T - 1.));

	fprintf(stderr,"Initial estimate: psat(%f) = %f bar\n",T,p/1e5);

	int i,j;
	char use_guess = 0;

#define FPROPS_MAX_SUCCSUBS 200

	double rhof = -1, rhog = -1;

	for(i=0;i<FPROPS_MAX_SUCCSUBS;++i){
		if(fprops_rho_pT(p,T,FPROPS_PHASE_LIQUID,use_guess, d, &rhof)){
			fprintf(stderr,"  increasing p, error with rho_f\n");
			p *= 1.005;
			continue;
		}

		if(fprops_rho_pT(p,T,FPROPS_PHASE_VAPOUR, use_guess, d, &rhog)){
			fprintf(stderr,"  decreasing p, error with rho_g\n");
			p *= 0.95;
			continue;
		}

		fprintf(stderr,"Iter %d: p = %f, rhof = %f, rhog = %f\n",i, p, rhof, rhog);

		use_guess = 1; /* after first run, start re-using current guess */

		double delta_a = helmholtz_a(T, rhog, d) - helmholtz_a(T, rhof, d);

		if(fabs(rhof - rhog) < 1e-8){
			fprintf(stderr,"FPROPS: densities converged to same value\n");
			*err = 1;
			*rhof_out = rhof;
			*rhog_out = rhog;
			return p;
		}

		delta_p = delta_a / (1./rhog - 1./rhof);
	
		/* note possible need for delp non-change test for certain fluids */
		if(abs(delta_p/p) < 1e-6){
	        p = p + delta_p;
		
			/* find vapour density, using guess */
			if(fprops_rho_pT(p, T, FPROPS_PHASE_VAPOUR, use_guess, d, &rhog)){
				fprintf(stderr,"FPROPS: fails to estimate vapour density\n");
				*err = 2;
				*rhof_out = rhof;
				*rhog_out = rhog;
				return p;
			}
			fprops_rho_pT(p, T, FPROPS_PHASE_LIQUID, use_guess, d, &rhof);

			/* re-evaluate exact p for the value of rhof calculated */
			p = helmholtz_p(T,rhof,d);

			if(p > d->p_c || rhof < d->rho_c){
				fprintf(stderr,"FPROPS: invalid converged value of p, beyond p_crit\n");
				*err = 3;
				*rhof_out = rhof;
				*rhog_out = rhog;
				return p;

			}
			*rhof_out = rhof;
			*rhog_out = rhog;
			return p;
		}
		delta_p_old = delta_p;

		if(fabs(delta_p) > 0.4 * p){
			/* reduce the size of delta_p steps */
			for(j=0;j<10;++j){
				delta_p *= 0.5;
				if(fabs(delta_p) < 0.4 * p)break;
			}
			fprintf(stderr,"FPROPS: delta_p was too large, has been shortened\n");
		}

		p = p + delta_p;
	}

	if(i = FPROPS_MAX_SUCCSUBS){
		fprintf(stderr,"FPROPS: too many iterations in %s, need to try alternative method\n",__func__);
		*err = 1;
		*rhof_out = rhof;
		*rhog_out = rhog;
		return p;
	}
}


/*
	Iterate to find density as a function of pressure and temperature

	@param use_guess whether (1) or not (0) to use first guess value of *rho.
	@param rho initial guess (in), and return value (out)
	@return non-zero on error
*/
int fprops_rho_pT(double p, double T, char phase, char use_guess, const HelmholtzData *d, double *rho){

	double tol = 1e-8;
	double vlog, Dguess, dpdrho;
	double vlog_prev;
	int i;

#if 0
	if(p < 1e-11 /*Pa*/){
		if(T > 0){
			return /* estimate based on ideal gas equation */
		}
	}
#endif

	double vc = 1./d->rho_c;
	if(vc <= 0)vc = 1;
	double vclog = log(vc);

	char use_liquid_calc = 0;

	if(p < d->p_c * (1. + 7.*(T/d->T_c - 1.)) && T > d->T_c){
		use_liquid_calc = 0;
	}else if(p > d->p_c || T > d->T_c){
        use_liquid_calc = 1;
	}else if(phase == 'L'){
		use_liquid_calc = 1;
	}else{
		use_liquid_calc = 0;
	}

	char bad_guess = 0;
	if(use_guess){
		if(*rho > 0)vlog = log(1. / *rho);
		else bad_guess = 1;
	}
	
	if(!use_guess || bad_guess){
		if(p > d->p_c || T > d->T_c){
			/* supercritical density estimate */
			vlog = log(0.5 * vc);
		}else if(use_liquid_calc){
			/* first guess liquid density */
			Dguess = fprops_rhof_T_rackett(T, d);

			/* TODO sure first guess is within permitted upper bounds */

			/* if Rackett estimate gives dp/drho < 1, increase guess */
			int k = 0;
			while(k++ < 8){
				double dpdrho = helmholtz_dpdrho_T(T,Dguess,d);
				if(dpdrho > 0)break;
				Dguess *= 1.1;				
			};
			vlog = log(1./Dguess);

			/* upper bound our first guess by log(0.5*v_crit) */
			double vlog_max = log(0.5 * vc);
			if(vlog > vlog_max)vlog = vlog_max;
        }else{
			/* for gas, first guess is to assume ideal gas */
			vlog=log(d->R * T / p);
        }

        if(!use_liquid_calc)vlog=log(d->R*T/p);
	}


	if (use_liquid_calc){

		/* ==== liquid phase iteration ==== */

		for(i=0; i<100; ++i){
			
			vlog_prev=vlog;

			*rho=1./exp(vlog);
			double p2 = helmholtz_p(T,*rho,d);
			double dpdrho = helmholtz_dpdrho_T(T,*rho,d);

			if(dpdrho < 0){
				// reduce spec volume if we have dp/drho < 0
				vlog=vlog-0.1;
			}else{
				double newt;
				double dpdlv = -*rho * dpdrho;
				newt = (p2-p) / dpdlv; // first-order Newton method

				/* loosen the tolerance if we're not getting there */
				if(i == 10)tol = tol*10;
				if(i == 15)tol = tol*10;

				if(fabs(newt/p) < 0.0001*tol){
					// iteration has converged
		            *rho = 1./exp(vlog - newt);
        		    return 0;
         		}else{
					// next guess
					vlog = vlog_prev - newt;

					// keep step-sizes under control...
					if(fabs(vlog-vlog_prev) > 0.1 && T < 1.5*d->T_c){
						vlog = vlog_prev + (vlog-vlog_prev > 0 ? 0.1 : -0.1);
					}
					if(vlog > vclog && T < d->T_c){
						vlog=0.5*(vlog_prev+vclog);
					}
					if (vlog < -5.0 && T > d->T_c){
						use_liquid_calc = 0;
						fprintf(stderr,"FPROPS: switching to vapour calc\n");
						break; /* switch to vapour calc */
					}
				}
			}

		}

		// iteration has not converged
        *rho = 1./exp(vlog);
        if (T > d->T_c){
			use_liquid_calc = 0;
        }

		if(use_liquid_calc){
			fprintf(stderr,"FPROPS: liquid iteration did not converge\n");
			return 1;
		}
	}

	/* ==== vapour phase iteration ==== */

    double plog=log(p);
	for(i=0; i<30; ++i){
		vlog_prev = vlog;
		*rho = 1./exp(vlog);
		double p2 = helmholtz_p(T,*rho,d);
		double dpdrho = helmholtz_dpdrho_T(T,*rho,d);
		if(dpdrho < 0. || p2 <= 0.){
			// increase spec vol if dpdrho < 0 or guess pressure < 0
			vlog += 0.1;
		}else{
			double dpdlv = - *rho * dpdrho;
			double fvdpl = (log(p2) - plog)*p2/dpdlv; // first-order Newton method

			if(fabs(fvdpl) > 25.)fvdpl=0.1e-5;

			// loosen tol if we're not getting there
			if(i == 10)tol *= 10;
			if(i == 15)tol *= 10;

			if(fabs(fvdpl) < 0.0001*tol){
				// converged, success
				*rho = 1./exp(vlog);
				return 0;
			}else{
				// next guess
				vlog=vlog-fvdpl;
			}
        }
	}

	// not converged
    *rho=1./exp(vlog);
	fprintf(stderr,"FPROPS: liquid iteration did not converge\n");
	return 1;
}



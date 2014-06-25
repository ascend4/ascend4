#include "refstate.h"
#include "fprops.h"
#include "sat.h"
#include "zeroin.h"

#include <stdio.h>
#include <math.h>

//#define REF_DEBUG
#ifdef REF_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

static ZeroInSubjectFunction refstate_perr_Trho;

typedef struct{
	PureFluid *P;
	double T;
	double p;
} RefStateTPData;

int fprops_set_reference_state(PureFluid *P, const ReferenceState *ref){
	FpropsError res = 0;
	P->data->cp0->c = 0;
	P->data->cp0->m = 0;
	int err;
	FluidState S1, S2;
	double T, p, rho, rho_f, rho_g, h1, h2, s1, s2, g2, resid;
#ifdef REF_DEBUG
	double u;
#endif

	if(ref->type == FPROPS_REF_REF0){
		ref = &(P->data->ref0);
	}

	switch(ref->type){
	case FPROPS_REF_PHI0:
		P->data->cp0->c = ref->data.phi0.c;
		P->data->cp0->m = ref->data.phi0.m;
		MSG("Set PHI0 reference state.");
		return 0;

	case FPROPS_REF_IIR:
		MSG("Setting IIR reference state.");
		/* need to calculate the saturated liquid state at 0 deg C */
		T = 273.15 + 0;

		if(T > P->data->T_c){
			MSG("T_ref (=%f) > T_c",T);
			return 4000;
		}
		if(T < P->data->T_t){
			MSG("T_ref < T_t");
			return 5000;
		}

		fprops_sat_T(T, &p, &rho_f, &rho_g, P, &res);
		if(res){
			MSG("Failed to solve saturation properties at T = %f K",T);
			return 1000 + res;
		}

		S1 = fprops_set_Trho(T,rho_f,P,&res);
		h1 = fprops_h(S1,&res);
		if(res){
			MSG("Error in evaluation of h(T,rhof)");
			return 2000+res;
		}

		s1 = fprops_s(S1,&res);
		if(res){
			MSG("Error in evaluation of s(T,rhof)");
			return 3000+res;
		}

		h2 = 200e3;
		s2 = 1e3;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

		MSG("Done");
		return 0;

	case FPROPS_REF_NBP:
		/* normal boiling point, need to solve for the temperature first! */
		p = 101.325e3;
		if(p > P->data->p_c){
			ERRMSG("Fluid is supercritical at atmospheric pressure; can't use this reference state");
			return 500;
		}
		MSG("p = %f < p_c = %f. T_c = %f", p, P->data->p_c, P->data->T_c);
		fprops_sat_p(p, &T,&rho_f, &rho_g, P, &res);
		MSG("Got Tsat(p) = %f",T);
		if(res){
			ERRMSG("Failed to solve saturation properties at atmospheric pressure");
			return 1000 + res;
		}
		MSG("Tsat(p = %e) = %f, rho_f = %f",p, T,rho_f);
		S1 = fprops_set_Trho(T,rho_f,P,&res);
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;
		s1 = fprops_s(S1,&res);
		if(res)return 3000+res;
		h2 = 0; s2 = 0;
		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		MSG("h at T,rhof = %f",fprops_h(S1,&res));
		MSG("s at T,rhof = %f",fprops_s(S1,&res));
		return 0;

	case FPROPS_REF_TRHS:
		T = ref->data.trhs.T0;
		rho = ref->data.trhs.rho0;
		S1 = fprops_set_Trho(T,rho,P,&res);
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;

		s1 = fprops_s(S1,&res);
		if(res)return 3000+res;

		h2 = ref->data.trhs.h0;
		s2 = ref->data.trhs.s0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		MSG("Set TRHS reference state.");
		return 0;

	case FPROPS_REF_TPUS:
		/* need to solve for T,p first... */
		T = ref->data.tpus.T0;
		p = ref->data.tpus.p0;
		{
			/* let's try zeroin... */
			RefStateTPData D = {P, T, p};
			MSG("Upper bound rho = %f",5*P->data->rho_c);
			err = zeroin_solve(&refstate_perr_Trho, &D, 1e-10, 5*P->data->rho_c, 1e-5, &rho, &resid);
			if(err){
				fprintf(stderr,"Unable to set T,p for reference state (T = %f K, p = %f kPa)\n",T,p/1e3);
				return 1000 + err;
			}
			MSG("Solved rho = %f for T = %f, p = %f",rho,T,p);
			MSG("Check: p(T,rho) = %f", fprops_p(fprops_set_Trho(T,rho,P,&res),&res));
		}

		S1 = fprops_set_Trho(T,rho,P,&res);
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;
		s1 = fprops_s(S1,&res);
		if(res)return 3000+res;

		h2 = ref->data.tpus.u0 + ref->data.tpus.p0 / rho;
		s2 = ref->data.tpus.s0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

#ifdef REF_DEBUG
		S1 = fprops_set_Trho(T,rho,P,&res);
		if(res)return 44000+res;
		u = fprops_u(S1,&res);
		s2 = fprops_s(S1,&res);
		p = fprops_p(S1,&res);
		if(res)return 4000+res;
		MSG("Resulting reference values: u = %f, s = %f, p = %f kPa",u,s2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);
#endif

		MSG("Set TRUS reference state.");
		return 0;

	case FPROPS_REF_TPHS:
		/* need to solve for T,p first... */
		T = ref->data.tphs.T0;
		p = ref->data.tphs.p0;
		{
			/* let's try zeroin... */
			RefStateTPData D = {P, T, p};
			MSG("Upper bound rho = %f",5*P->data->rho_c);
			err = zeroin_solve(&refstate_perr_Trho, &D, 1e-10, 5*P->data->rho_c, 1e-5, &rho, &resid);
			if(err){
				fprintf(stderr,"Unable to set T,p for reference state (T = %f K, p = %f kPa)\n",T,p/1e3);
				return 1000 + err;
			}
			MSG("Solved rho = %f for T = %f, p = %f",rho,T,p);
			MSG("Check: p(T,rho) = %f", fprops_p(fprops_set_Trho(T,rho,P,&res),&res));
		}

		S1 = fprops_set_Trho(T,rho,P,&res);
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;
		s1 = fprops_s(S1,&res);
		if(res)return 3000+res;

		h2 = ref->data.tphs.h0;
		s2 = ref->data.tphs.s0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

		S2 = fprops_set_Trho(T,rho,P,&res);
		h2 = fprops_h(S2,&res);
		s2 = fprops_s(S2,&res);
		p = fprops_p(S2,&res);
		if(res)return 4000+res;

		MSG("Resulting reference values: h = %f, s = %f, p = %f kPa",h2,s2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);

		MSG("Set TPHS reference state.");
		return 0;

	case FPROPS_REF_TPF:
		T = P->data->T_t;
		fprops_triple_point(&p,&rho_f,&rho_g,P, &res);
		if(res)return 1000+res;
		S1 = fprops_set_Trho(T,rho_f,P,&res);
		if(res)return 8000+res;
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;
		s1 = fprops_s(S1,&res);
		if(res)return 3000+res;
		h2 = 0;
		s2 = 0;
		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		MSG("Set TPF reference state.");
		return 0;

	case FPROPS_REF_TPFU:
		T = P->data->T_t;
		fprops_triple_point(&p,&rho_f,&rho_g,P, &res);
		if(res)return 1000+res;
		S1 = fprops_set_Trho(T,rho_f,P,&res);
		if(res)return 8000+res;
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;
		s1 = fprops_s(S1,&res);
		if(res)return 3000+res;
		h2 = p / rho_f;
		s2 = 0;
		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		MSG("Set TPFU reference state.");
		return 0;

	case FPROPS_REF_TPHG:
		MSG("Setting formation enthalpy/gibbs energy reference state");

		/* TODO What if p = 0 (ideal gas reference state?) --> density is also zero, does that cause errors? can we use the cp0 function? */

		/* as per TPHS, we need to solve for T,p first... */
		T = ref->data.tphg.T0;
		p = ref->data.tphg.p0;

		if(isnan(ref->data.tphg.h0)){
			ERRMSG("Unable to set reference state: h0 is not a number (missing data)");
			return 11000;
		}
		if(isnan(ref->data.tphg.g0)){
			ERRMSG("Unable to set reference state: g0 is not a number (missing data)");
			return 12000;
		}


		// find value rho in (T0,rho) corresponding to (T0,p0)
		{
			/* let's try zeroin... */
			RefStateTPData D = {P, T, p};
			MSG("Upper bound rho = %f",5*P->data->rho_c);
			err = zeroin_solve(&refstate_perr_Trho, &D, 1e-10, 5*P->data->rho_c, 1e-5, &rho, &resid);
			if(err){
				fprintf(stderr,"Unable to set T,p for reference state (T = %f K, p = %f kPa)\n",T,p/1e3);
				return 1000 + err;
			}
			MSG("Solved rho = %f for T = %f, p = %f",rho,T,p);
			MSG("Check: p(T,rho) = %f", fprops_p(fprops_set_Trho(T,rho,P,&res),&res));
		}

		S1 = fprops_set_Trho(T,rho,P,&res);
		h1 = fprops_h(S1,&res);
		if(res)return 2000+res;
		s1 = fprops_g(S1,&res);
		if(res)return 3000+res;

		// calculuate target entropy value from reference h0, g0, T0 (using g = h - Ts)
		h2 = ref->data.tphg.h0;
		s2 = (ref->data.tphg.h0 - ref->data.tphg.g0) / ref->data.tphg.T0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

		S2 = fprops_set_Trho(T,rho,P,&res);
		h2 = fprops_h(S2,&res);
		g2 = fprops_g(S2,&res);
		p = fprops_p(S2,&res);
		if(res)return 4000+res;

		MSG("Resulting reference values: h = %f, g = %f, p = %f kPa",h2,g2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);

		MSG("Set TPHG reference state.");
		return 0;

	default:
		fprintf(stderr,"%s: Unhandled case (type %d)\n",__func__,ref->type);
		return -1;
	}
}


double refstate_perr_Trho(double rho, void *user_data){
#define D ((RefStateTPData *)user_data)
	FpropsError err = 0;
	double p = fprops_p(fprops_set_Trho(D->T, rho, D->P, &err), &err);
	if(err)return -1;
	return p - D->p;
#undef D
}


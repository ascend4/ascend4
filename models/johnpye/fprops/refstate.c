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

#if 1
#include "ideal.h"
#include "incomp.h"
#endif


int fprops_set_reference_state(PureFluid *P, const ReferenceState *ref){
	if(!P){ERRMSG("P is NULL"); return 1;}
	if(!P->setref_fn){ERRMSG("P->setref_fn is NULL"); return 1;}
	return P->setref_fn(P,ref);
}
static ZeroInSubjectFunction refstate_perr_Trho;

typedef struct{
	PureFluid *P;
	double T;
	double p;
} RefStateTPData;

int refstate_set_for_phi0(PureFluid *P, const ReferenceState *ref){
	if(!P){ERRMSG("P is NULL"); return 1;}
	if(!P->data){ERRMSG("P->data is NULL"); return 1;}

	if(ref == NULL || ref->type == FPROPS_REF_REF0){
		MSG("Using the default reference state specified for this fluid");
		ref = &(P->data->ref0);
	}

	if(P->type == FPROPS_INCOMP){
		ERRMSG("This function cannot be used for incompressible fluids");
		return 1;
	}


	if(!P->data->cp0){ERRMSG("P->data->cp0 is NULL"); return 1;}
	FpropsError res = 0;
	P->data->cp0->c = 0;
	P->data->cp0->m = 0;
	int err;
	FluidState2 S1, S2;
	double T, p, rho, rho_f, rho_g, h1, h2, s1, s2, resid;
#ifdef REF_DEBUG
	double u, g2;
#endif

	MSG("Reference state type=%d",ref->type);

	switch(ref->type){
	case FPROPS_REF_PHI0:
		P->data->cp0->c = ref->data.phi0.c;
		P->data->cp0->m = ref->data.phi0.m;
		MSG("Set PHI0 reference state.");
		return 0;

	case FPROPS_REF_TPHS0:
		/* FIXME we seem to have errors here, as tested with fluids/oxygen.c */
		T = ref->data.tphs.T0;
		p = ref->data.tphs.p0;
		h1 = ref->data.tphs.h0;
		s1 = ref->data.tphs.s0;
		MSG("state: p=%f, T=%f",p,T);
		MSG("R = %f",P->data->R);
		/* at this state, rho0 = p/R*T -- by ideal gas equation. */
		double rho0 = p / P->data->R / T;

		P->data->cp0->m = 0;
		P->data->cp0->c = 0;


		h2 = ideal_h((FluidStateUnion){.Trho={T,rho0}},P->data,&res);
		s2 = ideal_s((FluidStateUnion){.Trho={T,rho0}},P->data,&res);
		MSG("h2 = %f",h2);

		P->data->cp0->m = -h1 / P->data->R / P->data->T_c;

#if 1
		P->data->cp0->m = -h1 / P->data->R / P->data->T_c;
		P->data->cp0->c = -s1/P->data->R - 1. - log(p/(P->data->rhostar*P->data->R*T)) + log(P->data->Tstar/T);
#else
		h2 = ideal_h(T,rho0,P->data,&res);
		if(res)return 9000+res;
		s2 = ideal_s(T,rho0,P->data,&res);
		if(res)return 10000+res;
		P->data->cp0->c = (s2 - s1)/P->data->R;
		P->data->cp0->m = -(h2 - h1) / P->data->R / P->data->T_c;
#endif
		MSG("m = %f",P->data->cp0->m);
		MSG("c = %f",P->data->cp0->c);
		if(1){
			MSG("ideal_h(p0,T0) = %g",ideal_h((FluidStateUnion){.Trho={T,rho0}},P->data,&res));
			MSG("ideal_s(p0,T0) = %g",ideal_s((FluidStateUnion){.Trho={T,rho0}},P->data,&res));
		}
		MSG("Set TPHS0 reference state.");
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
		if(P->type == FPROPS_IDEAL){
			ERRMSG("Not implemented: FPROPS_REF_NBP with this fluid type");
			return 1;
		}
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
		if(P->type == FPROPS_IDEAL){
			ERRMSG("Not implemented: FPROPS_REF_TRHS with this fluid type");
			return 1;
		}
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
		h2 = ref->data.tphs.h0;
		s2 = ref->data.tphs.s0;

		switch(P->type){
#if 0
		case FPROPS_INCOMP:
			S1 = fprops_set_Tp(T,p,P,&res);
			h1 = fprops_h(S1,&res);
			if(res){ERRMSG("Unable to calculate h(T0,p0)");return 1800+res;}
			s1 = fprops_s(S1,&res);
			if(res){ERRMSG("Unable to calculate s(T0,p0)");return 1900+res;}

			P->data->cp0->c = -(s2 - s1)/P->data->R;
			P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

			S2 = fprops_set_Tp(T,p,P,&res);
			break;
#endif
		case FPROPS_PENGROB:
		case FPROPS_HELMHOLTZ:
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

		S2 = fprops_set_Trho(T,rho,P,&res);
			break;
		default:
			ERRMSG("Not implemented: FPROPS_REF_TPHS with this fluid type.");
			return 1;
		}
		h2 = fprops_h(S2,&res);
		s2 = fprops_s(S2,&res);
		p = fprops_p(S2,&res);
		if(res)return 4000+res;

		MSG("Resulting reference values: h = %f, s = %f, p = %f kPa",h2,s2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);

		MSG("Set TPHS reference state.");
		return 0;

	case FPROPS_REF_TPF:
		if(P->type == FPROPS_IDEAL){
			ERRMSG("Not implemented: FPROPS_REF_TPF is not permitted with this fluid type.");
			return 1;
		}
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
		if(P->type == FPROPS_IDEAL){
			ERRMSG("Not implemented: FPROPS_REF_TPFU with this fluid type.");
			return 1;
		}
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

#ifdef REF_DEBUG
		S2 = fprops_set_Trho(T,rho,P,&res);
		h2 = fprops_h(S2,&res);
		g2 = fprops_g(S2,&res);
		p = fprops_p(S2,&res);
		if(res)return 4000+res;

		MSG("Resulting reference values: h = %f, g = %f, p = %f kPa",h2,g2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);
#endif

		MSG("Set TPHG reference state.");
		return 0;

	case FPROPS_REF_UNDEFINED:
		ERRMSG("Not implemented: FPROPS_REF_UNDEFINED with this fluid type.");
		return 1;
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

/*--------------------------------------------------------------------------------------*/

int refstate_set_for_incomp(PureFluid *P, const ReferenceState *ref){
	if(!P){ERRMSG("P is NULL"); return 1;}
	if(!P->data){ERRMSG("P->data is NULL"); return 1;}

	if(ref == NULL || ref->type == FPROPS_REF_REF0){
		MSG("Using the default reference state specified for this fluid (type %d)",P->data->ref0.type);
		ref = &(P->data->ref0);
	}

	if(P->type != FPROPS_INCOMP){
		ERRMSG("This function is only for compressible fluids");
		return 1;
	}

	if(!P->data->corr.incomp->cp0){ERRMSG("cp0 data is NULL"); return 1;}
	FpropsError err = 0;
	P->data->corr.incomp->const_h = 0;
	P->data->corr.incomp->const_s = 0;

	double h, s, h0, s0, T0, p0;

	switch(ref->type){
	case FPROPS_REF_TPHS:
		T0 = ref->data.tphs.T0;
		p0 = ref->data.tphs.p0;
		FluidStateUnion S0 = {.Tp={T0,p0}};
		h = incomp_h(S0, P->data, &err);
		s = incomp_s(S0, P->data, &err);
		MSG("h(S0) = %f", h);
		MSG("s(S0) = %f", s);
		P->data->corr.incomp->const_s = ref->data.tphs.s0 - s;
		P->data->corr.incomp->const_h = ref->data.tphs.h0 - h;
		h0 = incomp_h(S0, P->data, &err);
			s0 = incomp_s(S0, P->data, &err);
		MSG("h0(S0) = %f", h0);
		MSG("s0(S0) = %f", s0);
		if(fabs(h0 - ref->data.tphs.h0) > 1e-9){ERRMSG("Failed to set h0"); return 1;}
		if(fabs(s0 - ref->data.tphs.s0) > 1e-9){ERRMSG("Failed to set s0"); return 1;}
		if(err){ERRMSG("Failed to calculate h or s for incompressible reference state"); return 1;}
		MSG("Success; h_const = %f", P->data->corr.incomp->const_h);
		return 0;
	default:
		ERRMSG("Not implemented");
		return 1;
	}
}

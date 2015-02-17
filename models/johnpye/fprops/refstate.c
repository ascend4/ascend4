#include "refstate.h"
#include "fprops.h"
#include "sat.h"
#include "zeroin.h"

#include <stdio.h>

//#define REF_DEBUG
#ifdef REF_DEBUG
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
	double T, p, rho, rho_f, rho_g, h1, h2, s1, s2, resid;
#ifdef REF_DEBUG
	double u;
#endif
	switch(ref->type){
	case FPROPS_REF_PHI0:
		MSG("Setting PHI0 reference state...");
		P->data->cp0->c = ref->data.phi0.c;
		P->data->cp0->m = ref->data.phi0.m;
		return 0;

	case FPROPS_REF_IIR:
		/* need to calculate the saturated liquid state at 0 deg C */
		T = 273.15 + 0;

		if(T > P->data->T_c)return 4000;
		if(T < P->data->T_t)return 5000;

		res = fprops_sat_T(T, &p, &rho_f, &rho_g, P);
		if(res)return 1000+res;

		h1 = fprops_h(T,rho_f,P,&res);
		if(res)return 2000+res;

		s1 = fprops_s(T,rho_f,P,&res);
		if(res)return 3000+res;

		h2 = 200e3;
		s2 = 1e3;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

		return 0;
	case FPROPS_REF_NBP:
		/* normal boiling point, need to solve for the temperature first! */
		res = fprops_sat_p(101.325e3, &T,&rho_f, &rho_g, P);
		if(res)return 1000 + res;
		MSG("Tsat(atm) = %f, rho_f = %f",T,rho_f);
		h1 = fprops_h(T,rho_f,P,&res);
		if(res)return 2000+res;
		s1 = fprops_s(T,rho_f,P,&res);
		if(res)return 3000+res;
		h2 = 0; s2 = 0;
		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		MSG("h at T,rhof = %f",fprops_h(T, rho_f,P,&res));
		return 0;

	case FPROPS_REF_TRHS:
		T = ref->data.trhs.T0;
		rho = ref->data.trhs.rho0;

		h1 = fprops_h(T,rho,P,&res);
		if(res)return 2000+res;

		s1 = fprops_s(T,rho,P,&res);
		if(res)return 3000+res;

		h2 = ref->data.trhs.h0;
		s2 = ref->data.trhs.s0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
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
			MSG("Check: p(T,rho) = %f", fprops_p(T,rho,P,&res));
		}

		h1 = fprops_h(T,rho,P,&res);
		if(res)return 2000+res;
		s1 = fprops_s(T,rho,P,&res);
		if(res)return 3000+res;

		h2 = ref->data.tpus.u0 + ref->data.tpus.p0 / rho;
		s2 = ref->data.tpus.s0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

#ifdef REF_DEBUG
		u = fprops_u(T,rho,P,&res);
		s2 = fprops_s(T,rho,P,&res);
		p = fprops_p(T,rho,P,&res);
		if(res)return 4000+res;
#endif
		MSG("Resulting reference values: u = %f, s = %f, p = %f kPa",u,s2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);

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
			MSG("Check: p(T,rho) = %f", fprops_p(T,rho,P,&res));
		}

		h1 = fprops_h(T,rho,P,&res);
		if(res)return 2000+res;
		s1 = fprops_s(T,rho,P,&res);
		if(res)return 3000+res;

		h2 = ref->data.tphs.h0;
		s2 = ref->data.tphs.s0;

		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;

		h2 = fprops_h(T,rho,P,&res);
		s2 = fprops_s(T,rho,P,&res);
		p = fprops_p(T,rho,P,&res);
		if(res)return 4000+res;

		MSG("Resulting reference values: h = %f, s = %f, p = %f kPa",h2,s2,p);
		MSG("...at T = %f K , rho = %f kg/m3",T, rho);

		return 0;


	case FPROPS_REF_TPF:
		T = P->data->T_t;
		res = fprops_triple_point(&p,&rho_f,&rho_g,P);
		if(res)return 1000+res;
		h1 = fprops_h(T,rho_f,P,&res);
		if(res)return 2000+res;
		s1 = fprops_s(T,rho_f,P,&res);
		if(res)return 3000+res;
		h2 = 0;
		s2 = 0;
		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		return 0;

	case FPROPS_REF_TPFU:
		T = P->data->T_t;
		res = fprops_triple_point(&p,&rho_f,&rho_g,P);
		if(res)return 1000+res;
		h1 = fprops_h(T,rho_f,P,&res);
		if(res)return 2000+res;
		s1 = fprops_s(T,rho_f,P,&res);
		if(res)return 3000+res;
		h2 = p / rho_f;
		s2 = 0;
		P->data->cp0->c = -(s2 - s1)/P->data->R;
		P->data->cp0->m = (h2 - h1)/P->data->R/P->data->T_c;
		return 0;

	default:
		fprintf(stderr,"%s: Unhandled case (type %d)\n",__func__,ref->type);
		return -1;
	}
}


double refstate_perr_Trho(double rho, void *user_data){
#define D ((RefStateTPData *)user_data)
	FpropsError err = 0;
	double p = fprops_p(D->T, rho, D->P, &err);
	if(err)return -1;
	return p - D->p;
#undef D
}


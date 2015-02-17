/*
	Python Bindings for FPROPS 2.x with support
	for numerous different equations of state.
*/
%module fprops
%feature("autodoc");

%{
#include "rundata.h"
#include "fprops.h"
#include "fluids.h"
#include "sat.h"
#include "common.h"
#include "solve_ph.h"
#include "refstate.h"
#include "filedata.h"

const ReferenceState REF_IIR = {FPROPS_REF_IIR};
const ReferenceState REF_NBP = {FPROPS_REF_NBP};
const ReferenceState REF_TPF = {FPROPS_REF_TPF};
const ReferenceState REF_TPFU = {FPROPS_REF_TPFU};


ReferenceState *REF_TRHS(double T0, double rho0, double h0, double s0){
	ReferenceState ref = {FPROPS_REF_TRHS,{.trhs={T0,rho0,h0,s0}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

ReferenceState *REF_TPUS(double T0, double p0, double u0, double s0){
	ReferenceState ref = {FPROPS_REF_TPUS,{.trhs={T0,p0,u0,s0}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

ReferenceState *REF_TPHS(double T0, double p0, double h0, double s0){
	ReferenceState ref = {FPROPS_REF_TPHS,{.trhs={T0,p0,h0,s0}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

ReferenceState *REF_PHI0(double c, double m){
	ReferenceState ref = {FPROPS_REF_PHI0,{.phi0={.c=c,.m=m}}};
	ReferenceState *p = FPROPS_NEW(ReferenceState);
	*p = ref;
	return p;
}

%}

%rename(fluid) fprops_fluid;
%rename(num_fluids) fprops_num_fluids;
%rename(get_fluid) fprops_get_fluid;
%rename(corr_avail) fprops_corr_avail;

// FIXME check for None return from fprops_fluid

PureFluid *fprops_fluid(char *name, const char *corrtype = NULL, const char *source = NULL);
PureFluid *fprops_get_fluid(int i);
int fprops_num_fluids();

//PureFluid *fprops_corr_avail(XXXXX, const char *corrtype);

// FIXME if fprops_fluid or fprops_get_fluid returns NULL, throw an Exception

// TODO can we make these into a python submodule instead?
const ReferenceState REF_IIR;
const ReferenceState REF_NBP;
const ReferenceState REF_TPF;
const ReferenceState REF_TPFU;
const ReferenceState *REF_TRHS(double T0, double rho0, double h0, double s0);
const ReferenceState *REF_TPUS(double T0, double p0, double u0, double s0);
const ReferenceState *REF_TPHS(double T0, double p0, double h0, double s0);
const ReferenceState *REF_PHI0(double c, double m);

%nodefaultctor;
/* FIXME a dtor will be needed... */
%nodefaultdtor;

%typemap(in,numinputs=0,noblock=1) FpropsError *err{
	FpropsError fprops___err = 0;
	$1 = &fprops___err;
};

%apply double *OUTPUT {double *rho_f, double *rho_g, double *rho, double *T, double *T_sat, double *p_sat};

%exception {
	//fprintf(stderr,"running action\n");
	$action
	if(fprops___err != 0){
		PyErr_SetString(PyExc_RuntimeError,fprops_error(fprops___err));
		return NULL;
	}
}

typedef struct{
	%extend{
		void set_ref(ReferenceState *ref, FpropsError *err){
			int res;
			res = fprops_set_reference_state($self, ref);
			if(res)*err = FPROPS_NUMERIC_ERROR;
		}
	
		double p(double T, double rho, FpropsError *err){
			return fprops_p(T,rho,$self,err);
		}
		double h(double T, double rho, FpropsError *err){
			return fprops_h(T,rho,$self,err);
		}
		double u(double T, double rho, FpropsError *err){
			return fprops_u(T,rho,$self,err);
		}
		double s(double T, double rho, FpropsError *err){
			return fprops_s(T,rho,$self,err);
		}
		double cp(double T, double rho, FpropsError *err){
			return fprops_cp(T,rho,$self,err);
		}
		double cv(double T, double rho, FpropsError *err){
			return fprops_cv(T,rho,$self,err);
		}
		double a(double T, double rho, FpropsError *err){
			return fprops_a(T,rho,$self,err);
		}
		double g(double T, double rho, FpropsError *err){
			return fprops_g(T,rho,$self,err);
		}
		double w(double T, double rho, FpropsError *err){
			return fprops_w(T,rho,$self,err);
		}
		double alphap(double T, double rho, FpropsError *err){
			return fprops_alphap(T,rho,$self,err);
		}
		double betap(double T, double rho, FpropsError *err){
			return fprops_betap(T,rho,$self,err);
		}
		double cp0(double T, FpropsError *err){
			return fprops_cp0(T,$self,err);
		}
		double dpdrho_T(double T, double rho, FpropsError *err){
			return fprops_dpdrho_T(T,rho,$self,err);
		}

		double solve_ph(double p, double h, double *rho, FpropsError *err){
			double T;
			int res = fprops_solve_ph(p,h,&T, rho, 0, $self);
			if(res)*err = FPROPS_NUMERIC_ERROR;
			return T; 
		}

		double sat_T(double T, double *rho_f, double *rho_g, FpropsError *err){
			int res;
			double p_sat;
			res = fprops_sat_T(T, &p_sat, rho_f, rho_g, $self);
			if(res)*err = FPROPS_SAT_CVGC_ERROR;
			return p_sat;
		}

          double sat_T_cubic(double T, double *rho_f, double *rho_g, FpropsError *err){
			int res;
			double p_sat;
			res = fprops_sat_T_cubic(T, &p_sat, rho_f, rho_g, $self);
			if(res)*err = FPROPS_SAT_CVGC_ERROR;
			return p_sat;
		}

		double sat_p(double p, double *rho_f, double *rho_g, FpropsError *err){
			int res;
			double T_sat;
			res = fprops_sat_p(p, &T_sat, rho_f, rho_g, $self);
			if(res)*err = FPROPS_SAT_CVGC_ERROR;
			return T_sat;
		}

		double sat_hf(double hf, double *p_sat, double *rho_f, double *rho_g, FpropsError *err){
			int res;
			double T_sat;
			res = fprops_sat_hf(hf, &T_sat, p_sat, rho_f, rho_g, $self);
			if(res)*err = FPROPS_SAT_CVGC_ERROR;
			return T_sat;
		}

		double triple_point(double *rho_f, double *rho_g, FpropsError *err){
			double p;
			//fprintf(stderr,"Triple point at T = %f\n",$self->data->T_t);
			if(fprops_triple_point(&p,rho_f,rho_g,$self)){
				*err = FPROPS_SAT_CVGC_ERROR;
			}
			return p;
		}

		double psat_T_xiang(double T,FpropsError *err){
			return fprops_psat_T_xiang(T,$self);
		}

		double psat_T_acentric(double T,FpropsError *err){
			return fprops_psat_T_acentric(T,$self);
		}

		double rhof_T_rackett(double T,FpropsError *err){
			return fprops_rhof_T_rackett(T,$self);
		}

		double rhog_T_chouaieb(double T,FpropsError *err){
			return fprops_rhog_T_chouaieb(T,$self);
		}
		

		%immutable;
		%exception;
		const double T_t;
		const double T_c;
		const double rho_c;
		const double p_c;
		const double omega;
		const int type;
		const char *name;
	}
} PureFluid;

%exception;

%{
const char *PureFluid_name_get(PureFluid *P){
	return P->name;
}
const double PureFluid_T_t_get(PureFluid *P){
	return P->data->T_t;
}
const double PureFluid_T_c_get(PureFluid *P){
	return P->data->T_c;
}
const double PureFluid_rho_c_get(PureFluid *P){
	return P->data->rho_c;
}
const double PureFluid_p_c_get(PureFluid *P){
	return P->data->p_c;
}
const double PureFluid_omega_get(PureFluid *P){
	return P->data->omega;
}
const int PureFluid_type_get(PureFluid *P){
	return P->type;
}
%}



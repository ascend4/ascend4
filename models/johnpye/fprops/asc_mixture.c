/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, June 25-August 14, 2015

	Exports functions for initial model of ideal-solution mixing
 */

#include "asc_mixture.h"

/*
	Register all functions that will be exported
 */
extern ASC_EXPORT int mixture_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,
			"FPROPS in general, and IN PARTICULAR this mixture module, "
			"are still EXPERIMENTAL.  Use with caution.");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox( #NAME \
			, asc_mixture_prepare      /* function to initialize  */ \
			, NAME##_calc              /* function to find value  */ \
			, (ExtBBoxFunc *)NULL      /* derivatives -- none     */ \
			, (ExtBBoxFunc *)NULL      /* hessian -- none         */ \
			, (ExtBBoxFinalFunc *)NULL /* finalization -- none    */ \
			, INPUTS,OUTPUTS \
			, NAME##_help              /* help text/documentation */ \
			, 0.0 \
		)
	
	/* CALCFN(mixture_p,2,1); */
	CALCFN(mixture_rho,2,1);
	CALCFN(mixture_phase_rho,3,1);
    CALCFN(mixture_comps_rho,4,1);
	CALCFN(mixture_u,2,1);
	CALCFN(mixture_phase_u,3,1);
    CALCFN(mixture_comps_u,4,1);
	CALCFN(mixture_h,2,1);
	CALCFN(mixture_phase_h,3,1);
    CALCFN(mixture_comps_h,4,1);
	CALCFN(mixture_cp,2,1);
	CALCFN(mixture_phase_cp,3,1);
    CALCFN(mixture_comps_cp,4,1);
	CALCFN(mixture_cv,2,1);
	CALCFN(mixture_phase_cv,3,1);
    CALCFN(mixture_comps_cv,4,1);

	CALCFN(mixture_s,2,1);
	CALCFN(mixture_phase_s,3,1);
    CALCFN(mixture_comps_s,4,1);
	CALCFN(mixture_g,2,1);
	CALCFN(mixture_phase_g,3,1);
    CALCFN(mixture_comps_g,4,1);
	CALCFN(mixture_a,2,1);
	CALCFN(mixture_phase_a,3,1);
    CALCFN(mixture_comps_a,4,1);

	CALCFN(mixture_count_phases,2,4);
	CALCFN(mixture_count_components,3,1);
	CALCFN(mixture_component_frac,4,1);
	CALCFN(mixture_component_cnum,4,1);
	CALCFN(mixture_dew_p,1,1);
	CALCFN(mixture_bubble_p,1,1);
	CALCFN(mixture_dew_T,1,1);
	CALCFN(mixture_bubble_T,1,1);

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunctionBlackBox result is %d\n",result);
	}
	return result;
}

/*
	Function to prepare persistent data, namely the MixtureSpec struct which 
	will be used in modeling the mixture.
 */
int asc_mixture_prepare(struct BBoxInterp *bbox, struct Instance *data, struct gl_list_t *arglist){

#define CHECK_TYPE(VAR,TYPE,NAME,TYPENAME) \
	if(InstanceKind(VAR)!=TYPE){ \
		ERROR_REPORTER_HERE(ASC_USER_ERROR \
			, "DATA member '%s' has type-value %#o, but must have %s (value %#o)" \
			, NAME, InstanceKind(VAR), TYPENAME, TYPE); \
		return 1; \
	}else{ \
		/* ERROR_REPORTER_HERE(ASC_USER_NOTE, "DATA member %s has correct type %s" */ \
				/* , NAME, TYPENAME); */ \
	}

#define CHECK_EXIST_TYPE(VAR,TYPE,NAME,TYPENAME) \
	if(! VAR){ \
		ERROR_REPORTER_HERE(ASC_USER_ERROR \
			, "Couldn't locate '%s' in DATA, please check usage", NAME); \
	} \
	CHECK_TYPE(VAR,TYPE,NAME,TYPENAME)

	unsigned i;
	struct Instance *npureinst, *compinst, *xinst, *typeinst, *srcinst;
	unsigned npure;

	mix_symbols[0] = AddSymbol("npure");
	mix_symbols[1] = AddSymbol("components");
	mix_symbols[2] = AddSymbol("xs");
	mix_symbols[3] = AddSymbol("type");
	mix_symbols[4] = AddSymbol("source");

	/* check existence of 'data' Instance */
	if(! data){
		ERROR_REPORTER_HERE(ASC_USER_ERROR, "Couldn't locate 'data', please check usage");
	}

	/* number of pure components -- required */
	npureinst = ChildByChar(data, mix_symbols[NPURE_SYM]);
	CHECK_EXIST_TYPE(npureinst, INTEGER_CONSTANT_INST, "npure", "'integer constant'");
	npure = (int *)(IC_INST(npureinst)->value);
	/* ERROR_REPORTER_HERE(ASC_USER_NOTE, "Number of pures is %i", npure); */

	const struct gl_list_t *comps_gl;
	const struct gl_list_t *xs_gl;
	/* const */ char *type = NULL
		, **comps = ASC_NEW_ARRAY(/* const */ char *, npure)
		, **srcs = ASC_NEW_ARRAY(/* const */ char *, npure);
	double *xs = ASC_NEW_ARRAY(double, npure);

	/* Component names -- required */
	compinst = ChildByChar(data, mix_symbols[COMP_SYM]);
	CHECK_EXIST_TYPE(compinst, ARRAY_INT_INST, "components"
			, "array indexed with integers");
	comps_gl = ARY_INST(compinst)->children;

	/*
		Component correlation type (equation of state) -- not required, so check 
		for existence before checking type and assigning to the char array 'type'.
	 */
	typeinst = ChildByChar(data, mix_symbols[TYPE_SYM]);
	if(typeinst){
		CHECK_TYPE(typeinst, SYMBOL_CONSTANT_INST, "type", "'symbol constant'");
		type = SCP(SYMC_INST(typeinst)->value); /* read 'typeinst' into a string */
		if(type && strlen(type)==0){
			char t[] = "pengrob";
			type = t;
		}
	}else{
		char t[] = "pengrob";
		type = t;
	}

	/*
		Data string representing source -- not required, so check for existence 
		before checking type and assigning to the char array 'source'.
	 */
	srcinst = ChildByChar(data, mix_symbols[SOURCE_SYM]);
	if(srcinst){
		CHECK_TYPE(srcinst, SYMBOL_CONSTANT_INST, "source", "'symbol constant'");
		srcs[0] = SCP(SYMC_INST(srcinst)->value); /* read 'srcinst' into a string */
		if(srcs[0] && strlen(srcs[0])==0){
			srcs[0] = NULL;
		}else if(!srcs[0]){
			srcs[0] = NULL;
		}
	}else{
		srcs[0] = NULL;
	}
	for(i=1;i<npure;i++){
		srcs[i] = srcs[0];
	}

	/* Mass fractions -- required */
	xinst = ChildByChar(data, mix_symbols[X_SYM]);
	CHECK_EXIST_TYPE(xinst, ARRAY_INT_INST, "xinst"
			, "array indexed with integers");
	xs_gl = ARY_INST(xinst)->children;

	/*
		Check that the lengths of the arrays 'comps_gl' and 'xs_gl' are equal, 
		and equal to 'npure'
	 */
	if(xs_gl->length!=npure){
		if(comps_gl->length==xs_gl->length){
			ERROR_REPORTER_HERE(ASC_USER_ERROR
					, "The components and mass fractions arrays both differ in length"
					"\n\tfrom the given number of components 'npure', but are equal in"
					"\n\tlength to each other.  Setting npure = length of the arrays...");
			npure = xs_gl->length;
		}else if(comps_gl->length!=npure){
			ERROR_REPORTER_HERE(ASC_USER_ERROR
					, "The components and mass fractions arrays both differ in length"
					"\n\tfrom the given number of components 'npure', and are not equal"
					"\n\tin length to each other.  Setting npure = (smallest length)...");
			double lens[] = {npure, xs_gl->length, comps_gl->length};
			npure = min_element(3, lens);
		}else{
			ERROR_REPORTER_HERE(ASC_USER_ERROR
					, "The mass fractions array differs in length from the given number"
					"\n\tof components 'npure' and the length of the components array."
					"\n\tSetting npure = (smallest length)...");
			double lens[] = {npure, xs_gl->length};
			npure = min_element(2, lens);
		}
	}else if(comps_gl->length!=npure){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
				, "The components array differs in length from the given number of"
				"\n\tcomponents 'npure' and the length of the mass fractions array."
				"\n\tSetting npure = (smallest length)...");
		double lens[] = {npure, xs_gl->length};
		npure = min_element(2, lens);
	}

	/* Read contents of 'comps_gl' and 'xs_gl' into 'comps_child' and 'xs_child' */
	for(i=0;i<npure;i++){
		comps[i] = SCP(SYMC_INST(InstanceChild(compinst, i+1))->value);
		xs[i] = RC_INST(InstanceChild(xinst, i+1))->value;
	}

	/* Create mixture specification in a MixtureSpec struct */
	MixtureError merr = MIXTURE_NO_ERROR;
	bbox->user_data = (void *) build_MixtureSpec(npure, xs, (void **)comps, type, srcs, &merr);

	return 0;
}

/* ---------------------------------------------------------------------
	Functions which calculate results
 */
#if 0
int mixture_p_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);
	
	ERROR_REPORTER_HERE(ASC_USER_NOTE
		, "Function 'mixture_p_calc' -- this function has no contents as yet.");

	outputs[0] = 0.0;

	return 0;
}
#endif

/* ---------------------------------------------------------------------
	Mixture property-calculation functions
 */
/*
	Find and return the overall mixture density
 */
int mixture_rho_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
#define NPHASE PS->phases
#define PTYPE PS->ph_type[i]
	CALCPREP(2,1);
	CALCFLASH;
	PhaseMixState *PM = fill_PhaseMixState(T, p, PS, MS);

	double rhos[MS->pures];
	outputs[0] = mixture_rho(PM, rhos);
#if 0
    unsigned i;
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The overall mixture density is %g", outputs[0]);
	for(i=0;i<NPHASE;i++){
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tThe density of %s phase is %g kg/m3"
				, MIX_PHASE_STRING(PTYPE), rhos[i]);
	}
#endif
	return 0;
#undef PTYPE
#undef NPHASE
}

/*
	Find and return the mixture density for a single phase
 */
int mixture_phase_rho_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(3,1);
	CALCFLASH;
	PhaseMixState *PM = fill_PhaseMixState(T, p, PS, MS);

	double rhos[PS->phases] /* individual phase densities */
        ;

	mixture_rho(PM, rhos);
	outputs[0] = rhos[((int) inputs[2]) - 1]; /* assign density of one phase to output */

	return 0;
}

int mixture_comps_rho_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
    CALCPREP(4,1);
    CALCFLASH;

    unsigned phase = ((int) inputs[2]) - 1 /* number of the phase */
        , comp = ((int) inputs[3]) - 1; /* number of the component */
    outputs[0] = PS->PH[phase]->rhos[comp];
    return 0;
}

/*
	Macros used to generate mixture-property functions automatically
 */
#define MIX_PROP_EXTFUNC(PROP) \
	int mixture_##PROP##_calc(struct BBoxInterp *bbox, int ninputs, int noutputs, \
			double *inputs, double *outputs, double *jacobian){ \
		CALCPREP(2,1); \
		CALCFLASH; \
        PhaseMixState *PM = fill_PhaseMixState(T, p, PS, MS); \
		double props[PS->phases]; /* internal by-phase property values */ \
		outputs[0] = mixture_##PROP(PM, props, &err); \
		return 0; \
	}

#define MIX_PHASE_EXTFUNC(PROP) \
	int mixture_phase_##PROP##_calc(struct BBoxInterp *bbox, int ninputs, int noutputs, \
			double *inputs, double *outputs, double *jacobian){ \
		CALCPREP(3,1); \
		CALCFLASH; \
        PhaseMixState *PM = fill_PhaseMixState(T, p, PS, MS); \
		double props[PS->phases]; /* internal by-phase property values */ \
		mixture_##PROP(PM, props, &err); \
		outputs[0] = props[((int) inputs[2]) - 1]; /* assign property of one phase to output */ \
		return 0; \
	}

#define MIX_COMPS_EXTFUNC(PROP) \
    int mixture_comps_##PROP##_calc(struct BBoxInterp *bbox, int ninputs, int noutputs, \
			double *inputs, double *outputs, double *jacobian){ \
        CALCPREP(4,1); \
        CALCFLASH; \
        unsigned phase = ((int) inputs[2]) - 1 /* number of the phase */ \
            , comp = ((int) inputs[3]) - 1; /* number of the component */ \
        /* Access the property function from FPROPS for an individual component */ \
        outputs[0] = fprops_##PROP((FluidState){T, PS->PH[phase]->rhos[comp] \
                , PS->PH[phase]->PF[comp]}, &err); \
        return 0; \
    }

MIX_PROP_EXTFUNC(u); MIX_PROP_EXTFUNC(h); MIX_PROP_EXTFUNC(cp);
MIX_PROP_EXTFUNC(cv); MIX_PROP_EXTFUNC(s); MIX_PROP_EXTFUNC(g);
MIX_PROP_EXTFUNC(a);

MIX_PHASE_EXTFUNC(u); MIX_PHASE_EXTFUNC(h); MIX_PHASE_EXTFUNC(cp);
MIX_PHASE_EXTFUNC(cv); MIX_PHASE_EXTFUNC(s); MIX_PHASE_EXTFUNC(g);
MIX_PHASE_EXTFUNC(a);

MIX_COMPS_EXTFUNC(u); MIX_COMPS_EXTFUNC(h); MIX_COMPS_EXTFUNC(cp);
MIX_COMPS_EXTFUNC(cv); MIX_COMPS_EXTFUNC(s); MIX_COMPS_EXTFUNC(g);
MIX_COMPS_EXTFUNC(a);

/* ---------------------------------------------------------------------
	Phase-equilibrium functions
 */
/*
	Returns the number of phases and mass fraction for each phase 
	(supercritical, vapor, and liquid).  Mass fraction is zero if the phase is 
	not present.
 */
int mixture_count_phases_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(2,4);
	CALCFLASH;

	unsigned i;
	for(i=1;i<4;i++){
		/* Set all outputs to zero initially */
		outputs[i] = 0.0;
	}
	outputs[0] = (double) PS->phases;

	ERROR_REPORTER_HERE(ASC_USER_NOTE, "There are %u phases", PS->phases);
	for(i=0;i<PS->phases;i++){
		/* For all phases present, set the corresponding phase-fraction output 
		   equal to the mass fraction of the phase. */
		outputs[i+1] = PS->ph_frac[i];

		ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tPhase number %u is %s", i+1,
				MIX_PHASE_STRING(PS->ph_type[i]));
	}
	return 0;
}

/*
	Find and return the number of components of some n_th phase within the mixture.
 */
int mixture_count_components_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(3,1);
	CALCFLASH;

	unsigned n = ((unsigned) inputs[2]) - 1;
	if(n < PS->phases){
		outputs[0] = (double) PS->PH[n]->pures; /* the number of pures */
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR, "There is no phase number %u", n+1);
		outputs[0] = 0.0;
	}
	return 0;
}

/*
	Find and return the mass fraction of some j_th component within the i_th 
	phase of the mixture.
 */
int mixture_component_frac_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(4,1);
	CALCFLASH;

	unsigned i = ((unsigned) inputs[2]) - 1
		, j = ((unsigned) inputs[3]) - 1;
	if(i < PS->phases){
		if(j < PS->PH[i]->pures){
			outputs[0] = (double) PS->PH[i]->xs[j];
		}else{
			ERROR_REPORTER_HERE(ASC_USER_ERROR
					, "There is no component number %u in phase %u", j+1, i+1);
			outputs[0] = 0.0;
		}
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR, "There is no phase number %u", i+1);
		outputs[0] = 0.0;
	}
	return 0;
}

/*
	Find and return the value of the j_th index in member 'c' of some i_th phase 'PH' 
	within the mixture (that is, 'PS->PH[i]->c[j]').
 */
int mixture_component_cnum_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(4,1);
	CALCFLASH;

	unsigned i = ((unsigned) inputs[2]) - 1
		, j = ((unsigned) inputs[3]) - 1;
	if(i < PS->phases){
		if(j < PS->PH[i]->pures){
			outputs[0] = (double) PS->PH[i]->c[j];
		}else{
			ERROR_REPORTER_HERE(ASC_USER_ERROR
					, "There is no component number %u in phase %u", j+1, i+1);
			outputs[0] = 0.0;
		}
	}else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR, "There is no phase number %u", i+1);
		outputs[0] = 0.0;
	}
	return 0;
}

/*
	Find and return the mixture dew pressure at some temperature.
 */
int mixture_dew_p_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);

	int result;
	double T = inputs[0] /* the mixture temperature */
		, p_d            /* the mixture dew pressure (if there is any) */
		, tol = MIX_XTOL /* tolerance to use in solving for dew pressure */
		;

	result = mixture_dew_pressure(&p_d, MS, T, tol, &err);
	if(result){
		switch(result){
			case 1:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The dew pressure converged on a non-solution point.");
				break;
			case 2:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The dew pressure converged on Infinity or NaN.");
				break;
			case 3:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The root-finding algorithm that searches for the dew pressure "
						"\nfailed to converge in the maximum number of iterations.");
				break;
			case 4:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "There is no dew pressure; all components are supercritical at "
						"\ntemperature %g K.", T);
				break;
		}
		outputs[0] = MIN_P;
		return 1;
	}else{
		outputs[0] = p_d;
		return 0;
	}
}

/*
	Find and return the mixture bubble pressure at some temperature.
 */
int mixture_bubble_p_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);

	int result;
	double T = inputs[0] /* the mixture temperature */
		, p_b            /* the mixture bubble pressure (if there is any) */
		, tol = MIX_XTOL /* tolerance to use in solving for bubble pressure */
		;

	result = mixture_bubble_pressure(&p_b, MS, T, tol, &err);
	if(result){
		switch(result){
			case 1:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The bubble pressure converged on a non-solution point.");
				break;
			case 2:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The bubble pressure converged on Infinity or NaN.");
				break;
			case 3:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The root-finding algorithm that searches for the bubble pressure "
						"\nfailed to converge in the maximum number of iterations.");
				break;
			case 4:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "There is no bubble pressure; all components are supercritical at "
						"\ntemperature %g K.", T);
				break;
		}
		outputs[0] = MIN_P;
		return 1;
	}else{
		outputs[0] = p_b;
		return 0;
	}
}

/*
    Find and return the mixture dew temperature at some pressure.
 */
int mixture_dew_T_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);

    int result;
	double p = inputs[0] /* the mixture pressure */
		, T_d            /* the mixture dew temperature (if there is any) */
		, tol = MIX_XTOL /* tolerance to use in solving for dew temperature */
		;

	result = mixture_dew_temperature(&T_d, MS, p, tol, &err);
	if(result){
		switch(result){
			case 1:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The dew temperature converged on a non-solution point");
				break;
			case 2:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The dew temperature converged on Infinity or NaN.");
				break;
			case 3:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The root-finding algorithm that searches for the dew temperature "
						"\nfailed to converge in the maximum number of iterations.");
				break;
			case 4:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "There is no dew temperature; all components are supercritical at "
						"\npressure %.2f Pa.", p);
				break;
		}
		outputs[0] = MIN_T;
		return 1;
	}else{
		outputs[0] = T_d;
		return 0;
	}
}

/*
    Find and return the mixture bubble temperature at some pressure.
 */
int mixture_bubble_T_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);

    int result;
	double p = inputs[0] /* the mixture pressure */
		, T_b            /* the mixture bubble temperature (if there is any) */
		, tol = MIX_XTOL /* tolerance to use in solving for dew temperature */
		;

	result = mixture_bubble_temperature(&T_b, MS, p, tol, &err);
	if(result){
		switch(result){
			case 1:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The bubble temperature converged on a non-solution point");
				break;
			case 2:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The bubble temperature converged on Infinity or NaN.");
				break;
			case 3:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "The root-finding algorithm that searches for the bubble "
						"\ntemperature failed to converge in the maximum number of "
						"\niterations.");
				break;
			case 4:
				ERROR_REPORTER_HERE(ASC_USER_ERROR
						, "There is no bubble temperature; all components are supercritical "
						"\nat pressure %.2f Pa.", p);
				break;
		}
		outputs[0] = MIN_T;
		return 1;
	}else{
		outputs[0] = T_b;
		return 0;
	}
}


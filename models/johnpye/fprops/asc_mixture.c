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
	by Jacob Shealy, June 25-, 2015

	Exports functions for initial model of ideal-solution mixing
 */

#include <ascend/utilities/error.h>
#include <ascend/general/platform.h>
#include <ascend/general/list.h>
#include <ascend/compiler/extfunc.h>
#include <ascend/compiler/parentchild.h>
/*
	#include <ascend/compiler/child.h>
	#include <ascend/compiler/childinfo.h>
 */
/* #include <ascend/compiler/slist.h> */
/* #include <ascend/compiler/type_desc.h> */
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instance_enum.h>
#include <ascend/compiler/instance_types.h>
#include <ascend/compiler/instmacro.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/sets.h>
#include <ascend/compiler/arrayinst.h>

/* Code that is wrapped */
#include "mixtures/mixture_properties.h"
#include "mixtures/mixture_generics.h"
#include "mixtures/mixture_prepare.h"
#include "mixtures/mixture_phases.h"
#include "mixtures/mixture_struct.h"

#ifndef ASC_EXPORT
#error "ASC_EXPORT not found -- where is it?"
#endif

#define NULL_STR(STR) (STR==NULL) ? "NULL" : STR

/*
	Forward Declarations
 */
ExtBBoxInitFunc asc_mixture_prepare;
/* ExtBBoxFunc mixture_p_calc; */
#define MIX_EXTFUNC(NAME) ExtBBoxFunc mixture_##NAME##_calc;
#define MIX_PROP_EXTFUNC(PROP) MIX_EXTFUNC(PROP) MIX_EXTFUNC(phase_##PROP)

MIX_PROP_EXTFUNC(rho); MIX_PROP_EXTFUNC(u); MIX_PROP_EXTFUNC(h); MIX_PROP_EXTFUNC(cp);
MIX_PROP_EXTFUNC(cv); MIX_PROP_EXTFUNC(s); MIX_PROP_EXTFUNC(g); MIX_PROP_EXTFUNC(a);
#if 0
ExtBBoxFunc mixture_rho_calc;
ExtBBoxFunc mixture_phase_rho_calc;
ExtBBoxFunc mixture_u_calc;
ExtBBoxFunc mixture_phase_u_calc;
ExtBBoxFunc mixture_h_calc;
ExtBBoxFunc mixture_phase_h_calc;
ExtBBoxFunc mixture_cp_calc;
ExtBBoxFunc mixture_phase_cp_calc;
ExtBBoxFunc mixture_cv_calc;
ExtBBoxFunc mixture_phase_cv_calc;

ExtBBoxFunc mixture_s_calc;
ExtBBoxFunc mixture_phase_s_calc;
ExtBBoxFunc mixture_g_calc;
ExtBBoxFunc mixture_phase_g_calc;
ExtBBoxFunc mixture_a_calc;
ExtBBoxFunc mixture_phase_a_calc;

ExtBBoxFunc mixture_flash_phases_calc;
ExtBBoxFunc mixture_flash_component_calc;
ExtBBoxFunc mixture_bubble_p_calc;
ExtBBoxFunc mixture_dew_p_calc;

ExtBBoxFunc mixture_phase_components_calc;
ExtBBoxFunc mixture_component_cnum_calc;
#endif
MIX_EXTFUNC(flash_phases); MIX_EXTFUNC(flash_component); MIX_EXTFUNC(bubble_p);
MIX_EXTFUNC(dew_p); MIX_EXTFUNC(phase_components); MIX_EXTFUNC(component_cnum);

/*
	Global Variables
 */
static symchar *mix_symbols[6];
enum Symbol_Enum {NPURE_SYM, COMP_SYM, X_SYM, TYPE_SYM, SOURCE_SYM};

typedef struct UserData_Struct {
	MixtureSpec *MS;
	struct Instance *xinst;
} UsrData;

#define MIX_HELP_TEXT(NAME) "Calculate overall " NAME " of the mixture, using ideal-solution assumption."
#define MIX_PHASE_HELP_TEXT(NAME) "Calculate " NAME " of the mixture for a single phase, using ideal-solution assumption."

#define MIX_HELP_DECL(PROP,MESSAGE) static const char *mixture_##PROP##_help = MESSAGE;
#define MIX_PH_HELP_DECL(PROP,MESSAGE) static const char *mixture_phase_##PROP##_help = MESSAGE;

#define MIX_HELP_FUNC(PROP,NAME) MIX_HELP_DECL(PROP, MIX_HELP_TEXT(NAME)) \
								 MIX_PH_HELP_DECL(PROP, MIX_PHASE_HELP_TEXT(NAME))

/* static const char *mixture_p_help = "Calculate pressure for the mixture, using ideal solution assumption, and report if pressure is inconsistent among the densities."; */
MIX_HELP_FUNC(rho, "density");
MIX_HELP_FUNC(u, "internal energy");
MIX_HELP_FUNC(h, "enthalpy");
MIX_HELP_FUNC(cp, "constant-pressure heat capacity");
MIX_HELP_FUNC(cv, "contant-volume heat capacity");
MIX_HELP_FUNC(s, "entropy");
MIX_HELP_FUNC(g, "Gibbs energy");
MIX_HELP_FUNC(a, "Helmholtz energy");

MIX_HELP_DECL(flash_phases
		, "Calculate and return number of phases in the mixture, and mass fraction of the mixture in each phase.");
MIX_HELP_DECL(flash_component
		, "Return mass or mole fraction of a component within a phase in the mixture.");
MIX_HELP_DECL(bubble_p
		, "Return bubble pressure of the mixture, using ideal-solution assumption.");
MIX_HELP_DECL(dew_p
		, "Return dew pressure of the mixture, using ideal-solution assumption.");
MIX_HELP_DECL(phase_components
		, "Return number of components within a phase of the mixture.");
MIX_HELP_DECL(component_cnum
		, "Return index that gives the location of a component from a phase, within "
		"a mixture specification (MixtureSpec)");

/*
	Register all functions that will be exported
 */
extern ASC_EXPORT int mixture_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,
			"FPROPS in general, and IN PARTICULAR this mixture module, "
			"are still EXPERIMENTAL.  Use with caution.");

#define QQZX 3
	
#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox( #NAME \
			, asc_mixture_prepare \
			, NAME##_calc              /* value */ \
			, (ExtBBoxFunc *)NULL      /* derivatives -- none */ \
			, (ExtBBoxFunc *)NULL      /* hessian -- none */ \
			, (ExtBBoxFinalFunc *)NULL /* finalization -- none */ \
			, INPUTS,OUTPUTS \
			, NAME##_help              /* help text */ \
			, 0.0 \
		)
	
	/* CALCFN(mixture_p,2,1); */
	CALCFN(mixture_rho,2,1);
	CALCFN(mixture_phase_rho,3,1);
	CALCFN(mixture_u,2,1);
	CALCFN(mixture_phase_u,3,1);
	CALCFN(mixture_h,2,1);
	CALCFN(mixture_phase_h,3,1);
	CALCFN(mixture_cp,2,1);
	CALCFN(mixture_phase_cp,3,1);
	CALCFN(mixture_cv,2,1);
	CALCFN(mixture_phase_cv,3,1);

	CALCFN(mixture_s,2,1);
	CALCFN(mixture_phase_s,3,1);
	CALCFN(mixture_g,2,1);
	CALCFN(mixture_phase_g,3,1);
	CALCFN(mixture_a,2,1);
	CALCFN(mixture_phase_g,3,1);

	CALCFN(mixture_flash_phases,2,4);
	CALCFN(mixture_flash_component,4,1);
	CALCFN(mixture_phase_components,3,1);
	CALCFN(mixture_component_cnum,4,1);
	CALCFN(mixture_bubble_p,1,1);
	CALCFN(mixture_dew_p,1,1);

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunctionBlackBox result is %d\n",result);
	}
	return result;
}

/*
	Function which prepares persistent data
 */
int asc_mixture_prepare(struct BBoxInterp *bbox, struct Instance *data, struct gl_list_t *arglist){

#define II i+1 /* index increased by one, counts from one for Instance children */

#define CHECK_TYPE(VAR,TYPE,NAME,TYPENAME) \
	if(InstanceKind(VAR)!=TYPE){ \
		ERROR_REPORTER_HERE(ASC_USER_ERROR \
			, "DATA member '%s' has type-value %#o, but must have %s (value %#o)" \
			, NAME, InstanceKind(VAR), TYPENAME, TYPE); \
		return 1; \
	}else{ \
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "DATA member %s has correct type %s" \
				, NAME, TYPENAME); \
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
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "Number of pures is %i", npure);

	const struct gl_list_t *comps_gl;
	const struct gl_list_t *xs_gl;
	const char *type = NULL
		, **comps = ASC_NEW_ARRAY(const char *, npure)
		, **srcs = ASC_NEW_ARRAY(const char *, npure);
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
		comps[i] = SCP(SYMC_INST(InstanceChild(compinst, II))->value);
		xs[i] = RC_INST(InstanceChild(xinst, II))->value;
		/* xs[i] = 0.0; */
	}

	/* Create mixture specification in a MixtureSpec struct */
	MixtureSpec *MS = ASC_NEW(MixtureSpec);
	MixtureError merr = MIXTURE_NO_ERROR;

#if 0
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The location of the MixtureSpec is %p", MS);
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The number of components is %u", npure);
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "  -- from 'comps_gl', is %lu", comps_gl->length);
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "  -- from 'xs_gl', is %lu", xs_gl->length);
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The equation of state used is %s", NULL_STR(type));
	for(i=0;i<npure;i++){
		ERROR_REPORTER_HERE(ASC_USER_NOTE
				, "\tFor component #%i, %s, the source is %s and the mass fraction is %g"
				, i, comps[i], NULL_STR(srcs[i]), xs[i]);
	}
#endif

	mixture_specify(MS, npure, xs, (const void **)comps, type, srcs, &merr);
	bbox->user_data = (void *) MS;

	return 0;
}

/*
	Functions which calculate results
 */
#define CALCPREP(NIN,NOUT) \
	if(ninputs!=NIN){ \
		return -1; \
	} \
	if(noutputs!=NOUT){ \
		return -2; \
	} \
	if(inputs==NULL){ \
		return -3; \
	} \
	if(outputs==NULL){ \
		return -4; \
	} \
	if(bbox==NULL){ \
		return -5; \
	} \
	FpropsError err=FPROPS_NO_ERROR; \
	MixtureSpec *MS = (MixtureSpec *)bbox->user_data;

#define CALCFLASH \
	PhaseSpec *PS = new_PhaseSpec(MS->pures, 3); \
	double T = inputs[0] \
		, p = inputs[1]; \
	mixture_flash(PS, MS, T, p, &err); \
	mixture_rhos_sat(PS, T, p, &err); \
	PhaseMixState *PM = fill_PhaseMixState(T, p, PS, MS);

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

	unsigned i;

	double rhos[MS->pures];
	outputs[0] = mixture_rho(PM, rhos);
#if 0
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

	double rhos[PS->phases] /* individual phase densities */
		, rho;              /* the overall mixture density */

	rho = mixture_rho(PM, rhos);
	outputs[0] = rhos[((int) inputs[2]) - 1]; /* assign density of one phase to output */

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
		double props[PS->phases]; /* internal by-phase property values */ \
		outputs[0] = mixture_##PROP(PM, props, &err); \
		return 0; \
	}

#define MIX_PHASE_EXTFUNC(PROP) \
	int mixture_phase_##PROP##_calc(struct BBoxInterp *bbox, int ninputs, int noutputs, \
			double *inputs, double *outputs, double *jacobian){ \
		CALCPREP(3,1); \
		CALCFLASH; \
		double props[PS->phases] /* internal by-phase property values */ \
			, prop;              /* overall mixture property value */ \
		prop = mixture_##PROP(PM, props, &err); \
		outputs[0] = props[((int) inputs[2]) - 1]; /* assign property of one phase to output */ \
		return 0; \
	}

MIX_PROP_EXTFUNC(u); MIX_PROP_EXTFUNC(h); MIX_PROP_EXTFUNC(cp); MIX_PROP_EXTFUNC(cv);
MIX_PROP_EXTFUNC(s); MIX_PROP_EXTFUNC(g); MIX_PROP_EXTFUNC(a);

MIX_PHASE_EXTFUNC(u); MIX_PHASE_EXTFUNC(h); MIX_PHASE_EXTFUNC(cp);
MIX_PHASE_EXTFUNC(cv); MIX_PHASE_EXTFUNC(s); MIX_PHASE_EXTFUNC(g);
MIX_PHASE_EXTFUNC(a);

/* ---------------------------------------------------------------------
	Phase-equilibrium functions
 */
/*
	Returns the number of phases and mass fraction for each phase 
	(supercritical, vapor, and liquid).  Mass fraction is zero if the phase is 
	not present.
 */
int mixture_flash_phases_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(2,4);
	CALCFLASH;

	unsigned i;
	for(i=1;i<4;i++){
		/* Set all phase mass fraction outputs to zero initially */
		outputs[i] = 0.0;
	}

	ERROR_REPORTER_HERE(ASC_USER_NOTE, "There are %u phases", PS->phases);
	for(i=0;i<PS->phases;i++){
		/* For all phases present, set the corresponding phase-fraction output 
		   equal to the mass fraction of the phase. */
		outputs[i+1] = PS->ph_frac[i];
		ERROR_REPORTER_HERE(ASC_USER_NOTE, "\tPhase number %u is %s", i+1,
				MIX_PHASE_STRING(PS->ph_type[i]));
	}
	outputs[0] = (double) PS->phases;

	return 0;
}

/*
	Find and return the mass fraction of some j_th component within the i_th 
	phase of the mixture.
 */
int mixture_flash_component_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
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
	Find and return the number of components of some n_th phase within the mixture.
 */
int mixture_phase_components_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
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
	Find and return the mixture bubble pressure at some temperature.
 */
int mixture_bubble_p_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);

	double T = inputs[0]; /* the mixture temperature */

	outputs[0] = bubble_pressure(MS, T, &err);
	return 0;
}

/*
	Find and return the mixture dew pressure at some temperature.
 */
int mixture_dew_p_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);

	double T = inputs[0]; /* the mixture temperature */

	outputs[0] = dew_pressure(MS, T, &err);
	return 0;
}

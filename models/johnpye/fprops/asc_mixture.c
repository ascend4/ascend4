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
ExtBBoxFunc mixture_p_calc;
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

/*
	Global Variables
 */
static symchar *mix_symbols[5];
enum Symbol_Enum {NPURE_SYM, COMP_SYM, X_SYM, TYPE_SYM, SOURCE_SYM};

typedef struct UserData_Struct {
	MixtureSpec *MS;
	struct Instance *xinst;
} UsrData;

#define MIX_HELP(PROP) "Calculate overall " PROP " of the mixture, using ideal-solution assumption."
#define MIX_PHASE_HELP(PROP) "Calculate " PROP " of the mixture for a single phase, using ideal-solution assumption."

static const char *mixture_p_help = "Calculate pressure for the mixture, using ideal solution assumption, and report if pressure is inconsistent among the densities.";
static const char *mixture_rho_help = MIX_HELP("density");
static const char *mixture_phase_rho_help = MIX_PHASE_HELP("density");
static const char *mixture_u_help = MIX_HELP("internal energy");
static const char *mixture_phase_u_help = MIX_PHASE_HELP("internal energy");
static const char *mixture_h_help = MIX_HELP("enthalpy");
static const char *mixture_phase_h_help = MIX_PHASE_HELP("enthalpy");

static const char *mixture_cp_help = MIX_HELP("constant-pressure heat capacity");
static const char *mixture_phase_cp_help = MIX_PHASE_HELP("constant-pressure heat capacity");
static const char *mixture_cv_help = MIX_HELP("constant-volume heat capacity");
static const char *mixture_phase_cv_help = MIX_PHASE_HELP("constant-volume heat capacity");

static const char *mixture_s_help = MIX_HELP("entropy");
static const char *mixture_phase_s_help = MIX_PHASE_HELP("entropy");
static const char *mixture_g_help = MIX_HELP("Gibbs energy");
static const char *mixture_phase_g_help = MIX_PHASE_HELP("Gibbs energy");
static const char *mixture_a_help = MIX_HELP("Helmholtz energy");
static const char *mixture_phase_a_help = MIX_PHASE_HELP("Helmholtz energy");

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
	
	CALCFN(mixture_p,2,1);
	CALCFN(mixture_rho,2,1);
	CALCFN(mixture_phase_rho,3,1);
	CALCFN(mixture_u,2,1);
	CALCFN(mixture_phase_u,3,1);
	CALCFN(mixture_h,2,1);
	CALCFN(mixture_phase_h,3,1);
	// CALCFN(mixture_cp,2,1);
	// CALCFN(mixture_phase_cp,3,1);
	// CALCFN(mixture_cv,2,1);
	// CALCFN(mixture_phase_cv,3,1);

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
	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The pointer to the instance 'xs' is %p", xinst);

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

	/* Mixture model (ideal or real) -- not required */

	ERROR_REPORTER_HERE(ASC_USER_NOTE, "Mark 1");

	/* Read contents of 'comps_gl' and 'xs_gl' into 'comps_child' and 'xs_child' */
	for(i=0;i<npure;i++){
		comps[i] = SCP(SYMC_INST(InstanceChild(compinst, II))->value);
		xs[i] = RC_INST(InstanceChild(xinst, II))->value;
		/* xs[i] = 0.0; */
	}

	/* Create mixture specification in a MixtureSpec struct */
	MixtureSpec *MS = ASC_NEW(MixtureSpec);
	MixtureError merr = MIXTURE_NO_ERROR;

#if 1
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
	const MixtureSpec *MS = (const MixtureSpec *)bbox->user_data; \
	const PhaseSpec *PS = new_PhaseSpec(MS->pures, 3); \
	double T = inputs[0] \
		, p = inputs[1]; \
	mixture_flash(PS, MS, T, p, &err); \
	mixture_rhos_sat(PS, T, p, &err); \
	PhaseMixState *PM = fill_PhaseMixState(T, p, PS, MS);

int mixture_p_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(1,1);
	
	ERROR_REPORTER_HERE(ASC_USER_NOTE
		, "Function 'mixture_p_calc' -- this function has no contents as yet.");

	outputs[0] = 0.0;

	return 0;
}

int mixture_rho_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(2,1);

	unsigned i;
#if 0
	MixtureSpec *MS = (MixtureSpec *) bbox->user_data;
	PhaseSpec *PS = new_PhaseSpec(MS->pures, 3);

	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The number of pures is %u", MS->pures);
	for(i=0;i<MS->pures;i++){
		ERROR_REPORTER_HERE(ASC_USER_NOTE
				, "Mass fraction number %u is %g", i+1, MS->Xs[0]);
	}

	double T = inputs[0]
		, p = inputs[1];

	mixture_flash(PS, MS, T, p, &err);

	PhaseMixState *PM = ASC_NEW(PhaseMixState);
	PM->T = T;
	PM->p = p;
	PM->MS = MS;
	PM->PS = PS;
#endif

	double rhos[MS->pures];
	outputs[0] = mixture_rho(PM, rhos);

	ERROR_REPORTER_HERE(ASC_USER_NOTE, "The overall mixture density is %g", outputs[0]);
	for(i=0;i<PS->phases;i++){
		ERROR_REPORTER_HERE(ASC_USER_NOTE
				, "The density of phase number %u is %g kg/m3", i, rhos[i]);
	}

	return 0;
}

int mixture_phase_rho_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(3,1);

	double rhos[PS->phases] /* individual phase densities */
		, rho;              /* the overall mixture density */

	rho = mixture_rho(PM, rhos);
	outputs[0] = rhos[(int) inputs[2]]; /* assign density of one phase to output */

	return 0;
}

#define MIX_PROP_EXTFUNC(PROP) \
	int mixture_##PROP##_calc(struct BBoxInterp *bbox, int ninputs, int noutputs, \
			double *inputs, double *outputs, double *jacobian){ \
		CALCPREP(2,1); \
		double props[PS->phases]; /* internal by-phase property values */ \
		outputs[0] = mixture_##PROP(PM, props, &err); \
		return 0; \
	}

#define MIX_PHASE_EXTFUNC(PROP) \
	int mixture_phase_##PROP##_calc(struct BBoxInterp *bbox, int ninputs, int noutputs, \
			double *inputs, double *outputs, double *jacobian){ \
		CALCPREP(3,1); \
		double props[PS->phases] /* internal by-phase property values */ \
			, prop;              /* overall mixture property value */ \
		prop = mixture_##PROP(PM, props, &err); \
		outputs[0] = props[(int) inputs[2]]; /* assign property of one phase to output */ \
		return 0; \
	}

MIX_PROP_EXTFUNC(u); MIX_PROP_EXTFUNC(h);
MIX_PHASE_EXTFUNC(u); MIX_PHASE_EXTFUNC(h);

#if 0
int mixture_u_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(2,1);

	double u_s[PS->phases]; /* individual phase internal energies */
	outputs[0] = mixture_u(PM, u_s, &err);

	return 0;
}

int mixture_phase_u_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(3,1);

	double u_s[PS->phases] /* individual phase internal energies */
		, u;               /* the overall mixture internal energy */

	u = mixture_u(PM, u_s, &err);
	outputs[0] = u_s[(int) inputs[2]]; /* assign energy of one phase to output */

	return 0;
}

int mixture_h_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(2,1);

	return 0;
}

int mixture_phase_h_calc(struct BBoxInterp *bbox, int ninputs, int noutputs,
		double *inputs, double *outputs, double *jacobian){
	CALCPREP(3,1);

	return 0;
}
#endif

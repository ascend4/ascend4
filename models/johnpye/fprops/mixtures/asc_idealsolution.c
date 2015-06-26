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

	Function exports for initial model of ideal-solution mixing
 */

#include <ascend/utilities/error.h>
#include <ascend/general/platform.h>
#include <ascend/general/extfunc.h>
#include <ascend/general/list.h>
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
#include "init_mixfuncs.h" /* rewrite to ideal_solution.h, ideal_solution.c */

#ifndef ASC_EXPORT
#error "ASC_EXPORT not found -- where is it?"
#endif

/*
	Forward Declarations
 */
ExtBBoxInitFunc asc_mixture_prepare;
ExtBBoxFunc mixture_p_calc;
ExtBBoxFunc mixture_rho_calc;

/*
	Global Variables
 */
static symchar *mix_symbols[5];
enum Symbol_Enum {NPURE_SYM, COMP_SYM, X_SYM, TYPE_SYM, SOURCE_SYM};

static const char *mixture_p_help = "Calculate pressure for the mixture, using ideal solution assumption, and report if pressure is inconsistent among the densities"
static const char *mixture_rho_help = "Calculate overall density of the solution, using ideal-solution assumption"

/*
	Register all functions that will be exported
 */
extern ASC_EXPORT int mixture_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,
			"FPROPS in general, and IN PARTICULAR this mixture module, "
			"are still EXPERIMENTAL.  Use with caution.");
#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#Name \
			, asc_mixture_prepare \
			, NAME##_calc              /* value */ \
			, (ExtBBoxFunc *)NULL      /* derivatives -- none */ \
			, (ExtBBoxFunc *)NULL      /* hessian -- none */ \
			, (ExtBBoxFinalFunc *)NULL /* finalization -- none */ \
			, INPUTS,OUTPUTS \
			, NAME##_help              /* help text */ \
			, 0.0 \
		)
	
	CALCFN(mixture_p,1,1);
	CALCFN(mixture_rho,2,1);

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunctionBlackBox result is %d\n",result);
	}
	return result;
}

/*
	Function which prepares persistent data
 */
int asc_mixture_prepare(struct BBoxInterp *bbox, struct Instance *data, struct g_list_t *arglist){

#define CHECK_TYPE(VAR,TYPE,NAME,TYPENAME) \
	if(InstanceKind(VAR)!=TYPE){ \
		ERROR_REPORTER_HERE(ASC_USER_ERROR \
			, "DATA member '%s' has type-value %x, but must have %s (value %x)" \
			, NAME, InstanceKind(VAR), TYPENAME, TYPE); \
		return 1; \
	}

#define CHECK_EXIST_TYPE(VAR,TYPE,NAME,TYPENAME) \
	if(! VAR){ \
		ERROR_REPORTER_HERE(ASC_USER_ERROR \
			, "Couldn't locate '%s' in DATA, please check usage", NAME); \
	} \
	CHECK_TYPE(VAR,TYPE,NAME,TYPENAME)

	struct Instance *npureinst, *compinst, *xinst, *typeinst, *srcinst;
	const unsigned npure;

	mix_symbols[0] = AddSymbol("npure");
	mix_symbols[1] = AddSymbol("components");
	mix_symbols[2] = AddSymbol("xs");
	mix_symbols[3] = AddSymbol("type");
	mix_symbols[4] = AddSymbol("source");

	/* number of pure components -- required */
	npureinst = ChildByChar(data, mix_symbols[NPURE_SYM]);
	CHECK_EXIST_TYPE(npureinst,INTEGER_CONSTANT_INST, "npure", "integer_constant");
	/* if(!npureinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			, "Couldn't locate 'npure' in DATA, please check usage");
		return 1;
	}
	if(InstanceKind(npureinst)!=INTEGER_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			, "DATA member 'npure' was %x, but must be an integer_constant %x",
			InstanceKind(npureinst), INTEGER_CONSTANT_INST);
	} */
	npure = (int *)IC_INST(npureinst)->value;

	const gl_list_t *comps_gl;
	const char *comps[npure], *type=NULL, *src=NULL;
	const double *xs[npure];

	/* component names -- required */
	compinst = ChildByChar(data, mix_symbols[COMP_SYM]);
	/* if(!compinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			, "Couldn't locate 'component' in DATA, please check usage");
	}
	if(InstanceKind(compinst)!=ARRAY_ENUM_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			, "DATA member 'component' was of type number %x, "
			"but must be an array of symbol_constant(s) %x",
			InstanceKind(compinst), )
	} */
	CHECK_EXIST_TYPE(compinst, ARRAY_ENUM_INST, "components",
			"array of symbol_constant(s)");
	comps_gl = ARY_INST(compinst)->children;

	/* component correlation type (equation of state) -- not required */
	typeinst = ChildByChar(data, mix_symbols[TYPE_SYM]);
	if(typeinst){
		CHECK_TYPE(typeinst, SYMBOL_CONSTANT_INST, "type", "symbol_constant");
		type = SCP(SYMC_INST(typeinst)->value);
		if(type && strlen(type)==0){
			char t[] = "helmholtz";
			type = t;
		}
	}

	/* source data string -- not required */
	srcinst = ChildByChar(data, mix_symbols[SOURCE_SYM]);
	if(srcinst){
		CHECK_TYPE(srcinst, SYMBOL_CONSTANT_INST, "source", "symbol_constant");
		source = SCP(SYMC_INST(typeinst)->value);
		if(source && strlen(source)==0){
			source = NULL;
		}
	}

	/* mass fractions -- required */
	/*
		May extend this to handle mole fractions, at a later date?
	 */
	xinst = ChildByChar(data, mix_symbols[X_SYM]);
	ERROR_REPORTER_HERE(ASC_USER_ERROR
		, "The type of the mass-fraction array is %x", InstanceKind(xinst));

	/* mixture model (ideal or real) -- not required */

	/* bbox->user_data = (void *)mixture_specify(npure, xs, comps, type, src); */
	bbox->user_data = NULL;
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
	const MixtureSpec *MX = (const MixtureSpec *)bbox->user_data; \
	FpropsError err=FPROPS_NO_ERROR;

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

	ERROR_REPORTER_HERE(ASC_USER_NOTE
			, "Function 'mixture_rho_calc' -- this function has no contents as yet.");

	outputs[0] = 0.0;

	return 0;
}

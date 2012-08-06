/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Pinch model of heat exchanger calculated with FPROPS.
*/

//#define ASC_HEATEX_DEBUG

/* include the external function API from libascend... */
#include <ascend/compiler/extfunc.h>

/* include error reporting API as well, so we can send messages to user */
#include <ascend/utilities/error.h>

/* for accessing the DATA instance */
#include <ascend/compiler/child.h>
#include <ascend/general/list.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/childinfo.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/slist.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instmacro.h>
#include <ascend/compiler/instance_types.h>

/* the code that we're wrapping... */
#include "helmholtz.h"
#include "sat.h"
#include "solve_ph.h"

/* for the moment, species data are defined in C code, we'll implement something
better later on, hopefully. */
#include "fluids.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

typedef struct{
	const HelmholtzData *comp[2];/* 0 = cold, 1 = hot */
	int n;
} HeatExData;

typedef struct{
	double p,h,mdot;
} StreamData;

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc heatex_prepare;
ExtBBoxFunc heatex_calc;

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *heatex_symbols[3];
#define COMPONENT_SYM heatex_symbols[0]
#define COMPONENT_HOT_SYM heatex_symbols[1]
#define N_SYM heatex_symbols[2]

static const char *heatex_help = "Calculate heat exchanger pinch temperature (detailed analysis)";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT heatex"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int heatex_pinch_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"FPROPS is still EXPERIMENTAL. Use with caution.\n");

	result += CreateUserFunctionBlackBox("heatex_DT_phmphmQ"
		,heatex_prepare
		,heatex_calc
		,(ExtBBoxFunc*)NULL /* no derivatives yet */
		,(ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		,(ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		,7, 1 /* inputs, outputs */
		,heatex_help
		,0.0
	);
		
	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"result = %d\n",result);
	}
	return result;
}

/**
   'heatex_prepare' just gets the stream details and the number of slices for
	internal calculation, and checks that the stream names are valid in FPROPS.
*/
int heatex_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	HeatExData *hxd = ASC_NEW(HeatExData);
	if(!hxd)goto fail;

	struct Instance *compinst[2], *ninst;
	const char *comp[2];

	heatex_symbols[0] = AddSymbol("component");
	heatex_symbols[1] = AddSymbol("component_hot");
	N_SYM = AddSymbol("n");

	ninst = ChildByChar(data,N_SYM);
	if(!ninst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate '%s' in DATA, please check usage.",SCP(N_SYM)
		);
		goto fail;
	}
	if(InstanceKind(ninst)!=INTEGER_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member '%s' must be a symbol_constant",SCP(N_SYM));
		goto fail;
	}
	hxd->n = IC_INST(ninst)->value;

	int i;
	for(i=0;i<2;++i){
		/* get the component names for cold and hot sides */
		compinst[i] = ChildByChar(data,heatex_symbols[i]);
		if(!compinst[i]){
			ERROR_REPORTER_HERE(ASC_USER_ERROR
				,"Couldn't locate '%s' in DATA, please check usage."
				,SCP(heatex_symbols[i])
			);
			goto fail;
		}
		if(InstanceKind(compinst[i])!=SYMBOL_CONSTANT_INST){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member '%s' must be a symbol_constant",SCP(heatex_symbols[i]));
			goto fail;
		}
		comp[i] = SCP(SYMC_INST(compinst[i])->value);
		CONSOLE_DEBUG("%s: %s",SCP(heatex_symbols[i]),comp[i]);
		if(comp[i]==NULL || strlen(comp[i])==0){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"'%s' is NULL or empty",heatex_symbols[i]);
			goto fail;
		}

		hxd->comp[i] = fprops_fluid(comp[i]);
		if(hxd->comp[i] == NULL){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Heat exchanger %s name '%s' not recognised. Check list of supported species.",SCP(heatex_symbols[i]),comp[i]);
			goto fail;
		}
	}

	bbox->user_data = (void *)hxd;
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Heat exchanger data structure OK.\n",comp);
	return 0;

fail:
	if(hxd){
		/* TODO FIXME implement FPROPS freeing */
		//if(hxd->comp[0])ASC_FREE(hxd->comp[0]);
		//if(hxd->comp[1])ASC_FREE(hxd->comp[1]);
		ASC_FREE(hxd);
	}
	return 1;	
}

/*------------------------------------------------------------------------------
  EVALULATION ROUTINES
*/

#define CALCPREPARE(NIN,NOUT) \
	/* a few checks about the input requirements */ \
	if(ninputs != NIN)return -1; \
	if(noutputs != NOUT)return -2; \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5; \
	\
	/* the 'user_data' in the black box object will contain the */\
	/* coefficients required for this fluid; cast it to the required form: */\
	const HeatExData *heatex_data = (const HeatExData *)bbox->user_data

/**
	Evaluation function for 'helmholtz_phsx_vT'
	@return 0 on success
*/
int heatex_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(7,5);

	StreamData cold = {inputs[0],inputs[1],inputs[2]};
	StreamData hot = {inputs[3],inputs[4],inputs[5]};
	double Q = inputs[6];
	double DT_min = DBL_MAX;
	int i, n = heatex_data->n;
	
	double Th,Tc,rhoh,rhoc;
	/* loop from i=0 (cold inlet) to i=n (cold outlet) */
	for(i=0;i<=n;++i){
		double hh = hot.h - Q/hot.mdot*(n-i)/n;
		double hc = cold.h + Q/cold.mdot*i/n;
		/* FIXME make use of guess values? */
		if(fprops_solve_ph(hot.p, hh, &Th, &rhoh, 0, heatex_data->comp[1])){
			/* error solving (p,h) hotside */
		}
		if(fprops_solve_ph(cold.p, hc, &Tc, &rhoc, 0, heatex_data->comp[0])){
			/* error solving (p,h) coldside */
		}
		double DT = Th - Tc;
		if(DT<DT_min)DT_min = DT;
	}

	/* non-saturated */
	outputs[0] = DT_min;
	return 0;
}


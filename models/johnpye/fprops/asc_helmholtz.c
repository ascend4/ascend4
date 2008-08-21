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
	Wrapper for helmholtz.c to allow access to the routine from ASCEND.
*/

/* include the external function API from libascend... */
#include <compiler/extfunc.h>

/* include error reporting API as well, so we can send messages to user */
#include <utilities/error.h>

/* for accessing the DATA instance */
#include <compiler/child.h>
#include <general/list.h>
#include <compiler/module.h>
#include <compiler/childinfo.h>
#include <compiler/parentchild.h>
#include <compiler/slist.h>
#include <compiler/type_desc.h>
#include <compiler/packages.h>
#include <compiler/symtab.h>
#include <compiler/instquery.h>
#include <compiler/instmacro.h>
#include <compiler/instance_types.h>

/* the code that we're wrapping... */
#include "helmholtz.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif


/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc helmholtz_prepare;
ExtBBoxFunc helmholtz_p_calc;
ExtBBoxFunc helmholtz_u_calc;
ExtBBoxFunc helmholtz_h_calc;

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *helmholtz_symbols[1];
#define COMPONENT_SYM helmholtz_symbols[0]

/* Property data for Ammonia, from Tillner-Roth, Harms-Watzenberg and
Baehr, Eine neue Fundamentalgleichung für Ammoniak, DKV-Tagungsbericht,
20:167-181, 1993. This is the ammmonia property correlation recommended
by NIST in its program REFPROP 7.0. */
const HelmholtzData helmholtz_data_ammonia = {
	/* R */ 488.189 /* J/kg/K */
	, /* rho_star */225. /* kg/m³ */
	, /* T_star */ 405.40 /* K */

	, {
		/* a0_1 */ -15.815020
		,/* a0_2 */ 4.255726
		,/* a0_3 */ 11.474340
		,/* a0_4 */ -1.296211
		,/* a0_5 */ 0.5706757
	}

	, {
		/* a_i, t_i, d_i */
		/* 1 */{0.4554431E-1,  -0.5  ,  2}
		,{0.7238548E+0,   0.5 ,   1 }
		,{0.1229470E-1,     1 ,   4 }
		,{-0.1858814E+1,  1.5 ,   1 }
		/* 5 */,{0.2141882E-10,    3 ,  15 }
		,{-0.1430020E-1,    0 ,   3 }
		,{0.3441324E+0,     3 ,   3 } 
		,{-0.2873571E+0,    4 ,   1 }
		,{0.2352589E-4,     4 ,   8 }
		/* 10 */,{-0.3497111E-1,   5  ,  2}
		,{0.2397852E-1,    3  ,  1}
		,{0.1831117E-2,    5 ,   8}
		,{-0.4085375E-1,   6 ,   1}
		,{0.2379275E+0,    8 ,   2}
		/* 15 */,{-0.3548972E-1,   8 ,   3}
		,{-0.1823729E+0,   10,   2}
		,{0.2281556E-1,   10 ,   4}
		,{-0.6663444E-2,   5 ,   3}
		,{-0.8847486E-2,  7.5,   1}
		/* 20 */,{0.2272635E-2 ,  15 ,   2}
		,{-0.5588655E-3,  30,    4}
	}
};

static const char *helmholtz_p_help = "Calculate pressure from temperature and density, using Helmholtz fundamental correlation";
static const char *helmholtz_u_help = "Calculate specific internal energy from temperature and density, using Helmholtz fundamental correlation";
static const char *helmholtz_h_help = "Calculate specific enthalpy from temperature and density, using Helmholtz fundamental correlation";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT helmholtz"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int helmholtz_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Initialising HELMHOLTZ...\n");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, helmholtz_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFN(helmholtz_p,2,1);
	CALCFN(helmholtz_u,2,1);
	CALCFN(helmholtz_h,2,1);

#undef CALCFN

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/**
   'helmholtz_prepare' just gets the data member and checks that it's
	valid, and stores it in the blackbox data field.
*/
int helmholtz_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *compinst;
	const char *comp;

	helmholtz_symbols[0] = AddSymbol("component");

	/* get the data file name (we will look for this file in the ASCENDLIBRARY path) */
	compinst = ChildByChar(data,COMPONENT_SYM);
	if(!compinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'component' in DATA, please check usage of HELMHOLTZ."
		);
		return 1;
	}
	if(InstanceKind(compinst)!=SYMBOL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member 'component' must be a symbol_constant");
		return 1;
	}
	comp = SCP(SYMC_INST(compinst)->value);
	CONSOLE_DEBUG("COMPONENT: %s",comp);
	if(comp==NULL || strlen(comp)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'component' is NULL or empty");
		return 1;
	}

	if(strcmp(comp,"ammonia")!=0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Component must be 'ammonia' at this stage (only one component supported)");
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"PREPARING HELMHOLTZ...\n");

	bbox->user_data = (void*)&helmholtz_data_ammonia;

	return 0;
}

/*------------------------------------------------------------------------------
  EVALULATION ROUTINES
*/

#define CALCPREPARE \
	/* a few checks about the input requirements */ \
	if(ninputs != 2)return -1; \
	if(noutputs != 1)return -2; \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5; \
	\
	/* the 'user_data' in the black box object will contain the */\
	/* coefficients required for this fluid; cast it to the required form: */\
	HelmholtzData *helmholtz_data = (HelmholtzData *)bbox->user_data

/**
	Evaluation function for 'helmholtz_p'.
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_p_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	outputs[0] = helmholtz_p(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_u'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_u_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	outputs[0] = helmholtz_u(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}


/**
	Evaluation function for 'helmholtz_h'
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_h_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE;

	/* first input is temperature, second is molar density */
	outputs[0] = helmholtz_h(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */
	return 0;
}





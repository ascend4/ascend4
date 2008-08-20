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

ExtBBoxInitFunc helmholtz_p_prepare;
ExtBBoxFunc helmholtz_p_calc;

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *helmholtz_symbols[1];
#define COMPONENT_SYM helmholtz_symbols[0]

/* property data for R123, from J. Phys. Chem. Ref. Data, 23:731-779, 1994. */
const HelmholtzData helmholtz_data_ammonia = {
	/* R */ 488.189 /* J/kg/K */
	, /* rho_star */225. /* kg/m³ */
	, /* T_star */ 405.40 /* K */

	, (double[5]){
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

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT helmholtz"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int helmholtz_register(){
	const char *helmholtz_help = "Modified Benedict-Webb-Rubin correlation for thermodynamic properties";
	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Initialising HELMHOLTZ...\n");

	result += CreateUserFunctionBlackBox("helmholtz_p"
		, helmholtz_p_prepare
		, helmholtz_p_calc /* value */
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */
		, 2,1 /* inputs, outputs */
		, helmholtz_help
		, 0.0
	); /* returns 0 on success */

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/*------------------------------------------------------------------------------
  WRAPPING FOR 'helmholtz_p'...
*/

int helmholtz_p_prepare(struct BBoxInterp *bbox,
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

	if(strcmp(comp,"R123")!=0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Component must be 'R123' at this stage (only one component supported)");
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"PREPARING HELMHOLTZ_P...\n");

	bbox->user_data = (void*)&helmholtz_data_ammonia;

	return 0;
}

/**
	Evaluation function. This one does the actual calling to the
	'helmholtz_p' routine.
	@param jacobian ignored
	@return 0 on success
*/
int helmholtz_p_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	/* a few checks about the input requirements */
	if(ninputs != 2)return -1;
	if(noutputs != 1)return -2;
	if(inputs==NULL)return -3;
	if(outputs==NULL)return -4;
	if(bbox==NULL)return -5;

	/* the 'user_data' in the black box object will contain the coefficients
	required for this fluid; cast it to the required form: */
	HelmholtzData *helmholtz_data = (HelmholtzData *)bbox->user_data;

	/* first input is temperature, second is molar density */
	outputs[0] = helmholtz_p(inputs[0], inputs[1], helmholtz_data);

	/* no need to worry about error states etc. */

	return 0;
}

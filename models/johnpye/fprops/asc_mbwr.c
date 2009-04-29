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
	Wrapper for mbwr.c to allow access to the routine from ASCEND.
*/

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
#include "mbwr.h"

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif


/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

ExtBBoxInitFunc mbwr_p_prepare;
ExtBBoxFunc mbwr_p_calc;

/*------------------------------------------------------------------------------
  GLOBALS
*/

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *mbwr_symbols[1];
#define COMPONENT_SYM mbwr_symbols[0]

/* property data for R123, from J. Phys. Chem. Ref. Data, 23:731-779, 1994. */
const MbwrData mbwr_r123 = {
	1 /* ideal gas constant? */
	,13.2117771543124 /* rho_c */
	,{ /* beta[32] */
		-0.657453133659E-02,     0.293479845842E+01,   -0.989140469845E+02
		, 0.201029776013E+05,   -0.383566527886E+07,    0.227587641969E-02
		,-0.908726819450E+01,    0.434181417995E+04,    0.354116464954E+07
		,-0.635394849670E-03,    0.320786715274E+01,   -0.131276484299E+04
		,-0.116360713718E+00,   -0.113354409016E+02,   -0.537543457327E+04
		, 0.258112416120E+01,   -0.106148632128E+00,    0.500026133667E+02
		,-0.204326706346E+01,   -0.249438345685E+07,   -0.463962781113E+09
		,-0.284903429588E+06,    0.974392239902E+10,   -0.637314379308E+04
		, 0.314121189813E+06,   -0.145747968225E+03,   -0.843830261449E+07
		,-0.241138441593E+01,    0.108508031257E+04,   -0.106653193965E-01
		,-0.121343571084E+02,   -0.257510383240E+03
	}
};

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from "IMPORT mbwr"

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int mbwr_register(){
	const char *mbwr_help = "Modified Benedict-Webb-Rubin correlation for thermodynamic properties";
	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Initialising MBWR...\n");

	result += CreateUserFunctionBlackBox("mbwr_p"
		, mbwr_p_prepare
		, mbwr_p_calc /* value */
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */
		, 2,1 /* inputs, outputs */
		, mbwr_help
		, 0.0
	); /* returns 0 on success */

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/*------------------------------------------------------------------------------
  WRAPPING FOR 'mbwr_p'...
*/

int mbwr_p_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *compinst;
	const char *comp;

	mbwr_symbols[0] = AddSymbol("component");

	/* get the data file name (we will look for this file in the ASCENDLIBRARY path) */
	compinst = ChildByChar(data,COMPONENT_SYM);
	if(!compinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'component' in DATA, please check usage of MBWR."
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

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"PREPARING MBWR_P...\n");

	bbox->user_data = (void*)&mbwr_r123;

	return 0;
}

/**
	Evaluation function. This one does the actual calling to the
	'mbwr_p' routine.
	@param jacobian ignored
	@return 0 on success
*/
int mbwr_p_calc(struct BBoxInterp *bbox,
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
	MbwrData *mbwr_data = (MbwrData *)bbox->user_data;

	/* first input is temperature, second is molar density */
	outputs[0] = mbwr_p(inputs[0], inputs[1], mbwr_data);

	/* no need to worry about error states etc. */

	return 0;
}

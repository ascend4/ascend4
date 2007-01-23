/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
*//**
	This is intended to be a simplest-possible external function and was
	used by John Pye to test his updates/fixes to the Black Box implementation.
	Since Ben refactored the the black box stuff separately, this has stopped
	working (and now causes a runtime crash. The aim now is to work out why
	the crash occurs and ensure that it can't!

	Note that this external function does *not* provide its own jacobian; it is
	counting on the default numerical diff function provided by ASCEND.
*//*
	by John Pye, Jan 2006
*/

#include <stdio.h>

#include <utilities/ascConfig.h>
#include <utilities/error.h>

/*



#include <compiler/child.h>
#include <general/list.h>
#include <compiler/module.h>
#include <compiler/childinfo.h>
#include <compiler/slist.h>
#include <compiler/type_desc.h>
#include <compiler/packages.h>*/
#include <compiler/extfunc.h>

ExtBBoxInitFunc addone_prepare;
ExtBBoxFunc addone_calc;

/**
	This is the function called from "IMPORT extfntest"

	It sets up the functions in this external function library
*/

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

extern
ASC_EXPORT int extfntest_register(){
	const char *addone_help = "This is a test of the dynamic user packages functionality";
	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Initialising EXTFNTEST...\n");

	(void)CONSOLE_DEBUG("EVALUATION FUNCTION AT %p",addone_calc);

	result += CreateUserFunctionBlackBox("add_one"
		, addone_prepare
		, addone_calc /* value */
		, (ExtBBoxFunc*)NULL /* deriv */
		, (ExtBBoxFunc*)NULL /* deriv2 */
		, (ExtBBoxFinalFunc*)NULL /* final */
		, 1,1 /* inputs, outputs */
		, addone_help
		, 0.0
	); /* returns 0 on success */

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

int addone_prepare(struct BBoxInterp *slv_interp,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	const char *mystring = "MY STRING IS HERE";

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"PREPARING PKG EXTFNTEST...\n");
	slv_interp->user_data = (void *)mystring;

	return 0;
}

/* return 0 on success */
int addone_calc(struct BBoxInterp *slv_interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	if(noutputs != 1)return -1;
	if(ninputs != 1)return -2;
	if(inputs==NULL)return -3;
	if(outputs==NULL)return -4;
	if(slv_interp==NULL)return -5;

	outputs[0] = inputs[0] + 1;

	return 0;
}

/*	ASCEND modelling environment
	Copyright (C) 1999 Benjamin A Allan
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
*/

#include <stdio.h>

#include <utilities/ascConfig.h>
#include <utilities/error.h>

#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/child.h>
#include <general/list.h>
#include <compiler/module.h>
#include <compiler/childinfo.h>
#include <compiler/slist.h>
#include <compiler/type_desc.h>
#include <compiler/packages.h>

int addone_prepare(struct Slv_Interp *slv_interp, struct Instance *data, struct gl_list_t *arglist);
int addone_calc(struct Slv_Interp *slv_interp, int ninputs, int noutputs, double *inputs, double *outputs, double *jacobian);

/**
	This is the function called from "IMPORT extfntest"

	It sets up the functions in this external function library
*/

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

extern
ASC_EXPORT(int) extfntest_register(){
	const char *addone_help = "This is a test of the dynamic user packages functionality";
	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Initialising EXTFNTEST...\n");

	CONSOLE_DEBUG("EVALUATION FUNCTION AT %p",addone_calc);

	result += CreateUserFunctionBlackBox("add_one"
		, addone_prepare
		, addone_calc /* value */
		, (ExtBBoxFunc*)NULL /* deriv */
		, (ExtBBoxFunc*)NULL /* deriv2 */
		, (ExtBBoxInitFunc*)NULL /* final */
		, 1,1 /* inputs, outputs */
		, addone_help
	);

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	return result;
}

int addone_prepare(struct Slv_Interp *slv_interp,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"PREPARING PKG EXTFNTEST...\n");
	const char *mystring = "MY STRING IS HERE";
	slv_interp->user_data = (void *)mystring;
	return 0;
}

/* return 0 on success */
int addone_calc(struct Slv_Interp *slv_interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	char *mystring = (char *)slv_interp->user_data;

	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"ADDONE_CALC: mystring = %s\n",mystring); */

	/* ERROR_REPORTER_HERE(ASC_PROG_NOTE,"NINPUTS = %d, NOUTPUTS = %d\n",ninputs, noutputs); */

	/* CONSOLE_DEBUG("inputs[0] = %f, outputs[0] = %f",inputs[0],outputs[0]); */

	outputs[0] = inputs[0] + 1;

	/* CONSOLE_DEBUG("INPUT = %f, OUTPUT = %f",inputs[0],outputs[0]); */

	return 0; /* success */
}

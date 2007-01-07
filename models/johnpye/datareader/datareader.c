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
*/

#include <stdio.h>

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/error.h>

#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
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

#include <compiler/extfunc.h>

#include "dr.h"

/*------------------------------------------------------------------------------
  GLOBALS
*/

static symchar *dr_symbols[2];
#define FILENAME_SYM dr_symbols[0]
#define FORMAT_SYM dr_symbols[1]

/*------------------------------------------------------------------------------
  BINDINGS FOR THE DATA READER TO THE ASCEND EXTERNAL FUNCTIONS API
*/

ExtBBoxInitFunc asc_datareader_prepare;
ExtBBoxFunc asc_datareader_calc;
ExtBBoxFinalFunc asc_datareader_close;

#ifndef ASC_EXPORT
# error "Where is ASC_EXPORT?"
#endif

/**
	This is the function called from "IMPORT datareader"

	It sets up the functions in this external function library and tells ASCEND
	how many inputs and outputs it needs.
*/
extern
ASC_EXPORT int datareader_register(){
	const char *help = "The is the ASCEND Data Reader, for pulling in"
		" time-series data such as weather readings for use in simulations.";

	int result = 0;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Initialising data reader...\n");

	(void)CONSOLE_DEBUG("EVALUATION FUNCTION AT %p",asc_datareader_calc);

	result += CreateUserFunctionBlackBox("datareader"
		, asc_datareader_prepare
		, asc_datareader_calc /* value */
		, asc_datareader_calc /* deriv */
		, NULL /* deriv2 */
		, asc_datareader_close /* final */
		, 1,5 /* inputs, outputs */
		, help
		, 0.0
	); /* returns 0 on success */

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

/**
	This function prepares the data that we will use before starting the solver
	process.
*/
int asc_datareader_prepare(struct BBoxInterp *slv_interp,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *fninst, *fmtinst;
	const char *fn, *fmt;
	DataReader *d;

	dr_symbols[0] = AddSymbol("filename");
	dr_symbols[1] = AddSymbol("format");

	/* get the data file name (we will look for this file in the ASCENDLIBRARY path) */
	fninst = ChildByChar(data,FILENAME_SYM);
	if(!fninst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'filename', please check Data Reader usage."
		);
		return 1;
	}
	if(InstanceKind(fninst)!=SYMBOL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'filename' must be a symbol_constant");
		return 1;
	}
	fn = SCP(SYMC_INST(fninst)->value);
	CONSOLE_DEBUG("FILENAME: %s",fn);
	if(fn==NULL || strlen(fn)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'filename' is NULL or empty");
		return 1;
	}

	/* get the data reader format *//**
	This is the function called from "IMPORT extfntest"

	It sets up the functions in this external function library
*/

	fmtinst = ChildByChar(data,FORMAT_SYM);
	if(!fmtinst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'format', please check Data Reader usage."
		);
		return 1;
	}
	if(InstanceKind(fmtinst)!=SYMBOL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'format' must be a symbol_constant");
		return 1;
	}
	fmt = SCP(SYMC_INST(fmtinst)->value);
	CONSOLE_DEBUG("FORMAT: %s",fmt);
	if(fmt==NULL || strlen(fmt)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'format' is NULL or empty");
		return 1;
	}

	CONSOLE_DEBUG("Creating the datareader object...");

	/* create the data reader and tell it the filename */
	d = datareader_new(fn);

	if(fmt!=NULL){
		if(datareader_set_format(d,fmt)){
			CONSOLE_DEBUG("Invalid 'format'");
			return 1;
		}
	}

	if(datareader_init(d)){
		CONSOLE_DEBUG("Error initialising data reader");
		return 1;
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Created data reader at %p...",d);
	slv_interp->user_data = (void *)d;

	return 0;
}

/* return 0 on success */
int asc_datareader_calc(struct BBoxInterp *slv_interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	DataReader *d;
	int i;

	d = (DataReader *)slv_interp->user_data;
	if(!d){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Datareader was not initialised successfully"
		);
		return 1;
	}

	if(ninputs!=datareader_num_inputs(d)){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Invalid number of inputs, expected %d but received %d"
			,datareader_num_inputs(d), ninputs
		);
		return 1;
	}

	if(noutputs!=datareader_num_outputs(d)){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Invalid number of outputs, expected <=%d but received %d"
			,datareader_num_outputs(d), noutputs
		);
		return 1;
	}


	for(i=0; i< ninputs; ++i){
		CONSOLE_DEBUG("inputs[%d] = %f", i, inputs[i]);
	}


	switch(slv_interp->task){
		case bb_func_eval:
			CONSOLE_DEBUG("DATA READER EVALUATION");
			if(datareader_func(d,inputs,outputs)){
				CONSOLE_DEBUG("Datareader evaluation error");
				return 1;
			}
			for(i=0; i< noutputs; ++i){
				CONSOLE_DEBUG("outputs[%d] = %f", i, outputs[i]);
			}
			return 0; /* success */
		case bb_deriv_eval:
			CONSOLE_DEBUG("DATA READER DERIVATIVE");
			if(datareader_deriv(d,inputs,outputs)){
				CONSOLE_DEBUG("Datareader derivative evaluation error");
				return 1;
			}
			return 0; /* success */
		default:
			CONSOLE_DEBUG("UNHANDLED REQUEST");
			return 1;
	}
}

void asc_datareader_close(struct BBoxInterp *slv_interp){
	CONSOLE_DEBUG("NOT IMPLEMENTED");
}

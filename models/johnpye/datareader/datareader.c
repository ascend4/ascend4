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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>

#include <ascend/general/platform.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>


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

#include <ascend/compiler/extfunc.h>

#include "dr.h"

//#define DATAREADER_DEBUG
#ifdef DATAREADER_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

/*------------------------------------------------------------------------------
  GLOBALS
*/

static symchar *dr_symbols[3];
#define FILENAME_SYM dr_symbols[0]
#define FORMAT_SYM dr_symbols[1]
#define PARAM_SYM dr_symbols[2]

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

	MSG("EVALUATION FUNCTION AT %p",asc_datareader_calc);

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
	struct Instance *fninst, *fmtinst, *parinst;
	const char *fn, *fmt, *par;
	DataReader *d;
	char *partok = NULL; //token parser string for initialising datareader
	int noutputs; //number of outputs as per the arg file

	dr_symbols[0] = AddSymbol("filename");
	dr_symbols[1] = AddSymbol("format");
	dr_symbols[2] = AddSymbol("parameters");

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
	MSG("FILENAME: %s",fn);
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
	MSG("FORMAT: %s",fmt);
	if(fmt==NULL || strlen(fmt)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'format' is NULL or empty");
		return 1;
	}
/* get the datareader parameters. Ascend syntax is
   parameters :== 'col1:interp1,col2,interp2,..,coln:interpn';
	where	coln is the data file column assigned to corresponding variable
			interpn is the algorithm used to interpret that data column
*/
	parinst = ChildByChar(data,PARAM_SYM);
	if(!fninst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'parameters', please check Data Reader usage."
		);
		return 1;
	}
	if(InstanceKind(parinst)!=SYMBOL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'parameters' must be a symbol_constant");
		return 1;
	}
	par = SCP(SYMC_INST(parinst)->value);
	if(par==NULL || strlen(par)==0){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'parameters' is NULL or empty");
		return 1;
	}

	/* obtain number of outputs from the paramater statement */
	/* this enables to create a datareader object with the right size parameter tokens */
    /*
    	note: the reason for this string manipulations is that par is pointing directly
    	to the instance of the model parinst->value. Any string operations that are not
    	read only will affect this address, potentially leaving a NULL pointer in the
    	instance, or the datareader structure. even if passed down it wont be passed
    	by value, but by pointer, so other functions (such as datareader_set_parameters)
    	might affect this address, potentially causing a seg fault.

    */
	const char *par2[strlen(par)]; //allocate enough space for a copy of par
	strcpy(par2,par); //take a copy of par an

	/*datareader only! in rigour nouputs has to be derived by more
	  explicit methods, such as parsing or argument passing*/
	noutputs = gl_length(arglist)-1;

	/* create the data reader: tell it the filename and nouputs */
	d = datareader_new(fn,noutputs);
	//set datareader file format
	if(fmt!=NULL){
		if(datareader_set_format(d,fmt)){
			CONSOLE_DEBUG("Invalid 'format'");
			return 1;
		}
	}
	//initialise datareader object
	if(datareader_init(d)){
		CONSOLE_DEBUG("Error initialising data reader");
		return 1;
	}
	//asign user defined parameters
	if(par2!=NULL){
		if(datareader_set_parameters(d,par2)){
		CONSOLE_DEBUG("failed to set parameters");
		return 1;
		}
	}

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Created data reader at %p...",d);
	/*assign the succesfully created datareader object to the
	BlackBox Cache of the relation */
	slv_interp->user_data = (void *)d; //BROKEN AT THE MOMENT
	return 0;
}

/* return 0 on success */
int asc_datareader_calc(struct BBoxInterp *slv_interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	DataReader *d;
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
		//return 1; warn about incompatibility but keep going ...JZap
	}

#ifdef DATAREADER_DEBUG
	for(i=0; i< ninputs; ++i){
		MSG("inputs[%d] = %f", i, inputs[i]);
	}
#endif

	switch(slv_interp->task){
		case bb_func_eval:
			MSG("DATA READER EVALUATION");
			if(datareader_func(d,inputs,outputs)){
				CONSOLE_DEBUG("Datareader evaluation error");
				return 1;
			}
#ifdef DATAREADER_DEBUG
			for(i=0; i< noutputs; ++i){
				MSG("outputs[%d] = %f", i, outputs[i]);
			}
#endif
			return 0; /* success */
		case bb_deriv_eval:
			MSG("DATA READER DERIVATIVE");
			if(datareader_deriv(d,inputs,outputs)){
				MSG("Datareader derivative evaluation error");
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


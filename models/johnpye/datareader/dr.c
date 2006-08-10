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

#include "dr.h"
#include "tmy.h"

#include <utilities/config.h>
#include <general/ospath.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>
#include <utilities/ascPanic.h>

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/


/*------------------------------------------------------------------------------
  API FUNCTIONS
*/

/**
	Create a data reader object, with the filename specified. The filename
	will be searched for in a specified path, eg ASCENDLIBRARY.
*/		
DataReader *datareader_new(const char *fn){
	DataReader *d;
	
	d = ASC_NEW(DataReader);
	d->fn = fn;
	d->fp = NULL;
	d->f = NULL;
	d->noutputs = 0;

	return d;
}

/**
	Set data file format
	@return 0 on success
*/
int datareader_set_file_format(DataReader *d, const DataReaderFileFormat format){
	switch(format){
		case DATAREADER_FORMAT_TMY2:
			d->headerfn=&datareader_tmy2_header;
			d->datafn=&datareader_tmy2_data;
			break;
		default:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unknown file format specified");
			return 1;
	}
	return 0;
}

/**	
	Initialise the datareader: open the file, check the number of columns, etc.
	@return 0 on success
*/
int datareader_init(DataReader *d){
	d->fp = ospath_new(d->fn);
	if(d->fp==NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid filepath");
		return 1;
	}

	d->f = ospath_fopen(d->fp,"r");
	if(d->f == NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to open file '%s' for reading",d->fn);
		return 1;
	}

	if((*d->headerfn)(d)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error processing file header in '%s'",d->fn);
		fclose(d->f);
		return 1;
	}
	
	while(! (*d->eoffn)(d)){
		if((*d->datafn)(d)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error reading file data in '%s'",d->fn);
			fclose(d->f);
			return 1;
		}
	}
	fclose(d->f);

	return 0;
}

/**
	Shut down the data reader and deallocate any memory owned by it, then
	free the memory at d.
*/
int datareader_delete(DataReader *d){
	if(d->fp){
		ospath_free(d->fp);
		d->fp = NULL;
	}
	if(d->f){
		fclose(d->f);
		d->f = NULL;
	}
	ASC_FREE(d);
	return 0;
}

/**
	Return the number of inputs (independent variables) supplied in the
	DataReader's current file. Can only be 1 at this stage.
*/
int datareader_num_inputs(const DataReader *d){
	return 1;
}

/**
	Return the number of outputs (dependent variables) found in the DataReader's
	current file. Should be one or more.
*/
int datareader_num_outputs(const DataReader *d){
	return d->noutputs;
}


/**
	Return an interpolated set of output values for the given input values.
	This should be computed such that the output values are smooth in their 
	first derivatives.

	The required memory for the inputs and outputs must be allocated by the
	caller, and indicated by the pointers 'inputs' and 'outputs'.

	@see datareader_deriv
	@TODO implement this
*/
int datareader_func(DataReader *d, double *inputs, double *outputs){
	int i;
	double t;

	t = inputs[0];
	i = 0;

	CONSOLE_DEBUG("Not implemented");
	return 1;
}

/**
	Return an interpolated set of output derivatives for the given input
	values. These should be smooth.
	@see datareader_func
	@TODO implement this
*/
int datareader_deriv(DataReader *d, double *inputs, double *jacobian){
	CONSOLE_DEBUG("Not implemented");
	return 1;
}



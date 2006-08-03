#include "dr.h"
#include <general/ospath.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>

/**
	Record where in the file the data for a particular time can be found,
	for fast backtracking.
*/
struct IndexPoint{
	int pos;
	double t;
};

/**
	Structure to hold the data for a particular data point after being loaded.
*/
struct DataPoint{
	double t;
	double *outputs;
};

/**
	Need some kind of structure here to hold spline data for a particular
	interval.
*/

/**
	Top-level structure for the data reader. Keeps track of the file we're
	working with, the number of columns, etc.
*/
struct DataReader{
	const char *fn;
	struct FilePath *fp;
	FILE *f;
	int noutputs;
	InputFilterFn *iff;
};

/**
	Create a data reader object, with the filename specified. The filename
	will be searched for in a specified path, eg ASCENDLIBRARY.
*/		
int datareader_new(const char *fn){
	DataReader *d;
	
	d = ASC_NEW(DataReader);
	d->fn = fn;
	d->fp = NULL;
	d->noutputs = 0;
	d->iff = NULL;

	return d;
}

/**
	Assign an input filter to the data reader. This will permit pre-processing
	of data from the file, and reading of different formats, eg CSV, TDV, TMY,
	fixed-width, etc.
*/
int datareader_set_input_filter(DataReader *d, InputFilterFn *iff){
	d->iff = iff;
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
	/**
		@TODO implement this
	*/
	CONSOLE_DEBUG("Not implemented");
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
	@see datareader_deriv
	@TODO implement this
*/
int datareader_func(DataReader *d, double *inputs, double *outputs){
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

	

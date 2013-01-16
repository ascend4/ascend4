#ifndef FPROPS_FLUIDS_H
#define FPROPS_FLUIDS_H

#include "fprops.h"
#include "rundata.h"

/**
	Look up the named fluid and return its internal data structure, or
	NULL if not found.
*/
const PureFluid *fprops_fluid(const char *name, const char *corrtype);

/**
	@return number of fluids in the database.
*/
int fprops_num_fluids();

/**
	Retrieve fluid according to its position in the list of added fluids. The
	index number can't be assumed to be stable; this function is simply to
	allow iteration through all the fluids, searching, etc.
	@param i fluid index number [0,fprops_num_fluids()-1]
	@return NULL if i is out of bounds.
*/
const PureFluid *fprops_get_fluid(int i);

#endif


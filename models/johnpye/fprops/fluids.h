#ifndef FPROPS_FLUIDS_H
#define FPROPS_FLUIDS_H

#include "helmholtz.h"

/**
	Look up the named fluid and return its internal data structure, or
	NULL if not found.
*/
const HelmholtzData *fprops_fluid(const char *name);

/**
	@return number of fluids in the database.
*/
int fprops_num_fluids();

/**
	Retrieve fluid according to its position in the list of added fluids.
	@return NULL if i is out of bounds.
*/
const HelmholtzData *fprops_get_fluid(int i);

#endif


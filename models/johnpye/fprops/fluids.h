#ifndef FPROPS_FLUIDS_H
#define FPROPS_FLUIDS_H

#include "helmholtz.h"

/**
	Look up the named fluid and return its internal data structure, or
	NULL if not found.
*/
const HelmholtzData *fprops_fluid(const char *name);

#endif


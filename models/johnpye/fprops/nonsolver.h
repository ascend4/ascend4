/* two-way iterative solver using FPROPS for properties in the non-saturation
	region.
*/
#ifndef FPROPS_NONSOLVER_H
#define FPROPS_NONSOLVER_H

#include "helmholtz.h"

int fprops_nonsolver(FPROPS_CHAR A, FPROPS_CHAR B, double atarget, double btarget, double *T, double *rho, const HelmholtzData *D);

#endif


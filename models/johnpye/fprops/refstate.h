#ifndef FPROPS_REFSTATE_H
#define FPROPS_REFSTATE_H
#include "rundata.h"

/**
	Set/update the reference state on a pure fluid.

	@return 0 on success
*/
int fprops_set_reference_state(PureFluid *data, const ReferenceState *ref);

int refstate_set_for_incomp(PureFluid *data, const ReferenceState *ref);
int refstate_set_for_phi0(PureFluid *data, const ReferenceState *ref);
#endif


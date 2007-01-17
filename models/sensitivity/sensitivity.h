/*
	This external package is just a cheeky wrapper for the built-in Sensitivity
	routines in the base/generic/packages directory. Although we *want* to make
	sensitivity into a pure external module, we can't do that due to the
	dependency of the LSODE integrator. However, in order for sensitivity to
	work correctly with the new importhandler mechanism, *something* needs to be
	here, so we've got ther 'sensitivity_register' function, as well as the
	do_ wrapper functions in the .c file here.

	-- John Pye, Jan 2007
*/
#include <utilities/config.h>
#include <utilities/ascConfig.h>

ASC_DLLSPEC int sensitivity_register(void);

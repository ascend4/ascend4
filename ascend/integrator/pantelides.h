#ifndef ASC_INTEGRATOR_PANTELIDES_H
#define ASC_INTEGRATOR_PANTELIDES_H

#include <stdio.h>

#include <ascend/general/platform.h>
#include <ascend/system/slv_client.h>

/**
	Write a first-pass advisory Pantelides report for the active problem in
	`sys`. This analysis is read-only: it does not modify the instance tree,
	create generated relations, or alter solver lists.

	The report is intended to explain which equations would need to be
	differentiated, and which derivative quantities would be required, in order
	to reduce the index of the current system.

	@return 0 on success, non-zero on failure.
*/
ASC_DLLSPEC int integrator_pantelides_advisory(slv_system_t sys, FILE *fp);

#endif

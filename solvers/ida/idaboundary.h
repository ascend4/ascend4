/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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
*//** @file
	IDA integrator
*//*
	by John Pye, Jan 2011.
*/

#ifndef ASC_IDA_H
#define ASC_IDA_H

#include <ascend/integrator/integrator.h>


/*
 * Check to see if and of the system discrete variables have changed.
 *
 * @return 1 if any of the values have changed (does not necessarily mean
 * 			 the system needs to be reconfigured)
 */
int some_dis_vars_changed(slv_system_t sys);

/**
 * Setup LRSlv for use with IDA. Solve the problem at startup to check the analysis
 */
void ida_setup_lrslv(IntegratorSystem *integ);

/**
 * Throw out old values and reanalyse the system after a boundary crossing
 */
int ida_bnd_reanalyse(IntegratorSystem *integ);

/**
 * Update the relist, as equations may have been added/removed after a crossing
 *
 * @return 0 on success
 */
int ida_bnd_update_relist(IntegratorSystem *integ);

/*
 * Retrieve the new initial values from the IntegratorSystem and set them up as
 * appropriate for IDA
 */

/**
 * Create a new NVector, assign all data to 0
 * @param vec_length	number of data elements in the vector.
 */
N_Vector ida_bnd_new_zero_NV(long int vec_length);

void ida_bnd_update_IC(IntegratorSystem *integ, realtype t0, N_Vector y0, N_Vector yp0);

/**
	Reconfigure the IDA system after a boundary-crossing event. This will
	probably require the system to be re-analyzed, and completely reinitialized.

	@param rootsfound 		array as returned by IDAGetRootInfo from IDA.
	@param bnd_cond_states	running record of boundary states

	@return
		1 					if system is to be reconfigured
		0					crossing causes no change in the system
		-1					error with the logical solver
*/
int ida_cross_boundary(IntegratorSystem *integ, int *rootsfound,
		int *bnd_cond_states);

#endif  /* ASC_IDA_H */

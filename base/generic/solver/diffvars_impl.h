/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie-Mellon University

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*//**
	Implementation of the derivative chains data structures.

	This is a private header file to be used only by problem analysis modules.
	
	DO NOT include this file if you are implementing a new integration engine.
*//**
	Split from system_impl.h by John Pye -- Jan 2007
*/

#include "system.h"


/**
	Structure that holds a differential variable and the sequence of
	its derivatives as found explicity in the model.

	For example,
	  - x, dx/dt
*/
typedef struct SolverDiffVarSequenceStruct{
	long ode_id;
	short n;
	struct var_variable **vars; /* will be in order of ascending ode_type, starting from 1 */
} SolverDiffVarSequence;

/**
	Array of diff var sequences. Once generated, this will hold all of the 
	This would hold all of the differential and derivative variables found
	in the system. For example, with each row being a SolverDiffVarSequence:
	  - y, dy/dt, d2y/dt2
	  - x, dx/dt
      - z, dz/dt
*/
struct SolverDiffVarCollectionStruct{
	SolverDiffVarSequence *seqs;
	long nseqs;
	long nalg;
	long ndiff;
	struct var_variable **indep;
	long nindep;
	struct var_variable **obs;
	long nobs;
	long maxorder;
};

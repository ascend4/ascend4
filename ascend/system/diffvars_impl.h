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
*//** @defgroup system_diffvarsimpl System Differential Variables Internal Implementation
	Implementation of the derivative chains data structures.

	This is a private header file to be used only by problem analysis modules.
	
	DO NOT include this file if you are implementing a new integration engine.
*//**
	Split from system_impl.h by John Pye -- Jan 2007
*/

#include "system.h"

/** @addtogroup system_diffvarsimpl
	@{
*/

/**
    Structure that holds a differential variable and the sequence of
    its derivatives as found explicity in the model.

    For example,
      - x, dx/dt
*/
typedef struct SolverDiffVarSequenceStruct{
    long ode_id; /**< as assigned by user to this set of variables */
    short n; /**< length of this variable sequence */
    struct var_variable **vars; /**< variables in the sequence; will be in order of ascending ode_type, starting from 1 */
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
    long nseqs; /**< number of differential variable sequences */
    long nalg; /**< number of algebraic variables */
    long ndiff; /**< number of differential variables */
    struct var_variable **indep; /** independent variable(s), in case user mistakenly flags more than one. Normally this would be 'time' or similar. */
    long nindep; /**< number of independent variables */
    struct var_variable **obs; /**< observed variables. These will have been flags somehow so that the Integrator will record their value as solution proceeds. */
    long nobs; /**< number of observed variables */
    long maxorder; /**< Maximum length of derivative chains found. We can only usefully deal with maxorder==2 at this stage. */
};



/*
 *  Logical Relation Solver
 *  by Vicente Rico-Ramirez
 *  Created: 04/97
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: slv9a.h,v $
 *  Date last modified: $Date: 1997/07/29 15:48:18 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Logical Relation Solver registration module.
 *  <pre>
 *  Contents:     LRSlv module (Logical Relation Solver)
 *  </pre>
 */

#ifndef ASC_SLV9A_H
#define ASC_SLV9A_H

#include "solver.h"

/**	@addtogroup solver Solver
	@{
*/

# define HAVE_LRSLV 1

typedef struct slv9a_system_structure *slv9a_system_t;

SolverRegisterFn slv9a_register;

/* @} */

#endif  /* ASC_SLV9A_H */


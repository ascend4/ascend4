/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Conditional Modeling Solver (CMSlv) registration module.

	Requires:
	#include "utilities/ascConfig.h"
	#include "slv_client.h"
*//*
	by Vicente Rico-Ramirez
	Created: 04/97
	Version: $Revision: 1.4 $
	Version control file: $RCSfile: slv9.h,v $
	Date last modified: $Date: 1997/07/29 15:48:07 $
	Last modified by: $Author: rv2a $
*/

#ifndef ASC_SLV9_H
#define ASC_SLV9_H

/**	@addtogroup solver Solver
	@{
*/

#include "slv_client.h"

typedef struct slv9_system_structure *slv9_system_t;

/* used by StaticSolverRegistration to detect this solver: */
#if defined(STATIC_CMSLV) || defined(DYNAMIC_CMSLV)
# define HAVE_CMSLV 1
#else
# define HAVE_CMSLV 0
#endif


ASC_DLLSPEC int slv9_register(SlvFunctionsT *f);
/**<
 *  Registration function for the ASCEND CMSlv solver.
 *  This is the function that tells the system about the CMSlv solver.
 *  Our index is not necessarily going to be 9. That everything here is
 *  named slv9* is just a historical result and a convenient way of
 *  shutting up the linker.
 *
 *  @param f SlvFunctionsT to receive the solver registration info.
 *  @return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
 */

/* @} */

#endif  /* ASC_SLV9_H */


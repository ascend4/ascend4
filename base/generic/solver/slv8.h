/*	ASCEND modelling environment
	Copyright (C) 1997,2006-2007 Carnegie Mellon University

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
*//** @file
	Connection to the CONOPT solver/optimiser
*//*
	originally by Ken Tyner and Vicente Rico-Ramirez, Jun-Aug 1997.
	updated for CONOPT 3 by John Pye, July 2006.
	Last in CVS: $Revision: 1.3 $ $Date: 1997/08/12 16:43:46 $ $Author: rv2a $
*/

#ifndef ASC_SLV8_H
#define ASC_SLV8_H

#include "slv_client.h"

/**	@addtogroup solver Solver
	@{
*/

typedef struct slv8_system_structure *slv8_system_t;

#ifdef ASC_WITH_CONOPT
# define HAVE_CONOPT 1
#else
# define HAVE_CONOPT 0
#endif

int slv8_register(SlvFunctionsT *f);
/**<
	Registration function for the CONOPT solver.
	This is the function that tells the system about the CONOPT solver.
	Our index is not necessarily going to be 8. That everything here is
	named slv8* is just a historical result and a convenient way of
	shutting up the linker.
	
	@param f SlvFunctionsT to receive the solver registration info.
	@return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
*/

/* @} */

#endif  /* ASC_SLV8_H */


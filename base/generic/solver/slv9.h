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
*//*
	by Vicente Rico-Ramirez, 04/1997
	Last in CVS: $Revision: 1.4 $ $Date: 1997/07/29 15:48:07 $ $Author: rv2a $
*/

#ifndef ASC_SLV9_H
#define ASC_SLV9_H

/**	@addtogroup solver Solver
	@{
*/

#include "solver.h"

typedef struct slv9_system_structure *slv9_system_t;

#ifdef ASC_WITH_CONOPT
#define HAVE_CMSLV 1
#else
#define HAVE_CMSLV 0
#endif

SolverRegisterFn slv9_register;

/* @} */

#endif  /* ASC_SLV9_H */


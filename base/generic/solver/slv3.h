/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
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
*//** @file
	QRSlv solver registration module.

	02/95 - original version
	Test bed version of Slv for linear solver changes
	and eventually Slv kernel<-->ASCEND changes.
*//*
	by Karl Michael Westerberg and Ben Allan -- created 2/6/90
	Last in CVS: $Revision: 1.9 $ $Date: 1997/07/18 12:16:16 $ $Author: mthomas $
*/

#ifndef ASC_SLV3_H
#define ASC_SLV3_H

#include "solver.h"

/**	@addtogroup solver Solver
	@{
*/

typedef struct slv3_system_structure *slv3_system_t;

/**
	Solver ID for QRSlv. QRSlv will always have this number.
*/
#define SOLVER_QRSLV 3

#define HAVE_QRSLV 1

SolverRegisterFn slv3_register;


/* @} */

#endif  /* ASC_SLV3_H */


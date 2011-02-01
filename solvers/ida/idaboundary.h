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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	IDA integrator
*//*
	by John Pye, Jan 2011.
*/

#ifndef ASC_IDA_H
#define ASC_IDA_H

#include <ascend/integrator/integrator.h>

/**
	Reconfigure the IDA system after a boundary-crossing event. This will
	probably require the system to be re-analyzed, and completely reinitialized.

	@param rootsfound array as returned by IDAGetRootInfo from IDA.
	@return 0 on success
*/
int ida_cross_boundary(IntegratorSystem *integ, int *rootsfound);

#endif  /* ASC_IDA_H */

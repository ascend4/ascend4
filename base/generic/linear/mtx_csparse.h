/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1995 Benjamin Andrew Allan, Kirk Andre' Abbott
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
	Routine to translate 'mtx' matrices to and from CXSparse form.

	@see http://www.cise.ufl.edu/research/sparse/
*//*
	by John Pye, Mar 2007
*/
#ifndef ASC_MTX_CXSPARSE_H
#define ASC_MTX_CXSPARSE_H

#include "mtx.h"

#include <utilities/config.h>
#include <utilities/ascConfig.h>

/**
	This file will be effectively empty unless ASC_WITH_UFSPARSE is defined.
	So, when using mtx_to_cs, check first that the routine is available using
	#ifdef ASC_WITH_UFSPARSE.
*/

# ifdef ASC_WITH_UFSPARSE
#  include "ufsparse/cs.h"
/* defaults to (int,int,double) which is what we want. */

ASC_DLLSPEC cs *mtx_to_cs(mtx_matrix_t M);

ASC_DLLSPEC mtx_matrix_t mtx_from_cs(const cs *C);

# endif

#endif

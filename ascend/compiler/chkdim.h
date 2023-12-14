/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Dimensionality checks for relations.
	
	Previously solver/chkdim.h was written by someone (Ben?) but not implemented.
*//*
	by John Pye
	Created: Dec 2023
*/

#ifndef ASC_CHKDIM_H
#define ASC_CHKDIM_H

/**	@addtogroup compiler_dimen Compiler Dimensions
	@{
*/

#include <ascend/general/platform.h>
#include "instance_types.h"

ASC_DLLSPEC int chkdim_check_relation(CONST struct Instance *i);
/**<
 *  Returns zero if all checks OK. Error messages via error_reporter.
 */

/* @} */

#endif  /* ASC_CHKDIM_H */


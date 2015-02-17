/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1996 Benjamin Andrew Allan

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
*//** @defgroup system_analysis System Analysis
	Problem Analysis Routines for models which use the der() syntax.
*/

#ifndef ASC_K12ANALYZE_H
#define ASC_K12ANALYZE_H

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>

#include <ascend/compiler/instance_enum.h>
#include "slv_types.h"
#include "conditional.h"

/**	@addtogroup system_analysis
	@{
*/

extern int k12_analyze_make_problem(slv_system_t sys, struct Instance *inst);
/**<
	Takes a system and populates the guts of it from the instance. Called by	
	system_build -- you don't need to call it yourself.

	@TODO this routine should be part of the 'problem provider' API?

	@NOTE 
		This implementation of analyze is specific to the ASCEND Compiler
		back end.
*/

/* @} */

#endif /* ASC_ANALYZE_H */

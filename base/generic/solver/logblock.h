/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//*
	@file
	block partitioning for logic problems. Extracted from slv_stdcalls.*
*//*
	created John Pye 2007 from slv_stdcalls.h, orig by Ben Allan 1996.
*/

#ifndef ASC_LOGBLOCK_H
#define ASC_LOGBLOCK_H

#include <utilities/ascConfig.h>
#include <system/slv_types.h>

ASC_DLLSPEC int slv_log_block_partition(slv_system_t sys);
/**<
	Takes a system and reorders its solvers_dvars and solvers_logrels
	list so that they fall into a block lower triangular form and
	the system's block list is set to match.
	Only included and active logrelations and free, incident boolean_var
	are so ordered.
	The blocks derived are put in the slv_system_t block list.<br><br>

	@return 0 on success, 2 on out-of-memory, 1 on any other failure
*/

#endif

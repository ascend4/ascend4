/*	ASCEND modelling environment
	Copyright (C) 1997  Carnegie Mellon University
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
	Configuration parameters for the Tcl/Tk GUI\

	This header and tcl/tk headers are known to conflict. This header
	should be included AFTER tcl.h or tk.h, not before.

	@NOTE you must include <tcl.h> BEFORE this file for it to work properly.
*/

#ifndef ASCTK_CONFIG_H
#define ASCTK_CONFIG_H

#include <utilities/ascConfig.h>
/*
 * If we are in a tcl-infested file, define
 * CONST84 to be empty for back-compatibility with
 * tcl8.3
 */
#ifdef TCL_VERSION
# ifndef CONST84
#  define CONST84
#  define QUIET(x) x
#  define QUIET2(x) x
# else
/** use this macro to shut up const when const
    from tcl-land would be going into non-tcl C.
 */
#  define QUIET(x) ((char *)x)
#  define QUIET2(v) ((char **)v)
# endif
#endif

#endif


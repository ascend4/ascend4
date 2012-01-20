/*	ASCEND modelling environment
	Copyright (C) 2005 Jerry St.Clair
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
*/

/**
	Contains the FilePath to Test directory which can later be used to locate files in the
	ASCEND Directory in test-suites
	FIXME Any other better means than a global variable?
*/


#ifndef ASC_TEST_GLOBALS
#define ASC_TEST_GLOBALS

#include <ascend/general/ospath.h>

/* GLOBAL Variable to the Test Directory maintained by test/test.c */
char ASC_TEST_PATH[PATH_MAX];

#endif

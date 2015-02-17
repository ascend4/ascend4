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
*//**
	@file
	Print support utilities for ASCEND unit tests

	These functions provide support for managing a print vtable
	for use during unit testing.
*/

#ifndef PRINTUTIL_H_SEEN
#define PRINTUTIL_H_SEEN

/**
 *  Enables printing of ASCEND messages to the console.
 *  @return Returns 1 if an error occurs, 0 otherwise.
 */
int test_enable_printing(void);

/**
 *  Disables printing of ASCEND messages to the console.
 */
void test_disable_printing(void);

/**
 *  Queries whether printing to the console is enabled.
 *  @return Returns TRUE if printing is enabled, FALSE otherwise.
 */
int test_printing_enabled(void);

#endif  /* PRINTUTIL_H_SEEN */

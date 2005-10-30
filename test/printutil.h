/*
 *  Print support utilities for ASCEND unit tests
 *
 *  Copyright (C) 2005 Jerry St.Clair
 *
 *  This file is part of the Ascend Environment.
 *
 *  The Ascend Environment is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Environment is distributed in hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Print support utilities for ASCEND unit tests.
 *  These functions provide support for managing a print vtable
 *  for use during unit testing.
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

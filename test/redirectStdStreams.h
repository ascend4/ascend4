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
	Centralized redirection of standard streams to file.

	Many ASCEND routines print messages.  In a testing context, it
	is usually preferable to have output redirected to a file.  This
	module provides a centralized location for this code (which may
	end up somewhat platform-dependent).

	Requires:
	#include <stdio.h>
	#include "utilities/ascConfig.h"
*/

#ifndef REDIRECTSTDSTREAMS_H_SEEN
#define REDIRECTSTDSTREAMS_H_SEEN

#if 0 /* disabling this stuff (it was making it hard to debug tests) */

FILE *redirect_stderr(CONST char *filename);
/**<
 *  Redirects stderr to the specified file.
 *  Returns a pointer to the new stream.  If stderr was already being
 *  redirected to a file, the previous file is closed before the new
 *  redirection is set up.  Returns NULL if stderr could not be
 *  redirected.
 */

FILE *reset_stderr(void);
/**<
 *  Restores stderr to the console.
 *  This function closes the redirection file (if any) and
 *  restores stderr to console output.  It returns a pointer
 *  to the new stderr stream.  Returns NULL if stderr could not
 *  be reset.

 */

FILE *redirect_stdin(CONST char *filename);
/**<
 *  Redirects stdin to the specified file.
 *  Returns a pointer to the new stream.  If stdin was already being
 *  redirected to a file, the previous file is closed before the new
 *  redirection is set up.  Returns NULL if stdin could not be
 *  redirected.
 */

FILE *reset_stdin(void);
/**<
 *  Restores stdin to the console.
 *  This function closes the redirection file (if any) and
 *  restores stdin to console output.  It returns a pointer
 *  to the new stdin stream.  Returns NULL if stdin could not
 *  be reset.
 */

FILE *redirect_stdout(CONST char *filename);
/**<
 *  Redirects stdout to the specified file.
 *  Returns a pointer to the new stream.  If stdout was already being
 *  redirected to a file, the previous file is closed before the new
 *  redirection is set up.  Returns NULL if stdout could not be
 *  redirected.
 */

FILE *reset_stdout(void);
/**<
 *  Restores stdout to the console.
 *  This function closes the redirection file (if any) and
 *  restores stdout to console output.  It returns a pointer
 *  to the new stdout stream.  Returns NULL if stdout could not
 *  be reset.
 */

#endif

#endif  /* REDIRECTSTDSTREAMS_H_SEEN */

/*
 *  Standard stream redirection for ASCEND unit tests
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
 *  Centralized redirection of standard streams to file.
 *  Many ASCEND routines print messages.  In a testing context, it
 *  is usually preferable to have output redirected to a file.  This
 *  module provides a centralized location for this code (which may
 *  end up somewhat platform-dependent).
 *  <pre>
 *  Requires:     #include <stdio.h>
 *                #include "utilities/ascConfig.h"
 *
 */

#ifndef REDIRECTSTDSTREAMS_H_SEEN
#define REDIRECTSTDSTREAMS_H_SEEN

/**
 *  Redirects stderr to the specified file.
 *  Returns a pointer to the new stream.  If stderr was already being
 *  redirected to a file, the previous file is closed before the new
 *  redirection is set up.
 */
FILE *redirect_stderr(CONST char *filename);

/**
 *  Restores stderr to the console.
 *  This function closes the redirection file (if any) and
 *  restores stderr to console output.  It returns a pointer
 *  to the new stderr stream.

 */
FILE *reset_stderr(void);

/**
 *  Redirects stdin to the specified file.
 *  Returns a pointer to the new stream.  If stdin was already being
 *  redirected to a file, the previous file is closed before the new
 *  redirection is set up.
 */
FILE *redirect_stdin(CONST char *filename);

/**
 *  Restores stdin to the console.
 *  This function closes the redirection file (if any) and
 *  restores stdin to console output.  It returns a pointer
 *  to the new stdin stream.
 */
FILE *reset_stdin(void);

#endif  /* REDIRECTSTDSTREAMS_H_SEEN */

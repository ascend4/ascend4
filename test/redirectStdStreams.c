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

#include <stdio.h>
#include <assert.h>
#include "utilities/ascConfig.h"
#include "redirectStdStreams.h"

FILE *f_stderr_file = NULL;
FILE *f_stdin_file = NULL;

FILE *redirect_stderr(CONST char *filename)
{
  assert(NULL != filename);
  if (NULL != f_stderr_file) {
    fclose(f_stderr_file);
  }
  return (f_stderr_file = freopen(filename, "w", stderr));
}

FILE *reset_stderr(void)
{
  if (NULL != f_stderr_file) {
    fclose(f_stderr_file);
  }
  return freopen("CON", "w", stderr);
}

FILE *redirect_stdin(CONST char *filename)
{
  assert(NULL != filename);
  if (NULL != f_stdin_file) {
    fclose(f_stdin_file);
  }
  return (f_stdin_file = freopen(filename, "r", stdin));
}

FILE *reset_stdin(void)
{
  if (NULL != f_stdin_file) {
    fclose(f_stdin_file);
  }
  return freopen("CON", "r", stdin);
}


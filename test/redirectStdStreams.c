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

#include <utilities/ascConfig.h>

#ifdef __WIN32__
# include <io.h>
#else
# include <unistd.h>
#endif

#include "redirectStdStreams.h"

static FILE *f_stdin_file = NULL;
static int f_stdin_handle = -1;
static int f_stdin_fileno = -1;

FILE *redirect_stdin(CONST char *filename)
{
  assert(NULL != filename);

  /* close the old file, if any */
  if (NULL != f_stdin_file) {
    fflush(f_stdin_file);
    fclose(f_stdin_file);
    f_stdin_file = NULL;
  }
  fflush(stdin);

  /* the 1st time through, save the stdin fileno */
  if (-1 == f_stdin_fileno) {
    f_stdin_fileno = fileno(stdin);
  }

  /* if this is the 1st redirection, save the file handle */
  if (-1 == f_stdin_handle) {
    if (-1 == (f_stdin_handle = dup(f_stdin_fileno))) {
      return NULL;
    }
  }
  if (NULL == (f_stdin_file = freopen(filename, "r", stdin))) {
    reset_stdin();
  }
  return f_stdin_file;
}


FILE *reset_stdin(void)
{
  if (NULL != f_stdin_file) {
    fflush(f_stdin_file);
    fclose(f_stdin_file);
    f_stdin_file = NULL;
  }
  /* if stdin has been redirected, restore it to original stream */
  if ((-1 == f_stdin_handle) ||
      (0 != dup2(f_stdin_handle, f_stdin_fileno))) {
    return NULL;
  }
  /* reset stored handle and return */
  f_stdin_handle = -1;
  return stdin;
}

static FILE *f_stdout_file = NULL;
static int f_stdout_handle = -1;
static int f_stdout_fileno = -1;

FILE *redirect_stdout(CONST char *filename)
{
  assert(NULL != filename);

  /* close the old file, if any */
  if (NULL != f_stdout_file) {
    fflush(f_stdout_file);
    fclose(f_stdout_file);
    f_stdout_file = NULL;
  }
  fflush(stdout);

  /* the 1st time through, save the stdout fileno */
  if (-1 == f_stdout_fileno) {
    f_stdout_fileno = fileno(stdout);
  }

  /* if this is the 1st redirection, save the file handle */
  if (-1 == f_stdout_handle) {
    if (-1 == (f_stdout_handle = dup(f_stdout_fileno))) {
      return NULL;
    }
  }
  if (NULL == (f_stdout_file = freopen(filename, "w", stdout))) {
    reset_stdout();
  }
  return f_stdout_file;
}


FILE *reset_stdout(void)
{
  if (NULL != f_stdout_file) {
    fflush(f_stdout_file);
    fclose(f_stdout_file);
    f_stdout_file = NULL;
  }
  /* if stdout has been redirected, restore it to original stream */
  if ((-1 == f_stdout_handle) ||
      (0 != dup2(f_stdout_handle, f_stdout_fileno))) {
    return NULL;
  }
  /* reset stored handle and return */
  f_stdout_handle = -1;
  return stdout;
}

static FILE *f_stderr_file = NULL;
static int f_stderr_handle = -1;
static int f_stderr_fileno = -1;

FILE *redirect_stderr(CONST char *filename)
{
  assert(NULL != filename);

  /* close the old file, if any */
  if (NULL != f_stderr_file) {
    fflush(f_stderr_file);
    fclose(f_stderr_file);
    f_stderr_file = NULL;
  }
  fflush(stderr);

  /* the 1st time through, save the stderr fileno */
  if (-1 == f_stderr_fileno) {
    f_stderr_fileno = fileno(stderr);
  }

  /* if this is the 1st redirection, save the file handle */
  if (-1 == f_stderr_handle) {
    if (-1 == (f_stderr_handle = dup(f_stderr_fileno))) {
      return NULL;
    }
  }
  if (NULL == (f_stderr_file = freopen(filename, "w", stderr))) {
    reset_stderr();
  }
  return f_stderr_file;
}


FILE *reset_stderr(void)
{
  if (NULL != f_stderr_file) {
    fflush(f_stderr_file);
    fclose(f_stderr_file);
    f_stderr_file = NULL;
  }
  /* if stderr has been redirected, restore it to original stream */
  if ((-1 == f_stderr_handle) ||
      (0 != dup2(f_stderr_handle, f_stderr_fileno))) {
    return NULL;
  }
  /* reset stored handle and return */
  f_stderr_handle = -1;
  return stderr;
}


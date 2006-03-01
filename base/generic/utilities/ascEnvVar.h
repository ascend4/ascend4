 /*
 *  Ascend Environment Variable Imitation
 *  by Ben Allan
 *  Created: 6/3/97
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: ascEnvVar.h,v $
 *  Date last modified: $Date: 1997/07/18 12:04:08 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Ascend Environment Variable Imitation.
 *
 *  This file exists because win32, among others, can't keep their
 *  POSIX compliance up. In particular, getting and setting
 *  environment vars is exceedingly unreliable.  This file implements
 *  a general way to store and fetch multiple paths.
 *  <pre>
 *  Requires:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 *  Linking against this header requires linking optionally
 *  (depending on defines in list.h and the makefile):
 *      - ascmalloc.o,
 *      - pool.o,
 *  and definitely
 *      - list.o
 *      - ascPanic.o
 *      - ascEnvVar.o
 */

#ifndef __ascenv_h_seen__
#define __ascenv_h_seen__

#define MAX_ENV_VAR_LENGTH 4096
/**< Maximum length of an environment variable string. */

extern int Asc_InitEnvironment(unsigned long anticipated);
/**<
 *  Creates an empty environment of size anticipated.
 *  There is a minimum environment size, so the actual size
 *  can be larger than requested.  The size will be increased
 *  if necessary during use.  Calls to other functions in this
 *  module will fail until Asc_InitEnvironment() is called.  It
 *  should not, however, be called more than once without an
 *  intervening call to Asc_DestroyEnvironment().  It is not a
 *  fatal error to do so, but the subsequent calls will fail.
 *
 *  @param anticipated The expected number of envoronment variables.
 *  @return Returns 0 on success, 1 on failure.
 */

extern void Asc_DestroyEnvironment(void);
/**<
 *  Destroys any information currently in the stored environment.
 *  All memory associated with the storage is released.
 */

extern int Asc_SetPathList(CONST char *var, CONST char *path);
/**<
 *  Assigns the elements in path to ASCEND environment variable var.
 *  path should contain one or more strings delimited with :
 *  (Linux/unix) or ; (Windows).  Leading and trailing whitespace is
 *  stripped, as is whitespace around any delimiters.  Spaces may not
 *  be embedded in var.  Spaces embedded in the path string(s) are
 *  assumed significant and left in place.  Leading, trailing, and
 *  embedded double delimiters will be parsed as empty strings at the
 *  beginning, end, or middle of the list of paths.  Empty strings
 *  will not be stored, so if all substrings parse to empty, you will
 *  have an environment variable registered that has no path values.<br><br>
 *
 *  Note that if any of the substrings in path are longer than
 *  MAX_ENV_VAR_LENGTH, they will be truncated.  The input strings are
 *  not kept by this routine; internal copies are made as needed.<br><br>
 *
 *  If the environment variable is already assigned, it is replaced
 *  with the new path value.  Otherwise, a new ASCEND environement
 *  variable is created.<br><br>
 *
 *  An error condition occurs and non-zero is returned if:
 *    - Asc_InitEnvironment() has not been called
 *    - var or path is NULL
 *    - var or path is an empty string
 *    - var is longer than MAX_ENV_VAR_LENGTH
 *    - var contains embedded spaces
 *    - available memory is insufficient
 *
 *  If any of these conditions exist, then the current environment
 *  structure is not modifed.  That is, an existing environment variable
 *  will not have it's path value(s) modified if one of these occurs.
 *  Otherwise, you should assume that the value of the old variable will
 *  be destroyed even if the new value is not successfully assigned.
 *
 *  @param var  Name of the ASCEND environment variable to create
 *              or replace.
 *  @param path String(s) to assign to the new environment variable
 *              delimited by : (Linux/unix) or ; (Windows) as appropriate.
 *  @return Returns 0 if var is successfully created and assigned,
 *          1 otherwise.
 */

extern int Asc_PutEnv(CONST char *putenv_input_string);
/**<
 *  Creates an ASCEND environment variable from a putenv()-type string.
 *  The input string should have the form "varname=path".  The path
 *  portion should contain one or more substrings delimited with :
 *  (Linux/unix) or ; (Windows).  Leading and trailing whitespace is
 *  stripped, as is whitespace around any delimiters.  Spaces may not
 *  be embedded in the varname part of the string.  Spaces embedded in
 *  the path string(s) are assumed significant and left in place.
 *  Leading, trailing, and embedded double delimiters will be parsed
 *  as empty strings at the beginning, end, or middle of the list of
 *  paths.  Empty strings will not be stored, so if all substrings
 *  parse to empty, you will have an environment variable registered
 *  that has no path values.<br><br>
 *
 *  Note that putenv_input_string may not be longer than
 *  MAX_ENV_VAR_LENGTH.  The input string is not kept by this routine;
 *  internal copies are made as needed.<br><br>
 *
 *  If the environment variable is already assigned, it is replaced
 *  with the new path value.  Otherwise, a new ASCEND environement
 *  variable is created.<br><br>
 *
 *  An error condition occurs and non-zero is returned if:
 *    - Asc_InitEnvironment() has not been called
 *    - putenv_input_string is NULL
 *    - putenv_input_string is an empty string
 *    - putenv_input_string is longer than MAX_ENV_VAR_LENGTH
 *    - the variable name part of putenv_input_string is empty or
 *      contains embedded spaces
 *    - putenv_input_string does not contain an '=' char
 *    - available memory is insufficient
 *
 *  If any of these conditions exist, then the current environment
 *  structure is not modifed.  That is, an existing environment variable
 *  will not have it's path value(s) modified if one of these occurs.
 *  Otherwise, you should assume that the value of the old variable will
 *  be destroyed even if the new value is not successfully assigned.
 *
 *  @param putenv_input_string  A string having the form "varname=path"
 *                              which will be parsed to create an
 *                              ASCEND environment variable named
 *                              varname having the value path.
 *  @return Returns 0 if variable is successfully created and assigned,
 *          1 otherwise.
 */

extern int Asc_ImportPathList(CONST char *osEnvVar);
/**<
 *  Imports a system environment variable into the ASCEND environment.
 *  If osEnvVar is already a variable in the ASCEND environment space,
 *  it's value is replaced with the system version.  The imported path
 *  is parsed into substrings delimited by ':' or ';' depending on
 *  platform.<br><br>
 *
 *  Returns non-zero if the variable is not found or empty, memory cannot
 *  be allocated, or the syntax of the variable is unexpected (see
 *  Asc_SetPathList().
 *
 *  @param osEnvVar The name of a system environment variable to import.
 *  @return Returns 0 if variable is successfully imported, 1 otherwise.
 */

extern int Asc_AppendPath(char *envvar, char *newelement);
/**<
 *  Adds a new element to the list of values for an ASCEND environment
 *  variable.  If envvar does not exist, it is created.  newelement is
 *  not parsed into substrings or checked for validity.  The input strings
 *  are not kept by this routine; internal copies are made as needed.<br><br>
 *
 *  An error condition occurs and non-zero is returned if:
 *    - Asc_InitEnvironment() has not been called
 *    - envvar or newelement is NULL
 *    - envvar or newelement is an empty string
 *    - available memory is insufficient
 *
 *  @param envvar     Name of the ASCEND environment variable to modify.
 *  @param newelement New element to add to the list of values for envvar.
 *  @return Returns 0 if newelement was successfully added, 1 otherwise.
 */

extern char **Asc_GetPathList(char *envvar, int *argcPtr);
/**<
 *  Retrieve the current value(s) for ASCEND environment variable envvar.
 *  The values are returned as an array of pointers to the value strings.
 *  On return, argcPtr will be set to the number of elements in the
 *  returned array.  If envar or argcPtr is NULL, or Asc_InitEnvironment()
 *  has not been called, *argcPtr will be -1.  If envar cannot be found
 *  (including if envvar is empty), *argcPtr will be 0.  In either of
 *  these cases, the returned array will be NULL.  Otherwise, the caller
 *  is responsible for freeing argv;  when argv is freed all the data it
 *  contains is simultaneously freed.
 *
 *  @param envvar  Name of the ASCEND environment variable to query.
 *  @param argcPtr Address of an int variable to hold the number of
 *                 elements in the returned array.
 *  @return Returns an array of *argcPtr elements containing the string
 *          values of envvar.  If *argcPtr is -1 (error) or 0 (none found),
 *          the returned array pointer will be NULL.
 */

extern char *Asc_GetEnv(char *envvar);
/**<
 *  Retrieve the value(s) of ASCEND environment variable envvar
 *  as a delimited string.  The elements of envvar are assembled
 *  into a single string delimited by ':' or ';' depending on
 *  platform.  The returned pointer will be NULL if an error occurs,
 *  including envvar being NULL or empty, or Asc_InitEnvironment()
 *  not having been called.  The caller is responsible for freeing
 *  the returned pointer.
 *
 *  @param envvar  Name of the ASCEND environment variable to query.
 *  @return Pointer to a string containing the value(s) of envvar
 *          delimited by ':' or ';' depending on platform, or NULL
 *          on error.  The caller is responsible for freeing it.
 */

extern char **Asc_EnvNames(int *argc);
/**<
 *  Retrieve a list of currently defined ASCEND environment variables.
 *  Pointers to the variable names are returned in a NULL-terminated
 *  array having (*argc + 1) elements.  The caller is responsible for
 *  freeing argc, but should under no circumstances change any of the
 *  strings it points to.<br><br>
 *
 *  If Asc_InitEnvironment() has not been called or memory cannot be
 *  allocated, *argc will be -1.  Otherwise, *argc will be the number
 *  of names found and loaded into the returned array.  If *argc >= 0,
 *  the returned array should be freed by the caller.
 *
 *  @param argc Address of an int variable to hold the number of
 *              non-NULL elements in the returned array.
 *  @return Returns an array of *argc elements containing the string
 *          values of the environment variable names.  If *argc is -1
 *          (error) the returned array pointer will be NULL.  Otherwise,
 *          the caller should free the array when finished with it.
 */

#endif /* __ascenv_h_seen__ */


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
 *
 *  This file exists because win32, among others, can't keep their
 *  POSIX compliance up. In particular, getting and setting
 *  environment vars is exceedingly unreliable.
 *  This file implements a general way to store and fetch multiple
 *  paths.
 */

/* Using this header does not require using any other headers.
 * Linking against this header requires linking 
 * optionally (depending on defines in list.h and the makefile):
 * ascmalloc.o,
 * pool.o,
 * and definitely
 * list.o
 * ascpanic.o
 * ascEnvVar.o
 */

#ifndef __ASCENV_H_SEEN__
#define __ASCENV_H_SEEN__

extern int Asc_InitEnvironment(int);
/*
 * err = Asc_InitEnv(anticipated);
 * Creates an empty environment of size anticipated.
 * Calls to other functions in this file will fail until
 * this is done.
 */

extern void Asc_DestroyEnvironment(void);
/*
 * Destroys any information currently in the environment.
 */

extern int Asc_SetPathList(CONST char *, CONST char *);
/*
 * err = Asc_SetPathList(var,path);
 * Takes the value of path and assigns it to var,
 * allowing blanks around :,; if needed, as well as trimming leading
 * trailing whitespace..
 * Creates an ascend environment variable structure.
 * The input strings are not kept by this routine; they is
 * copied in various ways.
 * If available memory is insufficient, returns nonzero.
 * The path should be appropriate
 * to the platform the code is running on, as we parse it
 * out according to the POSIX or Microskunk conventions.
 * Spaces in the string are assumed significant
 * except for trailing whitespace.
 */

extern int Asc_PutEnv(char *);
/*
 * err = Asc_PutEnv(putenv_input_string);
 * Takes an input string of the form "%s=%s"
 * appropriate for a putenv (but allowing blanks around =,:,; if needed)
 * and creates an ascend environment variable structure.
 * The putenv_input_string is not kept by this routine; it is
 * copied in various ways.
 * If available memory is insufficient, returns nonzero.
 * The putenv_input_string should be appropriate
 * to the platform the code is running on, as we parse it
 * out according to the POSIX or Microskunk conventions.
 * Spaces in the string are assumed significant
 * except for trailing whitespace.
 */

extern int Asc_ImportPathList(CONST char *);
/*
 * err = Asc_ImportPathList(osEnvVar)
 * Looks for an environment variable using getenv() and
 * transports it into the asc environment space at its current value.
 * Returns nonzero if not found or empty or memory shortage
 * or unexpected syntax.
 * Interprets the path found in terms of : or ; depending on platform.
 */


extern int Asc_AppendPath(char *, char*);
/*
 * err = Asc_AppendPath(envvar, newelement);
 * Takes the string newelement and adds it to the list of
 * values for envvar. If envvar does not exist, it is created.
 * Neither envvar nor newelement is kept by the routine; they
 * may be copied in various ways.
 * If available memory is insufficient, returns nonzero.
 * The form of the newelement is not checked.
 */

extern char **Asc_GetPathList(char *, int *);
/*
 * argv = Asc_GetPathList(envvar, argcPtr);
 * Sets argcPtr to the length of the argv returned.
 * argv[0..*argcPtr-1] are the elements of the path.
 * If memory fail or bad input, *argcPtr == -1, else
 * it is 0 for unknown variables. In either case, argv will be NULL.
 * User is responsible for freeing argv, and when argv
 * is freed all the data it contains is simultaneously freed.
 */

extern char *Asc_GetEnv(char *);
/*
 * pathvar = Asc_GetEnv(envvar);
 * Returns the elements of envvar assembled according to the
 * platform convention (; on MS or : on UNIX) into a
 * single string. Not very useful in most cases.
 * User is responsible for freeding pathvar.
 */

extern char **Asc_EnvNames(int *);
/*
 * argvconst = Asc_EnvNames(&argc);
 * Returns an array of char * and fills argc with the size of the
 * array. The caller is responsible for freeing argv, but should
 * under no circumstances change any of the strings it points to.
 * If argc is -1, argv is NULL due to insufficient memory.
 * If argc is 0, argv should be freed.
 */

#endif /* __ASCENV_H_SEEN__ */

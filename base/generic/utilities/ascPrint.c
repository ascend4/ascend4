/*
 *  ASCEND Printf Substitutes
 *  by Mark Thomas
 *  Created: 27.May.1997
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascPrint.c,v $
 *  Date last modified: $Date: 1997/10/29 13:08:49 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND utilities.
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND utilities is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND utilities is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/*  ChangeLog
 *
 *  10/13/2005  Added Asc_PrintHasVTable() so user can tell if a vtable
 *              has already been registered.  (J.D. St.Clair)
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ascConfig.h"
#include "ascPrint.h"

/* <..> path is required here for the autotools build to be happy */
#include <utilities/config.h>

static struct Asc_PrintVTable *g_Asc_printVtables = NULL;

#define PRINT_BUFFER_SIZE 16380

int Asc_PrintPushVTable(struct Asc_PrintVTable *vtable)
{
  if ((vtable == NULL) ||
      (vtable->name == NULL) ||
      (vtable->print == NULL) ||
      (vtable->fflush == NULL) ||
      (vtable->next != NULL)) {
    return 1;
  }
  /* push it on the stack */
  vtable->next = g_Asc_printVtables;
  g_Asc_printVtables = vtable;
  return 0;
}

struct Asc_PrintVTable *Asc_PrintRemoveVTable(CONST char *name)
{
	struct Asc_PrintVTable *prev;
	struct Asc_PrintVTable *vt;

  /* skip notables cases */
  if ((g_Asc_printVtables == NULL) || (NULL == name)) {
    return NULL;
  }
  /* LIFO is easy */
  vt = g_Asc_printVtables;
  if ( strcmp(vt->name,name) == 0 ) {
    g_Asc_printVtables = vt->next;
    return vt;
  }
  /* middle chain is worse */
  prev = g_Asc_printVtables;
  vt = prev->next;
  while (vt != NULL) {
    if ( strcmp(vt->name,name) == 0 ) {
      prev->next = vt->next;
      return vt;
    }
    vt = vt->next;
    prev = prev->next;
  }
  return NULL;
}

int Asc_PrintHasVTable(CONST char *name)
{
  struct Asc_PrintVTable *vt;

  if ((g_Asc_printVtables == NULL) ||
      (NULL == name)) {
		return FALSE;
  }
  vt = g_Asc_printVtables;
  while (vt != NULL) {
    if (strcmp(vt->name, name) == 0) {
      return TRUE;
    }
    vt = vt->next;
  }
  return FALSE;
}


/*
 *  int Asc_Printf(format, variable_number_args)
 *      CONST char *format;
 *      variable_number_args;
 *
 *  Using the sprintf-style format string `format', print the
 *  `variable_number_args' to an approximation of stdout.
 *
 *  This function just initializes the variable_number_args into a
 *  va_list, and then calls AscPrint to actually do the work.
 */
int Asc_Printf(CONST char *format, ...)
{
  va_list args;    /* the variable number of arguments */
  int result = 0;  /* the result of the call to AscPrint; our return value */

  struct Asc_PrintVTable * vt = g_Asc_printVtables;
  while (vt != NULL) {
    /* create the va_list */
    va_start( args, format );
    result = vt->print( stdout, format, args );
    /* cleanup */
    va_end( args );
    vt = vt->next;
  }
  /* only the result of the last printer makes it out */
  return result;
}


/*
 *  int Asc_FPrintf(fp, format, variable_number_args)
 *      FILE *fp;
 *      CONST char *format;
 *      variable_number_args;
 *
 *  Using the sprintf-style format string `format', print the
 *  `variable_number_args' to the file pointer `fp'.
 *
 *  This function just initializes the variable_number_args into a
 *  va_list, and then calls AscPrint to actually do the work.
 */
int Asc_FPrintf(FILE *fp, CONST char *format, ...)
{
  va_list args;    /* the variable number of arguments */
  int result=0;    /* the result of the call to AscPrint; our return value */

  struct Asc_PrintVTable * vt = g_Asc_printVtables;
  while (vt != NULL) {
    /* create the va_list */
    va_start( args, format );
    result = vt->print( fp, format, args );
    /* cleanup and return */
    va_end( args );
    vt = vt->next;
  }
  /* only the result of the last printer makes it out */
  return result;
}

/**
	Var-arg version of the above, used by error.h
*/
int Asc_VFPrintf(FILE *fp, CONST char *format, va_list args){
  int result=0;    /* the result of the call to AscPrint; our return value */

  struct Asc_PrintVTable * vt;
  for(vt = g_Asc_printVtables; vt != NULL; vt = vt->next) {
    result = vt->print(fp, format, args);
  }
  return result;
}

/*
 *  int Asc_FFlush(fileptr)
 *      FILE *fileptr;
 *
 *  Flush output to the file pointed to by the file pointer `fileptr';
 *  return 0 for success and EOF for failure.
 *
 *  This is needed for consistency with Asc_FPrintf() and Asc_Printf().
 */
int Asc_FFlush( FILE *fileptr )
{
  int result = 0;
  struct Asc_PrintVTable * vt = g_Asc_printVtables;
  while (vt != NULL) {
    result = vt->fflush(fileptr);
    vt = vt->next;
  }
  /* only the result of the last printer makes it out */
  return result;
}


/*
 *  int Asc_FPutc( c, fileptr );
 *      int c;
 *      FILE *fileptr;
 *
 *  Print the character `c' to the output file pointed to by the
 *  file pointer `fileptr'; return 0 for success and EOF for failure.
 *
 *  This is needed for consistency with Asc_FPrintf() and Asc_Printf().
 */
int Asc_FPutc( int c, FILE *fileptr )
{
  /*
   *  Call vtable list for output to stdout and stderr, or the real putc
   *  for output to other file handles
   */
  if(( fileptr == stdout )  || ( fileptr == stderr )) {
    return Asc_FPrintf( fileptr, "%c", c );
  } else {
    return fputc( c, fileptr );
  }
}


/*
 *  int Asc_Putchar( c );
 *      int c;
 *
 *  Print the character `c' to `stdout'; return 0 for success and
 *  EOF for failure.
 *
 *  This is needed for consistency with Asc_FPrintf() and Asc_Printf().
 */
int Asc_Putchar( int c )
{
  return Asc_Printf( "%c", c );
}

#ifdef ASC_XTERM_COLORS
static int color_test(){
	static int use_xterm_color = 0;
	char *term;
	if(!use_xterm_color){
		term = getenv("TERM");
		if(term!=NULL){
			if(strcmp(term,"msys")==0 || strcmp(term,"xterm")==0){
				/* fprintf(stderr,"\n\n\nCOLOR CODES \033[1mWILL\033[0m BE USED\n\n\n"); */
				use_xterm_color=1;
			}else{
				use_xterm_color=-1;
				Asc_FPrintf(stderr,"\n\n\n----------------------------------\nCOLOR CODES WILL NOT BE USED\n\n\n");
			}
		}else{
			fprintf(stderr,"\n\n\n----------------------------------\nCOLOR CODES WILL NOT BE USED (NO ENV VAR 'TERM')\n\n\n");
			use_xterm_color=-1;
		}
	}else{
		Asc_FPrintf(stderr,"color=%d",use_xterm_color);
	}
	return use_xterm_color;
}
#endif /*ASC_XTERM_COLORS*/

/**
	Little routine to aid output of XTERM colour codes where supported.
	Not very efficient, so use sparingly.
*/
int color_on(FILE *f, const char *colorcode){
#ifdef ASC_XTERM_COLORS
	int use_color = color_test();

	if(colorcode!=NULL && use_color==1){
		return fprintf(f,"\033[%sm",colorcode);
	}
#else
	(void)f; (void)colorcode;
#endif
	return 0;
}

/**
	Little routine to aid output of XTERM colour codes where supported.
	Not very efficient, so use sparingly.
*/
int color_off(FILE *f){
#ifdef ASC_XTERM_COLORS
	int use_color = color_test();
	if(use_color==1){
		return fprintf(f,"\033[0m");
	}

#else
	(void)f;
#endif
	return 0;
}

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

#include <stdarg.h>
#include "tcl.h"
#include "utilities/ascConfig.h"
#include "utilities/ascPrint.h"
#include "utilities/ascPrintType.h"

/*
 *  Only compile this file if we are using Asc_Printf()
 */
#ifdef USE_ASC_PRINTF

#define PRINT_BUFFER_SIZE 16380

/*
 *  The Tcl channels for stdout and stderr.
 *
 *  These need to be initialized by calling Asc_PrintInit() before
 *  calls to Asc_Printf() will write to the Tcl Channels.
 */
static Tcl_Channel g_tclout = NULL;   /* the Tcl channel for stdout */
static Tcl_Channel g_tclerr = NULL;   /* the Tcl channel for stderr */

static struct Asc_PrintVTable g_Asc_PrintVTable_Tcl;
static int Asc_PrintTcl(FILE *fp, CONST char *format, va_list args);
static int Asc_FFlushTcl(FILE *fp);
static char *g_AscTclPrintName = "tclPrintChannels";

int Asc_PrintInit_TclVtable(void)
{
  g_Asc_PrintVTable_Tcl.name = g_AscTclPrintName;
  g_Asc_PrintVTable_Tcl.print = Asc_PrintTcl;
  g_Asc_PrintVTable_Tcl.fflush = Asc_FFlushTcl;
  g_Asc_PrintVTable_Tcl.next = 0;
  return Asc_PrintPushVTable(&g_Asc_PrintVTable_Tcl);
}

int Asc_PrintInit_Tcl(void)
{

  /*
   *  Initialize ASCEND's ideas of Tcl's idea of stdout and stderr.
   */
  g_tclout = Tcl_GetStdChannel( TCL_STDOUT );
  g_tclerr = Tcl_GetStdChannel( TCL_STDERR );

  if(( g_tclout == NULL ) || ( g_tclerr == NULL )) {
    return 1;
  }
  return 0;
}

void Asc_PrintFinalize_Tcl(void)
{
	Asc_PrintRemoveVTable(g_AscTclPrintName);
}


/*
 *  int AscPrint(fp, format, args)
 *      FILE *fp;
 *      CONST char *format;
 *      va_list args;
 *
 *  Using the sprintf-style format string `format', print the arguments in
 *  the va_list `args' to the file pointer `fp'.  Return the number of
 *  bytes printed.
 *
 *  NOTE: The last argument to this function is a VA_LIST, NOT a variable
 *  number of arguments.  You must initialize the va_list before calling
 *  this function, and cleanup the va_list afterwards.
 */
static
int Asc_PrintTcl(FILE *fp, CONST char *format, va_list args)
{
  static char buf[PRINT_BUFFER_SIZE]; /* the buffer that holds the output */

  if(( fp == stdout ) && ( g_tclout != NULL )) {
    vsprintf( buf, format, args );
    return Tcl_Write( g_tclout, buf, -1 );
  }
  else if(( fp == stderr ) && ( g_tclerr != NULL )) {
    vsprintf( buf, format, args );
    return Tcl_Write( g_tclerr, buf, -1 );
  }
  /* TODO: should this fail loudly when a NULL fp is passed in? */
  else if (fp != NULL) {
    return vfprintf( fp, format, args );
  }
  else {
    return 0;
  }
}


/*
 *  int Asc_FFlushTcl(fileptr)
 *      FILE *fileptr;
 *
 *  Flush output to the file pointed to by the file pointer `fileptr';
 *  return 0 for success and EOF for failure.
 *
 *  This is needed for consistency with Asc_FPrintf() and Asc_Printf().
 */
extern
int Asc_FFlushTcl( FILE *fileptr )
{
  if(( fileptr == stdout ) && ( g_tclout != NULL )) {
    if( Tcl_Flush( g_tclout ) != TCL_OK ) {
      return EOF;
    }
    return 0;
  }
  else if(( fileptr == stdout ) && ( g_tclerr )) {
    if( Tcl_Flush( g_tclerr ) != TCL_OK ) {
      return EOF;
    }
    return 0;
  }
  else {
    return fflush(fileptr);
  }
}

#endif /*  USE_ASC_PRINTF  */

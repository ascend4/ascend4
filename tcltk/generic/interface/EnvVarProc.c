/*
 *  Ascend Environment Variable Imitation Tcl interface
 *  by Ben Allan
 *  Created: 6/3/97
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: EnvVarProc.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:06 $
 *  Last modified by: $Author: ballan $
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
 *  It does not interact in any way the the Tcl global "env" array.
 */

#include <tcl.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascEnvVar.h>
#include "HelpProc.h"
#include "EnvVarProc.h"

#ifndef lint
CONST char *EnvVarProcID = "$Id: EnvVarProc.c,v 1.7 2003/08/23 18:43:06 ballan Exp $";
#endif

STDHLF(Asc_EnvVarCmd,(Asc_EnvVarCmdHL1, Asc_EnvVarCmdHL2,
  Asc_EnvVarCmdHL3, Asc_EnvVarCmdHL4, Asc_EnvVarCmdHL5, Asc_EnvVarCmdHL6,
  Asc_EnvVarCmdHL7, Asc_EnvVarCmdHL8, Asc_EnvVarCmdHL9, Asc_EnvVarCmdHL10,
  HLFSTOP));

int Asc_EnvVarCmd(ClientData cdata, Tcl_Interp *interp,
                  int argc, CONST84 char *argv[])
{
  const char **envargv;
  char *path;
  int envargc;
  int c;

  ASCUSE;
  if (argc <2) {
    Asc_HelpGetUsage(interp,argv[0]);
    return TCL_ERROR;
  }
  switch (argv[1][0]) {
  case 'a':
    /*
     * 	append	varname pathelement
     */
    if (argc != 4) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," append var pathelement",
        (char *)NULL);
      return TCL_ERROR;
    }
    if (Asc_AppendPath(QUIET(argv[2]),QUIET(argv[3]))!=0) {
      Tcl_AppendResult(interp,"error appending var ",argv[2]," with ",argv[4],
        (char *)NULL);
      return TCL_ERROR;
    }
    break;
  case 'e':
    /* 	export	var
     */
    if (argc != 3) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," export var",
        (char *)NULL);
      return TCL_ERROR;
    }
    path = Asc_GetEnv(QUIET(argv[2]));
    if (path !=NULL) {
      Tcl_SetVar2(interp,"env",argv[2],path,TCL_GLOBAL_ONLY);
      ascfree(path);
    } else {
      Tcl_AppendResult(interp,"export ascend environment var not found : ",
        argv[0],argv[1],argv[2],
        (char *)NULL);
      return TCL_ERROR;
    }
    break;
  case 'g':
    /* 	get 	var
     */
    if (argc != 3) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," get var",
        (char *)NULL);
      return TCL_ERROR;
    }
    path = Asc_GetEnv(QUIET(argv[2]));
    if (path == NULL) {
      Tcl_AppendResult(interp,"ascend environment var not found : ",
        argv[0],argv[1],argv[2],
        (char *)NULL);
      return TCL_ERROR;
    }
    Tcl_AppendResult(interp,path, (char *)NULL);
    ascfree(path);
    break;
  case 'i':
/* 	import	var
 */
    if (argc != 3) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," import var",
        (char *)NULL);
      return TCL_ERROR;
    }
    if (Asc_ImportPathList(argv[2])!=0) {
      Tcl_AppendResult(interp,"C environment var not found : ",
        argv[0],argv[1],argv[2],
        (char *)NULL);
      return TCL_ERROR;
    }
    break;
  case 'l':
    /* 	list	var
     */
    if (argc != 3) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," list var",
        (char *)NULL);
      return TCL_ERROR;
    }
    envargv = Asc_GetPathList(QUIET(argv[2]),&envargc);
    if (envargv==NULL || envargc <1) {
      Tcl_AppendResult(interp,"ascend environment var not found : ",
        argv[0],argv[1],argv[2],
        (char *)NULL);
      return TCL_ERROR;
    }
    for (c = 0; c < envargc; c++) {
      Tcl_AppendElement(interp,envargv[c]);
    }
    ascfree(envargv);
    break;
  case 'n':
    /* 	names
     */
    if (argc != 2) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," names",
        (char *)NULL);
      return TCL_ERROR;
    }
    envargv = Asc_EnvNames(&envargc);
    if (envargv==NULL || envargc <1) {
      Tcl_AppendElement(interp,"");
    } else {
      for (c = 0; c < envargc; c++) {
        Tcl_AppendElement(interp,envargv[c]);
      }
      ascfree(envargv);
    }
    break;
  case 'p':
    /* 	put	input_string
     */
    if (argc != 3) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," put putenv_string",
        (char *)NULL);
      return TCL_ERROR;
    }
    if (Asc_PutEnv(QUIET(argv[2]))!=0) {
      Tcl_AppendResult(interp,"error in processing: ",
        argv[0],argv[1],argv[2],
        (char *)NULL);
      return TCL_ERROR;
    }
    break;
  case 's':
    /* 	set	var path
     */
    if (argc != 4) {
      Tcl_AppendResult(interp,"wrong number of args : ",
        argv[0]," set var path",
        (char *)NULL);
      return TCL_ERROR;
    }
    if (Asc_SetPathList(argv[2],argv[3])!=0) {
      Tcl_AppendResult(interp,"error in processing: ",
        argv[0],argv[1],argv[2], argv[3],
        (char *)NULL);
      return TCL_ERROR;
    }
    break;
  default:
    Asc_HelpGetUsage(interp,argv[0]);
    return TCL_ERROR;
  }
  return TCL_OK;
}

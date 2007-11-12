/*
 *  Qlfdid.c
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.22 $
 *  Version control file: $RCSfile: Qlfdid.c,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#define ASC_BUILDING_INTERFACE

#include <tcl.h>
#include <tk.h>
#include "config.h"
#include <general/list.h>
#include <compiler/instance_enum.h>
#include <compiler/qlfdid.h>

int Asc_BrowQlfdidSearchCmd(ClientData cdata, Tcl_Interp *interp,
                        int argc, CONST84 char *argv[])
{
  char temp[MAXIMUM_ID_LENGTH];
  struct gl_list_t *search_list;
  struct SearchEntry *se;
  unsigned long len,c;

  UNUSED_PARAMETER(cdata);

  if ( argc != 2 ) {
    Tcl_SetResult(interp,"wrong # args : Usage is qlfdid \"name\"",TCL_STATIC);
    return TCL_ERROR;
  }
  search_list = Asc_BrowQlfdidSearch(QUIET(argv[1]),temp);
  g_relative_inst = g_search_inst;
  if ((g_search_inst==NULL) || (search_list==NULL)) {
    Tcl_AppendResult(interp,"Orphaned ",temp,(char *)NULL);
    return TCL_ERROR;
  } else {
    len = gl_length(search_list);
    for(c=1;c<=len;c++) {
      se = gl_fetch(search_list,c);
      Tcl_AppendResult(interp,se->name," ",(char *)NULL);
    }
    Asc_SearchListDestroy(search_list);
    return TCL_OK;
  }
}



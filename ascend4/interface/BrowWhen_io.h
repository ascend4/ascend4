/*
 *  BrowWhen_io.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: BrowWhen_io.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:04 $
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

#ifndef when_io_module
#define when_io_module

/*
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "BrowWhen_io.h"
 */


extern int Asc_BrowWriteWhenListCmd(ClientData cdata, Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/*
 *  Registered as: bgetwhens  ?cur?search? save
 *  Write a list of when statements to the Tcl interpreter. This option
 *  is active when the current instance is a Model or a WHEN.
 *
 *  If the Instance is a Model, both
 *  the list of WHENs inside the model    and
 *  the list of WHENs which include the Model in some CASE
 *  will be send to the Tcl interpreter, differentiating each of the two
 *  list.
 *
 *  If the Instance is a WHEN, both
 *  the WHEN statement by itself   and
 *  the WHEN statements which include this WHEN recursively
 *  will be send to the Tcl interpreteer.
 *
 *  If the Instance is an array of WHENs, only the list of WHENs will be
 *  provided (just by simplicity) and send to the Tcl interpreter.
 */



extern int Asc_BrowWriteWhensForInstanceCmd(ClientData cdata,
                                            Tcl_Interp *interp,
                                            int argc,
                                            CONST84 char *argv[]);
/*
 *  Registered as: __brow_whensforinstance ?cur?search.
 *
 *  Will return a proper Tcl list of all whens associated
 *  with the given instance. Works on the current or the search instance.
 *  Will return TCL_ERROR if the instance is NULL, or the atom is not
 *  appropriate.
 *
 *  By "associated" I mean:
 *
 *  1) If the instance is integer, symbol or boolean, this instance
 *  may be part of the list of variables of one/several WHEN statements.
 *  If this is the case, the list of "associated" WHENs will be send to
 *  the Tcl interpreter.
 *
 *  2) If the instance is a relation, the list of WHENs which include
 *  that relation in some CASE, will be send to the interpreter.
 *  When the instance is a Model or a WHEN, the previous function
 *  Asc_BrowWriteWhenListCmd is used instead.
 *
 *  IMPORTANT: For the case of relations, this function works if the
 *  WHEN include the equation explicitly. It will not work if the
 *  WHEN inlcude the equation implicitly by including the model which
 *  contains the relation.
 */

#endif /* module loaded */



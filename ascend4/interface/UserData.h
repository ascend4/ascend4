/*
 *  UserData.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: UserData.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:09 $
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

#ifndef UserData_module_loaded
#define UserData_module_loaded

/*
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "interface/UserData.h"
 */


extern void Asc_UserDataLibraryInitialize(void);
/*
 *  Initializes the user data library to NULL. Should only be
 *  called once. Does not check for existence or anything.
 */

extern int Asc_UserDataInitializeCmd(ClientData cdata, Tcl_Interp *interp,
                                     int argc, CONST84 char *argv[]);
/*
 *  Registered as:  __userdata_init
 *  The tcl command to allow initialization interactively.
 *  Initializes the user data library to NULL. Should only be
 *  called once. Does not check for existence or anything.
 */

extern int Asc_UserDataDestroyCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, CONST84 char *argv[]);
/*
 *  Registered as: __userdata_destroy ?one?all? id
 *  Usage : __userdata_destroy one foo;
 *  Usage : __userdata_destroy all;
 *  Will deallocate the memory associated with the user data with given
 *  id. In the 'all' mode, will deallocate everything and set up the
 *  user data library for initializion. This is the mode that would be
 *  used if the entire library were to be flushed if say, a simulation
 *  were to be deleted.
 */

extern int Asc_UserDataCreateCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, CONST84 char *argv[]);
/*
 *  Registered as : __userdata_create type
 *  This creates a node and stores it in the UserData table.
 *  Type is any valid string of "real_info", "inst_info",
 *  "probe_info", "list_info". The id of the user_data node created will
 *  be returned and has to be the refence id for subsequent queries on the
 *  object.
 */

extern int Asc_UserDataSaveValuesCmd(ClientData cdata, Tcl_Interp *interp,
                                 int argc, CONST84 char *argv[]);
/*
 *  Registered as : __userdata_save from to
 *  Here 'from' is either of current, search, of an id which has been
 *  previously been stored in the table as an inst_info.
 *  The userdata library is searched for the given from instance. If found
 *  will simply update the REAL_ATOM_INST in the virtual file. Otherwise
 *  it will create a userdata node, visit the instance tree to write to the
 *  virtual file, and add this node to the userdata table.
 *  The id of the node is returned if all goes well.
 */

extern int Asc_UserDataRestoreValuesCmd(ClientData cdata, Tcl_Interp *interp,
                                    int argc, CONST84 char *argv[]);
/*
 *  Registered as : __userdata_restore id
 *  The id is looked up in the userdata library. If not found an error will
 *  be returned. Otherwise the values stored in the virtual file will be
 *  copied back to the instance tree. The userdata associated with this id
 *  must be of type real_info.
 */

extern int Asc_UserDataQueryCmd(ClientData cdata, Tcl_Interp *interp,
                            int argc, CONST84 char *argv[]);
/*
 *  Registered as : __userdata_query ?type?exist? id.
 *  This provides some general query commands. At the moment the only
 *  queries supported are the type of an id and whether a given id
 *  exists within the user_data library. In case a given id does not
 *  exist the string "error_info" will be returned.
 */

extern int Asc_UserDataPrintLibrary(ClientData cdata, Tcl_Interp *interp,
                                int argc, CONST84 char *argv[]);
/*
 *  Registered as : __userdata_print
 *  This command will return a formatted string of containing the contents
 *  of the user_data library. The string will be a proper list with list
 *  elements of the form {id type}.
 */

/*
 * It is hoped to register two functions of the form.
 * __userdata__apply
 *       ?to_inst_info?to_list_info?to_real_info?to_probe_info?
 *       id funcname.
 *
 * where: id is the id of a user data stored in the user data library.
 * 	  funcname is the id of a function possibly also stored in
 * 	  the user data library, or maybe the external function library.
 *	  that will operate over the different user data types stored in
 * 	  in the userdata library.
 */
#endif /* UserData_module_loaded */

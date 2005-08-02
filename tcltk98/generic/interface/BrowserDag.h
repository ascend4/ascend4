/*
 *  BrowserDag.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: BrowserDag.h,v $
 *  Date last modified: $Date: 1997/07/18 12:22:08 $
 *  Last modified by: $Author: mthomas $
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

/** @file
 *  Browser Dag Routines
 *  <pre>
 *  To include this header, you must include the following:
 *      #include "tcl.h"
 *      #include "utilities/ascConfig.h"
 *      #include "interface/BrowserDag.h"
 *  </pre>
 *  @todo Fix comments in interface/BrowserDag.h
 */

#ifndef __BrowserDag_module__
#define __BrowserDag_module__

extern int Asc_BrowTreeListCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, char *argv[]);
/*
 *  Usage : __brow_tree_list ?cur?search?
 *  Returns a formatted string of the form:
 *  {instancename {child1_name child2_name}} for the entire instance
 *  tree rooted from the current or the search instance. This is the
 *  primitive used to build the DAG Browser.
 */

extern int Asc_DagWriteInstDagCmd(ClientData cdata, Tcl_Interp *interp,
                                  int argc, char *argv[]);

extern int Asc_DagWriteModelDagCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, char *argv[]);

extern int Asc_DagCouplingRelnsCmd(ClientData cdata, Tcl_Interp *interp,
                                   int argc, char *argv[]);

/*
 * This writes a file with a model - relation attr pair.
 * The first thing in the file is the n_models and n_relations.
 * It is written to a named file.
 */
extern int Asc_DagModelRelnsCmd(ClientData cdata, Tcl_Interp *interp,
                                int argc, char *argv[]);

extern int Asc_DagPartitionCmd(ClientData cdata, Tcl_Interp *interp,
                               int argc, char *argv[]);

extern int Asc_DagCountRelnsCmd(ClientData cdata, Tcl_Interp *interp,
                                int argc, char *argv[]);

extern int Asc_DagBuildDagCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, char *argv[]);

extern int Asc_DagPrepareCmd(ClientData cdata, Tcl_Interp *interp,
                             int argc, char *argv[]);

extern int Asc_DagShutdownCmd(ClientData cdata, Tcl_Interp *interp,
                              int argc, char *argv[]);



extern int Asc_BrowTreeListCmd2(ClientData cdata, Tcl_Interp *interp,
                                int argc, char *argv[]);
/*
 * FIX THESE COMMENTS		KAA_DEBUG
 */
/*
 *  Usage : __brow_tree_list2 ?cur?search?
 *  Returns a formatted string of the form:
 *  { {parentindex childndx1 childndx2} {parentndx childndx1 childndx2} ... }
 *  for the entire instance tree rooted from the current or the search
 *  instance. This is one of the primitives used to build the DAG Browser.
 *  NOTE: At this time the following is done:
 *  1) Only models or arrays of models are returned.
 *  2) Leaf models are not returned.
 *  3) The instance tree is left in the state that it was found.
 */

#endif  /*  __BrowserDag_module__ */


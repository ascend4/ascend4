/*
 *  ascFreeAllVars.h
 *  by Ben Allan
 *  February 24, 1998
 *  Part of ASCEND
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: ascFreeAllVars.h,v $
 *  Date last modified: $Date: 1998/06/16 16:42:10 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

/** @file
 *  Variable clearing routines.
 *  <pre>
 *  When #including ascFreeAllVars.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler/instance_enum.h"
 *         #include "compiler/compiler.h"
 *         #include "general/list.h"
 *         #include "compiler/extfunc.h"
 *  </pre>
 */

#ifndef ASC_ASCFREEALLVARS_H
#define ASC_ASCFREEALLVARS_H

extern int DLEXPORT Asc_ClearVarsInTree(struct Instance *i);
/**< Asc_ClearVarsInTree(i).
 * A service routine which assumes a solver_var modeling world
 * and clears (set var.fixed := FALSE) all var and refinements
 * of var in the DAG rooted at i.
 */

extern int DLEXPORT Asc_FreeAllVars( struct Instance *rootinstance,
                                    struct gl_list_t *arglist);
                                   
/**<
 *  err = Asc_FreeAllVars(NULL,rootinstance,arglist,0);
 *  All arguments except rootinstance are ignored.
 *  rootinstance is used as the argument to a call to Asc_ClearVarsInTree.
 *  This wrapper exists only for old EXTERNAL a la abbott compatibility
 *  and should be trashed ASAP.
 */

#endif /* ASC_ASCFREEALLVARS_H */


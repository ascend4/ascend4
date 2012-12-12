/*
 *  Tcl Printf Substitutes
 *  by Mark Thomas
 *  Created: 27.May.1997
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascPrint.h,v $
 *  Date last modified: $Date: 1997/10/29 13:08:50 $
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Tcl Printf Substitutes
 *  <pre>
 *  When including this file, you must include:
 *      #include <stdarg.h>
 *      #include "utilities/ascConfig.h"
 *      #include "compiler/compiler.h"
 *      #include "utilities/ascPrint.h"
 *  </pre>
 */

#ifndef _ASCPRINTTCL_H
#define _ASCPRINTTCL_H

extern int Asc_PrintInit_TclVtable(void);
/**<
 * Set up the vtable on the print stack.
 * This can be done before Asc_PrintInit_Tcl(), and
 * the result will be stderr/stdout output.
 * Always returns 0.
 */

extern int Asc_PrintInit_Tcl(void);
/**<
 *  Initialize tcltk interface pointers to the Tcl channels used for output.
 *
 *  This function should be called after the Tcl channels have been
 *  initialized.  Any calls to Asc_Printf() made prior to calling
 *  Asc_PrintInit() will use ordinary printf().  Returns 0 if the
 *  initialization is successful, non-zero if any of the Tcl
 *  channels could not be initialized.
 */

extern void Asc_PrintFinalize_Tcl(void);
/**<  Shut down the print channels of Tcl. */

#endif  /* _ASCPRINTTCL_H */


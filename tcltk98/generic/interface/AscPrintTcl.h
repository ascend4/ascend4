/**< 
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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

#ifndef _ASCPRINTTCL_H
#define _ASCPRINTTCL_H

/**< 
 *  When including this file, you must include:
 *      #include <stdarg.h>
 *      #include "compiler/compiler.h"
 *      #include "utilities/ascprint.h"
 */

extern int Asc_PrintInit_TclVtable(void);
/**< 
 * set up the vtable on the print stack.
 * This can be done before Asc_PrintInit_Tcl, and
 * the result will be stderr/stdout output.
 */

extern int Asc_PrintInit_Tcl(void);
/**< 
 *  int Asc_PrintInit()
 *
 *  Initialize tcltk98 interface pointers to the Tcl channels used for output.
 *  
 *  This function should be called after the Tcl channels have been
 *  initialized.  Any calls to Asc_Printf() made prior to calling
 *  Asc_PrintInit() will use ordinary printf().
 */

extern void Asc_PrintFinalize_Tcl(void);
/**< 
 * Shut down the print channels of Tcl.
 */

#endif /**< _ASCPRINTTCL_H */

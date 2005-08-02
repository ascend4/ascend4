/**<
 *  ASCEND Printf stdout/stderr Substitutes Dispatcher data type.
 *  by Benjamin Allan
 *  Created: 4.March.2005
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: ascPrint.h,v $
 *  Date last modified: $Date: 1997/10/29 13:08:50 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND utilities.
 *
 *  Copyright 2005, Benjamin Andrew Allan
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

/** @file
 *  ASCEND Printf stdout/stderr Substitutes Dispatcher data type.
 *  <pre>
 *  When including this file, you must include:
 *      #include <stdarg.h>
 *      #include "utilities/ascConfig.h"
 *      #include "compiler/compiler.h"
 *      #include "utilities/ascPrint.h"
 *  </pre>
 */

#ifndef _ASCPRINTTYPE_H
#define _ASCPRINTTYPE_H

/**
 * Output functions interceptor vtable. This should be constructed
 * and the functions fully operational before it is
 * pushed on the stack of output tables.
 */
struct Asc_PrintVTable {
	CONST char *name;              /**< name */
	int (*print)(FILE *fp, CONST char *format, va_list args);  /**< print function */
	int (*fflush)(FILE *);         /**< flush function */
	struct Asc_PrintVTable *next;  /**< next vtable in linked list */
};

#endif /**< _ASCPRINTTYPE_H */


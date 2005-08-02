/*
 *  File redirection for ascend
 *  by Ben Allan
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: redirectFile.h,v $
 *  Date last modified: $Date: 2000/01/25 02:26:22 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1999 Benjamin A Allan
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
 *  This module defines the fundamental constants used by the rest of
 *  Ascend and pulls in system headers.
 *  There is not corresponding compiler.c. The variables
 *  declared in this header are defined in ascParse.y.
 *
 *  This header and tcl/tk headers are known to conflict. This header
 *  should be included AFTER tcl.h or tk.h, not before.
 */

/** @file
 *  File redirection for ascend.
 *  <pre>
 *  When #including redirectFile.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef __redirectFile_h_seen_
#define __redirectFile_h_seen_

extern FILE *g_ascend_errors;       /**< File for error messages.  Default is stderr. */
extern FILE *g_ascend_warnings;     /**< File for warning messages.  Default is stderr. */
extern FILE *g_ascend_information;  /**< File for informational messages.  Default is stderr. */

extern void Asc_RedirectCompilerDefault(void);
/**< Set the default files/streams to receive ASCEND messages. */

#endif /* __redirectFile_h_seen_ */


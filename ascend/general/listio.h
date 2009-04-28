/*
 *  List I/O Routines
 *  by Ben Allan
 *  Created: 12/97
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: listio.h,v $
 *  Date last modified: $Date: 1998/06/16 15:47:42 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND Language Interpreter
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The ASCEND Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The ASCEND Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  List I/O Routines.
 *  <pre>
 *  Requires:
 *        #include "utilities/ascConfig.h"
 *        #include "general/list.h"
 *  </pre>
 */

#ifndef __listio_h_seen__
#define __listio_h_seen__

/**
 *  Write the data in a list (as ints/pointers) to a file stream.
 *  If fp is NULL, the the listing is to stderr.  The list pointer
 *  may not be NULL (checked by assertion).
 *
 *  @param fp File stream to receive listing (stderr if fp == NULL).
 *  @param l  The gl_list_t to write to fp (non-NULL).
 */
ASC_DLLSPEC void gl_write_list(FILE *fp, struct gl_list_t *l);

#endif  /* __listio_h_seen__ */


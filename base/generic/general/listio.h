/** 
 *  listio.h
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

#ifndef __LISTIO_H_SEEN__
#define __LISTIO_H_SEEN__

/** gl_write_list(fp,l);
 * write a list l (as ints/pointers) to file fp, or if fp NULL, to stderr.
 */
extern void gl_write_list(FILE *,struct gl_list_t *);

#endif

/**< 
 *  Basic Initializations for Ascend
 *  by Ben Allan
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: ascCompiler.h,v $
 *  Date last modified: $Date: 1997/07/18 12:27:56 $
 *  Last modified by: $Author: mthomas $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Benjamin Andrew Allan
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
 *  This module initializes the fundamental data structures used by the rest of
 *  Ascend and pulls in system headers. Largely this means memory management.
 */
#ifndef __ASCCOMPILER_H_SEEN__
#define __ASCCOMPILER_H_SEEN__

extern int Asc_CompilerInit(int);
/**< err = Asc_CompilerInit(simplify_relations);
 *
 * If this function returns nonzero, ascend cannot run and a ton
 * of memory might be leaked.
 *
 * The value of simplify_relations sets the initial value of a flag
 * which tells the compiler to simplify compiled equations or not.
 * It has no effect on the success or failure of the call.
 *
 * Bugs: at present it needs to more aggressively check the return
 * codes from the functions this calls. currently returns 0 regardless.
 */

extern void Asc_CompilerDestroy(void);
/**< This function should not be called while there are any clients
 * with pointers to any compiler structures, including gl_lists.
 */

#endif /**< __ASCCOMPILER_H_SEEN__*/

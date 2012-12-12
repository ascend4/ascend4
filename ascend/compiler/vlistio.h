/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2010 Carnegie Mellon University
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *//** @file
 *  Temporary variable list output routines.
 *//*
 *  Temporary variable list output routines
 *  by Tom Epperly. Last in CVS $Date: 1997/07/18 12:36:41 $ $Author: mthomas $
*/

#ifndef ASC_VLISTIO_H
#define ASC_VLISTIO_H

#include <stdio.h>
#include <ascend/general/platform.h>
#include <ascend/compiler/vlist.h>

/**	@addtogroup compiler_type Compiler Type Description
	@{
*/

extern void WriteVariableList(FILE *f, CONST struct VariableList *n);
/**<
 *  No leading or trailing white space is added
 */

extern void WriteVariableListNode(FILE *f, CONST struct VariableList *n);
/**<
 *  Write just this one variable list node, and not any of the ones
 *  following it.
 */

/* @} */

#endif /* ASC_VLISTIO_H */


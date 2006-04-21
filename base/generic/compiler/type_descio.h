/*
 *  Type Description Output
 *  by Tom Epperly
 *  Created: 1/15/89
 *  Version: $Revision: 1.9 $
 *  Version control file: $RCSfile: type_descio.h,v $
 *  Date last modified: $Date: 1998/03/26 20:40:28 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
 *  You should have received a copy of the GNU General Public License along
 *  with the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 */

/** @file
 *  Type Description Output.
 *  <pre>
 *  When #including type_descio.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "child.h"
 *         #include "type_desc.h"
 *  </pre>
 */

#ifndef ASC_TYPE_DESCIO_H
#define ASC_TYPE_DESCIO_H

ASC_DLLSPEC(void ) WriteDefinition(FILE *f, struct TypeDescription *desc);
/**<
 *  Write the type description structure to the given file in text.
 *  May include compiler derived information in comments.
 */

extern char *WriteDefinitionStringList(struct TypeDescription *d);
/**< 
 * Returns a string containing in braced list format (compatible
 * with tcl) the contents of a type description.
 * name is an identifier_t, kind is a string indicating base class
 * such as real, integer, symbol, etc., stuff in CAPS is literal,
 * or empty implies the field may be {} if no appropriate statement
 * exists, and value is as yet ill defined. statementlist will be
 * written as WriteStatementListString defines.
 * <pre>
 * family/format:
 * CONSTANTS/
 *    {kind} {UNIVERSAL or empty} {CONSTANT} {name2} {REFINES name1 or empty}
 *    {dims or empty} {value or empty}
 * ATOMS/
 *    {kind} {UNIVERSAL or empty} {ATOM} {name2} {REFINES name1 or empty}
 *    {dims or empty} {value or empty} {statementlist}
 * MODELS/
 *    {MODEL} {UNIVERSAL or empty} {MODEL} {name2} {REFINES name1 or empty}
 *    {parameter statements or empty} {where statements or empty}
 *    {refinement assignments or empty} {ancestor body statementlist or empty}
 *    {body statementlist or empty} {method names and type defined in or empty}
 * </pre>
 * @bug WriteDefinitionStringList() not implemented.
 * @bug WriteDefinitionStringList() does not handle 'DEFINITIONs' for relation
 *      types. Any user who want's to mess with system.a4l can do it by hand. 
 *      No GUI idiots need apply.
 */

ASC_DLLSPEC(void ) WriteDiffDefinition(FILE *f, struct TypeDescription *desc);
/**< 
 *  Write the type description structure to the given file in text but
 *  only those statements that are in the declarative section which are
 *  different from the refinement ancestor of the type. The procedures
 *  are not dealt with as that is messy. If no ancestor, defaults to
 *  writing all declarative statements.<br><br>
 *
 *  Note that the parameters, wheres, reductions, and absorbed
 *  statements of desc are NOT written.
 */

extern symchar *GetBaseTypeName(enum type_kind t);
/**< 
 * Returns the symbol for the kind of type given.
 * InitBaseTypeNames must have been called first.
 * ascCompiler takes care of that.
 */

extern void InitBaseTypeNames(void);
/**< Set up the basetypes symbol table. */

#endif /* ASC_TYPE_DESCIO_H */

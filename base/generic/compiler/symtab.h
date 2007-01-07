/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Symbol Table Management.

	Provide symbol table management facilities for Ascend compiler.  For
	now all the symbol table has to do is make sure that only one copy of each
	string is stored(to save space).  In the future it could have more
	information store in it.

	Requires:
	#include "utilities/ascConfig.h"
	#include "compiler.h"
*//*
	by Tom Epperly
	7/24/89
	Version: $Revision: 1.7 $
	Version control file: $RCSfile: symtab.h,v $
	Date last modified: $Date: 1998/02/05 16:38:07 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_SYMTAB_H
#define ASC_SYMTAB_H

/**	addtogroup compiler Compiler
	@{
*/

#include <utilities/ascConfig.h>

#define MAXIMUM_STRING_LENGTH (2*MAXTOKENLENGTH)

extern void DestroyStringSpace(void);
/**<
 *  This deallocates all the memory associated with the string space.
 */

ASC_DLLSPEC void InitSymbolTable(void);
/**<
 *  This procedure performs all the necessary initialization for the symbol
 *  table manager.  It should be called once and only once, and it must
 *  be called before any of the other symbol table routines have been
 *  called.
 *  Assumptions:
 *  Pre: none
 *  Post: the symbol table is initialized
 */

ASC_DLLSPEC symchar*AddSymbol(CONST char *c);
/**<
 *  This function adds the string c to the symbol table if it is not already
 *  defined.  This uses a copy of c rather than c itself.  c must be a
 *  NULL terminated string. allocation/deallocation of c is the users
 *  responsibility. The symchar pointer returned should never be freed
 *  by the user.<br><br>
 *
 *  Assumptions that must be satisfied before calling:
 *  Symbol table initialized and c is a null terminated string.
 *
 *  Assumptions valid after calling:
 *  The value of c has been added to the symbol table and the symbol table is
 *  not changed in any other way. The symchar * returned will be the same
 *  for the life of the ASCEND process.<br><br>
 *
 *  Any size string can be stored in this table, though it's a bad
 *  idea to store very large ones. Strings below MAXIMUM_STRING_LENGTH
 *  we store efficiently.
 *
 *  @example symchar *permanentstring = AddSymbol(c);
 */

ASC_DLLSPEC symchar *AddSymbolL(CONST char *c, int len);
/**<
 *  This function does exactly what add symbol does except the length of the
 *  string is passed as a parameter.  The length is the number of characters
 *  before the terminating '\0'.
 *  This function is mildly faster than AddSymbol if you already know clen
 *  for some other reason.<br><br>
 *
 *  Assumptions to be satisfied before calling:
 *  Symbol table initialized and c is a null terminated string.<br><br>
 *
 *  Assumptions valid after calling:
 *  The value of c has been added to the symbol table and the symbol table is
 *  not changed in any other way. The symchar * returned will be the same
 *  for the life of the ASCEND process.
 */

ASC_DLLSPEC symchar*AscFindSymbol(symchar *s);
/**<
 * Returns NULL if the pointer s given is not from the table.
 * Otherwise returns the pointer given.
 * This function does not check whether a string with the value
 * that s points to is in the table; this function checks that _s_
 * is in the table.
 */

extern void PrintTab(int noisy);
/**<
 *  Print a report on stdout about the string hash table.
 *  if (noisy) prints lots of goop.
 */

ASC_DLLSPEC void DestroySymbolTable(void);
/**<
 *  This function will deallocate all the memory associated with the
 *  symbol table and the symbols it contains.
 */

/* @} */

#endif  /* ASC_SYMTAB_H */


/**< 
 *  Basic Definitions for Ascend
 *  by Tom Epperly
 *  Version: $Revision: 1.26 $
 *  Version control file: $RCSfile: compiler.h,v $
 *  Date last modified: $Date: 2000/01/25 02:26:02 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
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
 *  This header and Tcl/Tk headers are known to conflict. This header
 *  should be included AFTER tcl.h or tk.h, not before.
 */

#ifndef __COMPILER_H_SEEN__
#define __COMPILER_H_SEEN__

/**< some influential defines and where to set them:
	ATDEBUG anontype.c -- controls anon type/relation sharing spew
	ASC_NO_POOL -- compiler -D flag. Turn off many memory recycles of small objects.
	ASC_NO_TRAPS -- compiler -D flag. Turn off traps/setjmp/longjump.
*/

#define TIMECOMPILER 1
/**< 
 * set to 1 for timing spew or 0 for not.
 */

/**< 
 * define to check internal and client compliance with symbol
 * table requirements. 0 for production code. 
 */
#ifdef NDEBUG
#define CHECK_SYMBOL_USE 0
#else
#define CHECK_SYMBOL_USE 1
#endif

#if CHECK_SYMBOL_USE
/**< This non-integral typedef
 * forces all the whines on virtually all compilers.
 * The only way to shut it up is to use AddSymbol,symchars,
 * and SCP() correctly. The visible typedef does not change
 * the actual symchar semantics, which is just a string.
 * Dereferencing a symchar when typedef'd as double will
 * usually cause a bus error unless the string happens to
 * occur at an 8byte round address. It's very easy to spot
 * code which abuses our symbol table conventions now.
 */
typedef CONST double symchar; 
#ifndef __GNUC__
/**< strcmp() is dumb in string.h. we want it to whine a lot. */
extern int strcmp(CONST char *, CONST char *);
#endif
#else
/**< This is the real definition for production builds. */
typedef CONST char symchar;
#endif
/**< 
 * Symchar exists so we can be very clear about strings coming from
 * symbol tables. If it's a pair of symchar *, it is a sufficient
 * comparison to compare the pointers if only equality is sought.
 * Sorting functions should use CmpSymchar.
 *
 * WARNING: this typedef may change in the future when we get a
 * real symbol table. Avoid treating symchar directly as char *,
 * or your code will most likely not compile and not run.
 */
#define SCP(s) ((CONST char *)(s))
/**< 
 * SCP(s) returns the string ptr from a symchar ptr, whatever a symchar is.
 * If you need a just plain (char *) for I/O (tcl perhaps) just write
 * (char *)SCP(foo)
 */

#define SCLEN(s) (*(int *)(((char *)s)-sizeof(int)))
/**< 
 * returns, at considerably less expense, strlen(SCP(s))
 * by looking up the length of the string in an int stored
 * just before the character string itself.
 */

/**< bracedtext atomic type. see braced.h */
struct bracechar;

/**< globals from ascParse.y that yacc won't put in ascParse.h generated. */

extern int g_compiler_warnings;
/**< 
 *  Flag to turn on ASCEND instantiation whinings in various ways.
 *  higher values mean more spew. 0 = no warnings.
 *  Variable is declared in ascParse.y.
 */

extern int g_parser_warnings;
/**< 
 *  Flag to turn on lintlike ASCEND whinings in various ways.
 *  higher values mean less spew. 0 = no warnings.
 *  Variable is declared in typelint.c.
 */

extern int g_parse_relns;
/**< abandon relation productions - very bad idea but useful for
 * benchmarking sometimes.
 */

extern int g_simplify_relations;
/**< 
 * turn on or off relation simplification as noted in
 * relation.h. This variable is defined in relation.c and
 * headered here for UI exports.
 */

extern int g_use_copyanon;
/**< 
 * turn on/off relation sharing. see instantiate.h.
 */


#define BASE_REAL_NAME          "real"
#define BASE_INTEGER_NAME       "integer"
#define BASE_SYMBOL_NAME        "symbol"
#define BASE_BOOLEAN_NAME       "boolean"
#define BASE_SET_NAME           "set"
/*** Above are the simple types eligible to be ATOM children. ***/
/*** Below are the simple types NOT eligible to be ATOM children. ***/
#define BASE_CON_REAL_NAME      "real_constant"
#define BASE_CON_INTEGER_NAME   "integer_constant"
#define BASE_CON_BOOLEAN_NAME   "boolean_constant"
#define BASE_CON_SYMBOL_NAME    "symbol_constant"
/**< relation, etc names */
#define BASE_REL_NAME "relation"
#define BASE_LOGREL_NAME "logic_relation"
#define BASE_WHEN_NAME "when"
#define BASE_EXT_NAME "EXTERNAL_MODEL"
#define BASE_UNSELECTED "unSELECTed_part"
/**< 
 *  Don't randomly change these, as ASCEND MODEL code assumes they
 *  are what they are. Changing these constitutes requiring a global
 *  revision of ASCEND models.
 *
 * Don't strcmp with these to a SCP(symchar) string: use
 * CmpSymchar(GetBaseTypeName(enum type_kind),symchar) instead from 
 * type_descio.h instead.
 */

#endif /**< __COMPILER_H_SEEN__ */

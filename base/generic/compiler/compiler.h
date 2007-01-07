/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly

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
*//*
	by Tom Epperly
	Last in CVS: $Revision: 1.26 $ $Date: 2000/01/25 02:26:02 $ $Author: ballan $
*//** @file
	Basic Definitions for Ascend

	@NOTE
	This header and tcl/tk headers are known to conflict. This header
	should be included AFTER tcl.h or tk.h, not before.

	This module defines the fundamental constants used by the rest of
	Ascend and pulls in system headers. There is not a corresponding
	compiler.c. The variables declared in this header are defined
	in ascParse.y.
*/

#ifndef ASC_COMPILER_H
#define ASC_COMPILER_H

/**	@addtogroup compiler Compiler
	@{
*/

#include <utilities/ascConfig.h>

/* some influential defines and where to set them:
	ATDEBUG anontype.c -- controls anon type/relation sharing spew
	ASC_NO_POOL -- compiler -D flag. Turn off many memory recycles of small objects.
	ASC_NO_TRAPS -- compiler -D flag. Turn off traps/setjmp/longjump.
*/

#ifndef TIMECOMPILER
#define TIMECOMPILER 1
/**<  Set to 1 for timing spew or 0 for not. */
#endif

#ifndef CHECK_SYMBOL_USE
#ifdef NDEBUG
#define CHECK_SYMBOL_USE 0
#else
#define CHECK_SYMBOL_USE 1
#endif
#endif
/**<
 * Define to check internal and client compliance with symbol
 * table requirements. 0 for production code.
 */

#if CHECK_SYMBOL_USE

#ifndef __GNUC__
/** strcmp() is dumb in string.h. We want it to whine a lot. */
extern int strcmp(CONST char *s1, CONST char *s2);
#endif  /* __GNU__ */
typedef CONST double symchar;
#else   /* !CHECK_SYMBOL_USE */
/* This is the real definition for production builds. */
typedef CONST char symchar;
#endif  /* CHECK_SYMBOL_USE */
/**<
 * Symchar exists so we can be very clear about strings coming from
 * symbol tables. If it's a pair of symchar *, it is a sufficient
 * comparison to compare the pointers if only equality is sought.
 * Sorting functions should use CmpSymchar. <br><br>
 *
 * WARNING: this typedef may change in the future when we get a
 * real symbol table. Avoid treating symchar directly as char *,
 * or your code will most likely not compile and not run.<br><br>
 *
 * Compile with CHECK_SYMBOL_USE defined to check for proper symchar
 * usage.  This non-integral typedef forces all the whines on
 * virtually all compilers.  The only way to shut it up is to use
 * AddSymbol() , symchars, and SCP() correctly. The visible typedef
 * does not change the actual symchar semantics, which is just a
 * string.  Dereferencing a symchar when typedef'd as double will
 * usually cause a bus error unless the string happens to
 * occur at an 8-byte round address. It's very easy to spot
 * code which abuses our symbol table conventions now.
 */

#define SCP(s) ((CONST char *)(s))
/**<
 * Returns the string ptr from a symchar ptr, whatever a symchar is.
 * If you need a just plain (char *) for I/O (tcl perhaps) just write
 * (char *)SCP(foo)
 */

#define SCLEN(s) (*(int *)(((char *)s)-sizeof(int)))
/**<
 * Returns, at considerably less expense, strlen(SCP(s)).
 * This macro looks up the length of the string in an int stored
 * just before the character string itself.
 */

/** bracedtext atomic type. see braced.h */
struct bracechar;

/* globals from ascParse.y that yacc won't put in ascParse.h generated. */

ASC_DLLSPEC int g_compiler_warnings;
/**<
 *  Flag to turn on ASCEND instantiation whinings in various ways.
 *  higher values mean more spew. 0 = no warnings.
 *  Variable is declared in ascParse.y.
 */

ASC_DLLSPEC int g_parser_warnings;
/**<
 *  Flag to turn on lint-like ASCEND whinings in various ways.
 *  higher values mean less spew. 0 = no warnings.
 *  Variable is declared in typelint.c.
 */

extern int g_parse_relns;
/**<
 * Flag to abandon relation productions.
 * A very bad idea, but useful for benchmarking sometimes.
 */

ASC_DLLSPEC int g_simplify_relations;
/**<
 * Turn on or off relation simplification as noted in
 * relation.h. This variable is defined in relation.c and
 * headered here for UI exports.
 */

ASC_DLLSPEC int g_use_copyanon;
/**<
 * Turn on/off relation sharing.
 *
 * If TRUE, anonymous type detection is used to enable relation
 * sharing. This variable is defined in instantiate.c and
 * headered for export in compiler.h.
 */

/* Simple types eligible to be ATOM children. */
#define BASE_REAL_NAME          "real"
#define BASE_INTEGER_NAME       "integer"
#define BASE_SYMBOL_NAME        "symbol"
#define BASE_BOOLEAN_NAME       "boolean"
#define BASE_SET_NAME           "set"
/* Simple types NOT eligible to be ATOM children. */
#define BASE_CON_REAL_NAME      "real_constant"
#define BASE_CON_INTEGER_NAME   "integer_constant"
#define BASE_CON_BOOLEAN_NAME   "boolean_constant"
#define BASE_CON_SYMBOL_NAME    "symbol_constant"
/* relation, etc names */
#define BASE_REL_NAME           "relation"
#define BASE_LOGREL_NAME        "logic_relation"
#define BASE_WHEN_NAME          "when"
#define BASE_EXT_NAME           "EXTERNAL_MODEL"
#define BASE_UNSELECTED         "unSELECTed_part"
/*
 *  Don't randomly change these, as ASCEND MODEL code assumes they
 *  are what they are. Changing these constitutes requiring a global
 *  revision of ASCEND models.
 *
 * Don't strcmp with these to a SCP(symchar) string: use
 * CmpSymchar(GetBaseTypeName(enum type_kind),symchar) instead from
 * type_descio.h instead.
 */

/* @} */

#endif  /* ASC_COMPILER_H */


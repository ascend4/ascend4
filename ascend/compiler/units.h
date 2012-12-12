/*	ASCEND modelling environment
	Copyright (C) 2006, 2011 Carnegie Mellon University
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Ascend Units Type definitions.
*//*
	by Tom Epperly 8/18/89
	Last in CVS: $Revision: 1.10 $ $Date: 1998/02/05 16:38:40 $ $Author: ballan $
*/

#ifndef ASC_UNITS_H
#define ASC_UNITS_H

#include <stdio.h>
#include <ascend/general/platform.h>
#include "compiler.h"
#include "fractions.h"
#include "dimen.h"

/**	@addtogroup compiler_units Compiler Units
	@{
*/

#ifdef _HPUX_SOURCE
#define ACAST char *
#define BCAST char *
#else
#define ACAST const char *
#define BCAST char *
#endif
/**< to shut up the hp lexer */

/**
	Value in system units = value in given units * conversion_factor.
 */
struct Units {
  double conversion_factor; /**< to convert from units to system units */
  symchar *description;     /**< description of units */
  CONST dim_type *dim;      /**< dimenions of units */
  struct Units *next;       /**< not for human consumption */
};

/**
	Temporary structure for parsing unit definitions in ascParse.y.
 */
struct UnitDefinition {
  symchar *new_name;
  CONST char *unitsexpr;
  CONST char *filename;
  int linenum;
};

#define UNITS_HASH_SIZE (1023)
/**<
	size of the hash table for unit structs
 */

/*
 * Name of the basic SI units for the 10 dimensions.
 * If you have better names for them, fix that here.
 */
#define UNIT_BASE_MASS               "kg"
/**< SI base unit symbol for mass. */
#define UNIT_BASE_QUANTITY           "mol"
/**< SI base unit symbol for quantity. */
#define UNIT_BASE_LENGTH             "m"
/**< SI base unit symbol for length. */
#define UNIT_BASE_TIME               "s"
/**< SI base unit symbol for time. */
#define UNIT_BASE_TEMPERATURE        "K"
/**< SI base unit symbol for temperature. */
#define UNIT_BASE_CURRENCY           "USD"
/**< Base unit (non SI) for currency (US dollar) */
#define UNIT_BASE_ELECTRIC_CURRENT   "A"
/**< SI base unit symbol for current. */
#define UNIT_BASE_LUMINOUS_INTENSITY "cd"
/**< SI base unit symbol for luminosity. */
#define UNIT_BASE_PLANE_ANGLE        "rad"
/**< Base unit (non SI) for plane angle (radian). */
#define UNIT_BASE_SOLID_ANGLE        "sr"
/**< Base unit (non SI) for solid angle (steradian). */

ASC_DLLSPEC struct Units *g_units_hash_table[];
/**<
 *  The hash table for unit structs.
 */

extern void InitUnitsTable(void);
/**<
	This routine initializes some internal variables, so that all the
	other units functions may be called.  It must be called once and
	only once when the program is starting up.
	Must be called after dimensions table initiatialization.
*/

extern void DestroyUnitsTable(void);
/**<
	This routine can be called to deallocate all of the units in the table.
*/

extern struct UnitDefinition *CreateUnitDef(symchar *new_name,
                                            CONST char *unitsexpr,
                                            CONST char *filename,
                                            int linenum);
/**<
 *  Create a new unit definition.
 *  @param new_name  Should be from the symbol table.
 *  @param unitsexpr A string that we will copy, so it does not
 *                   need to be persistent.
 *  @param filename  Should be persistent.
 *  @param linenum   Line number.
 */

extern void DestroyUnitDef(struct UnitDefinition *udp);
/**<
 * Destroys udp and its unitsexpr.
 */

extern void ProcessUnitDef(struct UnitDefinition *udp);
/**<
 * Attempts to add the info in udp to the units table.
 * messages to ascerr if not possible.
 */

ASC_DLLSPEC CONST struct Units*LookupUnits(CONST char *c);
/**<
 *  Check the units library for units with a description string which
 *  matches c.  If it is found, this function will return a non-NULL pointer;
 *  otherwise, it returns NULL to indicate that units c are undefined.
 *  c should not contain any blanks.
 */

extern CONST struct Units *DefineUnits(symchar *c, double conv, CONST dim_type *dim);
/**<
 *  Define the units c with conversion factor conv and dimensions *dim.
 *  This assumes that *dim was the value returned by FindOrAddDim.  This
 *  will check to prevent duplicate entries.  The resulting unit structure is
 *  returned.  If you enter a duplicate entry and the dimensions or conversion
 *  factor don't match, this function will return NULL.  In addition to
 *  the user defined units there is a wild units type which conversion equal
 *  to one and wild dimensions.  It is given the name "?".<br><br>
 *
 *  c should not contain any spaces!  It may add c to the symbol table if
 *  it is not already stored there.
 *
 *  @bug Memory leak if hitting an empty hash bucket. buckets never deallocated
 *       except at shutdown anyway, so not a big deal. 7bytes/hit. Don't off hand
 *       know where to fix it. BAA 6-94
 */

ASC_DLLSPEC CONST struct Units*FindOrDefineUnits(CONST char *c,
                                             unsigned long * CONST pos,
                                             int * CONST error_code);
/**<
 *  This function will attempt to parse the string c into a units
 *  description.  If the unit type has been defined before, the corresponding
 *  units pointer will be returned.  If this type hasn't been defined before,
 *  it will be defined and that pointer will be returned.  If it is
 *  unable to parse this string it will return NULL.  CheckUnitStr, below,
 *  can be used to diagnose why the unit didn't parse.
 *  This will not modify c in any way.  It may add c to the symbol table.
 *  <pre>
 *  RETURN VALUE OF error_code
 *    0   string is okay, value of pos not specified
 *    1   undefined unit used in string, pos indicates the first
 *           letter of the first occurence of the undefined unit
 *    2   unbalanced parenthesis, pos indicates the opening parenthesis
 *           that wasn't closed
 *    3   illegal character, pos indicates the position of the offending
 *           character
 *    4   illegal real value, pos indicates the first character of the
 *         real value
 *    5   oversized identifier or real, pos indicates the start of the
 *           offending token
 *    6   operator left out real followed by identifier or vice
 *           versa, pos indicates where an operator should have been
 *           inserted
 *    7   term missing after *,/, or (.
 *    8   missing term before *,/. pos is left at the operator
 *    9   too many closing parens.  pos is left at the extra paren.
 *   10   bad fraction exponent.  pos is at the left of the fraction field
 *   11   incompatible unit redefinition. (internal use only; not seen).
 *  </pre>
 */

ASC_DLLSPEC char**UnitsExplainError(CONST char *unitsexpr, int code, int pos);
/**<
 *  Returns an array of strings which may be helpful in
 *  explaining the error.
 *  - errv[0] is a message.
 *  - errv[1] is the unitsexpr.
 *  - errv[2] is a pointer to the error -------^ line.
 *            aligned with unitsexpr given.
 *  The user should never change or free errv or its content,
 *  nor should the user keep the pointers.
 */

#define UnitsDescription(u) ((u)->description)
/**<
 *  Returns the string description attribute of a units structure.
 */

#define UnitsConvFactor(u) ((u)->conversion_factor)
/**<
 *  Returns the conversion factor for a given units structure.
 */

#define UnitsDimensions(u) ((u)->dim)
/**<
 *  Returns the dimensions of the units structure.
 */

ASC_DLLSPEC char *UnitsStringSI(struct Units *up);
/**<
 *  Returns the SI form of the units for the dimensionality of up.
 *  Wild = *, Dimensionless = "", NULL up --> NULL return.
 *  Caller is responsible for freeing the string returned.
 */

ASC_DLLSPEC void DumpUnits(FILE *f);
/**<  Dump all defined units to f. */

/* @} */

#endif /* ASC_UNITS_H */


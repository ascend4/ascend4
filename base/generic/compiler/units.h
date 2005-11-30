/*
 *  Ascend Units Type definitions
 *  by Tom Epperly
 *  8/18/89
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: units.h,v $
 *  Date last modified: $Date: 1998/02/05 16:38:40 $
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

#ifndef ASC_UNITS_H
#define ASC_UNITS_H

/** @file
 *  Ascend Units Type definitions.
 *  <pre>
 *  When #including units.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *  </pre>
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
 *  Value in system units = value in given units * conversion_factor.
 */
struct Units {
  double conversion_factor; /**< to convert from units to system units */
  symchar *description;     /**< description of units */
  CONST dim_type *dim;      /**< dimenions of units */
  struct Units *next;       /**< not for human consumption */
};

/**
 * Temporary structure for parsing unit definitions in ascParse.y.
 */
struct UnitDefinition {
  symchar *new_name;
  CONST char *unitsexpr;
  CONST char *filename;
  int linenum;
};

#define UNITS_HASH_SIZE (1023)
/**<
 *  size of the hash table for unit structs
 */

/*
 * Name of the basic SI units for the 10 dimensions.
 * If you have better names for them, fix that here.
 */
#define UNIT_BASE_MASS               "kilogram"
/**< Name of basic SI unit for mass. */
#define UNIT_BASE_QUANTITY           "mole"
/**< Name of basic SI unit for quantity. */
#define UNIT_BASE_LENGTH             "meter"
/**< Name of basic SI unit for length. */
#define UNIT_BASE_TIME               "second"
/**< Name of basic SI unit for time. */
#define UNIT_BASE_TEMPERATURE        "Kelvin"
/**< Name of basic SI unit for temperature. */
#define UNIT_BASE_CURRENCY           "currency"
/**< Name of basic SI unit for currency. */
#define UNIT_BASE_ELECTRIC_CURRENT   "ampere"
/**< Name of basic SI unit for current. */
#define UNIT_BASE_LUMINOUS_INTENSITY "candela"
/**< Name of basic SI unit for luminosity. */
#define UNIT_BASE_PLANE_ANGLE        "radian"
/**< Name of basic SI unit for plane angle. */
#define UNIT_BASE_SOLID_ANGLE        "steradian"
/**< Name of basic SI unit for solid angle. */

extern struct Units *g_units_hash_table[];
/**<
 *  The hash table for unit structs.
 */

extern void InitUnitsTable(void);
/**<
 *  <!--  void InitUnitsTable()                                        -->
 *  This routine initializes some internal variables, so that all the
 *  other units functions may be called.  It must be called once and 
 *  only once when the program is starting up.  
 *  Must be called after dimensions table initiatialization.
 */
                       
extern void DestroyUnitsTable(void);
/**< 
 *  <!--  void DestroyUnitsTable()                                     -->
 *  This routine can be called to deallocate all of the units in the table.
 */

extern struct UnitDefinition *CreateUnitDef(symchar *new_name, 
                                            CONST char *unitsexpr,
                                            CONST char *filename,
                                            int linenum);
/**<
 * <!--  udptr = CreateUnitDef(new_name,unitsexpr,filename,linenum);   -->
 * <!--  new_name should be from the symbol table.                     -->
 * <!--  unitsexpr is a string that we will copy, so it does not       -->
 * <!--  need to be persistent.                                        -->
 * <!--  filename should be persistent.                                -->
 *  Create a new unit definition.
 *  @param new_name  Should be from the symbol table.
 *  @param unitsexpr A string that we will copy, so it does not
 *                   need to be persistent.
 *  @param filename  Should be persistent.
 *  @param linenum   Line number.
 */

extern void DestroyUnitDef(struct UnitDefinition *udp);
/**<
 * <!--  DestroyUnitDef(udp);                                          -->
 * Destroys udp and its unitsexpr.
 */

extern void ProcessUnitDef(struct UnitDefinition *udp);
/**<
 * <!--  ProcessUnitDef(udp);                                          -->
 * Attempts to add the info in udp to the units table.
 * messages to ascerr if not possible.
 */

extern CONST struct Units *LookupUnits(CONST char *c);
/**<
 *  <!--  const struct Units *LookupUnits(c)                           -->
 *  <!--  const char *c;                                               -->
 *  Check the units library for units with a description string which
 *  matches c.  If it is found, this function will return a non-NULL pointer;
 *  otherwise, it returns NULL to indicate that units c are undefined.
 *  c should not contain any blanks.
 */

extern CONST struct Units *DefineUnits(symchar *c, double conv, CONST dim_type *dim);
/**<
 *  <!--  const struct Units *DefineUnits(c,conv,dim)                  -->
 *  <!--  const char *c;                                               -->
 *  <!--  double conv;                                                 -->
 *  <!--  const dim_type *dim;                                         -->
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

extern CONST struct Units *FindOrDefineUnits(CONST char *c,
                                             unsigned long * CONST pos,
                                             int * CONST error_code);
/**<
 *  <!--  struct Units *FindOrDefineUnits(c,pos,error_code)            -->
 *  <!--  CONST char *c;                                               -->
 *  <!--  unsigned long * const pos;                                   -->
 *  <!--  int * const error_code;                                      -->
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

extern char **UnitsExplainError(CONST char *unitsexpr, int code, int pos);
/**< 
 *  <!--  errv = UnitsExplainError(unitsexpr,code,pos);                -->
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
 *  <!--  macro UnitsDescription(u)                                    -->
 *  <!--  struct Units *u;                                             -->
 *  Returns the string description attribute of a units structure.
 */

#define UnitsConvFactor(u) ((u)->conversion_factor)
/**<
 *  <!--  macro UnitsConvFactor(u)                                     -->
 *  <!--  struct Units *u;                                             -->
 *  Returns the conversion factor for a given units structure.
 */

#define UnitsDimensions(u) ((u)->dim)
/**<
 *  <!--  macro UnitsDimensions(u)                                     -->
 *  <!--  struct Units *u;                                             -->
 *  Returns the dimensions of the units structure.
 */

extern char *UnitsStringSI(struct Units *up);
/**<
 *  <!--  UnitsStringSI(up);                                           -->
 *  Returns the SI form of the units for the dimensionality of up.
 *  Wild = *, Dimensionless = "", NULL up --> NULL return.
 *  Caller is responsible for freeing the string returned.
 */

extern void DumpUnits(FILE *f);
/**<  Dump all defined units to f. */

#endif /* ASC_UNITS_H */


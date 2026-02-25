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
#include <ascend/general/list.h>
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
  long ladder_id;           /**< ladder membership id, or -1 if none */
  long ladder_rank;         /**< 0-based rank within ladder, undefined if ladder_id<0 */
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

/**
	Temporary structure for parsing UNITS LADDER definitions in ascParse.y.
	If is_anchor is nonzero, unitsexpr must be NULL and name references an
	existing ladder member used as an insertion anchor.
*/
struct UnitLadderItem {
  symchar *name;
  CONST char *unitsexpr;
  CONST char *filename;
  int linenum;
  int is_anchor;
};

/**
	Opaque holder for user display-units overrides.
 */
struct UnitsOverridesDB;
struct Instance;

/**
	Override key namespace.
 */
enum UnitsOverrideKind{
	UNITS_OVERRIDE_TYPE = 1, /**< key is type name */
	UNITS_OVERRIDE_NAME = 2  /**< key is canonical variable qlfdid */
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

ASC_DLLSPEC void InitUnitsTable(void);
/**<
	This routine initializes some internal variables, so that all the
	other units functions may be called.  It must be called once and
	only once when the program is starting up.
	Must be called after dimensions table initiatialization.
*/

ASC_DLLSPEC void DestroyUnitsTable(void);
/**<
	This routine can be called to deallocate all of the units in the table.
*/

ASC_DLLSPEC struct UnitDefinition *CreateUnitDef(symchar *new_name,
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

ASC_DLLSPEC void DestroyUnitDef(struct UnitDefinition *udp);
/**<
 * Destroys udp and its unitsexpr.
 */

ASC_DLLSPEC void ProcessUnitDef(struct UnitDefinition *udp);
/**<
 * Attempts to add the info in udp to the units table.
 * messages to ascerr if not possible.
 */

ASC_DLLSPEC struct UnitLadderItem *CreateUnitLadderItem(symchar *name,
                                                   CONST char *unitsexpr,
                                                   int is_anchor,
                                                   CONST char *filename,
                                                   int linenum);
/**<
 *  Create a new UNITS LADDER item.
 *  For anchor items, pass is_anchor nonzero and unitsexpr as NULL.
 */

ASC_DLLSPEC void DestroyUnitLadderItem(struct UnitLadderItem *item);
/**<
 *  Destroys one UNITS LADDER item.
 */

ASC_DLLSPEC int ProcessUnitLadder(struct gl_list_t *items);
/**<
 *  Process a UNITS LADDER item list.
 *  Returns number of errors encountered.
 */

ASC_DLLSPEC CONST struct Units *LookupUnits(CONST char *c);
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

ASC_DLLSPEC CONST struct Units *FindOrDefineUnits(CONST char *c,
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

ASC_DLLSPEC char **UnitsExplainError(CONST char *unitsexpr, int code, int pos);
/**<
	Returns an array of strings which may be helpful in
	explaining the error.
		- errv[0] is a message.
		- errv[1] is the unitsexpr.
		- errv[2] is a pointer to the error -------^ line.
			      aligned with unitsexpr given.
	The user should never change or free errv or its content,
	nor should the user keep the pointers.

	UPDATE (JP 2018): you can call UnitsExplainError(NULL,-1,0) to clear
	memory allocated by this function, but you shouldn't free it yourself
	because this function is storing the pointer in a global variable which
	it later tries to access internally.
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

#define UnitsLadderId(u) ((u)->ladder_id)
/**<
 *  Returns ladder id for this units object, or -1 if it is not in a ladder.
 */

#define UnitsLadderRank(u) ((u)->ladder_rank)
/**<
 *  Returns 0-based rank in the ladder (valid only if UnitsLadderId(u) >= 0).
 */

ASC_DLLSPEC CONST struct Units *LookupUnitsByLadder(long ladder_id, long ladder_rank);
/**<
 *  Lookup a units object by ladder id and ladder rank. Returns NULL if absent.
 */

ASC_DLLSPEC struct UnitsOverridesDB *UnitsOverridesCreate(void);
/**<
 *  Create an empty overrides DB.
 */

ASC_DLLSPEC void UnitsOverridesDestroy(struct UnitsOverridesDB *db);
/**<
 *  Destroy db and all contained entries.
 */

ASC_DLLSPEC void UnitsOverridesClear(struct UnitsOverridesDB *db);
/**<
 *  Remove all entries from db.
 */

ASC_DLLSPEC int UnitsOverridesSetSimroot(
	struct UnitsOverridesDB *db,
	CONST char *simroot
);
/**<
 *  Set simulation-root token used for save-time canonicalization of name keys.
 *  If simroot is non-empty, saving name overrides strips an exact "<simroot>."
 *  prefix from each name key before writing.
 *  Pass NULL or "" to clear.
 */

ASC_DLLSPEC int UnitsOverridesSet(struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name,
	CONST char *units
);
/**<
 *  Set one override entry.
 *  scope = "" means global (allowed for type overrides only).
 *  Returns 0 on success, nonzero on invalid input or units parse failure.
 */

ASC_DLLSPEC int UnitsOverridesUnset(struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name
);
/**<
 *  Remove one override entry; returns 0 if removed, nonzero if absent/invalid.
 */

ASC_DLLSPEC CONST struct Units *UnitsOverridesLookup(
	struct UnitsOverridesDB *db,
	enum UnitsOverrideKind kind,
	CONST char *scope,
	CONST char *name
);
/**<
 *  Lookup one override exactly by kind/scope/name.
 */

ASC_DLLSPEC CONST struct Units *UnitsOverridesResolve(
	struct UnitsOverridesDB *db,
	CONST char *scope,
	CONST char *type_name,
	CONST char *qlfdid,
	CONST dim_type *dim
);
/**<
 *  Resolve override with precedence:
 *    name(scope) -> type(scope) -> type(global)
 *  Name lookup tries qlfdid exactly, then qlfdid with a leading
 *  "<simroot>." stripped.
 *  Invalid dimensional overrides are reported (ASC_USER_ERROR) and dropped.
 *  Returns NULL if no applicable override exists.
 */

ASC_DLLSPEC CONST struct Units *UnitsResolveDisplayForInstance(
	struct UnitsOverridesDB *db,
	CONST struct Instance *inst,
	int autoscale,
	double lower,
	double upper
);
/**<
 *  Resolve display units for one real-valued instance.
 *  Precedence:
 *    name(scope) -> type(scope) -> type(global) -> declared units -> SI default.
 *  Here scope is model-scoped: "<owner-model-module-file>::<owner-model-type>".
 *  Name overrides are resolved using the variable path relative to the owning
 *  model instance.
 *  If autoscale is nonzero and the chosen units are in a ladder, selects the
 *  best ladder member based on the current SI value using [lower,upper) target
 *  range (typically 0.1..1000). Autoscaling is skipped for undefined, zero, or
 *  non-finite values.
 *  Returns NULL for invalid input or non-real instances.
 */

ASC_DLLSPEC int UnitsOverridesSetForInstance(
	struct UnitsOverridesDB *db,
	CONST struct Instance *inst,
	enum UnitsOverrideKind kind,
	int model_scope,
	CONST char *units
);
/**<
 *  Set one override entry using keys derived from a specific instance.
 *  For type overrides:
 *    model_scope != 0 -> scope is owner-model scope key
 *    model_scope == 0 -> global scope
 *  For name overrides:
 *    scope is always owner-model scope key (global name scope is invalid).
 *  Returns 0 on success, nonzero on invalid input or parse failures.
 */

ASC_DLLSPEC int UnitsOverridesUnsetForInstance(
	struct UnitsOverridesDB *db,
	CONST struct Instance *inst,
	enum UnitsOverrideKind kind,
	int model_scope
);
/**<
 *  Remove one override entry using keys derived from a specific instance.
 *  Scope semantics match UnitsOverridesSetForInstance.
 *  Returns 0 if removed, nonzero if absent/invalid.
 */

ASC_DLLSPEC int UnitsOverridesLoad(
	struct UnitsOverridesDB *db,
	CONST char *filename,
	unsigned *loaded,
	unsigned *errors
);
/**<
 *  Load overrides from INI-like file.
 *  Sections: [global], [<model-scope>]
 *  where <model-scope> is typically "<module-file>::<model-type>".
 *  Keys:
 *    type.<type_name> = <units>
 *    name.<qlfdid_without_simroot> = <units>   (scoped sections only)
 *  Load is additive; call UnitsOverridesClear(db) before load if desired.
 *  Returns 0 on success (including file not found), nonzero otherwise.
 */

ASC_DLLSPEC int UnitsOverridesSave(
	struct UnitsOverridesDB *db,
	CONST char *filename
);
/**<
 *  Save overrides in INI-like format. Returns 0 on success.
 */

ASC_DLLSPEC char *UnitsOverridesDefaultPath(void);
/**<
 *  Build default path for units-overrides file.
 *  Environment precedence:
 *    ASCEND_UNITS_OVERRIDES_PATH (full filename)
 *    XDG_CONFIG_HOME/ascend/units-overrides.ini
 *    HOME/.config/ascend/units-overrides.ini
 *    APPDATA/ascend/units-overrides.ini
 *  Caller owns returned memory and must free with ASC_FREE.
 */

ASC_DLLSPEC char *UnitsStringSI(CONST struct Units *up);
/**<
 *  Returns the SI form of the units for the dimensionality of up.
 *  Wild = *, Dimensionless = "", NULL up --> NULL return.
 *  Caller is responsible for freeing the string returned.
 */

ASC_DLLSPEC void DumpUnits(FILE *f);
/**<  Dump all defined units to f. */

/* @} */

#endif /* ASC_UNITS_H */

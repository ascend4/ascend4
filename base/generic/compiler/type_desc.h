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
*//**
	@file
	Atom description structure data type.

	Requires:
	#include <stdio.h>
	#include "utilities/ascConfig.h"
	#include "compiler/fractions.h"
	#include "compiler/compiler.h"
	#include "compiler/dimen.h"
	#include "compiler/child.h"
	#include "compiler/list.h"
	#include "compiler/module.h"
	#include "compiler/childinfo.h"
	#include "compiler/slist.h"
*//*
	by Tom Epperly
	9/3/89
	Version: $Revision: 1.37 $
	Version control file: $RCSfile: type_desc.h,v $
	Date last modified: $Date: 1998/05/18 16:36:52 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_TYPE_DESC_H
#define ASC_TYPE_DESC_H

#include <utilities/ascConfig.h>

/*------------------------------------------------------------------------------
	forward declarations and typedefs
*/

/**
 *  Bad pointer for a patently bogus type definition when needed.
 *  This token is an illegal address for any type definition,
 *  (illegal for alignment reasons) which we can use to denote
 *  the UNIVERSAL root MODEL which does not exist as a real
 *  type description in the library.
 */
#define ILLEGAL_DEFINITION ((struct TypeDescription *)0xF0F0F0F7)

/* some bit patterns to make basetype checking faster, etc. */
#define REAL_KIND     0x1
#define INT_KIND      0x2
#define BOOL_KIND     0x4
#define SYM_KIND      0x8
#define SET_KIND      0x10
#define ATOM_KIND     0x20
#define CONSTANT_KIND 0x40
#define COMPOUND_KIND 0x80
#define EQN_KIND      0x8000
#define DUMB_KIND     0x10000
#define ERROR_KIND  ~(REAL_KIND|INT_KIND|BOOL_KIND|SYM_KIND|SET_KIND| \
                      ATOM_KIND|CONSTANT_KIND|COMPOUND_KIND|0x100| \
                      0x200|0x400|0x1000|0x2000|0x4000|EQN_KIND|DUMB_KIND)
#define SIMPLE_KINDS \
     ( REAL_KIND | INT_KIND | BOOL_KIND | SYM_KIND | SET_KIND | EQN_KIND )

/**
	(t & ERROR_KIND) ==> bogus type, there may be other bogus types as well.)
*/
enum type_kind {
  /* complex base types */
  model_type =    COMPOUND_KIND | 0x100,
  array_type =    COMPOUND_KIND | 0x200,
  patch_type =    COMPOUND_KIND | 0x400,
  /* list_type? COMPOUND_KIND | 0x800, */
  /* oddly structured base types */
  relation_type = EQN_KIND | 0x1000,
  logrel_type =   EQN_KIND | 0x2000,
  when_type =     EQN_KIND | 0x4000,
  /* simply structured base types */
  real_type =     ATOM_KIND | REAL_KIND,
  integer_type =  ATOM_KIND | INT_KIND,
  boolean_type =  ATOM_KIND | BOOL_KIND,
  symbol_type =   ATOM_KIND | SYM_KIND,
  set_type =      ATOM_KIND | SET_KIND,
  /* list_type = ATOM_KIND | SET_KIND */
  /* baa: eventually become a variable ordered set */

  /* very simply structured base types */
  real_constant_type =    CONSTANT_KIND | REAL_KIND,
  integer_constant_type = CONSTANT_KIND | INT_KIND,
  boolean_constant_type = CONSTANT_KIND | BOOL_KIND,
  symbol_constant_type =  CONSTANT_KIND | SYM_KIND,
  /* set atoms should be down here */
  /* dummy type */
  dummy_type =    DUMB_KIND
};

struct ConstantTypeDesc {
  unsigned long byte_length;  /**< byte length of an instance */
  union {
    double defreal;           /**< default value for real instances */
    long definteger;          /**< default value for integer instances */
    symchar *defsymbol;       /**< default value for symbol instances */
    unsigned defboolean;      /**< default value for boolean instances */
  } u;                        /**< union of default values */
  unsigned defaulted;         /**< 0 -> ignore default value and units
                                   1 -> don't ignore them */
  CONST dim_type *dimp;       /**< dimensions of instance */
  /* no stinking child list */
};

struct AtomTypeDesc {
  unsigned long byte_length;  /**< byte length of an instance */
  struct ChildDesc *childinfo;/**< description of children */
  unsigned defaulted;         /**< 0 -> ignore default value and units
                                   1 -> don't ignore them */
  double defval;              /**< default value for real instances */
  union {
    double defval;            /**< default value for real instances */
    long defint;              /**< default value for integer instances */
    symchar *defsym;          /**< default value for symbol instances */
    unsigned defbool;         /**< default value for boolean instances */
  } u;                        /**< union of default values */
  CONST dim_type *dimp;       /**< dimensions of instance */
};

/**
	IndexType has been modified to store a key, and a string
	representation of the set of indices. As it stands the set
	of indices is of very little utility. For example, if we had
	"foo[1,2,3,4] IS_A thing;" then what would be saved is
	1,2,3,4. -- which is useful
	Likewise "foo[fooset - [thingset]] IS_A thing;" we would have
	"fooset thingset -"  -- which is not very useful.
	
	So we now save a string representation of the set.
	
	Apparently, the above statement about sets could only have been made
	by someone "with no real clue about the instantiate process", and
	"Both representations are critical to have, and since they
	are cheap, we will have both. The string IS a string from the symbol table."
*/
struct IndexType {
  struct Set *set;    /**< the original set */
  symchar *sptr;      /**< a string representation of the set */
  unsigned int_index; /**< 0 this index is an enumerate one,
                           1 this index is an integer one */
};

struct ArrayDesc {
  struct gl_list_t *indices;    /**< a list of IndexType objects */
  struct TypeDescription *desc; /**< the type of the array/common superclass*/
  /* 
	the following 4 need to be made bit flags.
	At present, this overuse of ints does not dominate the
	union cost at the end of the typedescription struct, so
	we have no incentive to do the fiddling with bits.
  */
  int isintset;       /**< in case the type of the array is a set */
  int isrelation;     /**< TRUE in case of an array of relations */
  int islogrel;       /**< TRUE in case of array of logical relation */
  int iswhen;         /**< TRUE in case of array of WHEN */
};

/** 
	ModelArgs is a structure in preparation for parameterized types 
*/
struct ModelArgs {
  struct StatementList *declarations; /**< list of the statements in the parameter list */
  struct StatementList *absorbed;
  /**<
	list of IS_A statements and their matching assignments
	removed in refinements.
  */
  struct StatementList *reductions;
  /**<
	list of assignments from the REFINES x(reductions)
	these statements are not directly used in instantiation.
	duplicates (references) end up in the absorbed list.
	this list is kept to enable printing types properly.
  */
  struct StatementList *wheres;
  /**<
	list of statements placing additional structural requirements
	on the arguments passed by reference. (WBTS's)
	these statements used for configuration checking in instantiation.
  */
  /* struct gl_list_t *argdata;
   * list of final values, etc if needed. (not yet in use)
   */
  unsigned int argcnt; /**< number of args required in an IS_A of this type. */
};

/**
	Type definition data structure.
*/
struct TypeDescription {
  symchar *name;                    /**< type name */
  enum type_kind t;                 /**< base type of the type */
  short universal;                  /**< TRUE universal type, FALSE non-universal */
  unsigned short flags;             /**< various boolean flags */
  struct TypeDescription *refines;  /**< the type it refines or NULL */
  struct module_t *mod;             /**< module where it is defined */
  ChildListPtr children;            /**< list of children. Never NULL for models */
  struct gl_list_t *init;           /**< initialization procedures */
  struct StatementList *stats;      /**< statements */
  struct gl_list_t *refiners;       /**< list of types that refine this. alpha */
  unsigned long ref_count;          /**< count includes instances, other types */
  long int parseid;                 /**< n as in 'nth definition made' */
  union {
    struct ArrayDesc array;             /**< description of array things */
    struct AtomTypeDesc atom;           /**< atom description stuff */
    struct ConstantTypeDesc constant;   /**< constant description stuff */
    struct ModelArgs modarg;            /**< parameter list stuff */
  } u;                              /**< union of description stuff */
};

#define MAKEARRAYNAMES 1
/**<
	If MAKEARRAYNAMES != 0 we will make up names for arrays
	so clients that want everything to have a name are happy.
	Note that virtually NOTHING works anymore if MAKEARRAYNAMES
	is 0.
*/

/*------------------------------------------------------------------------------
  GETTER / QUERY FUNCTIONS
*/

#ifdef NDEBUG
#define GetChildList(d) ((d)->children)
#else
#define GetChildList(d) GetChildListF(d)
#endif
/**<
 *  Return the childlist field of d.
 *  @param d CONST struct TypeDescription*, type to query.
 *  @return The childlist field as a ChildListPtr.
 *  @see GetChildListF()
 */
ASC_DLLSPEC(ChildListPtr) GetChildListF(CONST struct TypeDescription *d);
/**<
 *  Returns the childlist field of d.
 *  Implementation function for GetChildList() (debug mode).
 *  Do not call this function directly - use GetChildList() instead.
 */

#ifdef NDEBUG
#define GetBaseType(d) ((d)->t)
#else
#define GetBaseType(d) GetBaseTypeF(d)
#endif
/**<
 *  Return the base type of a type description.
 *  @param d CONST struct TypeDescription*, type to query.
 *  @return The base type as an enum type_kind.
 *  @see GetBaseTypeF()
 */
ASC_DLLSPEC(enum type_kind ) GetBaseTypeF(CONST struct TypeDescription *d);
/**<
 *  Implementation function for GetBaseType() (debug mode).
 *  Do not call this function directly - use GetBaseType() instead.
 */

#define BaseTypeIsReal(d)     ((d)->t & REAL_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is real valued. */
#define BaseTypeIsInteger(d)  ((d)->t & INT_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is integer valued. */
#define BaseTypeIsBoolean(d)  ((d)->t & BOOL_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is truth valued. */
#define BaseTypeIsSymbol(d)   ((d)->t & SYM_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is char * valued. */
#define BaseTypeIsSet(d)      ((d)->t & SET_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is a set of anything. */
#define BaseTypeIsAtomic(d)   ((d)->t & ATOM_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is among the atoms. */
#define BaseTypeIsConstant(d) ((d)->t & CONSTANT_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is among the constants. */
#define BaseTypeIsCompound(d) ((d)->t & COMPOUND_KIND)
/**<
 *  Returns TRUE (int) if basetype of TypeDescription *d is among the compound types.
 *  Note that several types are not simple or compound --
 *  we can't really make up our minds whether relations/whens/etc are
 *  simple or compound. In general simple means having a scalar(set)
 *  value. Relations have several values (satisfaction, residual,etc)
 *  and so are not considered simple. Obviously arrays and models are
 *  compound.
 */
#define BaseTypeIsEquation(d) ((d)->t & EQN_KIND)
/**<  Returns TRUE (int) if basetype of TypeDescription *d is among the equation types. */
#define BaseTypeIsSimple(d)   ((d)->t & SIMPLE_KINDS)
/**<
 *  Returns TRUE (int) if basetype of TypeDescription *d is among the simple types.
 *  Note that several types are not simple or compound --
 *  we can't really make up our minds whether relations/whens/etc are
 *  simple or compound. In general simple means having a scalar(set)
 *  value. Relations have several values (satisfaction, residual,etc)
 *  and so are not considered simple. Obviously arrays and models are
 *  compound.
 */

#ifdef NDEBUG
#define GetStatementList(d) ((d)->stats)
#else
#define GetStatementList(d) GetStatementListF(d)
#endif
/**<
 *  Returns the statement list for models and atoms.  In the case of
 *  models this will never return NULL.  In some cases of atoms, this may
 *  return NULL which indicates that the instance doesn't have any
 *  pending statements.
 *  A nonNULL list may, however, be an empty list.
 *  @param d CONST struct TypeDescription*, the type description to query.
 *  @return The statement list as a CONST struct StatementList*.
 *  @see GetStatementListF()
 */
ASC_DLLSPEC(CONST struct StatementList *) GetStatementListF(
	CONST struct TypeDescription *d
);
/**<
 *  Implementation function for GetStatementList() (debug mode).
 *  Do not call this function directly - use GetStatementList() instead.
 */

#ifdef NDEBUG
#define GetInitializationList(d) ((d)->init)
#else
#define GetInitializationList(d) GetInitializationListF(d)
#endif
/**<
 *  Returns the list of initialization procedures.
 *  @param d CONST struct TypeDescription*, the type description to query.
 *  @return The list as a CONST struct gl_list_t*.
 *  @see GetInitializationListF()
 */
ASC_DLLSPEC(struct gl_list_t)
*GetInitializationListF(CONST struct TypeDescription *d);
/**<
 *  Implementation function for GetInitializationList() (debug mode).
 *  Do not call this function directly - use GetInitializationList() instead.
 */

/*------------------------------------------------------------------------------
	the following two functions make it much easier to debug methods
	interactively without reconstructing complex objects.
	Since methods are fully interpretted at run time, this is not an
	unreasonable thing to do.

	It might be a good idea to procedure lock methods in types
	found in files *.a4l.
*/

extern int AddMethods(struct TypeDescription *d,
                      struct gl_list_t *pl,
                      int err);
/**<
	Inserts new methods into a type and all its refinements.
	pl should contain (struct InitProcedure *) to the methods to
	add to d.  The methods named in pl must not conflict with any
	method in d or its refinements.  If err != 0, rejects pl.<br><br>
	
	If return is 1, caller is responsible for pl, OTHERWISE we manage it
	from here on.  If caller supplied d == ILLEGAL_DEFINITION, the caller
	can unconditionally forget pl.
	
	@param d   The type to which the methods in pl should be added.
	@param pl  A gl_list_t of (struct InitProcedure *) to add to d.
	@param err If non-zero, pl is rejected and no methods are added.
	@return Returns 0 if successful, 1 if not.
*/

extern int ReplaceMethods(struct TypeDescription *d,
                          struct gl_list_t *pl,
                          int err);
/**<
	Replaces listed methods in a type and all its refinements that do not
	themselves redefine the methods.  The methods in pl must exist in d or
	else an error condition is returned.  Methods not named in pl but found
	in d or its refinements are left undisturbed. If err != 0, rejects pl.<br><br>
	
	If return is 1, caller is responsible for pl, OTHERWISE we manage it
	from here on.
	
	@param d   The type to which the methods in pl should be replaced.
	@param pl  A gl_list_t of (struct InitProcedure *) to replace in.
	@param err If non-zero, pl is rejected and no methods are replaced.
	@return Returns 0 if successful, 1 if not.
*/

#ifdef NDEBUG
#define CopyTypeDesc(d) ((d)->ref_count++)
#else
#define CopyTypeDesc(d) CopyTypeDescF(d)
#endif
/**<
	Increment the reference count.
	@param d CONST struct TypeDescription*, the type description to query.
	@return No return value.
	@see CopyTypeDescF()

	@todo Why is this called 'CopyTypeDesc' when in fact it's not doing any
	copying?
*/

extern void CopyTypeDescF(struct TypeDescription *d);
/**<
 *  Implementation function for CopyTypeDesc() (debug mode).
 *  Do not call this function directly - use CopyTypeDesc() instead.
 */

extern void DeleteTypeDesc(struct TypeDescription *d);
/**<
	Decrement the reference count.  Eventually, this should delete it
	when ref_count == 0.  

	@NOTE ('to myself')  Remember that array type
	descriptions need to be removed from a list too.
 */

extern void DeleteNewTypeDesc(struct TypeDescription *d);
/**<
	Checks that the type has a refcount of 1 before passing it on to
	DeleteTypeDesc. This is (or should be) the case when deleting
	types that are newly parsed but not yet in the type library.
	Essentially, this is a one-client (AddType) function.
*/


#ifdef NDEBUG
#define GetByteSize(d) ((unsigned)(d)->u.atom.byte_length)
#else
#define GetByteSize(d) GetByteSizeF(d)
#endif
/**<
	Return the byte size of an atom type description.
	@param d CONST struct TypeDescription*, the type description to query.
	@return The size as an unsigned.
	@see GetByteSizeF()
*/
extern unsigned GetByteSizeF(CONST struct TypeDescription *d);
/**<
	Implementation function for GetByteSize() (debug mode).
	Do not call this function directly - use GetByteSize() instead.
*/


#ifdef NDEBUG
#define GetChildDesc(d) ((d)->u.atom.childinfo)
#else
#define GetChildDesc(d) GetChildDescF(d)
#endif
/**<
	Return the child description field of an atom type description.
	@param d CONST struct TypeDescription*, the type description to query.
	@return The child description field as a CONST struct ChildDesc*.
	@see GetChildDescF()
*/
extern CONST struct ChildDesc *GetChildDescF(CONST struct TypeDescription *d);
/**<
	Implementation function for GetChildDesc() (debug mode).
	Do not call this function directly - use GetChildDesc() instead.
*/


#ifdef NDEBUG
#define GetUniversalFlag(d) ((d)->universal)
#else
#define GetUniversalFlag(d) GetUniversalFlagF(d)
#endif
/**<
	Return the universal flag of a type definition
	Gets a short all to itself for no apparent reason.
	@param d CONST struct TypeDescription*, the type description to query.
	@return The flag as an int.
	@see GetUniversalFlagF()
*/
extern int GetUniversalFlagF(CONST struct TypeDescription *d);
/**<
	Implementation function for GetUniversalFlag() (debug mode).
	Do not call this function directly - use GetUniversalFlag() instead.
*/

/*------------------------------------------------------------------------------
	'TYPE FLAGS'
*/

#ifdef NDEBUG
#define GetTypeFlags(d) ((d)->flags)
#else
#define GetTypeFlags(d) GetTypeFlagsF(d)
#endif
/**<
	Return the type flags of a type definition
	There are a number of flags we might want in various bit positions.
	@param d CONST struct TypeDescription*, the type description to query.
	@return The flags as an unsigned short.
	@see GetTypeFlagsF()
*/
extern unsigned short GetTypeFlagsF(CONST struct TypeDescription *d);
/**<
	Implementation function for GetTypeFlags() (debug mode).
	Do not call this function directly - use GetTypeFlags() instead.
*/


#define TYPECONTAINSDEFAULTS 0x1
#define TYPECONTAINSPARINSTS 0x2
/**<
	We can add more, up to the minimum number of bits we expect a short
	to have under any C compiler.
	Universal really should be under this protocol.
*/
#define TYPESHOW 0x100
/**< For browsing purposes */


#ifdef NDEBUG
#define TypeHasDefaultStatements(d) (GetTypeFlags(d)&TYPECONTAINSDEFAULTS)
#else
#define TypeHasDefaultStatements(d) TypeHasDefaultStatementsF(d)
#endif
/**<
	Tells if the statement list of a type has any default ( := ) statements.
	Returns 0 if not.
	This does not refer to the DEFAULT keyword in atoms.
	@param d CONST struct TypeDescription*, the type description to query.
	@return An unsigned.
	@see TypeHasDefaultStatementsF()
*/
extern unsigned TypeHasDefaultStatementsF(CONST struct TypeDescription *d);
/**<
	Implementation function for TypeHasDefaultStatements() (debug mode).
	Do not call this function directly - use TypeHasDefaultStatements() instead.
*/


#ifdef NDEBUG
#define TypeHasParameterizedInsts(d) (GetTypeFlags(d)&TYPECONTAINSPARINSTS)
#else
#define TypeHasParameterizedInsts(d) TypeHasParameterizedInstsF(d)
#endif
/**<
	Tells if the statement lists of a type involve parameters in any way.
	A type that has parameters, or has children that have parameters or
	recursively so returns nonzero result (TYPECONTAINSPARINSTS).
	Returns 0 if not.
	This does not refer to the DEFAULT keyword in atoms.
	@param d CONST struct TypeDescription*, the type description to query.
	@return An unsigned.
	@see TypeHasParameterizedInstsF()
*/
ASC_DLLSPEC(unsigned) TypeHasParameterizedInstsF(CONST struct TypeDescription *d);
/**<
	Implementation function for TypeHasParameterizedInsts() (debug mode).
	Do not call this function directly - use TypeHasParameterizedInsts() instead.
*/

/*------------------------------------------------------------------------------
	DEFAULT VALUES
*/

#define AtomDefaulted(d) ((d)->u.atom.defaulted)
/**<
	Returns TRUE if the atom has a default value; otherwise returns FALSE.
*/

#define ConstantDefaulted(d) ((d)->u.constant.defaulted)
/**<
	Returns TRUE if the Constant has a default value; otherwise returns FALSE.
*/

#define GetIntDefault(d) ((d)->u.atom.u.defint)
/**<  Returns the long default value of TypeDescription *d. */

#define GetSymDefault(d) ((d)->u.atom.u.defsym)
/**<  Returns the symchar* default value of TypeDescription *d. */


#ifdef NDEBUG
#define GetRealDefault(d) ((d)->u.atom.u.defval)
#else
#define GetRealDefault(d) GetRealDefaultF((d),__FILE__,__LINE__)
#endif
/**<  Returns the double default value of TypeDescription *d. */
extern double GetRealDefaultF(CONST struct TypeDescription *d,
                              CONST char *f, CONST int l);
/**<
	Implementation function for GetRealDefault() (debug mode).
	Do not call this function directly - use GetRealDefault() instead.
*/


#ifdef NDEBUG
#define GetBoolDefault(d) ((d)->u.atom.u.defbool)
#else
#define GetBoolDefault(d) GetBoolDefaultF((d),__FILE__,__LINE__)
#endif
/**<  Returns the unsigned default value of TypeDescription *d. */
extern unsigned GetBoolDefaultF(CONST struct TypeDescription *d,
                                CONST char *f, CONST int l);
/**<
	Implementation function for GetBoolDefault() (debug mode).
	Do not call this function directly - use GetBoolDefault() instead.
*/


#define GetConstantDefReal(d)    ((d)->u.constant.u.defreal)
/**<  Returns the double default value of constant TypeDescription *d. */

#define GetConstantDefInteger(d) ((d)->u.constant.u.definteger)
/**<  Returns the long default value of constant TypeDescription *d. */

#define GetConstantDefBoolean(d) ((d)->u.constant.u.defboolean)
/**<  Returns the short default value of constant TypeDescription *d. */

#define GetConstantDefSymbol(d)  ((d)->u.constant.u.defsymbol)
/**<  Returns the symchar* default value of constant TypeDescription *d. */


/*------------------------------------------------------------------------------
	DIMENSIONS
*/

#ifdef NDEBUG
#define GetRealDimens(d) ((d)->u.atom.dimp)
#else
#define GetRealDimens(d) GetRealDimensF((d),__FILE__,__LINE__)
#endif
/**<
	Returns the dimensions of the atom.
	If atoms is not numeric, return value is uncertain.
	Will not work on constants.
	@param d CONST struct TypeDescription*, the type description to query.
	@return The dimensions as a CONST dim_type*.
	@see GetRealDimensF()
*/
ASC_DLLSPEC(CONST dim_type*) GetRealDimensF(CONST struct TypeDescription *d,
                                      CONST char *f, CONST int l);
/**<
	Implementation function for GetRealDimens() (debug mode).
	Do not call this function directly - use GetRealDimens() instead.
*/


#ifdef NDEBUG
#define GetConstantDimens(d) ((d)->u.constant.dimp)
#else
#define GetConstantDimens(d) GetConstantDimensF((d),__FILE__,__LINE__)
#endif
/**<
	Returns the dimensions of the constant.
	All constants have dimensionality. nonreal constants have the
	dim for DIMENSIONLESS. nonconstants and unassigned real constants
	will return WildDimension().
	@param d CONST struct TypeDescription*, the type description to query.
	@return The dimensions as a CONST dim_type*.
	@see GetConstantDimensF()
*/
ASC_DLLSPEC(CONST dim_type*) GetConstantDimensF(CONST struct TypeDescription *d,
                                          CONST char *f, CONST int l);
/**<
	Implementation function for GetConstantDimens() (debug mode).
	Do not call this function directly - use GetConstantDimens() instead.
*/


#ifdef NDEBUG
#define GetName(d) ((d)->name)
#else
#define GetName(d) GetNameF(d)
#endif
/**<
	Returns the name of the type that this structure defines.
	@param d CONST struct TypeDescription*, the type description to query.
	@return The name as a symchar*.
	@see GetNameF()
*/
ASC_DLLSPEC(symchar*) GetNameF(CONST struct TypeDescription *d);
/**<
	Implementation function for GetName() (debug mode).
	Do not call this function directly - use GetName() instead.
*/


#define GetParseId(d) ((d)->parseid)
/**<
	Returns the parseid of type d.
	
	@param d The type to query (TypeDescription *).
	@return The parseid of d as a long int.
*/

#define GetRefinement(d) ((d)->refines)
/**<
	Returns the refinement of type d, or NULL if none.
	
	@param d The type to query (TypeDescription *).
	@return The refinement of d as a TypeDescription *.
*/

#define GetRefiners(d) ((d)->refiners)
/**<
	Returns a list of refiners of type d, or NULL if none.
	
	@param d The type to query (TypeDescription *).
	@return The refiners of d as a gl_list of TypeDescription *.
*/

ASC_DLLSPEC(struct gl_list_t *) GetAncestorNames(CONST struct TypeDescription *d);
/**<
	Return the names of ancestors of type d given.
	If none, list may be empty. The list is of symchar *.
	The caller should destroy the list but not its contents.

	@param d The type to query.
	@return Pointer to a gl_list_t containing the names of the ancestors of d.
*/

#define GetModelParameterList(d) ((d)->u.modarg.declarations)
/**<
	Return the statements (forward declarations that constitute the
	parameter list for d. The statement list returned will contain
	willbes (possibly with value), and isas.
	Any attempt to use the type d in a RHS must fill in all the
	statements on this list with appropriate instances.

	@param d The type to query (TypeDescription *).
	@return The parameter list of d as a struct StatementList *.
*/

#define GetModelParameterCount(d) ((d)->u.modarg.argcnt)
/**<
	Returns the number of arguments required when IS_A'ing a MODEL type.
	Any attempt to use the type d in a RHS must fill in this many slots.

	@param d The type to query (TypeDescription *).
	@return The count as an unsigned int.
*/

#define GetModelAbsorbedParameters(d) ((d)->u.modarg.absorbed)
/**<
 *  Return the list of ISAs and CASGNs that absorbed them.
 *  May return NULL.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The list as a struct StatementList *.
 */

#define GetModelParameterReductions(d) ((d)->u.modarg.reductions)
/**<
 *  Return the list of statements that absorb parameter isas in the
 *  statement of this MODEL. Used for display purposes primarily.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The list as a struct StatementList *.
 */

#define GetModelParameterWheres(d) ((d)->u.modarg.wheres)
/**<
 *  Return the list of statements that restrict the structure of
 *  WILL_BE passed arguments in this MODEL.
 *  Used for instantiation-time checks.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The list as a struct StatementList *.
 */

#define GetModelParameterValues(d) ((d)->u.modarg.argdata)
/**<
 *  NOT IMPLEMENTED.
 *  Returns the list of values that match the ParameterLists.
 *  Possibly this is a redundancy.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The list as a gl_list_t *.
 */

#define GetArrayBaseType(d) ((d)->u.array.desc)
/**<
 *  Returns the base type of the array type description.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The base type as a TypeDescription *.
 */

#define GetArrayBaseIsRelation(d) ((d)->u.array.isrelation)
/**<
 *  Returns TRUE if the array is an array of relations.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return An int indicating whether d is an array of relations.
 */

#define GetArrayBaseIsLogRel(d) ((d)->u.array.islogrel)
/**<
 *  Returns TRUE if the array is an array of logical relations.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return An int indicating whether d is an array of logical relations.
 */

#define GetArrayBaseIsWhen(d) ((d)->u.array.iswhen)
/**<
 *  Returns TRUE if the array is an array of WHEN's.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return An int indicating whether d is an array of WHENs.
 */

#define GetArrayBaseIsInt(d) ((d)->u.array.isintset)
/**<
 *  Returns TRUE if the array is a set.
 *  This is only meaningful when the base type is a refinement of
 *  set, but it is always available.  Its value is inconsequential
 *  when the base type is not a set.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return An int indicating whether d is a set.
 */

#define GetArrayIndexList(d) ((d)->u.array.indices)
/**<
 *  Returns a list of the indices of an array.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return A l_list_t * containing the indices of d.
 */

#define GetModule(d) ((d)->mod)
/**<
 *  Returns the module in which type d is defined.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The module associated with d as a module_t *.
 */

#define GetPatchOriginal(d)  ((d)->refines)
/**<
 *  Returns the original type of the TypeDescription being patched.
 *  NULL must be treated as an error, as a patch is meaningless without
 *  the original type. Internally it uses the same slot as refines.
 *
 *  @param d The type to query (TypeDescription *).
 *  @return The original type of d as a TypeDescription *.
 */

extern struct IndexType *CreateIndexType(struct Set *set, int int_index);
/**<
 *  Creates an IndexType. The set becomes the property of the index.
 *
 *  @param set The set to use for the new IndexType.
 *  @param int_index Flag for whether set is enumerated (0) or int (non-zero).
 *  @return The new IndexType.
 */

extern struct IndexType *CreateDummyIndexType(int int_index);
/**<
 *  Returns the dummy IndexType for int (int_index != 0) or for enum
 *  (int_index == 0).  There is exactly one of each and you should not
 *  do anything with it except destroy it.
 *
 *  @param int_index Flag for whether to return dummy for enum (0)
 *                   or int (non-zero).
 *  @return The dummy IndexType of the specified kind.
 */

extern struct IndexType *CreateIndexTypeFromStr(char *str, int int_index);
/**<
 *  Creates an IndexType from a string. We do NOT keep the
 *  string given; we keep the symbol table's version of the string.
 *  This operator is exceedingly redundant and should not be used.
 *
 *  @param str The string to use to create the new IndexType.
 *  @param int_index Flag for whether the set is enumerated (0) or int (non-zero).
 *  @return The new IndexType.
 */

extern void DestroyIndexType(struct IndexType *ind);
/**<
 *  Deallocates this structure and its internals.  The string is
 *  assumed NOT to belong to the main symbol table, and thus freed.
 *
 *  @param ind The IndexType to destroy.
 */

#define GetIndexType(p) ((p)->int_index)
/**<
 *  Returns the kind flag for IndexType *p.
 *
 *  @param p The type to query (IndexType *).
 *  @return The kind flag for p as an unsigned (0 => enum, 1 => int).
 */

#define GetIndexSet(p) ((p)->set)
/**<
 *  Returns the set associated with IndexType *p.
 *  Note this may return NULL in odd circumstances.
 *
 *  @param p The type to query (IndexType *).
 *  @return The set associated with p as a Set *.
 */

#define GetIndexSetStr(p) ((p)->sptr)
/**<
 *  Returns the string associated with IndexType *p.
 *  Note this may return NULL in odd circumstances.
 *
 *  @param p The type to query (IndexType *).
 *  @return The set string associated with p as a symchar *.
 */

/*------------------------------------------------------------------------------
  CONSTRUCTORS
*/

extern struct TypeDescription
*CreateModelTypeDesc(symchar *name,
                     struct TypeDescription *rdesc,
                     struct module_t *mod,
                     ChildListPtr cl,
                     struct gl_list_t *pl,
                     struct StatementList *sl,
                     int univ,
                     struct StatementList *psl,
                     struct StatementList *rsl,
                     struct StatementList *tsl,
                     struct StatementList *vsl);
/**<
 *  Creates a Model TypeDescription structure with the parameters given.
 *  sl, psl, rsl, wsl should NEVER be NULL, though they may be empty lists.
 *  tsl may be NULL.
 *
 *  @param name   Name of the type.
 *  @param rdesc  Type that it refines or NULL.
 *  @param mod    Module it is defined in.
 *  @param cl     List of the type's child names.
 *  @param pl     List of initialization procedures.
 *  @param sl     List of declarative statements.
 *  @param univ   TRUE universal FALSE non-universal.
 *  @param psl    List of parameter statements.
 *  @param rsl    List of parameter reducing statements.
 *  @param tsl    List of reduced statements.
 *  @param vsl    List of parameter constraint statements.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription *CreateDummyTypeDesc(symchar *name);
/**<
 *  Creates the universal dummy type for unselected instances.
 *
 *  @param name The name for the dummy type.
 *  @return A pointer to the new dummy TypeDescription structure.
 */

extern struct TypeDescription
*CreateConstantTypeDesc(symchar *name,
                        enum type_kind t,
                        struct TypeDescription *rdesc,
                        struct module_t *mod,
                        unsigned long bytesize,
                        int defaulted,
                        double rval,
                        CONST dim_type *dim,
                        long ival,
                        symchar *sval,
                        int univ);
/**<
 *  Create a Constant TypeDescription structure with the parameters given.
 *
 *  @param name         Name of type.
 *  @param t            Base type of Const(real_constant_type,etc).
 *  @param rdesc        Type description what it refines.
 *  @param mod          Module where the type is defined.
 *  @param bytesize     Size of an instance in bytes.
 *  @param defaulted    TRUE indicates default value was assigned.
 *  @param rval         Default value for real atoms.
 *  @param dim          Dimensions of default value.
 *  @param ival         Default value for int/bool constants.
 *  @param sval         Default value for symbol constants.
 *  @param univ         TRUE universal FALSE non-universal.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateAtomTypeDesc(symchar *name,
                    enum type_kind t,
                    struct TypeDescription *rdesc,
                    struct module_t *mod,
                    ChildListPtr childl,
                    struct gl_list_t *procl,
                    struct StatementList *statl,
                    unsigned long bytesize,
                    struct ChildDesc *childd,
                    int defaulted,
                    double dval,
                    CONST dim_type *ddim,
                    int univ,
                    long ival,
                    symchar *sval);
/**<
 *  Creates an Atom TypeDescription structure with the parameters given.
 *
 *  @param name       Name of type.
 *  @param t          Base type of atom (real_type, boolean_type, etc.)
 *  @param rdesc      Type description what it refines.
 *  @param mod        Module where the type is defined.
 *  @param childl     List of children names.
 *  @param procl      List of initialization procedures.
 *  @param statl      List of declarative statements.
 *  @param bytesize   Size of an instance in bytes.
 *  @param childd     Description of the atom's children.
 *  @param defaulted  Valid only for real atoms.  TRUE indicates default value assigned.
 *  @param dval       Default value for real atoms.
 *  @param ddim       Dimensions of default value.
 *  @param univ       TRUE universal FALSE non-universal.
 *  @param ival       Defalut value for integer/boolean atoms.
 *  @param sval       Default value for symbol atoms.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateRelationTypeDesc(struct module_t *mod,
                        ChildListPtr clist,
                        struct gl_list_t *plist,
                        struct StatementList *statl,
                        unsigned long bytesize,
                        struct ChildDesc *childd);
/**<
 *  Creates a Relation TypeDescription structure with the parameters given.
 *
 *  @param mod        The module where it is definde.
 *  @param clist      The list of children names.
 *  @param plist      The list of initialization procedures.
 *  @param statl      The list of declarative statements.
 *  @param bytesize   The byte length.
 *  @param childd     The description of the children.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateLogRelTypeDesc(struct module_t *mod,
                      ChildListPtr clist,
                      struct gl_list_t *plist,
                      struct StatementList *statl,
                      unsigned long bytesize,
                      struct ChildDesc *childd);
/**<
 *  Creates a Logical Relation TypeDescription structure with the parameters given.
 *
 *  @param mod        The module where it is defined.
 *  @param clist      The list of children names.
 *  @param plist      The list of initialization procedures.
 *  @param statl      The list of declarative statements.
 *  @param bytesize   The byte length.
 *  @param childd     The description of the children.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateWhenTypeDesc(struct module_t *mod,
                    struct gl_list_t *pl,
                    struct StatementList *sl);
/**<
 *  Creates a When TypeDescription structure with the parameters given.
 *
 *  @param mod The module where it is defined.
 *  @param pl  The list of initialization procedures.
 *  @param sl  The list of declarative statements.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreateArrayTypeDesc(struct module_t *mod,
                     struct TypeDescription *desc,
                     int isintset,
                     int isrel,
                     int islogrel,
                     int iswhen,
                     struct gl_list_t *indices);
/**<
 *  Creates an Array TypeDescription structure with the parameters given.
 *
 *  @param mod      The module of the statement where the array is instanced.
 *  @param desc     The type description of the base type.
 *  @param isintset Used only when "desc" is a refinement of set.
 *  @param isrel    TRUE only when it is an array of relations.
 *  @param islogrel TRUE only when it is an array of logical relations.
 *  @param iswhen   TRUE only when it is an array of WHEN's.
 *  @param indices  A list of IndexType's this describes each array index.
 *  @return A pointer to the new TypeDescription structure.
 */

extern struct TypeDescription
*CreatePatchTypeDesc(symchar *name,
                     struct TypeDescription *rdesc,
                     struct module_t *mod,
                     struct gl_list_t *pl,
                     struct StatementList *sl);
/**<
 *  Creates a Patch TypeDescription structure with the parameters given.
 *
 *  Patches are constructs which are not strictly part of the main
 *  language. They have been developed to enable automatic generation
 *  of code from an ASCEND instance whilst maintaing proper scope,
 *  and naming etc..., without having to generate entire copies of the
 *  type libraries, which may differ in just a few key ways.
 *  They do just what they say: given an original type description
 *  (mandatory), patch it with the new statements and intializations.
 *  The patch does *not* change the original type; it will be applied
 *  at the time of instantiation. Otherwise patches look just like
 *  other types.<br><br>
 *
 *  Patch files need some strong sanity checking to be implemented.
 *
 *  @param name   The name of the patch.
 *  @param rdesc  The type description of the type being patched.
 *  @param mod    The module that the patch was defined in.
 *  @param pl     List of procedural statements.
 *  @param sl     List of declarative statements.
 *  @return A pointer to the new TypeDescription structure.
 */

/*------------------------------------------------------------------------------
	POLYMORPHISM STUFF
*/

ASC_DLLSPEC(struct TypeDescription*) MoreRefined(CONST struct TypeDescription *desc1,
                                           CONST struct TypeDescription *desc2);
/**<
 *  Returns the more refined of desc1 or desc2, or
 *  NULL if they are unconformable.
 */

extern CONST struct TypeDescription
*GreatestCommonAncestor(CONST struct TypeDescription *desc1,
                        CONST struct TypeDescription *desc2);
/**<
 *  Returns the most refined common ancestor type of desc1,desc2.
 *  If either of desc1 or desc2 directly REFINES the other, then
 *  the return value will be the less refined of the two types.
 *  Returns NULL if unconformable.<br><br>
 *
 *  Due to malloc/free calls, this is not the cheapest call to make.
 *  If you need it for a time critical test, then you need to redo
 *  the implementation of internal functions Create/DestroyAncestor
 *  to reuse memory.
 */

extern int TypesAreEquivalent(CONST struct TypeDescription *desc1,
                              CONST struct TypeDescription *desc2);
/**<
 *  Returns 1 if types appear to be equivalent semantically.
 *  This means comparing statment lists, methods, and so forth.
 *  Uses, among others, CompareStatementLists CompareStatement,
 *  and CompareProcedureLists, which is broken in a FALSE negative way.
 *  Array and patch types are never equivalent.
 */

extern void DifferentVersionCheck(CONST struct TypeDescription *desc1,
                                  CONST struct TypeDescription *desc2);
/**<
 *  Checks that desc1 and desc2 are unconformable.
 *  It is assumed that desc1 and desc2 are unconformable.  This routine
 *  tries to check if they are unconformable because of different versions
 *  of the types in their type hierarchy.  This can happen if one of these
 *  type descriptions is based on an edited version of the type.
 *
 *  It reports its findings to standard error.
 */

extern struct TypeDescription *GetStatTypeDesc(CONST struct Statement *s);
/**<
 *  Returns the type field of an IS_A, WILL_BE, or IS_REFINED_TO statement
 *  converted into a TypeDescription.  Note that in some cases the result
 *  may be NULL -- and the user should check.  sets and arrays do not
 *  return a proper type this way -- if you consider proper to be an
 *  array type description or a set description of the relevant base type.
 *  This should also not be used on statements defining children
 *  of atoms because children of atoms do not have TypeDescriptions.
 *  It must be passed one of these types of statement.
 *  Other statements will return NULL or crash.
 *  Given all the caveats, this is a darn handy operator.
 *
 *  @param s The statement to evaluate.
 *  @return The type field of the statement, which may be NULL.
 */

extern void WriteArrayTypeList(FILE *fp);
/**<
 *  Writes the internal (implicit) array types to file fp.
 *  This is a debugging function.  The information written isn't likely
 *  to be very meaningful.  It relies on internal variables, so we
 *  can't put it in type_descio.c.
 */

/*------------------------------------------------------------------------------
  'SHOW' FLAG
*/

#ifdef NDEBUG
#define TypeShow(d) (GetTypeFlags(d)&TYPESHOW)
#else
#define TypeShow(d) TypeShowF(d)
#endif
/**<
 *  Tells if the type description should be shown or not.
 *  Returns 1 is the type is going to be shown. Returns 0 if not.
 *  @param d The type description to query (CONST struct TypeDescription*).
 *  @return The result as an unsigned.
 *  @see TypeShowF()
 */
ASC_DLLSPEC(unsigned) TypeShowF(CONST struct TypeDescription *d);
/**<
 *  Implementation function for TypeShow() (debug mode).
 *  Do not call this function directly - use TypeShow() instead.
 */

ASC_DLLSPEC(void ) SetTypeShowBit(struct TypeDescription *d, int value);
/**<
 *  Sets the bit TYPESHOW.  This bit is for browsing purposes.
 *  It will tell is the user wants to view the instances of a specific type.
 *  The value of the bit will be selected in the Library window.
 *
 *  @param d     The type to modify.
 *  @param value Flag for whether to set (non-zero) or clear (0) the show bit.
 */

#endif  /* ASC_TYPE_DESC_H */


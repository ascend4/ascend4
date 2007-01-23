/*
 *  Atom Child Description List
 *  by Tom Epperly
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: childinfo.h,v $
 *  Date last modified: $Date: 1998/02/05 16:35:41 $
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 *  These routines provide a list for storing descriptions of children of
 *  atoms.
 */

/** @file
 *  Atom Child Description List.
 *  <pre>
 *  When #including childinfo.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *         #include "list.h"
 *         #include "dimen.h"
 *         #include "setinstval.h"
 *         #include "instance_enum.h"
 *  </pre>
 */

#ifndef ASC_CHILDINFO_H
#define ASC_CHILDINFO_H

#include "dimen.h"

/**	@addtogroup compiler Compiler
	@{
*/

/** Atom child value for Real type. */
struct ChildDescR {
  double value;
  CONST dim_type *dims;
};

/** Atom child value for Integer type. */
struct ChildDescI {
  long value;     /**< always mutable */
};

/** Atom child value for Set type. */
struct ChildDescS{
  struct set_t *slist;  /**< set list */
  unsigned is_int;      /**< integer 1 or symbol 0 set */
};

/** Atom child value - union of all types. */
union ChildDescUnion {
  int bvalue;               /**< boolean value */
  struct ChildDescS s;      /**< set information */
  struct ChildDescI ivalue; /**< integer value */
  struct ChildDescR rvalue; /**< real value */
  symchar *svalue;          /**< symbol value */
};

/** Atom child types. */
enum ChildDescT {
  bad_child,       /**< invalid type.
                    *  This should never, ever, ever be seen except in childless
                    *  atoms who need a place holder.  bad_child-ren are never
                    *  assigned and the UNION meaningless
                    */
  real_child,     /**< real type. */
  integer_child,  /**< integer type. */
  boolean_child,  /**< boolean type. */
  set_child,      /**< set type. */
  symbol_child    /**< symbol type. */
};

/** Atom child description. */
struct ChildDesc {
  enum ChildDescT t;      /**< Child type. */
  unsigned assigned;      /**< Assigned flag = 0 or 1 */
  union ChildDescUnion u; /**< Child value. */
};

#ifdef NDEBUG
/** Initialize a child description structure (do nothing in release mode). */
#define ICDESC(x)
/** Initialize a child description structure via a pointer (do nothing in release mode). */
#define ICDESCPTR(y)
#else
/**
 *  Initialize a child description structure.
 *  x must be a struct ChildDesc variable, NOT a pointer to same.
 *  If you want to init a pointer contents, use ICDESCPTR().
 */
#define ICDESC(x) CDescInit(&(x))
/**
 *  Initialize a child description structure via a pointer.
 *  y must be a pointer to a struct ChildDesc variable.
 *  If you want to init a ChildDesc variable directly, use ICDESC().
 */
#define ICDESCPTR(y) CDescInit(y)
#endif

extern void CDescInit(struct ChildDesc *c);
/**<
 *  <!--  CDescInit(c)                                                 -->
 *  Initializes the contents of c to 0.
 *  Do not call this function -- use ICDESC() or IDESCPTR() instead.
 */

extern struct ChildDesc *CreateChildDescArray(unsigned long l);
/**<
 *  <!--  struct ChildDesc *CreateChildDescArray(l)                    -->
 *  <!--  unsigned long l;                                             -->
 *  Allocate space for an array of length l.  This will not initialize
 *  the array in any special way.  Use ICDESC() or ICDESCPTR() to initialize
 *  the elements of the array.  The array should be accessed using
 *  AssignChildArrayElement() and GetChildArrayElement().  When finished
 *  with the array, deallocate it using DestroyChildDescArray().
 */

extern struct ChildDesc *CreateEmptyChildDescArray(void);
/**<
 *  <!--  struct ChildDesc *CreateEmptyChildDescArray();               -->
 *  Allocate space for an array of length 1.
 *  The array element will be initialized as type bad_child (its value is
 *  not initialized).  The array should be accessed using
 *  AssignChildArrayElement() and GetChildArrayElement().  When finished
 *  with the array, deallocate it using DestroyChildDescArray().
 */

extern void DestroyChildDescArray(struct ChildDesc *c,
                                  unsigned long l);
/**<
 *  <!--  void DestroyChildDescArray(c,l)                              -->
 *  <!--  struct ChildDesc *c;                                         -->
 *  <!--  unsigned long l;                                             -->
 *  This routine will deallocate the space for array c of length l.
 *  Any elements of c which are sets will have all subelements
 *  properly deallocated also.
 */

#ifdef NDEBUG
#define AssignChildArrayElement(a,n,e) (a)[(n)-1] = (e)
#else
#define AssignChildArrayElement(a,n,e) AssignChildArrayElementF((a),(n),(e))
#endif
/**<
 *  Set an element of a ChildDesc array.
 *  @param a  struct ChildDesc*, the array to modify.
 *  @param n  unsigned long, index of the element to set.
 *            Should be between 1 and the length of "a" (never 0).
 *  @param e  struct ChildDesc, the value to which element n should be set.
 *  @return No return value.
 *  @see AssignChildArrayElementF()
 */
extern void AssignChildArrayElementF(struct ChildDesc *a,
                                     unsigned long n,
                                     struct ChildDesc e);
/**<
 *  <!--  macro AssignChildArrayElement(a,n,e)                           -->
 *  <!--  void AssignChildArrayElementF(a,n,e)                           -->
 *  <!--  struct ChildDesc *a;                                           -->
 *  <!--  unsigned long n;                                               -->
 *  <!--  struct ChildDesc e;                                            -->
 *  <!--                                                                 -->
 *  <!--  Set the n'th child's description to e.  n should range from 1  -->
 *  <!--  to the length of "a".  n should never be zero.                 -->
 *  Implementation function for AssignChildArrayElement().
 *  Do not use this function directly - use AssignChildArrayElement() instead.
 */

#ifdef NDEBUG
#define GetChildArrayElement(a,n) ((a)[n-1])
#else
#define GetChildArrayElement(a,n) GetChildArrayElementF((a),(n))
#endif
/**<
 *  Get an element from a ChildDesc array.
 *  @param a  struct ChildDesc*, the array to query.
 *  @param n  unsigned long, index of the element to retrieve.
 *            Should be between 1 and the length of "a" (never 0).
 *  @return  Returns a ChildDesc corresponding to element n.
 *  @see GetChildArrayElementF()
 */
extern struct ChildDesc GetChildArrayElementF(CONST struct ChildDesc *a,
                                              unsigned long n);
/**<
 *  <!--  macro GetChildArrayElement(a,n)                              -->
 *  <!--  struct ChildDesc GetChildArrayElementF(a,n)                  -->
 *  <!--  const struct ChildDesc *a;                                   -->
 *  <!--  unsigned long n;                                             -->
 *  <!--  Return the n'th child description from "a".  n ranges from 1 -->
 *  <!--  to the length of "a".                                        -->
 *  Implementation function for GetChildArrayElement().
 *  Do not use this function directly - use GetChildArrayElement() instead.
 */

#ifdef NDEBUG
#define ChildDescType(e) ((e).t)
#else
#define ChildDescType(e) ChildDescTypeF(e)
#endif
/**<
 *  Return the type of e.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns the ChildDescT of e.
 *  @see ChildDescTypeF()
 */
extern enum ChildDescT ChildDescTypeF(struct ChildDesc e);
/**<
 *  <!--  macro ChildDescType(e)                                       -->
 *  <!--  enum ChildDescT ChildDescF(e)                                -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the type of e.                                        -->
 *  Implementation function for ChildDescType().
 *  Do not use this function directly - use ChildDescType() instead.
 */

#ifdef NDEBUG
#define ValueAssigned(e) ((e).assigned)
#else
#define ValueAssigned(e) ValueAssignedF(e)
#endif
/**<
 *  Test whether ChildDesc e has been assigned a value.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as an int a true value if e has been assigned a
 *           value, a false value otherwise.
 *  @see ValueAssignedF()
 */
extern int ValueAssignedF(struct ChildDesc e);
/**<
 *  <!--  macro ValueAssigned(e)                                        -->
 *  <!--  int ValueAssignedF(e)                                         -->
 *  <!--  struct ChildDesc e;                                           -->
 *  <!--  Return a true value if e the child has been assigned a value. -->
 *  <!--  Otherwise, returns a false value.                             -->
 *  Implementation function for ValueAssigned().
 *  Do not use this function directly - use ValueAssigned() instead.
 */

#ifdef NDEBUG
#define IntegerDefault(e) ((e).u.ivalue.value)
#else
#define IntegerDefault(e) IntegerDefaultF(e)
#endif
/**<
 *  Return the default value of integer ChildDesc e.  The value is
 *  meaningless if e is not an integer_child, or if ValueAssigned(e)
 *  is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as a long the default value of e.
 *  @see IntegerDefaultF()
 */
extern long IntegerDefaultF(struct ChildDesc e);
/**<
 *  <!--  macro IntegerDefault(e)                                         -->
 *  <!--  long IntegerDefaultF(e)                                         -->
 *  <!--  struct ChildDesc e;                                             -->
 *  <!--  Return the integer default value assuming that e is an integer_child -->
 *  <!--  This value is meaningless if ValueAssigned(e) is not true.      -->
 *  Implementation function for IntegerDefault().
 *  Do not use this function directly - use IntegerDefault() instead.
 */

#ifdef NDEBUG
#define BooleanDefault(e) ((e).u.bvalue)
#else
#define BooleanDefault(e) BooleanDefaultF(e)
#endif
/**<
 *  Return the default value of boolean ChildDesc e.  The value is
 *  meaningless if e is not an boolean_child, or if ValueAssigned(e)
 *  is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as an int the default value of e.
 *  @see BooleanDefaultF()
 */
extern int BooleanDefaultF(struct ChildDesc e);
/**<
 *  <!--  macro BooleanDefault(e)                                      -->
 *  <!--  int BooleanDefaultF(e)                                       -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the boolean default value assuming that e is a boolean_child. -->
 *  <!--  This value is meaningless if ValueAssigned(e) is not true.   -->
 *  Implementation function for BooleanDefault().
 *  Do not use this function directly - use BooleanDefault() instead.
 */

#ifdef NDEBUG
#define SetDefault(e) ((e).u.s.slist)
#else
#define SetDefault(e) SetDefaultF(e)
#endif
/**<
 *  Return the default value of set ChildDesc e.  The value is
 *  meaningless if e is not an set_child, or if ValueAssigned(e)
 *  is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as a CONST struct set_t* the default value of e.
 *  @see SetDefaultF()
 */
extern CONST struct set_t *SetDefaultF(struct ChildDesc e);
/**<
 *  <!--  macro SetDefault(e)                                          -->
 *  <!--  const struct set_t *SetDefaultF(e)                           -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the set default value assuming that e is a set_child. -->
 *  <!--  This value is meaningless if ValueAssigned(e) is not true.   -->
 *  Implementation function for SetDefault().
 *  Do not use this function directly - use SetDefault() instead.
 */

#ifdef NDEBUG
#define SetIsIntegerSet(e) ((e).u.s.is_int)
#else
#define SetIsIntegerSet(e) SetIsIntegerSetF(e)
#endif
/**<
 *  Return the set type of e.  The value is meaningless if e is not a
 *  set_child, or if ValueAssigned(e) is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns an int:  0 for a symbol set, 1 for an integer set.
 *  @see SetIsIntegerSetF()
 */
extern int SetIsIntegerSetF(struct ChildDesc e);
/**<
 *  <!--  macro SetIsIntegerSet(e)                                     -->
 *  <!--  int SetIsIntegerSetF(e)                                      -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the set type.  0 symbol and 1 integer.                -->
 *  Implementation function for SetIsIntegerSet().
 *  Do not use this function directly - use SetIsIntegerSet() instead.
 */

#ifdef NDEBUG
#define SymbolDefault(e) ((e).u.svalue)
#else
#define SymbolDefault(e) SymbolDefaultF(e)
#endif
/**<
 *  Return the default value of symbol ChildDesc e.  The value is
 *  meaningless if e is not an symbol_child, or if ValueAssigned(e)
 *  is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as a symchar* the default value of e.
 *  @see SymbolDefaultF()
 */
extern symchar *SymbolDefaultF(struct ChildDesc e);
/**<
 *  <!--  macro SymbolDefault(e)                                       -->
 *  <!--  symchar *SymbolDefaultF(e)                                   -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the symbol default value assuming that e is a symbol_child. -->
 *  <!--  This value is meaningless is ValueAssigned(e) is not true.   -->
 *  Implementation function for SymbolDefault().
 *  Do not use this function directly - use SymbolDefault() instead.
 */

#ifdef NDEBUG
#define RealDefaultValue(e) ((e).u.rvalue.value)
#else
#define RealDefaultValue(e) RealDefaultValueF(e)
#endif
/**<
 *  Return the default value of real ChildDesc e.  The value is
 *  meaningless if e is not an real_child, or if ValueAssigned(e)
 *  is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as a double the default value of e.
 *  @see RealDefaultValueF()
 */
extern double RealDefaultValueF(struct ChildDesc e);
/**<
 *  <!--  macro RealDefaultValue(e)                                    -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the real default value assuming that e is a real_child. -->
 *  <!--  This value is meaningless if ValueAssigned(e) is not true.   -->
 *  Implementation function for RealDefaultValue().
 *  Do not use this function directly - use RealDefaultValue() instead.
 */

#ifdef NDEBUG
#define RealDimensions(e) ((e).u.rvalue.dims)
#else
#define RealDimensions(e) RealDimensionsF(e)
#endif
/**<
 *  Return the units pointer of real ChildDesc e.  The value is
 *  meaningless if e is not an real_child, or if ValueAssigned(e)
 *  is not true.
 *  @param e  struct ChildDesc, the ChildDesc to query.
 *  @return  Returns as a CONST dim_type* the units pointer of e.
 *  @see RealDimensionsF()
 */
extern CONST dim_type *RealDimensionsF(struct ChildDesc e);
/**<
 *  <!--  macro RealDimensions(e)                                      -->
 *  <!--  struct ChildDesc e;                                          -->
 *  <!--  Return the units pointer assuming that e is a real_child.    -->
 *  <!--  This value is meaningless if ValueAssigned(e) is not true.   -->
 *  Implementation function for RealDimensions().
 *  Do not use this function directly - use RealDimensions() instead.
 */

extern struct ChildDesc MakeRealDesc(int assigned,
                                     double v,
                                     CONST dim_type *dims);
/**<
 *  <!--  struct ChildDesc MakeRealDesc(assigned,v,dims);              -->
 *  <!--  int assigned;                                                -->
 *  <!--  double v;                                                    -->
 *  <!--  const dim_type *dims;                                        -->
 *  Make a real child with default v, and dimensions dims.  Those
 *  values can be garbage if assigned is FALSE.
 */

extern struct ChildDesc MakeIntegerDesc(int assigned, long i);
/**<
 *  <!--  struct ChildDesc MakeIntegerDesc(assigned,i)                 -->
 *  <!--  int assigned;                                                -->
 *  <!--  long i;                                                      -->
 *  Make an integer child with default value i.  The value i is ignored if
 *  assigned is false.
 */

extern struct ChildDesc MakeBooleanDesc(int assigned, int b);
/**<
 *  <!--  struct ChildDesc MakeBooleanDesc(assigned,b)                 -->
 *  <!--  int assigned,b;                                              -->
 *  Make a boolean child with default value b.  The value b is ignored if
 *  assigned is false.
 */

extern struct ChildDesc MakeSetDesc(int assigned, int intset, struct set_t *s);
/**<
 *  <!--  struct ChildDesc MakeSetDesc(assigned,intset,s)              -->
 *  <!--  int assigned;                                                -->
 *  <!--  int intset;                                                  -->
 *  <!--  struct gl_list_t *s;                                         -->
 *  Make a set child with default value s.  The value of s is ignored if
 *  assigned is false.
 */

extern struct ChildDesc MakeSymbolDesc(int assigned, symchar *str);
/**<
 *  <!--  struct ChildDesc MakeSymbolDesc(assigned,str)                -->
 *  <!--  int assigned;                                                -->
 *  <!--  char *str;                                                   -->
 *  Make a symbol child description with default value str.  The value of
 *  str is ignored if assigned is false.
 */

/* @} */

#endif  /* ASC_CHILDINFO_H */


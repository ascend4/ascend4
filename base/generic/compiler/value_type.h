/*
 *  Value Type Definitions
 *  by Tom Epperly
 *  Created: 1/16/90
 *  Version: $Revision: 1.17 $
 *  Version control file: $RCSfile: value_type.h,v $
 *  Date last modified: $Date: 1998/02/05 16:38:45 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Weidner Epperly
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
 *  Value Type Definitions.
 *
 *  Note:
 *  This file is a mess as we are passing around 24 byte structures rather than
 *  pointers to structures. ewww! Anytime you are returned a struct value_t
 *  from any of the functions in this file, remember to call DestroyValue
 *  when you are done with it since values may carry pointers to memory
 *  which this module owns and we don't want to leak memory.<br><br>
 *
 *  3/96 Ben Allan: Added memory manager for struct value_t (internal).
 *  Use the Init, Destroy, and Report functions as required.
 *  Note to implementors: all the objects in a list_value list must be
 *  really allocated values and not just a piece of stack space.<br><br>
 *
 *  Added initialization for automatic variables conditional on
 *  NDEBUG flag. If it is not defined, this module is slower. If it
 *  IS defined, the module returns a UNION with parts uninitialized
 *  which really annoys memory auditors like purify.
 *  <pre>
 *  When #including value_type.h, make sure these files are #included first:
 *         #include <stdio.h>
 *         #include "utilities/ascConfig.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "types.h"
 *         #include "list.h"
 *         #include "exprs.h"
 *         #include "functype.h"
 *         #include "setinstval.h"
 *  </pre>
 */

#ifndef __VALUE_TYPE_H_SEEN__
#define __VALUE_TYPE_H_SEEN__

/** Evaluation error types. */
enum evaluation_error{
  type_conflict,
  name_unfound,       /**< inst of name not made yet */
  incorrect_name,     /**< name can never be found */
  temporary_variable_reused,
  undefined_value,    /**< inst exists without being assigned */
  dimension_conflict, /**< arithmetic error in dimensionality */
  incorrect_such_that,
  empty_choice,       /**< CHOICE() on an empty set */
  empty_intersection,
  illegal_set_use     /**< set used in list context */
};

/** Unique types of values. */
enum value_kind {
  real_value,
  integer_value,
  symbol_value,
  boolean_value,
  list_value,       /**< keep set and list together */
  set_value,        /**< set is unique and sorted, or ought to be */
  error_value
};

/** Real value type data structure. */
struct real_value_t{
  double value;           /**< real value */
  CONST dim_type *dimp;   /**< dimension info */
};

/** Union of values for all possible value types. */
union value_union{
  struct real_value_t r;      /**< real value */
  long i;                     /**< integer value */
  int b;                      /**< boolean value */
  symchar *sym_ptr;           /**< symbol value */
  struct set_t *sptr;         /**< set structure */
  struct gl_list_t *lvalues;  /**< list of values */
  enum evaluation_error t;    /**< type of evaluation error */
};

/**
 * Value type data structure.
 *
 * v.constant is true if the data from which the value is derived is
 * impossible to change. Useful in some applications.
 * At present it should only be examined for values of type
 * real, boolean, integer, symbol. always true for sets at present.
 * It really could have 31 other flags with 0x1 defined for the
 * constant bitfield.<br><br>
 *
 * Be sure that within value_t the union is aligned on an 8 byte boundary
 * or alignment errors will occur. As of 3/96 it does on all
 * CMU architectures (sparc, hp, alpha).
 *
 * @todo Someone really should redo this struct value_t so that it doesn't
 *       waste so much space. It takes 24 bytes instead of the 16 that it
 *       should. BAA.
 */
struct value_t {
  enum value_kind t;      /**< The type of this value. */
  unsigned int constant;  /**< since the union aligns on the double, this free */
  union value_union u;    /**< The actual value. */
};

extern void InitValueManager(void);
/**<
 *  <!--  InitValueManager();                                          -->
 *  Sets up value memory management. This must be called once
 *  before any value_t can be built, ideally at startup time.
 *  Do not call it again unless DestroyValueManager is called first
 *  and all outstanding value_t have been destroyed.
 *  If insufficient memory to compile anything at all, does exit(2).
 */

extern void DestroyValueManager(void);
/**<
 *  <!--  DestroyValueManager();                                       -->
 *  Destroy value memory management. This must be called to
 *  clean up before shutting down ASCEND.
 *  Do attempt to evaluate anything after you call this unless you
 *  have recalled InitValueManager().
 */

extern void ReportValueManager(FILE *f);
/**<
 *  <!--  ReportValueManager(f);                                       -->
 *  <!--  FILE *f;                                                     -->
 *  Reports on the value manager to f.
 */

#ifdef NDEBUG
#define IVAL(x)
#else
#define IVAL(x) ValInit(&(x))
#endif
/**<
 * Initialize a struct value_t variable (NOT a pointer to same).
 * Does nothing if NDEBUG is defined.
 * If you want to init a pointer to a value_t, use IVALPTR().<br><br>
 *
 * IVAL(stackvar) (or IVARPTR(&stackvar)) should be called on
 * locally allocated value_t before any other action using them
 * is taken. When NDEBUG is not defined, it causes the stack memory
 * to be initialized to 0. Normally it is a do nothing macro.
 * Proper initialization helps us separate signal from noise in
 * gdb and purify.
 * @param x The struct value_t to initialize.
 * @return No return value.
 * @see ValInit()
 */
#ifdef NDEBUG
#define IVALPTR(y)
#else
#define IVALPTR(y) ValInit(y)
#endif
/**<
 * Initialize a struct value_t variable via a pointer.
 * Does nothing if NDEBUG is defined.
 * If you want to init a struct value_t (i.e. not a pointer to
 * same), use IVAL().
 * @param y Pointer to the struct value_t to initialize.
 * @return No return value.
 * @see ValInit()
 */
extern void ValInit(struct value_t *v);
/**<
 *  <!--  ValInit(v)                                                   -->
 *  <!--  Inits the contents of v to 0.                                -->
 *  <!--  Do not call this function -- use the IVAL macros             -->
 *  Implementation function for IVAL() and IVALPTR().
 *  Do not call this function directly - use IVAL() or IVALPTR() instead.
 */

#define ValueKind(v) ((v).t)
/**<
 *  <!--  macro ValueKind(v)                                           -->
 *  <!--  struct value_t v;                                            -->
 *  Return the value of a value_t.
 */

#define IntegerValue(v) ((v).u.i)
/**<
 *  <!--  macro IntegerValue(v)                                        -->
 *  <!--  struct value_t v;                                            -->
 *  Return the value of an integer or integer_constant value_t.
 */

#define RealValue(v) ((v).u.r.value)
/**<
 *  <!--  macro RealValue(v)                                           -->
 *  <!--  struct value_t v;                                            -->
 *  Return the real value of a real or real_constant value_t.
 */

#define BooleanValue(v) ((v).u.b)
/**<
 *  <!--  macro BooleanValue(v)                                        -->
 *  <!--  struct value_t v;                                            -->
 *  Return the boolean value of a boolean or boolean_constant value_t.
 */

#define RealValueDimensions(v) ((v).u.r.dimp)
/**<
 *  <!--  macro RealValueDimensions(v)                                 -->
 *  <!--  struct value_t v;                                            -->
 *  Return the dimensions of the real or real_constant value_t.
 */

#define SetValue(v) ((v).u.sptr)
/**<
 *  <!--  macro SetValue(v)                                            -->
 *  <!--  struct value_t v;                                            -->
 *  Return the set value of a set value_t.
 */

#define SymbolValue(v) ((v).u.sym_ptr)
/**<
 *  <!--  macro SymbolValue(v)                                         -->
 *  <!--  struct value_t v;                                            -->
 *  Return the symbol value of a symbol or symbol_constant value_t.
 *  This will be a symchar *.
 */

#define ErrorValue(v) ((v).u.t)
/**<
 *  <!--  macro ErrorValue(v)                                          -->
 *  <!--  struct value_t v;                                            -->
 *  Return the error type.
 */

extern struct value_t CopyValue(struct value_t value);
/**<
 *  <!--  struct value_t CopyValue(value)                              -->
 *  <!--  struct value_t value;                                        -->
 *  Return a copy of the value.
 */

extern struct value_t CreateRealValue(double value,CONST dim_type *dim, unsigned constant);
/**<
 *  <!--  struct value_t CreateRealValue(value,dim,constant)           -->
 *  <!--  double value;                                                -->
 *  <!--  const dim_type *dim;                                         -->
 *  Create a real value node from the given value and dimensions.
 *  Value created is created marked as variable if constant is 0
 *  and constant if constant is 1.
 */

extern struct value_t CreateIntegerValue(long value, unsigned constant);
/**<
 *  <!--  struct value_t CreateIntegerValue(value,constant)            -->
 *  <!--  long value;                                                  -->
 *  Create an integer value.
 *  Value created is created marked as variable if constant is 0
 *  and constant if constant is 1.
 */

extern struct value_t CreateSymbolValue(symchar *sym_ptr, unsigned constant);
/**<
 *  <!--  struct value_t CreateSymbolValue(sym_ptr,constant)           -->
 *  <!--  symchar *sym_ptr;                                            -->
 *  Create a symbol value.
 *  Value created is created marked as variable if constant is 0
 *  and constant if constant is 1.
 */

extern struct value_t CreateBooleanValue(int truth, unsigned constant);
/**<
 *  <!--  struct value_t CreateBooleanValue(truth,constant)            -->
 *  <!--  int truth;                                                   -->
 *  Create a boolean value.
 *  Value created is created marked as variable if constant is 0
 *  and constant if constant is 1.
 */

extern struct value_t CreateSetValue(struct set_t *sptr);
/**<
 *  <!--  struct value_t CreateSetValue(sptr)                          -->
 *  <!--  struct set_t *sptr;                                          -->
 *  Create a set value.
 *  Value created is created marked as constant. Mark it as variable
 *  if you need to.
 *  @bug BUG BUG BUG. When we have variable sets, this needs to be cleaned up.
 */

extern struct value_t CreateSetFromList(struct value_t value);
/**<
 *  <!--  struct value_t CreateSetFromList(value)                      -->
 *  <!--  struct value_t value;                                        -->
 *  Create a set from a list of values. Does not damage the list value given.
 *  Value created is created marked as constant. Mark it as variable
 *  if you need to.
 *  The values given may be int, str, or set of int/str, but must be of
 *  all the same type.
 *  @bug BUG BUG BUG. When we have variable sets, this needs to be cleaned up.
 */

extern struct value_t CreateOrderedSetFromList(struct value_t value);
/**<
 *  <!--  struct value_t CreateOrderedSetFromList(value)               -->
 *  <!--  struct value_t value;                                        -->
 *  Create a set from a list of values. The set that will be created will
 *  NOT have unique elements, nor will the elments be sorted. In this way
 *  the set that is created behaves more like a list. Useful for processing
 *  arguments to multivariate functions. Appropriate errors are returned
 *  in the event of an error/inconsistency in the elements.
 *  Value created is created marked as variable. Mark it as constant
 *  if you need to.
 */

extern struct value_t CreateErrorValue(enum evaluation_error t);
/**<
 *  <!--  struct value_t CreateErrorValue(t)                           -->
 *  <!--  enum evaluation_error t;                                     -->
 *  Create an error value.
 *  Value created is created marked as variable. Mark it as constant
 *  if you need to.
 */

extern struct value_t CreateVacantListValue(void);
/**<
 *  <!--  struct value_t CreateEmptyListValue()                        -->
 *  Create a list value with no elements and minimal memory.
 *  Use this when you expect the list to die soon and without expansion.
 *  Value created is created marked as variable. Mark it as constant
 *  if you need to.
 */

extern struct value_t CreateEmptyListValue(void);
/**<
 *  <!--  struct value_t CreateEmptyListValue()                        -->
 *  Create a list value with no elements but some memory.
 *  Value created is created marked as variable. Mark it as constant
 *  if you need to.
 */

extern void AppendToListValue(struct value_t list, struct value_t value);
/**<
 *  <!--  void AppendToListValue(list,value)                           -->
 *  <!--  struct value_t list,value;                                   -->
 *  Add "value" to the list value "list".  This procedure will destory
 *  "value" if it needs to be.
 */

#define IsConstantValue(v) ((v).constant)
/**<
 *  Return 1 if value is marked constant, 0 if not.
 */

#define BothConstantValue(va,vb) ((va).constant && (vb).constant)
/**<
 *  Return 1 if both args marked constant, 0 if not.
 */

#define SetConstantValue(v) ((v).constant = 1)
/**<
 *  Mark value as constant.
 */

#define SetVariableValue(v) ((v).constant = 0)
/**<
 *  Mark value as constant.
 */

extern void DestroyValue(struct value_t *value);
/**<
 *  This function will deallocate the sets and lists of a value.
 *  Note this requires a pointer. This function does NOT free the
 *  pointer sent it. It DOES free all the values contained in the list of
 *  a list_value and calls DestroySet on a set_value. This function is
 *  potentially recursive.
 */

/*
 *  OPERATIONS: None of the operations below will ever deallocate memory.
 */

extern struct value_t AddValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t AddValues(value1,value2);                     -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 + value2.
 *  If both args are constant, result is.
 *  Inputs and return must be real, integer or set.
 *  ifdef CATTEST, inputs may be symbols as well.
 *  Bad input will return error_value.
 */

extern struct value_t SubtractValues(struct value_t value1,struct value_t value2);
/**<
 *  <!--  struct value_t SubtractValues(value1,value2)                 -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 - value2.
 *  If both args are constant, result is.
 *  Inputs and return must be real, integer, boolean, symbol or list.
 *  Bad input will return error_value.
 */

extern struct value_t MultiplyValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t MultiplyValues(value1,value2)                 -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 * value2.
 *  If both args are constant, result is.
 *  Inputs must be real or integer. If either is real, the result
 *  is real. Dimensionality of result will be derived from inputs.
 *  Inputs may also be sets if both value1 and 2 are sets.
 *  Result will then be the INTERSECTION of those sets.
 *  Bad input will return error_value.
 */

extern struct value_t DivideValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t DivideValues(struct value_t,struct value_t)   -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 / value2.
 *  If both args are constant, result is.
 *  Inputs must be real or integer. If either is real, the result
 *  is real. Dimensionality of result will be derived from inputs.
 *  If both input values are integer, integer division will be performed
 *  without promotion to real first and return value will be integer.
 *  Bad input will return error_value.
 */

extern struct value_t PowerValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t PowerValues(struct value_t,struct value_t)    -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 ^ value2.
 *  If both args are constant, result is.
 *  Performs integer and real power functions.
 *  IF value2 is nonintegral real, then value1 must be > 0 and DIMENSIONLESS.
 *  If both arguments are integer, result is.
 */

extern struct value_t CardValues(struct value_t value);
/**<
 *  <!--  struct value_t CardValues(value)                             -->
 *  <!--  struct value_t value;                                        -->
 *  Return the cardinality of the set in value.
 *  Value is marked constant.
 *  Result is an integer.
 */

extern struct value_t ChoiceValues(struct value_t value);
/**<
 *  <!--  struct value_t ChoiceValues(value)                           -->
 *  <!--  struct value_t value;                                        -->
 *  Return an arbitrary but consistent member of the set in value.
 *  That is it always returns the same member from a given set.
 *  Value is marked constant.
 */

#define FIRSTCHOICE 1
/**<
 *  If FIRSTCHOICE = 0 ChoiceValues uses a fancy method to pick the
 *  set member, else it will always return the first (in internal
 *  storage) set member. You can guess what our storage is.
 *  The DEFAULT value of this is 1 because it gives deterministic
 *  behavior that is platform independent across platforms with an
 *  identical collating sequence.
 */

extern struct value_t SumValues(struct value_t value);
/**<
 *  <!--  struct value_t SumValues(value)                              -->
 *  <!--  struct value_t value;                                        -->
 *  Return the summation of the value.
 *  If args are constant, result is.
 *  Sums of reals and integers are promoted to real.
 *  Sums of reals must be dimensionally consistent or an error_value will
 *  be returned.
 */

extern struct value_t ProdValues(struct value_t value);
/**<
 *  <!--  struct value_t ProdValues(value)                             -->
 *  <!--  struct value_t value;                                        -->
 *  Return the product of the value.
 *  If args are constant, result is.
 *
 */

extern struct value_t UnionValues(struct value_t value);
/**<
 *  <!--  struct value_t UnionValues(value)                            -->
 *  <!--  struct value_t value;                                        -->
 *  Return the union of the value.
 *  If args are constant, result is.
 *  Returns the set UNION of the set or list given.
 */

extern struct value_t IntersectionValues(struct value_t value);
/**<
 *  <!--  struct value_t IntersectionValues(value)                     -->
 *  <!--  struct value_t value;                                        -->
 *  Return the intersection of the value.  If value is an empty list,
 *  this returns an error.
 *  If args are constant, result is.
 *  Returns the set INTERSECTION  of the set or list given.
 */

extern struct value_t OrValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t OrValues(value1,value2)                       -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 OR value2. Arguments and result are boolean.
 *  If args are constant, result is.
 */

extern struct value_t AndValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t AndValues(value1,value2)                      -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 AND value2. Arguments and result are boolean.
 *  If args are constant, result is.
 */

extern struct value_t InValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t InValues(value1,value2)                       -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 IN value2.
 *  value1 is a integer or symbol. value2 is a set.
 *  Return is a boolean.
 */

extern struct value_t EqualValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t EqualValues(value1,value2)                    -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 == value2. Result is boolean.
 *  Comparison exact, and values must be of same type except that
 *  integer/real comparisons are promoted as necessary.
 *  If args are constant, result is.
 */

extern struct value_t NotEqualValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t NotEqualValues(value1,value2)                 -->
 *  <!--  sturct value_t value1,value2;                                -->
 *  Return value1 != value2. Result is boolean.
 *  If args are constant, result is.
 *  Comparison exact, and values must be of same type except that
 *  integer/real comparisons are promoted as necessary.
 *  Has problems dealing with sets. This constitutes a bug to be fixed.
 */

extern struct value_t LessValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t LessValues(value1,value2)                     -->
 *  <!--  struct value_t value1,value2                                 -->
 *  Return value1 < value2. Result is boolean.
 *  Values must be real, integer,symbol. Sets are not handled.
 *  Comparison exact, and values must be of same type except that
 *  integer/real comparisons are promoted as necessary.
 *  If args are constant, result is.
 */

extern struct value_t GreaterValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t GreaterValues(value1,value2)                  -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 > value2. Result is boolean.
 *  Values must be real, integer,symbol. Sets are not handled.
 *  Comparison exact, and values must be of same type except that
 *  integer/real comparisons are promoted as necessary.
 *  If args are constant, result is.
 */

extern struct value_t LessEqValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t LessEqValues(value1,value2)                   -->
 *  <!--  struct value_t value1,value2                                 -->
 *  Return value1 <= value2. Result is boolean.
 *  Values must be real, integer,symbol. sets are not handled.
 *  Comparison exact, and values must be of same type except that
 *  integer/real comparisons are promoted as necessary.
 *  If args are constant, result is.
 */

extern struct value_t GreaterEqValues(struct value_t value1, struct value_t value2);
/**<
 *  <!--  struct value_t GreaterEqValues(value1,value2)                -->
 *  <!--  struct value_t value1,value2;                                -->
 *  Return value1 >= value2. Result is boolean.
 *  Values must be real, integer,symbol. sets are not handled.
 *  Comparison exact, and values must be of same type except that
 *  integer/real comparisons are promoted as necessary.
 *  If args are constant, result is.
 */

extern struct value_t ApplyFunction(struct value_t value, CONST struct Func *f);
/**<
 *  <!--  struct value_t ApplyFunction(value,f)                        -->
 *  <!--  struct value_t value;                                        -->
 *  <!--  const struct Func *f;                                        -->
 *  Apply the function f to value.  Note all function evaluations require
 *  appropriately dimensioned (or wild) arguments.
 *  If args are constant, result is.
 */

extern struct value_t NegateValue(struct value_t value);
/**<
 *  <!--  struct value_t NegateValue(value)                            -->
 *  <!--  struct value_t value;                                        -->
 *  Return - value.
 *  If args are constant, result is.
 */

extern struct value_t NotValue(struct value_t value);
/**<
 *  <!--  struct value_t NotValue(value)                               -->
 *  <!--  struct value_t value;                                        -->
 *  Return NOT value; Value and result are boolean.
 *  If args are constant, result is.
 */

#endif /* __VALUE_TYPE_H_SEEN__ */


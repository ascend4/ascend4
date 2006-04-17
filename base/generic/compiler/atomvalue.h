/*
 *  Ascend Instance Atom Value Functions
 *  by Tom Epperly & Ben Allan
 *  8/16/89
 *  Version: $Revision: 1.10 $
 *  Version control file: $RCSfile: atomvalue.h,v $
 *  Date last modified: $Date: 1998/02/05 16:35:24 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1996 Ben Allan
 *  based on instance.c
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
 */

/** @file
 *  Atom value querying and assignment routines.
 *
 *  These routines provide access to atom values and related attributes.
 *  All of the following routines may only be called on atomic instances.
 *  <pre>
 *  When #including atomvalue.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "fractions.h"
 *         #include "compiler.h"
 *         #include "dimen.h"
 *         #include "sets.h"
 *  </pre>
 */

#ifndef __ATOMVALUE_H_SEEN__
#define __ATOMVALUE_H_SEEN__

extern unsigned ASC_DLLSPEC AtomAssigned(CONST struct Instance *i);
/**<
 *  <!--  unsigned AtomAssigned(i)                                     -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return a nonzero value if the atom instance has been assigned; otherwise,
 *  return false.  It will actually return the number of times that the
 *  atom has been assigned.
 *  Valid for *_INST, *_ATOM_INSTANCE, *_CONSTANT_INSTANCE.
 */

extern unsigned AtomMutable(CONST struct Instance *i);
/**<
 *  <!--  unsigned AtomMutable(i);                                     -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return a true value if the atom instance is mutable; otherwise, return
 *  false.
 *  Valid for *_INST, *_ATOM_INSTANCE, *_CONSTANT_INSTANCE.
 *  Often rather a redundant operator, since *_INST and *_ATOM_INSTANCE are
 *  permanently mutable.
 */

extern unsigned DepthAssigned(CONST struct Instance *i);
/**<
 *  <!--  unsigned DepthAssigned(i)                                    -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return the depth of the assignment.  This only works on real and
 *  boolean instances.
 */

/*
 *  The following Get/Set*AtomValue functions apply to all the scalar
 *  kinds integer, symbol, boolean, and real. This includes
 *  fundamentals (atom children), atoms, and constants. We would take
 *  the word Atom out of the function names except that, e.g.,
 *  RealValue is a name used in value_type.h.
 */

extern double ASC_DLLSPEC RealAtomValue(CONST struct Instance *i);
/**<
 *  <!--  double RealAtomValue(i)                                      -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return the real value of real instance i.  This works only on
 *  REAL_INST or REAL_ATOM_INST or REAL_CONSTANT_INSTANCE.
 */

extern void ASC_DLLSPEC SetRealAtomValue(struct Instance *i, double d, unsigned depth);
/**<
 *  <!--  void SetRealAtomValue(i,d,depth)                             -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  double d;                                                    -->
 *  <!--  unsigned depth;                                              -->
 *  Assign the value d(in system units) to instance i.  This works only on
 *  REAL_INST or REAL_ATOM_INST or REAL_CONSTANT_INSTANCEs not assigned.
 *  This will set the assigned attribute also.
 *  This will set the depth too. <br><br>
 *
 *  It is recommended that the interface use depth = 0 which will take
 *  precidence over every other assignment.
 */

extern CONST dim_type* ASC_DLLSPEC RealAtomDims(CONST struct Instance *i);
/**<
 *  <!--  const dim_type *RealAtomDims(i)                              -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return the dimensions attribute of instance i.  This works only on
 *  REAL_INST or REAL_ATOM_INST or REAL_CONSTANT_INSTANCE.
 *  This should never return a NULL pointer.
 */

extern void ASC_DLLSPEC SetRealAtomDims(struct Instance *i, CONST dim_type *dim);
/**<
 *  <!--  void SetRealAtomDims(i,dim)                                  -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  const dim_type *dim;                                         -->
 *  Set i's dimensions to dim.
 *  dim should already be in the dimensions list.
 *  This works only on
 *  REAL_INST or REAL_ATOM_INST or REAL_CONSTANT_INSTANCEs not dimensioned.
 */

extern long ASC_DLLSPEC GetIntegerAtomValue(CONST struct Instance *i);
/**<
 *  <!--  long GetIntegerAtomValue(i)                                  -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return the value of the integer instance i.  This only works on
 *  INTEGER_INST or INTEGER_ATOM_INST or INTEGER_CONSTANT_INST type instances.
 */

extern void SetIntegerAtomValue(struct Instance *i, long v, unsigned depth);
/**<
 *  <!--  void SetIntegerAtomValue(i,v,depth)                          -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  long v;                                                      -->
 *  <!--  unsigned depth;                                              -->
 *  Set the value of instance i to v.  This only works on INTEGER_INST or
 *  INTEGER_ATOM_INST or unassigned INTEGER_CONSTANT_INST type instances.
 *
 *  This also sets the assigned as needed.
 *  Calling on an improper integer will cause a crash.
 */

extern int ASC_DLLSPEC GetBooleanAtomValue(CONST struct Instance *i);
/**<
 *  <!--  int GetBooleanAtomValue(i)                                   -->
 *  <!--  struct Instance *i;                                          -->
 *  Return a true value if i is TRUE; otherwise return a false value.
 *  This only works on BOOLEAN_INST or BOOLEAN_ATOM_INST
 *  or BOOLEAN_CONSTANT_INST type instances.
 */

extern void ASC_DLLSPEC SetBooleanAtomValue(struct Instance *i, int truth, unsigned depth);
/**<
 *  <!--  void SetBooleanAtomValue(i,truth,depth)                      -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  int truth;                                                   -->
 *  <!--  unsigned depth;                                              -->
 *  If truth is a true value, i will be set to TRUE; otherwise, i will be
 *  set to FALSE.  This only works on BOOLEAN_INST or BOOLEAN_ATOM_INST
 *  or unassigned BOOLEAN_CONSTANT_INST type instances.
 *  This will set the assigned attribute of i.<br><br>
 *
 *  It is recommended that the interface use depth = 0 because it will
 *  override every other assignment.
 */

extern CONST struct set_t* ASC_DLLSPEC SetAtomList(CONST struct Instance *i);
/**<
 *  <!--  struct set_t *SetAtomList(i)                                 -->
 *  <!--  struct Instance *i;                                          -->
 *  Return the list from the set instance i.  This only works on SET_INST
 *  and SET_ATOM_INST type instances.  This returns NULL when i has not
 *  been assigned.
 */

extern int AssignSetAtomList(struct Instance *i, struct set_t *list);
/**<
 *  <!--  void AssignSetAtomList(i,list)                               -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  struct set_t *list;                                          -->
 *  Set i's list to list.  This will set the assigned attribute unless
 *  list is NULL.  A set's value cannot be changed after it has been assigned.
 *  Returns 1 if successful. Returns 0 if not successful.
 *  If the return is 0, the user is responsible for list since it did not
 *  become part of i.
 */

extern int GetSetAtomKind(CONST struct Instance *i);
/**<
 *  <!--  int GetSetAtomKind(i);                                       -->
 *  <!--  const struct Instance *i;                                    -->
 *  Return the kind of the set. 1 ==> integer set; 0 ==> string set.
 */

extern symchar* ASC_DLLSPEC GetSymbolAtomValue(CONST struct Instance *i);
/**<
 *  <!--  symchar *GetSymbolAtomValue(i)                               -->
 *  <!--  const struct Instance *i;                                    -->
 *  This returns the symbol that instance i is assigned to.
 *  If i is unassigned, it will return NULL.  This only works on
 *  SYMBOL_INST and SYMBOL_ATOM_INST or SYMBOL_CONSTANT_INST type instances.
 *  The character string does not contain the single quote delimeters.
 *  The symchar can be converted to a string with macro SCP from compiler.h.
 */

extern void SetSymbolAtomValue(struct Instance *i, symchar *s);
/**<
 *  <!--  void SetSymbolAtomValue(i,s)                                 -->
 *  <!--  struct Instance *i;                                          -->
 *  <!--  symchar *s;                                                  -->
 *  This makes an assignment to i. Instance i should be of type
 *  SYMBOL_INST or SYMBOL_ATOM_INST or unassigned SYMBOL_CONSTANT_INST.
 *  Character strings should not contain the single quote delimiters.
 *  Buggy at present, since symbolatominst are supposed to be mutable.
 */

extern int CmpAtomValues(CONST struct Instance *i1, CONST struct Instance *i2);
/**<
 *  <!--  int CmpAtomValues(i1,i2)                                     -->
 *  <!--  struct Instance *i1,*i2;                                     -->
 *  Return -1,0,1 as i2 < == or > i1. i1 and i2 should be of the same base
 *  type. The following types are allowed:
 *  REAL_*INST, BOOLEAN_*INST, SYMBOL_*INST, INTEGER_*INST SET_*INST.
 *  Calls with other types will not return, nor will calls with mismatched
 *  types.
 */

#endif  /* __ATOMVALUE_H_SEEN__ */


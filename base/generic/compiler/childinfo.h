/*
 *  Atom Child Description List
 *  by Tom Epperly
 *  Part of Ascend
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

#ifndef __CHILDINFO_H_SEEN__
#define __CHILDINFO_H_SEEN__
/* requires
 *# #include"compiler.h"
 *# #include"list.h"
 *# #include"dimen.h"
 *# #include"setinstval.h"
 */

struct ChildDescR {
  double value;
  CONST dim_type *dims;
};

struct ChildDescI {
  long value;		/* always mutable */
};

struct ChildDescS{
  struct set_t *slist;		/* set list */
  unsigned is_int;		/* integer 1 or symbol 0 set */
};

union ChildDescUnion {
  int bvalue;			/* boolean value */
  struct ChildDescS s;		/* set information */
  struct ChildDescI ivalue;	/* integer value */
  struct ChildDescR rvalue;	/* real value */
  symchar *svalue;		/* symbol value */
};

enum ChildDescT {
  bad_child,
  /*
   * this should never, ever, ever be seen except in childless atoms who need
   * a place holder. bad_child-ren are never assigned and the UNION meaningless
   */
  real_child,
  integer_child,
  boolean_child,
  set_child,
  symbol_child
};

struct ChildDesc {
  enum ChildDescT t;
  unsigned assigned; /* 0,1 */
  union ChildDescUnion u;
};

/*
 * In the macro ICDESC, x must be a struct ChildDesc variable,
 * NOT a pointer to same.
 * If you want to init a pointer contents, y, use ICDESCPTR.
 */
#ifdef NDEBUG
/* do nothings */
#define ICDESC(x)
#define ICDESCPTR(y)
#else
/* init to 0 */
#define ICDESC(x) CDescInit(&(x))
#define ICDESCPTR(y) CDescInit(y)
#endif
extern void CDescInit(struct ChildDesc *);
/*
 *  CDescInit(c)
 *  Inits the contents of c to 0.
 *  Do not call this function -- use the ICDESC macros
 */

extern struct ChildDesc *CreateChildDescArray(unsigned long);
/*
 *  struct ChildDesc *CreateChildDescArray(l)
 *  unsigned long l;
 *  Allocate space for an array of length l.  This will not initialize
 *  the array in any special way.  The array should be access through
 *  the routines provided below, and then dispose of when it is uneeded.
 */

extern struct ChildDesc *CreateEmptyChildDescArray(void);
/*
 *  struct ChildDesc *CreateEmptyChildDescArray();
 *  Allocate space for an array of length 1.
 *  This will be initialized to contain a bad_child.
 *  The array should be access through
 *  the routines provided below, and then dispose of when it is uneeded.
 */


extern void DestroyChildDescArray(struct ChildDesc *,
      unsigned long);
/*
 *  void DestroyChildDescArray(c,l)
 *  struct ChildDesc *c;
 *  unsigned long l;
 *  This routine will deallocate the space for the array c and any allocated
 *  parts inside of it(eg. the sets).
 */

#ifdef NDEBUG
#define AssignChildArrayElement(a,n,e) (a)[(n)-1] = (e)
#else
#define AssignChildArrayElement(a,n,e) AssignChildArrayElementF((a),(n),(e))
#endif
extern void AssignChildArrayElementF(struct ChildDesc *,
         unsigned long,
         struct ChildDesc);
/*
 *  macro AssignChildArrayElement(a,n,e)
 *  void AssignChildArrayElementF(a,n,e)
 *  struct ChildDesc *a;
 *  unsigned long n;
 *  struct ChildDesc e;
 *
 *  Set the n'th child's description to e.  n should range from 1 to the
 *  length of "a".  n should never be zero.
 */

#ifdef NDEBUG
#define GetChildArrayElement(a,n) ((a)[n-1])
#else
#define GetChildArrayElement(a,n) GetChildArrayElementF((a),(n))
#endif
extern struct ChildDesc GetChildArrayElementF(CONST struct ChildDesc *,
           unsigned long);
/*
 *  macro GetChildArrayElement(a,n)
 *  struct ChildDesc GetChildArrayElementF(a,n)
 *  const struct ChildDesc *a;
 *  unsigned long n;
 *  Return the n'th child description from "a".  n ranges from 1 to the
 *  length of "a".
 */

#ifdef NDEBUG
#define ChildDescType(e) ((e).t)
#else
#define ChildDescType(e) ChildDescTypeF(e)
#endif
extern enum ChildDescT ChildDescTypeF(struct ChildDesc);
/*
 *  macro ChildDescType(e)
 *  enum ChildDescT ChildDescF(e)
 *  struct ChildDesc e;
 *  Return the type of e.
 */

#ifdef NDEBUG
#define ValueAssigned(e) ((e).assigned)
#else
#define ValueAssigned(e) ValueAssignedF(e)
#endif
extern int ValueAssignedF(struct ChildDesc);
/*
 *  macro ValueAssigned(e)
 *  int ValueAssignedF(e)
 *  struct ChildDesc e;
 *  Return a true value if e the child has been assigned a value.  Otherwise,
 *  returns a false value.
 */

#ifdef NDEBUG
#define IntegerDefault(e) ((e).u.ivalue.value)
#else
#define IntegerDefault(e) IntegerDefaultF(e)
#endif
extern long IntegerDefaultF(struct ChildDesc);
/*
 *  macro IntegerDefault(e)
 *  long IntegerDefaultF(e)
 *  struct ChildDesc e;
 *  Return the integer default value assuming that e is an integer_child.
 *  This value is meaningless if ValueAssigned(e) is not true.
 */

#ifdef NDEBUG
#define BooleanDefault(e) ((e).u.bvalue)
#else
#define BooleanDefault(e) BooleanDefaultF(e)
#endif
extern int BooleanDefaultF(struct ChildDesc);
/*
 *  macro BooleanDefault(e)
 *  int BooleanDefaultF(e)
 *  struct ChildDesc e;
 *  Return the boolean default value assuming that e is a boolean_child.
 *  This value is meaningless if ValueAssigned(e) is not true.
 */

#ifdef NDEBUG
#define SetDefault(e) ((e).u.s.slist)
#else
#define SetDefault(e) SetDefaultF(e)
#endif
extern CONST struct set_t *SetDefaultF(struct ChildDesc);
/*
 *  macro SetDefault(e)
 *  const struct set_t *SetDefaultF(e)
 *  struct ChildDesc e;
 *  Return the set default value assuming that e is a set_child.
 *  This value is meaningless if ValueAssigned(e) is not true.
 */

#ifdef NDEBUG
#define SetIsIntegerSet(e) ((e).u.s.is_int)
#else
#define SetIsIntegerSet(e) SetIsIntegerSetF(e)
#endif
extern int SetIsIntegerSetF(struct ChildDesc);
/*
 *  macro SetIsIntegerSet(e)
 *  int SetIsIntegerSetF(e)
 *  struct ChildDesc e;
 *  Return the set type.  0 symbol and 1 integer.
 */

#ifdef NDEBUG
#define SymbolDefault(e) ((e).u.svalue)
#else
#define SymbolDefault(e) SymbolDefaultF(e)
#endif
extern symchar *SymbolDefaultF(struct ChildDesc);
/*
 *  macro SymbolDefault(e)
 *  symchar *SymbolDefaultF(e)
 *  struct ChildDesc e;
 *  Return the symbol default value assuming that e is a symbol_child.
 *  This value is meaningless is ValueAssigned(e) is not true.
 */

#ifdef NDEBUG
#define RealDefaultValue(e) ((e).u.rvalue.value)
#else
#define RealDefaultValue(e) RealDefaultValueF(e)
#endif
extern double RealDefaultValueF(struct ChildDesc);
/*
 *  macro RealDefaultValue(e)
 *  struct ChildDesc e;
 *  Return the real default value assuming that e is a real_child.
 *  This value is meaningless if ValueAssigned(e) is not true.
 */

#ifdef NDEBUG
#define RealDimensions(e) ((e).u.rvalue.dims)
#else
#define RealDimensions(e) RealDimensionsF(e)
#endif
extern CONST dim_type *RealDimensionsF(struct ChildDesc);
/*
 *  macro RealDimensions(e)
 *  struct ChildDesc e;
 *  Return the units pointer assuming that e is a real_child.
 *  This value is meaningless if ValueAssigned(e) is not true.
 */

extern struct ChildDesc MakeRealDesc(int,double, CONST dim_type *);
/*
 *  struct ChildDesc MakeRealDesc(assigned,v,dims);
 *  int assigned;
 *  double v;
 *  const dim_type *dims;
 *  Make a real child with default v, and dimensions dims.  Those
 *  values can be garbage if assigned is FALSE.
 */

extern struct ChildDesc MakeIntegerDesc(int,long);
/*
 *  struct ChildDesc MakeIntegerDesc(assigned,i)
 *  int assigned;
 *  long i;
 *  Make an integer child with default value i.  The value i is ignored if
 *  assigned is false.
 */

extern struct ChildDesc MakeBooleanDesc(int,int);
/*
 *  struct ChildDesc MakeBooleanDesc(assigned,b)
 *  int assigned,b;
 *  Make a boolean child with default value b.  The value b is ignored if
 *  assigned is false.
 */

extern struct ChildDesc MakeSetDesc(int,int,struct set_t *);
/*
 *  struct ChildDesc MakeSetDesc(assigned,intset,s)
 *  int assigned;
 *  int intset;
 *  struct gl_list_t *s;
 *  Make a set child with default value s.  The value of s is ignored if
 *  assigned is false.
 */

extern struct ChildDesc MakeSymbolDesc(int,symchar *);
/*
 *  struct ChildDesc MakeSymbolDesc(assigned,str)
 *  int assigned;
 *  char *str;
 *  Make a symbol child description with default value str.  The value of
 *  str is ignored if assigned is false.
 */
#endif /* __CHILDINFO_H_SEEN__ */

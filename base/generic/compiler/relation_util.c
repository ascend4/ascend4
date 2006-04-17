/*
 *  Relation utility functions for Ascend
 *  Version: $Revision: 1.44 $
 *  Version control file: $RCSfile: relation_util.c,v $
 *  Date last modified: $Date: 1998/04/23 23:51:09 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Interpreter.
 *
 *  Copyright (C) 1990 Thomas Guthrie Epperly, Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph James Zaher
 *  Copyright (C) 1993, 1994 Benjamin Andrew Allan, Joseph James Zaher
 *  Copyright (C) 1997 Carnegie Mellon University
 *
 *  The Ascend Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  ASCEND is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 *  This module defines the dimensionality checking and some other
 *  relation auxillaries for Ascend.
 *
 */
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/list.h>
#include <general/dstring.h>
#include "compiler.h"
#include "symtab.h"
#include "functype.h"
#include "safe.h"
#include "fractions.h"
#include "dimen.h"
#include "types.h"
#include "vlist.h"
#include "dimen_io.h"
#include "instance_enum.h"
#include "bintoken.h"
#include "find.h"
#include "atomvalue.h"
#include "instance_name.h"
#include "relation_type.h"
#include "relation.h"
#include "relation_util.h"
#include "instance_io.h"
#include "instquery.h"
#include "visitinst.h"
#include "mathinst.h"
#include "extfunc.h"
#include "rootfind.h"
#include "func.h"


int g_check_dimensions_noisy = 1;
#define GCDN g_check_dimensions_noisy

#define START 10000  /* largest power of 10 held by a short */
static struct fraction real_to_frac(double real)
{
   short  num, den;
   for( den=START; den>1 && fabs(real)*den>SHRT_MAX; den /= 10 ) ;
   num = (short)(fabs(real)*den + 0.5);
   if( real < 0.0 ) num = -num;
   return( CreateFraction(num,den) );
}
#undef START


/*  Create a static buffer to use for temporary memory storage  */

char *tmpalloc(int nbytes)
/**
 ***  Temporarily allocates a given number of bytes.  The memory need
 ***  not be freed, but the next call to this function will reuse the
 ***  previous allocation. Memory returned will NOT be zeroed.
 ***  Calling with nbytes==0 will free any memory allocated.
 **/
{
  static char *ptr = NULL;
  static int cap = 0;

  if( nbytes > 0 ) {
    if( nbytes > cap ) {
      if( ptr ) ascfree(ptr);
      ptr = (char *)ascmalloc(nbytes);
      cap = nbytes;
    }
  }
  else {
    if( ptr ) ascfree(ptr);
    ptr = NULL;
    cap = 0;
  }
  return ptr;
}



/* it is questionable whether this should be unified with the
   ArgsForToken function in relation.c */
int ArgsForRealToken(enum Expr_enum type)
{
   switch(type) {
   case e_zero:
   case e_int:
   case e_real:
   case e_var:
     return(0);

   case e_func:
   case e_uminus:
      return(1);

   case e_plus:
   case e_minus:
   case e_times:
   case e_divide:
   case e_power:
   case e_ipower:
      return(2);

   default:
      FPRINTF(ASCERR,"ArgsForRealToken: Unknown relation term type.\n");
      return(0);
   }
}

struct dimnode {
   dim_type d;
   enum Expr_enum type;
   short int_const;
   double real_const;
   struct fraction power;
};

static int IsZero(struct dimnode *node)
{
  if( node->type==e_zero || (node->type==e_real && node->real_const==0.0) )
    return TRUE;
  return FALSE;
}

/* what this does needs to be documented here */
static void apply_term_dimensions(struct relation *rel,
                                  struct relation_term *rt,
                                  struct dimnode *first,
                                  struct dimnode *second,
                                  int *con,
                                  int *wild)
{
   enum Expr_enum type;

   switch(type=RelationTermType(rt)) {
      case e_zero:
         CopyDimensions(WildDimension(),&(first->d));
         first->real_const = 0.0;
         first->type = type;
         break;

      case e_int:
         CopyDimensions(Dimensionless(),&(first->d));
         first->int_const = (short)TermInteger(rt);
         first->type = type;
         break;

      case e_real:
         CopyDimensions(TermDimensions(rt),&(first->d));
         first->real_const = TermReal(rt);
         first->type = type;
         break;

      case e_var: {
         struct Instance *var = RelationVariable(rel,TermVarNumber(rt));
         CopyDimensions(RealAtomDims(var),&(first->d));
         first->type = type;
         break;
      }
      case e_func: {
         enum Func_enum id = FuncId(TermFunc(rt));
         switch( id ) {
            case F_ABS:
            case F_HOLD:
               /* no checking or scaling */
               break;

            case F_SQR:
               /* no checking, just simple scaling */
               first->d = ScaleDimensions(&(first->d),CreateFraction(2,1));
               break;

            case F_CUBE:
               /* no checking, just simple scaling */
               first->d = ScaleDimensions(&(first->d),CreateFraction(3,1));
               break;

            case F_SQRT:
               /* no checking, just simple scaling */
               first->d = ScaleDimensions(&(first->d),CreateFraction(1,2));
               break;

            case F_CBRT:
               /* no checking, just simple scaling */
               first->d = ScaleDimensions(&(first->d),CreateFraction(1,3));
               break;

            case F_EXP:
            case F_LN:
            case F_LNM:
            case F_LOG10:
#ifdef HAVE_ERF
            case F_ERF:
#endif /* HAVE_ERF */
            case F_SINH:
            case F_COSH:
            case F_TANH:
            case F_ARCSINH:
            case F_ARCCOSH:
            case F_ARCTANH:
               /**
                ***  first must now be dimensionless.  It will
                ***  end up dimensionless as well.
                **/
               if( IsWild(&(first->d)) && !IsZero(first) ) {
                  if( !*wild ) *wild = TRUE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Relation has wild dimensions\n");
                    FPRINTF(ASCERR,"        in function %s.\n",
                      FuncName(TermFunc(rt)));
                  }
               } else if( !IsWild(&(first->d)) &&
                         CmpDimen(&(first->d),Dimensionless()) ) {
                  if( *con ) *con = FALSE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Function %s called with\n",
                      FuncName(TermFunc(rt)));
                    FPRINTF(ASCERR,"        dimensions ");
                    WriteDimensions(ASCERR,&(first->d));
                    FPRINTF(ASCERR,".\n");
                  }
               }
               CopyDimensions(Dimensionless(),&(first->d));
               break;

            case F_SIN:
            case F_COS:
            case F_TAN: {
               /**
                ***  first must now be of dimension D_PLANE_ANGLE.
                ***  It will then be made dimensionless.
                **/
               if( IsWild(&(first->d)) && !IsZero(first) ) {
                  if( !*wild ) *wild = TRUE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Relation has wild dimensions\n");
                    FPRINTF(ASCERR,"        in function %s.\n",
                      FuncName(TermFunc(rt)) );
                  }
               } else {
                 if( !IsWild(&(first->d)) &&
                         CmpDimen(&(first->d),TrigDimension()) ) {
                  if( *con ) *con = FALSE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Function %s called with\n",
                      FuncName(TermFunc(rt)));
                    FPRINTF(ASCERR,"        dimensions ");
                    WriteDimensions(ASCERR,&(first->d));
                    FPRINTF(ASCERR,".\n");
                  }
                 }
               }
               CopyDimensions(Dimensionless(),&(first->d));
               break;
            }

            case F_ARCSIN:
            case F_ARCCOS:
            case F_ARCTAN:
               /**
                ***  first must now be dimensionless.  It will
                ***  end up with dimension D_PLANE_ANGLE
                **/
               if( IsWild(&(first->d)) && !IsZero(first) ) {
                  if( !*wild ) *wild = TRUE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Relation has wild dimensions\n");
                    FPRINTF(ASCERR,"        in function %s.\n",
                      FuncName(TermFunc(rt)));
                  }
               } else if( !IsWild(&(first->d)) &&
                         CmpDimen(&(first->d),Dimensionless()) ) {
                  if( *con ) *con = FALSE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Function %s called with\n",
                      FuncName(TermFunc(rt)));
                    FPRINTF(ASCERR,"        dimensions ");
                    WriteDimensions(ASCERR,&(first->d));
                    FPRINTF(ASCERR,".\n");
                  }
               }
               CopyDimensions(TrigDimension(),&(first->d));
               break;
         }
         first->type = type;
         break;
      }

      case e_uminus:
         first->type = type;
         break;

      case e_times:
         first->d = AddDimensions(&(first->d),&(second->d));
         first->type = type;
         break;

      case e_divide:
         first->d = SubDimensions(&(first->d),&(second->d));
         first->type = type;
         break;

      case e_power: /* fix me and add ipower */
         if( IsWild(&(second->d)) && !IsZero(second) ) {
            if( !*wild ) *wild = TRUE;
            if (GCDN) {
              FPRINTF(ASCERR,"ERROR:  Relation has wild dimensions\n");
              FPRINTF(ASCERR,"        in exponent.\n");
            }
         } else if( !IsWild(&(second->d)) &&
                   CmpDimen(&(second->d),Dimensionless()) ) {
            if( *con ) *con = FALSE;
            if (GCDN) {
              FPRINTF(ASCERR,"ERROR:  Exponent has dimensions ");
              WriteDimensions(ASCERR,&(second->d));
              FPRINTF(ASCERR,".\n");
            }
         }
         CopyDimensions(Dimensionless(),&(second->d));
         switch( second->type ) {
            case e_int:
               if( !IsWild(&(first->d)) &&
                  CmpDimen(&(first->d),Dimensionless()) ) {
                  struct fraction power;
                  power = CreateFraction(second->int_const,1);
                  first->d = ScaleDimensions(&(first->d),power);
               }
               break;

            case e_real:
               if( !IsWild(&(first->d)) &&
                  CmpDimen(&(first->d),Dimensionless()) ) {
                  struct fraction power;
                  power = real_to_frac(second->real_const);
                  first->d = ScaleDimensions(&(first->d),power);
               }
               break;

            /* what about e_zero? */
            default:
               if( IsWild(&(first->d)) && !IsZero(first) ) {
                  if( !*wild ) *wild = TRUE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR: Relation has wild dimensions\n");
                    FPRINTF(ASCERR,"       raised to a non-constant power.\n");
                  }
               } else if( !IsWild(&(first->d)) &&
                         CmpDimen(&(first->d),Dimensionless()) ) {
                  if( *con ) *con = FALSE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  Dimensions ");
                    WriteDimensions(ASCERR,&(first->d));
                    FPRINTF(ASCERR," are\n");
                    FPRINTF(ASCERR,"       raised to a non-constant power.\n");
                  }
               }
               CopyDimensions(Dimensionless(),&(first->d));
               break;

         }
         first->type = type;
         break;

      case e_plus:
      case e_minus:
         if( IsWild(&(first->d)) && IsZero(first) ) {
            /* first wild zero */
            CopyDimensions(&(second->d),&(first->d));
            first->type = second->type;
            if( second->type==e_int )
               first->int_const = second->int_const;
            if( second->type==e_real )
               first->real_const = second->real_const;
         } else if( IsWild(&(first->d)) && !IsZero(first) ) {
            /* first wild non-zero */
            if( IsWild(&(second->d)) && !IsZero(second) ) {
               /* second wild non-zero */
               if( !*wild ) *wild = TRUE;
               if (GCDN) {
                 FPRINTF(ASCERR,"ERROR:  %s has wild dimensions on\n",
                       type==e_plus ? "Addition":"Subtraction");
                 FPRINTF(ASCERR,"        left and right hand sides.\n");
               }
               first->type = type;
            } else if( !IsWild(&(second->d)) ) {
               /* second not wild */
               if( !*wild ) *wild = TRUE;
               if (GCDN) {
                 FPRINTF(ASCERR,"ERROR:  %s has wild dimensions on\n",
                       type==e_plus ? "Addition":"Subtraction");
                 FPRINTF(ASCERR,"        left hand side.\n");
               }
               CopyDimensions(&(second->d),&(first->d));
               first->type = type;
            }
         } else if( !IsWild(&(first->d)) ) {
            /* first not wild */
            if( IsWild(&(second->d)) && !IsZero(second) ) {
               /* second wild non-zero */
               if( !*wild ) *wild = TRUE;
               if (GCDN) {
                 FPRINTF(ASCERR,"ERROR:  %s has wild dimensions on\n",
                       type==e_plus ? "Addition":"Subtraction");
                 FPRINTF(ASCERR,"        right hand side.\n");
               }
               first->type = type;
            } else if ( !IsWild(&(second->d)) ) {
               /* second not wild */
               if( CmpDimen(&(first->d),&(second->d)) ) {
                  if( *con ) *con = FALSE;
                  if (GCDN) {
                    FPRINTF(ASCERR,"ERROR:  %s has dimensions ",
                      type==e_plus ? "Addition":"Subtraction");
                    WriteDimensions(ASCERR,&(first->d));
                    FPRINTF(ASCERR," on left\n");
                    FPRINTF(ASCERR,"        and dimensions ");
                    WriteDimensions(ASCERR,&(second->d));
                    FPRINTF(ASCERR," on right.\n");
                  }
               }
               first->type = type;
            }
         }
         break;

      default:
         FPRINTF(ASCERR,"ERROR:  Unknown relation term type.\n");
         if( *con ) *con = FALSE;
         first->type = type;
         break;
   }
}

int RelationCheckDimensions(struct relation *rel, dim_type *dimens)
{
  struct dimnode *stack, *sp;
  int consistent = TRUE;
  int wild = FALSE;
  unsigned long c, len;

  if ( !IsWild(RelationDim(rel)) ) { /* don't do this twice */
    CopyDimensions(RelationDim(rel),dimens);
    return 2;
  }
  sp = stack = (struct dimnode *)
    ascmalloc(RelationDepth(rel)*sizeof(struct dimnode));
  switch( RelationRelop(rel) ) {
  case e_less:
  case e_lesseq:
  case e_greater:
  case e_greatereq:
  case e_equal:
  case e_notequal:
     /* Working on the left-hand-side */
    len = RelationLength(rel,TRUE);
    for( c = 1; c <= len; c++ ) {
      struct relation_term *rt;
      rt = (struct relation_term *)RelationTerm(rel,c,TRUE);
      sp += 1-ArgsForRealToken(RelationTermType(rt));
      apply_term_dimensions(rel,rt,sp-1,sp,&consistent,&wild);
    } /* stack[0].d contains the dimensions of the lhs expression */

    /* Now working on the right-hand_side */
    len = RelationLength(rel,FALSE);
    for( c = 1; c <= len; c++ ) {
      struct relation_term *rt;
      rt = (struct relation_term *) RelationTerm(rel,c,FALSE);
      sp += 1-ArgsForRealToken(RelationTermType(rt));
      apply_term_dimensions(rel,rt,sp-1,sp,&consistent,&wild);
    } /* stack[1].d contains the dimensions of the rhs expression */

    if( IsWild(&(stack[0].d)) || IsWild(&(stack[1].d)) ) {
      if( IsWild(&(stack[0].d)) && !IsZero(&(stack[0])) ) {
        if( !wild ) wild = TRUE;
        if (GCDN) {
          FPRINTF(ASCERR,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(ASCERR,"        on left hand side.\n");
        }
      }
      if( IsWild(&(stack[1].d)) && !IsZero(&(stack[1])) ) {
        if( !wild ) wild = TRUE;
        if (GCDN) {
          FPRINTF(ASCERR,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(ASCERR,"        on right hand side.\n");
        }
      }
    } else {
      if( CmpDimen(&(stack[0].d),&(stack[1].d)) ) {
        if( consistent ) consistent = FALSE;
        if (GCDN) {
          FPRINTF(ASCERR,"ERROR:  Relation has dimensions ");
          WriteDimensions(ASCERR,&(stack[0].d));
          FPRINTF(ASCERR," on left\n");
          FPRINTF(ASCERR,"        and dimensions ");
          WriteDimensions(ASCERR,&(stack[1].d));
          FPRINTF(ASCERR," on right.\n");
        }
      }
    }
    break;
  case e_maximize:
  case e_minimize:
    /* Working on the left-hand-side */
    len = RelationLength(rel,TRUE);
    for( c = 1; c <= len; c++ ) {
      struct relation_term *rt;
      rt = (struct relation_term *) RelationTerm(rel,c,TRUE);
      sp += 1-ArgsForRealToken(RelationTermType(rt));
      apply_term_dimensions(rel,rt,sp-1,sp,&consistent,&wild);
    } /* stack[0].d contains the dimensions of the lhs expression */

    if( IsWild(&(stack[0].d)) && !IsZero(&(stack[0])) ) {
      if( !wild ) wild = TRUE;
      if (GCDN) {
        FPRINTF(ASCERR,"ERROR:  Objective has wild dimensions.\n");
      }
    }
    break;

  default:
    FPRINTF(ASCERR,"ERROR:  Unknown relation type.\n");
    if( consistent ) consistent = FALSE;
    break;
  }
  CopyDimensions(&(stack[0].d),dimens);
  ascfree(stack);
  return( consistent && !wild );
}

/*********************************************************************\
  calculation functions
\*********************************************************************/

/* global relation pointer to avoid passing a relation recursively */
static struct relation *glob_rel;

/* Note that ANY function calling RelationBranchEvaluator should set
   glob_rel to point at the relation being evaluated.  The calling
   function should also set glob_rel = NULL when it is done. */

static double RelationBranchEvaluator(struct relation_term *term)
{
  assert(term != NULL);
  switch(RelationTermType(term)) {
  case e_func:
    return FuncEval(TermFunc(term),
      RelationBranchEvaluator(TermFuncLeft(term)) );
  case e_var:
    return TermVariable(glob_rel , term);
  case e_int:
    return (double)TermInteger(term);
  case e_real:
    return TermReal(term);
  case e_zero:
    return 0.0;

  case e_plus:
    return (RelationBranchEvaluator(TermBinLeft(term)) +
      RelationBranchEvaluator(TermBinRight(term)));
  case e_minus:
    return (RelationBranchEvaluator(TermBinLeft(term)) -
      RelationBranchEvaluator(TermBinRight(term)));
  case e_times:
    return (RelationBranchEvaluator(TermBinLeft(term)) *
      RelationBranchEvaluator(TermBinRight(term)));
  case e_divide:
    return (RelationBranchEvaluator(TermBinLeft(term)) /
      RelationBranchEvaluator(TermBinRight(term)));
  case e_power:
  case e_ipower:
    return pow( RelationBranchEvaluator(TermBinLeft(term)) ,
               RelationBranchEvaluator(TermBinRight(term)) );
  case e_uminus:
    return - RelationBranchEvaluator(TermBinLeft(term));
  default:
    FPRINTF(ASCERR, "error in RelationBranchEvaluator routine\n");
    FPRINTF(ASCERR, "relation term type not recognized\n");
    return 0.0;
  }
}

/* RelationEvaluatePostfixBranch
 * This function is passed a relation pointer, r, a pointer, pos, to a
 * position in the postfix version of the relation (0<=pos<length), and
 * a flag, lhs, telling whether we are interested in the left(=1) or
 * right(=0) side of the relation.  This function will tranverse and
 * evaluate the subtree rooted at pos and will return the value as a
 * double.  To do its evaluation, this function goes backwards through
 * the postfix representation of relation and calls itself at each
 * node--creating a stack of function calls.
 * NOTE: This function changes the value of pos---to the position of
 * the deepest leaf visited
 */
static double
RelationEvaluatePostfixBranch(CONST struct relation *r,
                              unsigned long *pos,
                              int lhs)
{
  CONST struct relation_term *term; /* the current term */
  CONST struct Func *funcptr;       /* a pointer to a function */
  double x, y;                      /* temporary values */

  term = NewRelationTerm(r, *pos, lhs);
  assert(term != NULL);
  switch( RelationTermType(term) ) {
  case e_zero:
  case e_real:
    return TermReal(term);
  case e_int:
    return TermInteger(term);
  case e_var:
    return TermVariable(r, term);
  case e_plus:
    (*pos)--;
    y = RelationEvaluatePostfixBranch(r, pos, lhs); /* y==right-side of '+' */
    (*pos)--;
    return RelationEvaluatePostfixBranch(r, pos, lhs) + y;
  case e_minus:
    (*pos)--;
    y = RelationEvaluatePostfixBranch(r, pos, lhs); /* y==right-side of '-' */
    (*pos)--;
    return RelationEvaluatePostfixBranch(r, pos, lhs) - y;
  case e_times:
    (*pos)--;
    y = RelationEvaluatePostfixBranch(r, pos, lhs); /* y==right-side of '*' */
    (*pos)--;
    return RelationEvaluatePostfixBranch(r, pos, lhs) * y;
  case e_divide:
    (*pos)--;
    y = RelationEvaluatePostfixBranch(r, pos, lhs); /* y is the divisor */
    (*pos)--;
    return RelationEvaluatePostfixBranch(r, pos, lhs) / y;
  case e_power:
    (*pos)--;
    y = RelationEvaluatePostfixBranch(r, pos, lhs); /* y is the power */
    (*pos)--;
    x = RelationEvaluatePostfixBranch(r, pos, lhs); /* x is the base */
    return pow(x, y);
  case e_ipower:
    (*pos)--;
    y = RelationEvaluatePostfixBranch(r, pos, lhs); /* y is the power */
    (*pos)--;
    x = RelationEvaluatePostfixBranch(r, pos, lhs); /* x is the base */
    return asc_ipow(x, (int)y);
  case e_uminus:
    (*pos)--;
    return -1.0 * RelationEvaluatePostfixBranch(r, pos, lhs);
  case e_func:
    funcptr = TermFunc(term);
    (*pos)--;
    return FuncEval(funcptr, RelationEvaluatePostfixBranch(r, pos, lhs));
  default:
    Asc_Panic(2, NULL,
              "Don't know this type of relation type\n"
              "in function RelationEvaluatePostfixBranch\n");
    exit(2);/* Needed to keep gcc from whining */
    break;
  }
}

static double
RelationEvaluatePostfixBranchSafe(CONST struct relation *r,
                                  unsigned long *pos,
                                  int lhs,
                                  enum safe_err *serr)
{
  CONST struct relation_term *term; /* the current term */
  double x, y;                      /* temporary values */

  term = NewRelationTerm(r, *pos, lhs);
  assert(term != NULL);
  switch( RelationTermType(term) ) {
  case e_zero:
  case e_real:
    return TermReal(term);
  case e_int:
    return TermInteger(term);
  case e_var:
    return TermVariable(r, term);
  case e_plus:
    (*pos)--;
    /* y is right-hand-side of '+' */
    y = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    (*pos)--;
    return
      safe_add_D0(RelationEvaluatePostfixBranchSafe(r,pos,lhs,serr), y, serr);
  case e_minus:
    (*pos)--;
    /* y is right-had-side of '-' */
    y = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    (*pos)--;
    return
      safe_sub_D0(RelationEvaluatePostfixBranchSafe(r,pos,lhs,serr), y, serr);
  case e_times:
    (*pos)--;
    /* y is right-hand-side of '*' */
    y = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    (*pos)--;
    return
      safe_mul_D0(RelationEvaluatePostfixBranchSafe(r,pos,lhs,serr), y, serr);
  case e_divide:
    (*pos)--;
    /* y is the divisor */
    y = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    (*pos)--;
    return
      safe_div_D0(RelationEvaluatePostfixBranchSafe(r,pos,lhs,serr), y, serr);
  case e_power:
    (*pos)--;
    /* y is the power */
    y = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    (*pos)--;
    /* x is the base */
    x = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    return safe_pow_D0(x, y, serr);
  case e_ipower:
    (*pos)--;
    /* y is the power */
    y = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    (*pos)--;
    /* x is the base */
    x = RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
    return safe_ipow_D0(x, (int)y, serr);
  case e_uminus:
    (*pos)--;
    return -1.0 * RelationEvaluatePostfixBranchSafe(r, pos, lhs, serr);
  case e_func:
    (*pos)--;
    return
      FuncEvalSafe(TermFunc(term),
                   RelationEvaluatePostfixBranchSafe(r,pos,lhs,serr),serr);
  default:
    Asc_Panic(2, NULL,
              "Don't know this type of relation type\n"
              "in function RelationEvaluatePostfixBranchSafe\n");
    exit(2);/* Needed to keep gcc from whining */
    break;
  }
}

/* RelationEvaluateResidualPostfix
 * Yet another function for calculating the residual of a relation.
 * This function also uses the postfix version of the relations, but it
 * manages a stack(array) of doubles and calculates the residual in this
 * stack; therefore the function is not recursive.  If the funtion
 * cannot allocate memory for its stack, it returns 0.0, so there is
 * currently no way of knowing if this function failed.
 */
static double
RelationEvaluateResidualPostfix(CONST struct relation *r)
{
  unsigned long t;       /* the current term in the relation r */
  int lhs;               /* looking at left(=1) or right(=0) hand side */
  double *res_stack;     /* the stack we use for evaluating the residual */
  long s = -1;           /* the top position in the stacks */
  unsigned long length_lhs, length_rhs;
  CONST struct relation_term *term;
  CONST struct Func *funcptr;


  length_lhs = RelationLength(r, 1);
  length_rhs = RelationLength(r, 0);
  if( (length_lhs+length_rhs) == 0 ) return 0.0;

  /* create the stacks */
  res_stack = tmpalloc_array((1+MAX(length_lhs,length_rhs)),double);
  if( res_stack == NULL ) return 0.0;

  lhs = 1;
  t = 0;
  while (1) {
    if( lhs && (t >= length_lhs) ) {
      /* finished processing left hand side, switch to right if it exists */
      if( length_rhs ) {
        lhs = t = 0;
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_rhs==0), the
         * length_lhs must be > 0, thus s>=0
         */
        return (res_stack[s]);
      }
    }
    else if( (!lhs) && (t >= length_rhs) ) {
      /* finished processing right hand side */
      if( length_lhs ) {
        /* we know length_lhs and length_rhs are both > 0, since if
         * length_rhs == 0, we would have exited above.
         */
        return (res_stack[s-1] - res_stack[s]);
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_lhs==0), the
         * length_rhs must be > 0, thus s>=0
         */
        return (-1.0 * res_stack[s]);
      }
    }

    term = NewRelationTerm(r, t++, lhs);
    switch( RelationTermType(term) ) {
    case e_zero:
      s++;
      res_stack[s] = 0.0;
      break;
    case e_real:
      s++;
      res_stack[s] = TermReal(term);
      break;
    case e_int:
      s++;
      res_stack[s] = TermInteger(term);
      break;
    case e_var:
      s++;
      res_stack[s] = TermVariable(r, term);
      break;
    case e_plus:
      res_stack[s-1] += res_stack[s];
      s--;
      break;
    case e_minus:
      res_stack[s-1] -= res_stack[s];
      s--;
      break;
    case e_times:
      res_stack[s-1] *= res_stack[s];
      s--;
      break;
    case e_divide:
      res_stack[s-1] /= res_stack[s];
      s--;
      break;
    case e_uminus:
      res_stack[s] *= -1.0;
      break;
    case e_power:
    case e_ipower:
      res_stack[s-1] = pow(res_stack[s-1], res_stack[s]);
      s--;
      break;
    case e_func:
      funcptr = TermFunc(term);
      res_stack[s] = FuncEval(funcptr, res_stack[s]);
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type\n"
                "in function RelationEvaluateResidualPostfix\n");
      break;
    }
  }
}


/* RelationEvaluateResidualGradient
 * This function evaluates the residual and the gradient for the relation
 * r.  The calling function must provide a pointer to a double for the
 * residual and an array of doubles for the gradients.  This function
 * assumes r exists and that the pointers to the residual and gradients
 * are not NULL.  This function returns 0 is everythings goes o.k., and
 * 1 otherwise (out of memory).  The function computes the gradients by
 * maintaining a n stacks, where n = (number-of-variables-in-r + 1)
 * The +1 is for the residual.  The stacks come from a single array which
 * this function gets by calling tmpalloc_array.  Two macros are defined
 * to make referencing this array easier.
 */
static int
RelationEvaluateResidualGradient(CONST struct relation *r,
                                 double *residual,
                                 double *gradient)
{
  unsigned long t;       /* the current term in the relation r */
  unsigned long num_var; /* the number of variables in the relation r */
  unsigned long v;       /* the index of the variable we are looking at */
  int lhs;               /* looking at left(=1) or right(=0) hand side of r */
  double *stacks;        /* the memory for the stacks */
  unsigned long stack_height; /* height of each stack */
  long s = -1;           /* the top position in the stacks */
  double temp, temp2;    /* temporary variables to speed gradient calculatns */
  unsigned long length_lhs, length_rhs;
  CONST struct relation_term *term;
  CONST struct Func *fxnptr;

  num_var = NumberVariables(r);
  length_lhs = RelationLength(r, 1);
  length_rhs = RelationLength(r, 0);
  if( (length_lhs + length_rhs) == 0 ) {
    for( v = 0; v < num_var; v++ ) gradient[v] = 0.0;
    *residual = 0.0;
    return 0;
  }
  else {
    stack_height = 1 + MAX(length_lhs,length_rhs);
  }

  /* create the stacks */
  stacks = tmpalloc_array(((num_var+1)*stack_height),double);
  if( stacks == NULL ) return 1;

#define res_stack(s)    stacks[(s)]
#define grad_stack(v,s) stacks[((v)*stack_height)+(s)]

  lhs = 1;
  t = 0;
  while(1) {
    if( lhs && (t >= length_lhs) ) {
      /* need to switch to the right hand side--if it exists */
      if( length_rhs ) {
        lhs = t = 0;
      }
      else {
        /* Set the pointers we were passed to the tops of the stacks.
         * We do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_rhs==0), the
         * length_lhs must be > 0, thus s>=0
         */
        for( v = 1; v <= num_var; v++ ) gradient[v-1] = grad_stack(v,s);
        *residual = res_stack(s);
        return 0;
      }
    }
    else if( (!lhs) && (t >= length_rhs) ) {
      /* we have processed both sides, quit */
      if( length_lhs ) {
        /* Set the pointers we were passed to lhs - rhs
         * We know length_lhs and length_rhs are both > 0, since if
         * length_rhs == 0, we would have exited above.
         */
        for( v = 1; v <= num_var; v++ ) {
          gradient[v-1] = grad_stack(v,s-1) - grad_stack(v,s);
        }
        *residual = res_stack(s-1) - res_stack(s);
        return 0;
      }
      else {
        /* Set the pointers we were passed to -1.0 * top of stacks.
         * We do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_lhs==0), the
         * length_rhs must be > 0, thus s>=0
         */
        for( v = 1; v <= num_var; v++ ) {
          gradient[v-1] = -grad_stack(v,s);
        }
        *residual = -res_stack(s);
        return 0;
      }
    }

    term = NewRelationTerm(r, t++, lhs);
    switch( RelationTermType(term) ) {
    case e_zero:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      res_stack(s) = 0.0;
      break;
    case e_real:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      res_stack(s) = TermReal(term);
      break;
    case e_int:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      res_stack(s) = TermInteger(term);
      break;
    case e_var:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      grad_stack(TermVarNumber(term),s) = 1.0;
      res_stack(s) = TermVariable(r, term);
      break;
    case e_plus:
      /* d(u+v) = du + dv */
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s-1) += grad_stack(v,s);
      res_stack(s-1) += res_stack(s);
      s--;
      break;
    case e_minus:
      /* d(u-v) = du - dv */
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s-1) -= grad_stack(v,s);
      res_stack(s-1) -= res_stack(s);
      s--;
      break;
    case e_times:
      /* d(u*v) = u*dv + v*du */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) = ((res_stack(s-1) * grad_stack(v,s)) +
                             (res_stack(s) * grad_stack(v,s-1)));
      }
      res_stack(s-1) *= res_stack(s);
      s--;
      break;
    case e_divide:
      /*  d(u/v) = du/v - u*dv/(v^2) = (1/v) * [du - (u/v)*dv]  */
      res_stack(s) = 1.0 / res_stack(s);      /*  1/v  */
      res_stack(s-1) *= res_stack(s);         /*  u/v  */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) = (res_stack(s) *
                             (grad_stack(v,s-1) -
                              (res_stack(s-1) * grad_stack(v,s))));
      }
      s--;
      break;
    case e_uminus:
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = -grad_stack(v,s);
      res_stack(s) = -res_stack(s);
      break;
    case e_power:
      /*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
      /*  First compute:  v*u^(v-1)  */
      temp = res_stack(s) * pow( res_stack(s-1), (res_stack(s) - 1.0) );
      /*  Now compute:  ln(u)  */
      temp2 = FuncEval( LookupFuncById(F_LN), res_stack(s-1) );
      /*  Next compute:  u^v  */
      res_stack(s-1) = pow(res_stack(s-1), res_stack(s));
      /*  Compute:  [ln(u)] * [u^v]  */
      temp2 *= res_stack(s-1);
      /*  Finally, compute:  [v*u^(v-1)] * [du] + [ln(u)*u^v] * [dv]  */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) = ((temp * grad_stack(v,s-1)) +
                             (temp2 * grad_stack(v,s)));
      }
      s--;
      break;
    case e_ipower:
      /*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
      /*  First compute:  v*u^(v-1)  */
      temp = asc_d1ipow( res_stack(s-1), ((int)res_stack(s)) );
      /*  Now compute:  ln(u)  */
      temp2 = FuncEval( LookupFuncById(F_LN), res_stack(s-1) );
      /*  Next compute:  u^v  */
      res_stack(s-1) = asc_ipow( res_stack(s-1), ((int)res_stack(s)) );
      /*  Compute:  [ln(u)] * [u^v]  */
      temp2 *= res_stack(s-1);
      /*  Finally, compute:  [v*u^(v-1)] * [du] + [ln(u)*u^v] * [dv]  */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) = ((temp * grad_stack(v,s-1)) +
                             (temp2 * grad_stack(v,s)));
      }
      s--;
      break;
    case e_func:
      /*
      funcptr = TermFunc(term);
      for (v = 0; v < num_var; v++) {
        grad_stack(v,s) = FuncDeriv(funcptr, grad_stack(v,s));
      }
      res_stack(s) = FuncEval(funcptr, res_stack(s));  */
      fxnptr = TermFunc(term);
      temp = FuncDeriv( fxnptr, res_stack(s) );
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) *= temp;
      res_stack(s) = FuncEval( fxnptr, res_stack(s) );
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type\n"
                "in function RelationEvaluateResidualGradient\n");
      break;
    }
  }
#undef grad_stack
#undef res_stack
}

static int
RelationEvaluateResidualGradientSafe(CONST struct relation *r,
                                     double *residual,
                                     double *gradient,
                                     enum safe_err *serr)
{
  unsigned long t;       /* the current term in the relation r */
  unsigned long num_var; /* the number of variables in the relation r */
  unsigned long v;       /* the index of the variable we are looking at */
  int lhs;               /* looking at left(=1) or right(=0) hand side of r */
  double *stacks;        /* the memory for the stacks */
  unsigned long stack_height; /* height of each stack */
  long s = -1;           /* the top position in the stacks */
  double temp, temp2;    /* temporary variables to speed gradient calculatns */
  unsigned long length_lhs, length_rhs;
  CONST struct relation_term *term;
  CONST struct Func *fxnptr;

  num_var = NumberVariables(r);
  length_lhs = RelationLength(r, 1);
  length_rhs = RelationLength(r, 0);
  if( (length_lhs + length_rhs) == 0 ) {
    for( v = 0; v < num_var; v++ ) gradient[v] = 0.0;
    *residual = 0.0;
    return 0;
  }
  else {
    stack_height = 1 + MAX(length_lhs,length_rhs);
  }

  /* create the stacks */
  stacks = tmpalloc_array(((num_var+1)*stack_height),double);
  if( stacks == NULL ) return 1;

#define res_stack(s)    stacks[(s)]
#define grad_stack(v,s) stacks[((v)*stack_height)+(s)]

  lhs = 1;
  t = 0;
  while(1) {
    if( lhs && (t >= length_lhs) ) {
      /* need to switch to the right hand side--if it exists */
      if( length_rhs ) {
        lhs = t = 0;
      }
      else {
        /* Set the pointers we were passed to the tops of the stacks.
         * We do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_rhs==0), the
         * length_lhs must be > 0, thus s>=0
         */
        for( v = 1; v <= num_var; v++ ) gradient[v-1] = grad_stack(v,s);
        *residual = res_stack(s);
        return 0;
      }
    }
    else if( (!lhs) && (t >= length_rhs) ) {
      /* we have processed both sides, quit */
      if( length_lhs ) {
        /* Set the pointers we were passed to lhs - rhs
         * We know length_lhs and length_rhs are both > 0, since if
         * length_rhs == 0, we would have exited above.
         */
        for( v = 1; v <= num_var; v++ ) {
          gradient[v-1] = safe_sub_D0(grad_stack(v,s-1),grad_stack(v,s),serr);
        }
        *residual = safe_sub_D0(res_stack(s-1), res_stack(s), serr);
        return 0;
      }
      else {
        /* Set the pointers we were passed to -1.0 * top of stacks.
         * We do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_lhs==0), the
         * length_rhs must be > 0, thus s>=0
         */
        for( v = 1; v <= num_var; v++ ) {
          gradient[v-1] = -grad_stack(v,s);
        }
        *residual = -res_stack(s);
        return 0;
      }
    }

    term = NewRelationTerm(r, t++, lhs);
    switch( RelationTermType(term) ) {
    case e_zero:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      res_stack(s) = 0.0;
      break;
    case e_real:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      res_stack(s) = TermReal(term);
      break;
    case e_int:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      res_stack(s) = TermInteger(term);
      break;
    case e_var:
      s++;
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = 0.0;
      grad_stack(TermVarNumber(term),s) = 1.0;
      res_stack(s) = TermVariable(r, term);
      break;
    case e_plus:
      /* d(u+v) = du + dv */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1)=safe_add_D0(grad_stack(v,s-1),grad_stack(v,s),serr);
      }
      res_stack(s-1) = safe_add_D0(res_stack(s-1),res_stack(s),serr);
      s--;
      break;
    case e_minus:
      /* d(u-v) = du - dv */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1)=safe_sub_D0(grad_stack(v,s-1),grad_stack(v,s),serr);
      }
      res_stack(s-1) = safe_sub_D0(res_stack(s-1),res_stack(s),serr);
      s--;
      break;
    case e_times:
      /* d(u*v) = u*dv + v*du */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) =
          safe_add_D0(safe_mul_D0(res_stack(s-1),grad_stack(v,s),serr),
                      safe_mul_D0(res_stack(s),grad_stack(v,s-1),serr),
                      serr);
      }
      res_stack(s-1) = safe_mul_D0(res_stack(s-1),res_stack(s),serr);
      s--;
      break;
    case e_divide:
      /*  d(u/v) = du/v - u*dv/(v^2) = (1/v) * [du - (u/v)*dv]  */
      res_stack(s) = safe_rec(res_stack(s),serr);                     /* 1/v */
      res_stack(s-1) = safe_mul_D0(res_stack(s-1),res_stack(s),serr); /* u/v */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) =
          safe_mul_D0(res_stack(s),
                      safe_sub_D0(grad_stack(v,s-1),
                                  safe_mul_D0(res_stack(s-1),
                                              grad_stack(v,s),
                                              serr),serr),serr);
      }
      s--;
      break;
    case e_uminus:
      for( v = 1; v <= num_var; v++ ) grad_stack(v,s) = -grad_stack(v,s);
      res_stack(s) = -res_stack(s);
      break;
    case e_power:
      /*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
      /*  v*u^(v-1)  */
      temp = safe_pow_D1( res_stack(s-1), res_stack(s), 0, serr );
      /*  ln(u)*u^v  */
      temp2 = safe_pow_D1( res_stack(s-1), res_stack(s), 1, serr );
      /*  Compute:  [v*u^(v-1)] * [du] + [ln(u)*u^v] * [dv]  */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) =
          safe_add_D0(safe_mul_D0(temp, grad_stack(v,s-1), serr),
                      safe_mul_D0(temp2, grad_stack(v,s), serr), serr);
      }
      /*  u^v  */
      res_stack(s-1) = safe_pow_D0( res_stack(s-1), res_stack(s), serr );
      s--;
      break;
    case e_ipower:
      /*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
      /*  v*u^(v-1)  */
      temp = safe_ipow_D1( res_stack(s-1), res_stack(s), 0, serr );
      /*  ln(u)*u^v */
      temp2 = safe_ipow_D1( res_stack(s-1), res_stack(s), 1, serr );
      /*  Compute:  [v*u^(v-1)] * [du] + [ln(u)*u^v] * [dv]  */
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s-1) =
          safe_add_D0(safe_mul_D0(temp, grad_stack(v,s-1), serr),
                      safe_mul_D0(temp2, grad_stack(v,s), serr), serr);
      }
      /*  Next compute:  u^v  */
      res_stack(s-1) = safe_ipow_D0( res_stack(s-1), res_stack(s), serr );
      s--;
      break;
    case e_func:
      fxnptr = TermFunc(term);
      temp = FuncDerivSafe( fxnptr, res_stack(s), serr);
      for( v = 1; v <= num_var; v++ ) {
        grad_stack(v,s) = safe_mul_D0( grad_stack(v,s), temp, serr );
      }
      res_stack(s) = FuncEvalSafe( fxnptr, res_stack(s), serr);
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type\n"
                "in function RelationEvaluateResidualGradientSafe\n");
      break;
    }
  }
#undef grad_stack
#undef res_stack
}

/* RelationEvaluateDerivative
 * This function evaluates and returns the derivative of the
 * relation r with respect to the variable whose index is pos.
 * This function assumes r exists and that pos is within the proper range.
 * The function computes the gradients by maintaining 2 stacks, one for
 * the residual and one for the derivative.  The stacks come from a
 * single array which this gets by calling tmpalloc_array.  Two macros
 * are defined to make referencing this array easier.  Of the malloc fails,
 * this function returns 0.0, so there is currently no way to know if
 * the function failed.
 */
static double
RelationEvaluateDerivative(CONST struct relation *r,
                           unsigned long pos)
{
  unsigned long t;       /* the current term in the relation r */
  int lhs;               /* looking at left(=1) or right(=0) hand side of r */
  double *stacks;        /* the memory for the stacks */
  unsigned long stack_height; /* height of each stack */
  long s = -1;           /* the top position in the stacks */
  unsigned long length_lhs, length_rhs;
  CONST struct relation_term *term;
  CONST struct Func *fxnptr;

  length_lhs = RelationLength(r, 1);
  length_rhs = RelationLength(r, 0);
  if( (length_lhs + length_rhs) == 0 ) {
    return 0.0;
  }
  else {
    stack_height = 1 + MAX(length_lhs,length_rhs);
  }

  /* create the stacks */
  stacks = tmpalloc_array((2*stack_height),double);
  if( stacks == NULL ) return 0.0;

#define res_stack(s)  stacks[(s)]
#define grad_stack(s) stacks[stack_height+(s)]

  lhs = 1;
  t = 0;
  while(1) {
    if( lhs && (t >= length_lhs) ) {
      /* need to switch to the right hand side--if it exists */
      if( length_rhs ) {
        lhs = t = 0;
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_rhs==0), the
         * length_lhs must be > 0, thus s>=0
         */
        return grad_stack(s);
      }
    }
    else if( (!lhs) && (t >= length_rhs) ) {
      /* we have processed both sides, quit */
      if( length_lhs ) {
        /* we know length_lhs and length_rhs are both > 0, since if
         * length_rhs == 0, we would have exited above.
         */
        return (grad_stack(s-1) - grad_stack(s));
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_lhs==0), the
         * length_rhs must be > 0, thus s>=0
         */
        return (-1.0 * grad_stack(s));
      }
    }

    term = NewRelationTerm(r, t++, lhs);
    switch( RelationTermType(term) ) {
    case e_zero:
      s++;
      grad_stack(s) = res_stack(s) = 0.0;
      break;
    case e_real:
      s++;
      grad_stack(s) = 0.0;
      res_stack(s) = TermReal(term);
      break;
    case e_int:
      s++;
      grad_stack(s) = 0.0;
      res_stack(s) = TermInteger(term);
      break;
    case e_var:
      s++;
      grad_stack(s) = ( (pos == TermVarNumber(term)) ? 1.0 : 0.0 );
      res_stack(s) = TermVariable(r, term);
      break;
    case e_plus:
      /* d(u+v) = du + dv */
      grad_stack(s-1) += grad_stack(s);
      res_stack(s-1) += res_stack(s);
      s--;
      break;
    case e_minus:
      /* d(u-v) = du - dv */
      grad_stack(s-1) -= grad_stack(s);
      res_stack(s-1) -= res_stack(s);
      s--;
      break;
    case e_times:
      /* d(u*v) = u*dv + v*du */
      grad_stack(s-1) = ((res_stack(s-1) * grad_stack(s)) +
                         (res_stack(s) * grad_stack(s-1)));
      res_stack(s-1) *= res_stack(s);
      s--;
      break;
    case e_divide:
      /*  d(u/v) = du/v - u*dv/(v^2) = [du - (u/v)*dv]/v  */
      res_stack(s-1) = res_stack(s-1) / res_stack(s);
      grad_stack(s-1) = ((grad_stack(s-1) - (res_stack(s-1) * grad_stack(s))) /
                         res_stack(s));
      s--;
      break;
    case e_uminus:
      grad_stack(s) = -grad_stack(s);
      res_stack(s) = -res_stack(s);
      break;
    case e_power:
      /*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
      /*  First we compute:  v*u^(v-1)*du  */
      grad_stack(s-1) *= (pow(res_stack(s-1), (res_stack(s) - 1.0)) *
                          res_stack(s));
      /*  Now compute:  ln(u)*dv  */
      grad_stack(s) *= FuncEval( LookupFuncById(F_LN), res_stack(s-1) );
      /*  Next compute:  u^v  */
      res_stack(s-1) = pow( res_stack(s-1), res_stack(s) );
      /*  Finally, compute:  [v*u^(v-1)*du] + [u^v] * [ln(u)*dv]  */
      grad_stack(s-1) += (res_stack(s-1) * grad_stack(s));
      s--;
      break;
    case e_ipower:
      /*  d(x^y) = y * dx * x^(y-1) + ln(x) * dy * x^y  */
      /*  First we compute:  v*u^(v-1)*du  */
      grad_stack(s-1) *= asc_d1ipow( res_stack(s-1), ((int)res_stack(s)) );
      /*  Now compute:  ln(u)*dv  */
      grad_stack(s) *= FuncEval( LookupFuncById(F_LN), res_stack(s-1) );
      /*  Next compute:  u^v  */
      res_stack(s-1) = asc_ipow( res_stack(s-1), ((int)res_stack(s)) );
      /*  Finally, compute:  [v*u^(v-1)*du] + [u^v] * [ln(u)*dv]  */
      grad_stack(s-1) += (res_stack(s-1) * grad_stack(s));
      s--;
      break;
    case e_func:
      fxnptr = TermFunc(term);
      grad_stack(s) *= FuncDeriv( fxnptr, res_stack(s) );
      res_stack(s) = FuncEval( fxnptr, res_stack(s) );
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type\n"
                "in function RelationEvaluateDerivative\n");
      break;
    }
  }
#undef grad_stack
#undef res_stack
}

static double
RelationEvaluateDerivativeSafe(CONST struct relation *r,
                               unsigned long pos,
                               enum safe_err *serr)
{
  unsigned long t;       /* the current term in the relation r */
  int lhs;               /* looking at left(=1) or right(=0) hand side of r */
  double *stacks;        /* the memory for the stacks */
  unsigned long stack_height; /* height of each stack */
  long s = -1;           /* the top position in the stacks */
  unsigned long length_lhs, length_rhs;
  CONST struct relation_term *term;
  CONST struct Func *fxnptr;

  length_lhs = RelationLength(r, 1);
  length_rhs = RelationLength(r, 0);
  if( (length_lhs + length_rhs) == 0 ) {
    return 0.0;
  }
  else {
    stack_height = 1 + MAX(length_lhs,length_rhs);
  }

  /* create the stacks */
  stacks = tmpalloc_array((2*stack_height),double);
  if( stacks == NULL ) return 0.0;

#define res_stack(s)  stacks[(s)]
#define grad_stack(s) stacks[stack_height+(s)]

  lhs = 1;
  t = 0;
  while(1) {
    if( lhs && (t >= length_lhs) ) {
      /* need to switch to the right hand side--if it exists */
      if( length_rhs ) {
        lhs = t = 0;
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_rhs==0), the
         * length_lhs must be > 0, thus s>=0
         */
        return grad_stack(s);
      }
    }
    else if( (!lhs) && (t >= length_rhs) ) {
      /* we have processed both sides, quit */
      if( length_lhs ) {
        /* we know length_lhs and length_rhs are both > 0, since if
         * length_rhs == 0, we would have exited above.
         */
        return safe_sub_D0(grad_stack(s-1), grad_stack(s), serr);
      }
      else {
        /* do not need to check for s>=0, since we know that
         * (length_lhs+length_rhs>0) and that (length_lhs==0), the
         * length_rhs must be > 0, thus s>=0
         */
        return (-grad_stack(s));
      }
    }

    term = NewRelationTerm(r, t++, lhs);
    switch( RelationTermType(term) ) {
    case e_zero:
      s++;
      grad_stack(s) = res_stack(s) = 0.0;
      break;
    case e_real:
      s++;
      grad_stack(s) = 0.0;
      res_stack(s) = TermReal(term);
      break;
    case e_int:
      s++;
      grad_stack(s) = 0.0;
      res_stack(s) = TermInteger(term);
      break;
    case e_var:
      s++;
      grad_stack(s) = ( (pos == TermVarNumber(term)) ? 1.0 : 0.0 );
      res_stack(s) = TermVariable(r, term);
      break;
    case e_plus:
      /* d(u+v) = du + dv */
      grad_stack(s-1) = safe_add_D0( grad_stack(s-1), grad_stack(s), serr );
      res_stack(s-1) = safe_add_D0( res_stack(s-1), res_stack(s), serr );
      s--;
      break;
    case e_minus:
      /* d(u-v) = du - dv */
      grad_stack(s-1) = safe_sub_D0( grad_stack(s-1), grad_stack(s), serr );
      res_stack(s-1) = safe_sub_D0( res_stack(s-1), res_stack(s), serr );
      s--;
      break;
    case e_times:
      /* d(u*v) = u*dv + v*du */
      grad_stack(s-1) =
        safe_add_D0(safe_mul_D0( res_stack(s-1), grad_stack(s), serr),
                    safe_mul_D0( res_stack(s), grad_stack(s-1), serr), serr);
      res_stack(s-1) = safe_mul_D0( res_stack(s-1), res_stack(s), serr );
      s--;
      break;
    case e_divide:
      /*  d(u/v) = du/v - u*dv/(v^2) = [du - (u/v)*dv]/v  */
      res_stack(s-1) = safe_div_D0( res_stack(s-1), res_stack(s), serr);
      grad_stack(s-1) =
        safe_div_D0(safe_sub_D0(grad_stack(s-1),
                                safe_mul_D0(res_stack(s-1),grad_stack(s),serr),
                                serr),
                    res_stack(s),serr);
      s--;
      break;
    case e_uminus:
      grad_stack(s) = -grad_stack(s);
      res_stack(s) = -res_stack(s);
      break;
    case e_power:
      /*  d(u^v) = v * u^(v-1) * du + ln(u) * u^v * dv  */
      grad_stack(s-1) =
        safe_add_D0(safe_mul_D0(safe_pow_D1(res_stack(s-1),
                                            res_stack(s),0,serr),
                                grad_stack(s-1), serr),
                    safe_mul_D0(safe_pow_D1(res_stack(s-1),
                                            res_stack(s),1,serr),
                                grad_stack(s),serr), serr);
      /*  u^v  */
      res_stack(s-1) = safe_pow_D0( res_stack(s-1), res_stack(s), serr);
      s--;
      break;
    case e_ipower:
      /*  d(x^y) = y * dx * x^(y-1) + ln(x) * dy * x^y  */
      grad_stack(s-1) =
        safe_add_D0(safe_mul_D0(safe_ipow_D1(res_stack(s-1),
                                             res_stack(s),0,serr),
                                grad_stack(s-1), serr),
                    safe_mul_D0(safe_ipow_D1(res_stack(s-1),
                                             res_stack(s),1,serr),
                                grad_stack(s),serr), serr);
      /*  u^v  */
      res_stack(s-1) = safe_ipow_D0( res_stack(s-1), res_stack(s), serr);
      s--;
      break;
    case e_func:
      fxnptr = TermFunc(term);
      grad_stack(s) = safe_mul_D0(FuncDerivSafe( fxnptr, res_stack(s), serr ),
                                  grad_stack(s), serr);
      res_stack(s) = FuncEvalSafe( fxnptr, res_stack(s), serr);
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type\n"
                "in function RelationEvaluateDerivativeSafe\n");
      break;
    }
  }
#undef grad_stack
#undef res_stack
}


/*********************************************************************\
  external relation/relation term queries section.
\*********************************************************************/

/* return =, <, >, etc, etc. not e_token, e_glassbox, etc */
enum Expr_enum RelationRelop(CONST struct relation *rel)
{
  AssertAllocatedMemory(rel,sizeof(struct relation));
  return rel->share->s.relop;
}

/*
 * This query only applies to TokenRelations and OpcodeRelations.
 */
unsigned long RelationLength(CONST struct relation *rel, int lhs)
{
  assert(rel!=NULL);
  AssertAllocatedMemory(rel,sizeof(struct relation));
  if (lhs){
    if (RTOKEN(rel).lhs) return (RTOKEN(rel).lhs_len);
    else return 0;
  }
  if (RTOKEN(rel).rhs) return (RTOKEN(rel).rhs_len);
  else return 0;
}

/*
 * This query only applies to TokenRelations. It assumes the
 * user still thinks tokens number from [1..len].
 */
CONST struct relation_term *RelationTerm(CONST struct relation *rel,
        				 unsigned long int pos, int lhs)
{
  assert(rel!=NULL);
  AssertAllocatedMemory(rel,sizeof(struct relation));
  if (lhs){
    if (RTOKEN(rel).lhs)
      return A_TERM(&(RTOKEN(rel).lhs[pos-1]));
    else return NULL;
  }
  else{
    if (RTOKEN(rel).rhs)
      return A_TERM(&(RTOKEN(rel).rhs[pos-1]));
    else return NULL;
  }
}

/*
 * This query only applies to TokenRelations. It assumes the
 * clued user thinks tokens number from [0..len-1], which they do.
 */
CONST struct relation_term
*NewRelationTermF(CONST struct relation *rel, unsigned long pos, int lhs)
{
  assert(rel!=NULL);
  AssertAllocatedMemory(rel,sizeof(struct relation));
  if (lhs){
    if (RTOKEN(rel).lhs != NULL)
      return A_TERM(&(RTOKEN(rel).lhs[pos]));
    else return NULL;
  } else {
    if (RTOKEN(rel).rhs != NULL)
      return A_TERM(&(RTOKEN(rel).rhs[pos]));
    else return NULL;
  }
}

/*
 * This query only applies to sides from TokenRelations. It assumes the
 * clued user thinks tokens number from [0..len-1], which they do,
 * and that the side came from a token relation instance.
 */
CONST struct relation_term
*RelationSideTermF(CONST union RelationTermUnion *side, unsigned long pos)
{
  assert(side!=NULL);
  return A_TERM(&(side[pos]));
}

/*
 * This query only applies to TokenRelations. It assumes the
 * clued user thinks tokens number from [0..len-1], which they do.
 */
enum Expr_enum RelationTermTypeF(CONST struct relation_term *term)
{
  AssertMemory(term);
  return term->t;
}

unsigned long TermVarNumber(CONST struct relation_term *term)
{
  assert(term&&term->t == e_var);
  AssertMemory(term);
  return V_TERM(term)->varnum;
}

long TermInteger(CONST struct relation_term *term)
{
  assert(term&&(term->t==e_int));
  AssertMemory(term);
  return I_TERM(term)->ivalue;
}

double TermReal(CONST struct relation_term *term)
{
  assert(term&&( term->t==e_real || term->t==e_zero));
  AssertMemory(term);
  return R_TERM(term)->value;
}

double
TermVariable(CONST struct relation *rel, CONST struct relation_term *term)
{
  return
    RealAtomValue((struct Instance*)RelationVariable(rel,TermVarNumber(term)));
}

CONST dim_type *TermDimensions(CONST struct relation_term *term)
{
  assert( term && (term->t==e_real || term->t == e_int || term->t == e_zero) );
  AssertMemory(term);
  if (term->t==e_real) return R_TERM(term)->dimensions;
  if (term->t==e_int) return Dimensionless();
  if (term->t==e_zero) return WildDimension();
  return NULL;
}

CONST struct Func *TermFunc(CONST struct relation_term *term)
{
  assert(term&&(term->t == e_func));
  AssertMemory(term);
  return F_TERM(term)->fptr;
}

struct relation_term *RelationINF_Lhs(CONST struct relation *rel)
{
  return RTOKEN(rel).lhs_term;
}

struct relation_term *RelationINF_Rhs(CONST struct relation *rel)
{
  return RTOKEN(rel).rhs_term;
}


/*
 * For picking apart a BlackBox relation and its
 * ExternalCall node.
 */

struct ExtCallNode *BlackBoxExtCall(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return RBBOX(rel).ext;
}

int *BlackBoxArgs(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return RBBOX(rel).args;
}

/*
 * For picking apart a GlassBox relation.
 */
struct ExternalFunc *GlassBoxExtFunc(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return RGBOX(rel).efunc;
}

int GlassBoxRelIndex(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return RGBOX(rel).index;
}

int *GlassBoxArgs(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return RGBOX(rel).args;
}


/*
 * For picking apart a relation. Not all of these queries may be
 * applied to all relation types. Those that cannot be are so
 * marked.
 */

CONST struct gl_list_t *RelationVarList(CONST struct relation *rel)
{
  return (CONST struct gl_list_t *)rel->vars;
}

dim_type *RelationDim(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return rel->d;
}

int SetRelationDim(struct relation *rel, dim_type *d)
{
  if (!rel) return 1;
  rel->d = d;
  return 0;
}

double RelationResidual(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return rel->residual;
}

void SetRelationResidual(struct relation *rel, double value)
{
  assert(rel!=NULL);
  rel->residual = value;
}

double RelationMultiplier(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return rel->multiplier;
}

void SetRelationMultiplier(struct relation *rel, double value)
{
  assert(rel!=NULL);
  rel->multiplier = value;
}

double RelationNominal(CONST struct relation *rel)
{
  assert(rel!=NULL);
  return rel->nominal;
}

void SetRelationNominal(struct relation *rel, double value)
{
  assert(rel!=NULL);
  rel->nominal = (fabs(value) > 0.0) ? fabs(value) : rel->nominal;
}


int RelationIsCond(CONST struct relation *rel)
{
  if ( rel != NULL) {
    return rel->iscond;
  }
  return 0;
}

void SetRelationIsCond(struct relation *rel)
{
  if ( rel != NULL) {
    rel->iscond = 1;
  } else {
    FPRINTF(ASCERR,"ERROR: SetRelationIsCond called with NULL relation\n");
  }
}

unsigned long NumberVariables(CONST struct relation *rel)
{
  unsigned long n;
  assert(rel!=NULL);
  n = (rel->vars!=NULL) ? gl_length(rel->vars) : 0;
  return n;
}

struct Instance *RelationVariable(CONST struct relation *rel,
        			  unsigned long int varnum)
{
  assert(rel!=NULL);
  return (struct Instance *)gl_fetch(rel->vars,varnum);
}

static void CalcDepth(CONST struct relation *rel,
               int lhs,
               unsigned long int *depth,
               unsigned long int *maxdepth)
{
  unsigned long c,length;
  CONST struct relation_term *term;
  length = RelationLength(rel,lhs);
  for(c=0;c<length;c++){
    term = NewRelationTerm(rel,c,lhs);
    switch(RelationTermType(term)){
    case e_zero:
    case e_int:
    case e_real:
    case e_var:
      if (++(*depth) > *maxdepth) *maxdepth = *depth;
      break;
    case e_func:
    case e_uminus:
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      (*depth)--;
      break;
    default:
      Asc_Panic(2, NULL,
                "Don't know this type of relation type.\n"
                "in function CalcDepth\n");
      break;
    }
  }
}

unsigned long RelationDepth(CONST struct relation *rel)
{
  unsigned long depth=0,maxdepth=0;
  switch(RelationRelop(rel)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    CalcDepth(rel,1,&depth,&maxdepth);
    CalcDepth(rel,0,&depth,&maxdepth);
    assert(depth == 2);
    break;
  case e_maximize:
  case e_minimize:
    CalcDepth(rel,1,&depth,&maxdepth);
    assert(depth == 1);
    break;
  default:
    Asc_Panic(2, NULL, "Unknown relation type.\n");
    break;
  }
  return maxdepth;
}

/***************************************************
  The following routines are used for equation scaling.
  Documentation will be added at a later date.
****************************************************/

static double FindMaxAdditiveTerm(struct relation_term *s)
{
  enum safe_err serr;
  double lhs, rhs;

  switch (RelationTermType(s)) {
  case e_plus:
  case e_minus:
    /** note these used to be inlined with max, but a bug in gcc323 caused it to be split out. */
    lhs = FindMaxAdditiveTerm(TermBinLeft(s));
    rhs = FindMaxAdditiveTerm(TermBinRight(s));
    return MAX(fabs(lhs), fabs(rhs));
  case e_uminus:
    return (FindMaxAdditiveTerm(TermUniLeft(s)));
  case e_times:
    return (FindMaxAdditiveTerm(TermBinLeft(s))*
      FindMaxAdditiveTerm(TermBinRight(s)));
  case e_divide:
    /* bug patch / 0 */
    return safe_div_D0(FindMaxAdditiveTerm(TermBinLeft(s)) ,
      RelationBranchEvaluator(TermBinRight(s)),&serr);
  default:
    return RelationBranchEvaluator(s);
  }
}

static double FindMaxFromTop(struct relation *s)
{
  double lhs;
  double rhs;
  if (s == NULL) {
    return 0;
  }
  /** note these used to be inlined with max, but a bug in gcc323 caused it to be split out. */
  lhs = FindMaxAdditiveTerm(Infix_LhsSide(s));
  rhs = FindMaxAdditiveTerm(Infix_RhsSide(s));
  return MAX(fabs(lhs), fabs(rhs));
}

double CalcRelationNominal(struct Instance *i)     /* send in relation */
{
  enum Expr_enum reltype;

  char *iname;
  iname = WriteInstanceNameString(i,NULL);
  ascfree(iname);

  glob_rel = NULL;
  if (i == NULL){
    FPRINTF(ASCERR, "error in CalcRelationNominal routine\n");
    return (double)0;
  }
  if (InstanceKind(i) != REL_INST) {
    FPRINTF(ASCERR, "error in CalcRelationNominal routine\n");
    return (double)0;
  }
  glob_rel = (struct relation *)GetInstanceRelation(i,&reltype);
  if (glob_rel == NULL) {
    FPRINTF(ASCERR, "error in CalcRelationNominal routine\n");
    return (double)0;
  }

  if (reltype == e_token) {
    double temp;
    temp = FindMaxFromTop(glob_rel);
    if (isnan(temp) || !finite(temp)) {
      glob_rel = NULL;
      return (double)1;
    }
    if ( temp > 0) { /* this could return some really small numbers */
      glob_rel = NULL;
      return temp;
    }
  }
  if (reltype == e_blackbox)
  {
    FPRINTF(ASCERR, "error in CalcRelationNominal:\n");
    FPRINTF(ASCERR, "blackbox not implemented yet. Assuming 1.0\n");
  }
  if (reltype == e_glassbox)
  {
    FPRINTF(ASCERR, "error in CalcRelationNominal:\n");
    FPRINTF(ASCERR, "glassbox not implemented yet. Assuming 1.0\n");
  }
  if (reltype == e_opcode)
  {
    FPRINTF(ASCERR, "error in CalcRelationNominal:\n");
    FPRINTF(ASCERR, "opcode not supported.\n");
  }
  glob_rel = NULL;
  return (double)1;
}

void PrintScale(struct Instance *i)
{
  if (InstanceKind(i) == REL_INST) {
    double j;
    j = CalcRelationNominal(i);
    PRINTF(" scale constant = %g\n", j);
  }
}

void PrintRelationNominals(struct Instance *i)
{
  VisitInstanceTree(i,PrintScale, 0, 0);
}

/**
 *** CALCULATION ROUTINES
 */

/*
 * Load ATOM values into an array of doubles.
 * The array of doubles is indexed from 0 while the
 * var list is indexed from 1. The ultimate client of
 * the array calling this function thinks vars index from 0.
 */
static 
void RelationLoadDoubles(struct gl_list_t *varlist, double *vars)
{
  unsigned long c;
  vars--; /* back up the pointer so indexing from 1 puts data right */
  for (c= gl_length(varlist); c > 0; c--) {
    vars[c] = RealAtomValue((struct Instance *)gl_fetch(varlist,c));
  }
}

/* only called on token relations. */
int RelationCalcResidualBinary(CONST struct relation *r, double *res)
{
  double *vars;
  double tres;
  int old_errno;

  if (r == NULL || res == NULL) {
    return 1;
  }
  vars = tmpalloc_array(gl_length(r->vars),double);
  if (vars == NULL) {
    return 1;
  }
  RelationLoadDoubles(r->vars,vars);
  old_errno = errno; /* push C global errno */
  errno = 0;
  if (BinTokenCalcResidual(RTOKEN(r).btable,RTOKEN(r).bindex,vars,&tres)) {
    if (errno == 0) { /* pop if unchanged */
      errno = old_errno;
    }
    return 1;
  }
  if (!finite(tres) || errno == EDOM || errno == ERANGE ) {
    if (errno == 0) { /* pop if unchanged */
      errno = old_errno;
    }
    return 1;
  }
  if (errno == 0) { /* pop if unchanged */
    errno = old_errno;
  }
  *res = tres;
  return 0;
}

enum safe_err
RelationCalcResidualPostfixSafe(struct Instance *i, double *res)
{
  struct relation *r;
  enum Expr_enum reltype;
  enum safe_err not_safe = safe_ok;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR,
            "error in RelationCalcResidualPostfixSafe: null instance\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if (res == NULL){
    FPRINTF(ASCERR,
            "error in RelationCalcResidualPostfixSafe: null relationptr\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR,
            "error in RelationCalcResidualPostfixSafe: not relation\n");
    not_safe = safe_problem;
    return not_safe;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR,
            "error in RelationCalcResidualPostfixSafe: null relation\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( reltype == e_token ) {
    unsigned long length_lhs, length_rhs;

    length_lhs = RelationLength(r, 1);
    length_rhs = RelationLength(r, 0);
    if( length_lhs > 0 ) {
      length_lhs--;
      *res = RelationEvaluatePostfixBranchSafe(r, &length_lhs, 1,&not_safe);
    }
    else {
      *res = 0.0;
    }
    if( length_rhs > 0 ) {
      length_rhs--;
      *res -= RelationEvaluatePostfixBranchSafe(r, &length_rhs, 0,&not_safe);
    }
    safe_error_to_stderr(&not_safe);
    return not_safe;
  } else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    if (reltype == e_blackbox)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidualPostfixSafe:\n");
      FPRINTF(ASCERR, "blackbox not implemented yet.\n");
    }
    if (reltype == e_glassbox)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidualPostfixSafe:\n");
      FPRINTF(ASCERR, "glassbox not implemented yet.\n");
    }
    if (reltype == e_opcode)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidualPostfixSafe:\n");
      FPRINTF(ASCERR, "opcode not supported.\n");
    }
    not_safe = safe_problem;
    return not_safe;
  } else {
    Asc_Panic(2, NULL, "error in RelationCalcResidualPostfixSafe:\n"
              "reached end of routine\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}


int
RelationCalcResidualPostfix(struct Instance *i, double *res)
{
  struct relation *r;
  enum Expr_enum reltype;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix: null instance\n");
    return 1;
  }
  if (res == NULL){
    FPRINTF(ASCERR,"error in RelationCalcResidualPostfix: null relationptr\n");
    return 1;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix: not relation\n");
    return 1;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix: null relation\n");
    return 1;
  }
  if( reltype == e_token ) {
    unsigned long length_lhs, length_rhs;

    length_lhs = RelationLength(r, 1);
    length_rhs = RelationLength(r, 0);
    if( length_lhs > 0 ) {
      length_lhs--;
      *res = RelationEvaluatePostfixBranch(r, &length_lhs, 1);
    }
    else {
      *res = 0.0;
    }
    if( length_rhs > 0 ) {
      length_rhs--;
      *res -= RelationEvaluatePostfixBranch(r, &length_rhs, 0);
    }
    return 0;
  } else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    if (reltype == e_blackbox)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidualPostfix:\n");
      FPRINTF(ASCERR, "blackbox not implemented yet.\n");
    }
    if (reltype == e_glassbox)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidualPostfix:\n");
      FPRINTF(ASCERR, "glassbox not implemented yet.\n");
    }
    if (reltype == e_opcode)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidualPostfix:\n");
      FPRINTF(ASCERR, "opcode not supported.\n");
    }
    return 1;
  } else {
    Asc_Panic(2, NULL,
              "error in RelationCalcResidualPostfix:\n"
              "reached end of routine\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

int RelationCalcExceptionsInfix(struct Instance *i)
{
  enum Expr_enum reltype;
  double res;
  int result = 0;
  int old_errno;

  glob_rel = NULL;
#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcExceptionsInfix: NULL instance\n");
    return -1;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcExceptionsInfix: not relation\n");
    return -1;
  }
#endif
  glob_rel = (struct relation *)GetInstanceRelation(i, &reltype);
  if( glob_rel == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcExceptionsInfix: NULL relation\n");
    return -1;
  }
  if( reltype == e_token ) {
    if (Infix_LhsSide(glob_rel) != NULL) {
      old_errno = errno;
      errno = 0; /* save the last errno, because we don't know why */
      res = RelationBranchEvaluator(Infix_LhsSide(glob_rel));
      if (!finite(res) || errno == EDOM || errno == ERANGE) {
        result |= RCE_ERR_LHS;
        if (isnan(res)) {
          result |= RCE_ERR_LHSNAN;
        } else {
          if (!finite(res)) {
            result |= RCE_ERR_LHSINF;
          }
        }
      }
      if (errno == 0) {
        errno = old_errno;
      } /* else something odd happened in evaluation */
    }
    if(Infix_RhsSide(glob_rel) != NULL) {
      res = RelationBranchEvaluator(Infix_RhsSide(glob_rel));
      if (!finite(res)) {
        result |= RCE_ERR_RHS;
        if (isnan(res)) {
          result |= RCE_ERR_RHSNAN;
        } else {
          if (!finite(res)) {
            result |= RCE_ERR_LHSINF;
          }
        }
      }
    }
    glob_rel = NULL;
    return result;
  } else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    FPRINTF(ASCERR, "error in RelationCalcExceptionsInfix:\n");
    FPRINTF(ASCERR, "reltype not implemented yet\n");
    glob_rel = NULL;
    return -1;
  } else {
    Asc_Panic(2, NULL,
              "error in RelationCalcExceptionsInfix:\n"
              "reached end of routine\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}

int RelationCalcResidualInfix(struct Instance *i, double *res)
{
  enum Expr_enum reltype;
  glob_rel = NULL;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualInfix: NULL instance\n");
    return 1;
  }
  if (res == NULL){
    FPRINTF(ASCERR,"error in RelationCalcResidualInfix: NULL residual ptr\n");
    return 1;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualInfix: not relation\n");
    return 1;
  }
#endif
  glob_rel = (struct relation *)GetInstanceRelation(i, &reltype);
  if( glob_rel == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualInfix: NULL relation\n");
    return 1;
  }
  if( reltype == e_token ) {
    if(Infix_LhsSide(glob_rel) != NULL) {
      *res = RelationBranchEvaluator(Infix_LhsSide(glob_rel));
    } else {
      *res = 0.0;
    }
    if(Infix_RhsSide(glob_rel) != NULL) {
      *res -= RelationBranchEvaluator(Infix_RhsSide(glob_rel));
    }
    glob_rel = NULL;
    return 0;
  } else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    FPRINTF(ASCERR, "error in RelationCalcResidualInfix:\n");
    FPRINTF(ASCERR, "reltype not implemented yet\n");
    glob_rel = NULL;
    return 1;
  } else {
    Asc_Panic(2, NULL,
              "error in RelationCalcResidualInfix:\n"
              "reached end of routine\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}


/* RelationCalcResidualPostfix2
 * Yes, yet another function to calculate the residual
 */
int
RelationCalcResidualPostfix2(struct Instance *i,
                             double *res)
{
  struct relation *r;
  enum Expr_enum reltype;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix2: null instance\n");
    return 1;
  }
  if( res == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix2: %s\n",
            "null relation ptr");
    return 1;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix2: not relation\n");
    return 1;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix2: null relation\n");
    return 1;
  }

  if( reltype == e_token ) {
    *res = RelationEvaluateResidualPostfix(r);
    return 0;
  } else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    FPRINTF(ASCERR, "error in RelationCalcResidualPostfix2:\n");
    FPRINTF(ASCERR, "reltype not implemented yet\n");
    return 1;
  } else {
    Asc_Panic(2, NULL,
              "error in RelationCalcResidualPostfix2:\n"
              "reached end of routine\n");
    exit(2);/* Needed to keep gcc from whining */
  }
}


/*  RelationCalcGradient
 *  simply call the version that calculates the gradient and the residual,
 *  then ignore the residual
 */
int
RelationCalcGradient(struct Instance *r,
                     double *grad)
{
  double residual;
  return RelationCalcResidGrad(r, &residual, grad);
}

/*  RelationCalcGradientSafe
 *  simply call the version that calculates the gradient and the residual,
 *  then ignore the residual
 */
enum safe_err
RelationCalcGradientSafe(struct Instance *r,
                         double *grad)
{
  double residual;

  return RelationCalcResidGradSafe(r, &residual, grad);
}


int
RelationCalcResidGrad(struct Instance *i,
                      double *residual,
                      double *gradient)
{
  struct relation *r;
  enum Expr_enum reltype;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGrad: null instance\n");
    return 1;
  }
  if( residual == NULL || gradient == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGrad: passed a null pointer\n");
    return 1;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGrad: not relation\n");
    return 1;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGrad: null relation\n");
    return 1;
  }

  if( reltype == e_token ) {
    return RelationEvaluateResidualGradient(r, residual, gradient);
  }
  else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    FPRINTF(ASCERR, "error in RelationCalcResidGrad %s\n",
            "reltype not implemented");
    return 1;
  }
  else {
    Asc_Panic(2, NULL,
              "error in RelationCalcResidGrad:\n"
              "reached end of routine");
    exit(2);/* Needed to keep gcc from whining */
  }
}

enum safe_err
RelationCalcResidGradSafe(struct Instance *i,
                          double *residual,
                          double *gradient)
{
  struct relation *r;
  enum Expr_enum reltype;
  enum safe_err not_safe = safe_ok;
  int dummy_int;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGradSafe: null instance\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( residual == NULL || gradient == NULL ) {
    FPRINTF(ASCERR,
            "error in RelationCalcResidGradSafe: passed a null pointer\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGradSafe: not relation\n");
    not_safe = safe_problem;
    return not_safe;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcResidGradSafe: null relation\n");
    not_safe = safe_problem;
    return not_safe;
  }

  if( reltype == e_token ) {
    dummy_int =
      RelationEvaluateResidualGradientSafe(r, residual, gradient, &not_safe);
    return not_safe;
  }
  else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    if (reltype == e_blackbox)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidGradSafe:\n");
      FPRINTF(ASCERR, "blackbox not implemented yet.\n");
    }
    if (reltype == e_glassbox)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidGradSafe:\n");
      FPRINTF(ASCERR, "glassbox not implemented yet.\n");
    }
    if (reltype == e_opcode)
    {
      FPRINTF(ASCERR, "error in RelationCalcResidGradSafe:\n");
      FPRINTF(ASCERR, "opcode not supported.\n");
    }
    not_safe = safe_problem;
    return not_safe;
  }
  else {
    Asc_Panic(2, NULL,
              "error in RelationCalcResidGradSafe:\n",
              "reached end of routine");
    exit(2);/* Needed to keep gcc from whining */
  }
}


/*  RelationCalcDerivative
 *  calculate the derivative with respect to a single variable
 *  whose index is index, where 1<=index<=NumberVariables(r)
 */
int
RelationCalcDerivative(struct Instance *i,
                       unsigned long index,
                       double *gradient)
{
  struct relation *r;
  enum Expr_enum reltype;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivative: null instance\n");
    return 1;
  }
  if( gradient == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivative: passed null pointer\n");
    return 1;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivative: not relation\n");
    return 1;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivative: null relation\n");
    return 1;
  }
  if( (index < 1) || (index > NumberVariables(r)) ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivative: index out of bounds\n");
    return 1;
  }

  if( reltype == e_token ) {
    *gradient = RelationEvaluateDerivative(r, index);
    return 0;
  }
  else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    FPRINTF(ASCERR, "error in RelationCalcDerivative %s\n",
            "reltype not implemented");
    return 1;
  }
  else {
    Asc_Panic(2, NULL,
              "error in RelationCalcDerivative: \n"
              "reached end of routine");
    exit(2);/* Needed to keep gcc from whining */
  }
}

enum safe_err
RelationCalcDerivativeSafe(struct Instance *i,
                           unsigned long index,
                           double *gradient)
{
  struct relation *r;
  enum Expr_enum reltype;
  enum safe_err not_safe = safe_ok;

#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivativeSafe: null instance\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( gradient == NULL ) {
    FPRINTF(ASCERR,
            "error in RelationCalcDerivativeSafe: passed null pointer\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivativeSafe: not relation\n");
    not_safe = safe_problem;
    return not_safe;
  }
#endif
  r = (struct relation *)GetInstanceRelation(i, &reltype);
  if( r == NULL ) {
    FPRINTF(ASCERR, "error in RelationCalcDerivativeSafe: null relation\n");
    not_safe = safe_problem;
    return not_safe;
  }
  if( (index < 1) || (index > NumberVariables(r)) ) {
    FPRINTF(ASCERR,
            "error in RelationCalcDerivativeSafe: index out of bounds\n");
    not_safe = safe_problem;
    return not_safe;
  }

  if( reltype == e_token ) {
    *gradient = RelationEvaluateDerivativeSafe(r, index, &not_safe);
    return not_safe;
  }
  else if (reltype >= TOK_REL_TYPE_LOW && reltype <= TOK_REL_TYPE_HIGH) {
    FPRINTF(ASCERR, "error in RelationCalcDerivativeSafe %s\n",
            "reltype not implemented");
    not_safe = safe_problem;
    return not_safe;
  }
  else {
    Asc_Panic(2, NULL,
              "error in RelationCalcDerivativeSafe:\n"
              "reached end of routine");
    exit(2);/* Needed to keep gcc from whining */
  }
}

/**
 *** Function for testing residual and gradient calulations
 **/

void PrintGradients(struct Instance *i)
{
  if (InstanceKind(i) == REL_INST) {
    double res, grads[1000];
    unsigned long vars, v;
    enum Expr_enum type;
    enum safe_err safe;

    vars = NumberVariables((struct relation *)GetInstanceRelation(i,&type));

    /*****  use the non safe versions  *****/
    for( v = 0; v < vars; v++ ) {
      if( ! RelationCalcDerivative(i, v+1, &res) ) {
        PRINTF("derivative in%5ld =\t%g\n", v+1, res);
      }
      else {
        PRINTF("**** RelationCalcDerivative returned nonzero status\n");
      }
    }

    if( ! RelationCalcResidGrad(i,&res,grads) ) {
      for (v = 0; v < vars; v++) {
        PRINTF("gradient in %6ld =\t%g\n", v+1, grads[v]);
      }
      PRINTF("residual from grad =\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidGrad returned nonzero status\n");
    }

    if( !RelationCalcResidualInfix(i,&res) ) {
      PRINTF("    infix residual =\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidualInfix returned nonzero status\n");
    }

    if( !RelationCalcResidualPostfix(i,&res) ) {
      PRINTF("  postfix residual =\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidualPostfix returned nonzero status\n");
    }

    if( !RelationCalcResidualPostfix2(i,&res) ) {
      PRINTF(" postfix2 residual =\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidualPostfix2 returned nonzero status\n");
    }

    /*****  use the safe versions  *****/
    for( v = 0; v < vars; v++ ) {
      if(safe_ok == (safe = RelationCalcDerivativeSafe(i, v+1, &res)) ) {
        PRINTF("safe deriv in%5ld =\t%g\n", v+1, res);
      }
      else {
        PRINTF("**** RelationCalcDerivativeSafe returned nonzero: %d\n", safe);
      }
    }

    if(safe_ok == (safe = RelationCalcResidGradSafe(i,&res,grads)) ) {
      for (v = 0; v < vars; v++) {
        PRINTF("safe grad in%6ld =\t%g\n", v+1, grads[v]);
      }
      PRINTF("safe resid ala grad=\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidGradSafe returned nonzero: %d\n", safe);
    }

  /*****  not implemented
    if( ! (safe = RelationCalcResidualInfixSafe(i,&res)) ) {
      PRINTF("safe infix residual=\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidualInfixSafe returned nonzero: %d\n",
             safe);
    }
  *****/

    if(safe_ok == (safe = RelationCalcResidualPostfixSafe(i,&res)) ) {
      PRINTF("safe postfix resid =\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidualPostfixSafe returned nonzero: %d\n",
             safe);
    }

  /*****  not implemented
    if( ! (safe = RelationCalcResidualPostfix2Safe(i,&res)) ) {
      PRINTF("safe postfix2 resd =\t%g\n", res);
    }
    else {
      PRINTF("**** RelationCalcResidualPostfix2Safe returned nonzero: %d\n",
             safe);
    }
  *****/

    PRINTF("\n");
  }
}
void PrintRelationGradients(struct Instance *i)
{
  VisitInstanceTree(i,PrintGradients, 0, 0);
}

/* this function may make an fpe for method 2 or 3.
 * list must be of nonnull struct relation * for
 * meth = m_BIN and struct Instance * for 1-3.
 */
#define m_BIN 0
#define m_PFS 1
#define m_PF  2
#define m_IF  3
void TimeCalcResidual(struct gl_list_t *rlist,int method)
{
  unsigned long c,len;
  double res;

  if (rlist==NULL) return;
  switch (method) {
  case m_BIN:
    for (c=1,len=gl_length(rlist); c <= len; c++) {
      RelationCalcResidualBinary(gl_fetch(rlist,c),&res);
    }
    break;
  case m_PFS:
    for (c=1,len=gl_length(rlist); c <= len; c++) {
      RelationCalcResidualPostfixSafe(gl_fetch(rlist,c),&res);
    }
    break;
  case m_PF:
    for (c=1,len=gl_length(rlist); c <= len; c++) {
      RelationCalcResidualPostfix(gl_fetch(rlist,c),&res);
    }
    break;
  case m_IF:
    for (c=1,len=gl_length(rlist); c <= len; c++) {
      RelationCalcResidualInfix(gl_fetch(rlist,c),&res);
    }
    break;
  default:
   break;
  }
  return;
}

void PrintResidual(struct Instance *i)
{
  enum safe_err se;
  struct relation *rel;
  enum Expr_enum reltype;
  int errb;
#ifndef M_PI
#define M_PIE 3.141590271828
#else
#define M_PIE M_PI
#endif
  double post=M_PIE,in=M_PIE,postsafe=M_PIE,binary=M_PIE;

  if (InstanceKind(i) == REL_INST) {
    rel = (struct relation *)GetInstanceRelation(i,&reltype);
    if (reltype == e_token) {
      errb = RelationCalcResidualBinary(rel,&(binary));
    } else {
      errb = 1;
    }
    se = RelationCalcResidualPostfixSafe(i,&(postsafe));
    if (errb || se != safe_ok) {
      FPRINTF(ASCERR,"Skipping Postfix,Infix\n");
    } else {
      RelationCalcResidualPostfix(i,&(post));
      RelationCalcResidualInfix(i,&(in));
    }
    PRINTF("binary residual  = %.18g\n",binary);
    PRINTF("postfix safe res = %.18g\n",postsafe);
    if (errb||se!= safe_ok) {
      PRINTF("postfix residual = %.18g\n",post);
      PRINTF("  infix residual = %.18g\n",in);
    }
    if(binary != postsafe) {
      PRINTF("!!!!!!!ERROR!!!!!!! %g \n", binary-post);
    }
    PRINTF("(Unchanged residuals = %.18g\n\n",M_PIE);
  }
}

void PrintRelationResiduals(struct Instance *i)
{
  VisitInstanceTree(i,PrintResidual, 0, 0);
}


/**
 *** The following functions support RelationFindRoots which
 *** is the compiler implementation of our old DirectSolve
 *** function.  These functions can be catagorized as follows:
 *** Memory Management and Copying functions:
 ***      RelationCreateTmp, RelationTmpCopySide,
 ***      RelationTmpTokenCopy, append_soln, remove_soln
 *** Direct Solve Functions:
 ***      InsertBranchResult, SearchEval_Branch, SetUpInvertToken,
 ***      SetUpInvertTokenTop, RelationInvertToken, RelationInvertTokenTop
 *** Rootfinding Functions:
 ***      CalcResidGivenValue, RootFind
 *** Eternal Function:
 ***      RelationFindRoots
 **/

/*************************************************************************/
/****************Memory Management and Copying Functions******************/
/*************************************************************************/

/*
 * RelationCreateTmp creates a struct relation of type e_token
 * and passes back a pointer to the relation.  The lengths of
 * the right and left sides (lhslen and rhslen) of the relation
 * are supplied by the calling function.
 * User is responsible for setting RTOKEN(return).*_len.
 * Basically, all this does is manage memory nicely.
 *
 * IF called with all 0/NULL, frees internal recycles.
 */
static
struct relation *RelationCreateTmp(unsigned long lhslen, unsigned long rhslen,
                                   enum Expr_enum relop)
{
  static struct relation *rel=NULL;
  static unsigned long lhscap=0, rhscap=0;

  /* check for recycle clear and free things if needed. */
  if (lhslen==0 && rhslen == 0 && relop == e_nop) {
    if (rel != NULL) {
      if (rel->share != NULL) {
        if (RTOKEN(rel).lhs!=NULL) {
          ascfree(RTOKEN(rel).lhs);
        }
        if (RTOKEN(rel).rhs!=NULL)  {
          ascfree(RTOKEN(rel).rhs);
        }
        ascfree(rel->share);
      }
      ascfree(rel);
      rel = NULL;
    }
    lhscap = rhscap = 0;
    return NULL;
  }
  if (rel == NULL) {
    rel = CreateRelationStructure(relop,crs_NEWUNION);
  }
  if (lhscap < lhslen) {
    lhscap = lhslen;
    if ( RTOKEN(rel).lhs != NULL) {
      ascfree(RTOKEN(rel).lhs);
    }
    RTOKEN(rel).lhs = (union RelationTermUnion *)
        ascmalloc(lhscap*sizeof(union RelationTermUnion));
  }
  if (rhscap < rhslen) {
    rhscap = rhslen;
    if ( RTOKEN(rel).rhs != NULL) {
      ascfree(RTOKEN(rel).rhs);
    }
    RTOKEN(rel).rhs = (union RelationTermUnion *)
        ascmalloc(rhscap*sizeof(union RelationTermUnion));
  }
  return rel;
}

/**
 *** The following global variables are used thoughout the
 *** functions called by RelationFindroot.
 *** These should probably be located at the top of this
 *** file alonge with glob_rel.
 **/
static unsigned long glob_varnum;
static int glob_done;

/**
 *** The following is documentation from the old exprman
 *** file.  RelationTmpCopySide and RelationTmpCopyToken
 *** are reimplimentations of exprman functions.
 **/
/*
 * We can now just do a memcopy and the infix pointers
 * all adjust by the difference between the token
 * arrays that the gl_lists are hiding. Cool, eh?
 * Note, if any turkey ever tries to delete an individual
 * token from these gl_lists AND deallocate it,
 * they will get a severe headache.
 *
 * This is a full blown copy and not copy by reference.
 * You do not need to remake the infix pointers after
 * calling this function. return 0 if ok, 1 if error.
 */
static int RelationTmpCopySide(union RelationTermUnion *old,
        			unsigned long len,
        			union RelationTermUnion *arr)
{
  struct relation_term *term;
  unsigned long c;
  long int delta;

  if (old==NULL || !len) return 1;
  if (arr==NULL) {
    FPRINTF(ASCERR,"RelationTmpCopySide: null RelationTermUnion :-(.\n");
    return 1;
  }
  memcpy( (VOIDPTR)arr, (VOIDPTR)old, len*sizeof(union RelationTermUnion));
 /*
  *  Difference in chars between old and arr ptrs. It should me a multiple
  *  of sizeof(double) but may not be a multiple of sizeof(union RTU).
  *  Delta may easily be negative.
  *  Normally, though arr > old.
  */
  delta = (char *)arr - (char *)old;
#ifdef ADJPTR
#undef ADJPTR
#endif
#define ADJPTR(p) ( (p) = A_TERM((char *)(p)+delta) )
  for (c=0;c<len;c++) {
    term = A_TERM(&(arr[c]));
    switch (term->t) {
    /* unary terms */
    case e_uminus:
      ADJPTR(U_TERM(term)->left);
      break;
    /* binary terms */
    case e_plus:
    case e_minus: case e_times:
    case e_divide: case e_power: case e_ipower:
      ADJPTR(B_TERM(term)->left);
      ADJPTR(B_TERM(term)->right);
      break;
    case e_zero:
    case e_var:			/* the var number will be correct */
    case e_int:
    case e_real:
      break;
    case e_func:
      ADJPTR(F_TERM(term)->left);
      break;
    /* don't know how to deal with the following relation operators.
       they may be binary or unary, but InfixArr_MakeSide never set them. */
    case e_maximize: case e_minimize:
    case e_equal: case e_notequal: case e_less:
    case e_greater: case e_lesseq: case e_greatereq:
    default:
      Asc_Panic(2, NULL, "Unknown term type in RelationSide\n");
      break;
    }
  }
#undef ADJPTR

  return 0;
}

/*
 * The relation returned by this function should have
 * NO persistent pointers made to it, as it is still
 * our property. The vars in the relation do not
 * know about these references to them, as this is
 * a tmp rel.
 */
static
struct relation *RelationTmpTokenCopy(CONST struct relation *src)
{
  struct relation *result;
  long int delta;
  assert(src!=NULL);

  result = RelationCreateTmp(RTOKEN(src).lhs_len,RTOKEN(src).rhs_len,
                             RelationRelop(src));

  if(RelationTmpCopySide(RTOKEN(src).lhs,RTOKEN(src).lhs_len,
        		RTOKEN(result).lhs) == 0) {
    delta = UNION_TERM(RTOKEN(src).lhs_term) - RTOKEN(src).lhs;
    RTOKEN(result).lhs_term = A_TERM(RTOKEN(result).lhs+delta);
    RTOKEN(result).lhs_len = RTOKEN(src).lhs_len;
  } else {
    RTOKEN(result).lhs_term = NULL;
    RTOKEN(result).lhs_len = 0;
  }

  if( RelationTmpCopySide(RTOKEN(src).rhs,RTOKEN(src).rhs_len,
        		  RTOKEN(result).rhs) == 0) {
    delta = UNION_TERM(RTOKEN(src).rhs_term) - RTOKEN(src).rhs;
    RTOKEN(result).rhs_term = A_TERM(RTOKEN(result).rhs+delta);
    RTOKEN(result).rhs_len = RTOKEN(src).rhs_len;
  } else {
    RTOKEN(result).rhs_term = NULL;
    RTOKEN(result).rhs_len = 0;
  }
  result->vars = src->vars;
  result->d = src->d;
  result->residual = src->residual;
  result->multiplier = src->multiplier;
  result->nominal = src->nominal;
  result->iscond = src->iscond;
  return result;
}

struct ds_soln_list {
   int length,capacity;
   double *soln;
};


#define alloc_array(nelts,type)   \
   ((nelts) > 0 ? (type *)ascmalloc((nelts)*sizeof(type)) : NULL)
#define copy_nums(from,too,nnums)  \
   asc_memcpy((from),(too),(nnums)*sizeof(double))

static 
void append_soln( struct ds_soln_list *sl, double soln)
/**
 ***  Appends the solution onto the solution list
 **/
{
  if( sl->length == sl->capacity ) {
    int newcap;
    double *newlist;

    newcap = sl->capacity + 10;
    newlist = alloc_array(newcap,double);
    copy_nums((char *)sl->soln,(char *)newlist,sl->length);
    if( sl->soln != NULL ) {
      ascfree(sl->soln);
    }
    sl->soln = newlist;
    sl->capacity = newcap;
  }

  sl->soln[sl->length++] = soln;
}

static 
void remove_soln( struct ds_soln_list *sl, int ndx)
/*
 *  Removes solution at given index from solution list.
 */
{
  copy_nums((char *)(sl->soln+ndx+1),
            (char *)(sl->soln+ndx), --(sl->length) - ndx);
}

/*************************************************************************/
/*************************Direct Solve Functions**************************/
/*************************************************************************/

/**
 *** InsertBranchResult changes a relation term type to e_real and
 *** fills the value field of this term.  In subsequent passes of
 *** the RelationBranchEvaluator the term will be considered to
 *** be a leaf.
 */
static void InsertBranchResult(struct relation_term *term, double value)
{
  assert(term!=NULL);
  term->t = e_real;
  R_TERM(term)->value = value;
}

/**
 *** SearchEval_Branch simplifies branches of a relation
 *** (the relation pointed to by glob_rel).  Only terms
 *** of type e_real, e_int, e_zero, and e_var are left
 *** hanging off the operators on the path to the
 *** variable (with varnum = glob_varnum) being direct
 *** solved for.
 *** This may need to be changed to only leave e_reals
 *** so that the inversion routine can make faster decisions???
 *** Probably not.
 ***
 *** Returns >= 1 if glob_varnum spotted, else 0 (or at least <1).
 **/
static int SearchEval_Branch(struct relation_term *term)
{
  int result = 0;
  assert(term != NULL);
  switch(RelationTermType(term)) {
  case e_var:
    if(TermVarNumber(term) == glob_varnum) {
        ++glob_done;
        return 1;
    } else {
        return 0;
    }
  case e_func:
    /* if the hold function is accepted, this and the next if
     * should be combined with the fhold condition checked
     * first.
     */
    if (FuncId(TermFunc(term))==F_HOLD) {
      /* The quantity inside a hold is considered a
       * constant, however complicated it may be.
       * We need to call the appropriate evaluator here
       * and return the value. We don't care if we see
       * glob_varnum inside the hold func.
       */
      InsertBranchResult(term,RelationBranchEvaluator(term));
      return 0;
    }
    if(SearchEval_Branch(TermFuncLeft(term)) < 1) {
      InsertBranchResult(term,RelationBranchEvaluator(term));
      return 0;
    }
    return 1;
/* Note that this algorithm could use some work.  Here we go back up the
 * tree only to call relationbranchevaluator to turn these into reals.
 */
  case e_int:
  case e_real:
  case e_zero:
    return 0;

  case e_plus:
  case e_minus:
  case e_times:
  case e_divide:
  case e_power:
  case e_ipower:
    if(SearchEval_Branch(TermBinLeft(term)) < 1) {
      InsertBranchResult(TermBinLeft(term),
                         RelationBranchEvaluator(TermBinLeft(term)));
    } else {
        ++result;
    }
    if(SearchEval_Branch(TermBinRight(term)) < 1) {
      InsertBranchResult(TermBinRight(term),
                         RelationBranchEvaluator(TermBinRight(term)));
    } else {
        ++result;
    }
    if(result == 0){
        InsertBranchResult(term,RelationBranchEvaluator(term));
    }
    return result;

 case e_uminus:
    if(SearchEval_Branch(TermBinLeft(term)) < 1) {
      InsertBranchResult(term,RelationBranchEvaluator(term));
        return 0;
    }
    return 1;
 default:
   Asc_Panic(2, NULL,
             "error in SearchEval_Branch routine\n"
             "relation term type not recognized\n");
    return 1;
  }
}

/**
 *** SetUpInvertToken selects the side of the relation which
 *** will be inverted next.  It also fills the value which is
 *** currently being inverted on.
 *** This function assumes SearchEval_Branch has been called
 *** previously.
 **/
static int SetUpInvertToken(struct relation_term *term,
        	     struct relation_term **invert_side,
        	     double *value)
{
  switch(RelationTermType(term)) {
  case e_uminus:
      *invert_side = TermBinLeft(term);
      return 0;
  case e_func:
      *invert_side = TermFuncLeft(term);
      return 0;
  case e_var:
      assert(TermVarNumber(term)==glob_varnum);
      *invert_side = term;
      return 0; /*could set glob_done here??*/
  default:
      switch(RelationTermType(TermBinRight(term))) {
      case e_real:/*Note: only e_real should be found here:no ints or zeros*/
      case e_int:
      case e_zero:
          *value = RelationBranchEvaluator(TermBinRight(term));
          *invert_side = TermBinLeft(term);
          return 0;
      case e_var:
          if (TermVarNumber(TermBinRight(term)) != glob_varnum) {
              *value = RelationBranchEvaluator(TermBinRight(term));
              *invert_side = TermBinLeft(term);
              return 0;
          }
          break;
      default:
          break;
      }
      *value = RelationBranchEvaluator(TermBinLeft(term));
      *invert_side = TermBinRight(term);
      return 1;
  }
}


static void SetUpInvertTokenTop(struct relation_term **invert_side,
        	     double *value)
{
  switch(RelationTermType(Infix_RhsSide(glob_rel))) {
  case e_real:
  case e_int:
  case e_zero:
      *value = RelationBranchEvaluator(Infix_RhsSide(glob_rel));
      *invert_side = Infix_LhsSide(glob_rel);
      return;
  case e_var:
      if (TermVarNumber(Infix_RhsSide(glob_rel)) != glob_varnum) {
          *value = RelationBranchEvaluator(Infix_RhsSide(glob_rel));
          *invert_side = Infix_LhsSide(glob_rel);
          return;
      }
      break;
  default:
      break;
  }
  *value = RelationBranchEvaluator(Infix_LhsSide(glob_rel));
  *invert_side = Infix_RhsSide(glob_rel);
  return;
}

/**
 *** RelationInvertToken inverts tokens until the variable
 *** being solved for is found.  It is assumed that this
 *** variable only resides at ONE leaf of the relation tree.
 *** It is the calling function's responsibility to make sure
 *** this is the case and call another solver if needed.
 *** If the variable (with varnum = glob_varnum) is found,
 *** the solution list will contain all solutions to the
 *** equations.  It is the calling function's responsibility
 *** to select the root that suits his needs.
 *** Returns TRUE for success, FALSE for failure.
 **/

/* Note that there appears to be some redundant checking here and
 * we could probably be more efficient
 */
int RelationInvertToken(struct relation_term **term,
        		struct ds_soln_list *soln_list,
        		enum safe_err *not_safe)
{
  int side,ndx;
  double value = 0.0;
  struct relation_term *invert_side;
  assert(term!=NULL);
  side = SetUpInvertToken(*term,&(invert_side),&value);
  for( ndx = soln_list->length ; --ndx >= 0 ; ) {
    switch(RelationTermType(*term)) {
    case e_plus:
      soln_list->soln[ndx] -= value;
      break;
    case e_minus:
      if(side == 0) {
        soln_list->soln[ndx] += value;
      } else {
        soln_list->soln[ndx] = value - soln_list->soln[ndx];
      }
      break;
    case e_times:
      if(soln_list->soln[ndx] == 0.0 && value == 0.0)
        return(FALSE);
      soln_list->soln[ndx] =
        safe_div_D0(soln_list->soln[ndx],value,not_safe);
      if( *not_safe != safe_ok) {
        remove_soln(soln_list,ndx);
      }
      break;
    case e_divide:
      if(side == 0) {
        soln_list->soln[ndx] =
          safe_mul_D0(soln_list->soln[ndx],value,not_safe);
        if( *not_safe != safe_ok || value == 0.0) {
          remove_soln(soln_list,ndx);
        }
      } else {
        if(value == 0.0 && soln_list->soln[ndx] == 0.0 )
          return(FALSE);
        soln_list->soln[ndx] =
          safe_div_D0(value,soln_list->soln[ndx], not_safe);
        if( *not_safe != safe_ok|| soln_list->soln[ndx] == 0.0 ) {
          remove_soln(soln_list,ndx);
        }
      }
      break;
    case e_uminus:
      soln_list->soln[ndx] = -soln_list->soln[ndx];
      break;
    case e_func:
      switch(FuncId(TermFunc(*term))) {
      case F_EXP:
        soln_list->soln[ndx] = safe_ln_D0(soln_list->soln[ndx],not_safe);
        if( *not_safe != safe_ok ) {
          remove_soln(soln_list,ndx);
        }
        break;
      case F_LOG10:
        soln_list->soln[ndx] *= 1.0/safe_LOG10_COEF;

      case F_LN:   /* FALL-THROUGH */
        soln_list->soln[ndx] = safe_exp_D0(soln_list->soln[ndx],not_safe);
        if( *not_safe != safe_ok ) {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_LNM:
        soln_list->soln[ndx] = safe_lnm_inv(soln_list->soln[ndx],not_safe);
        if( *not_safe != safe_ok ) {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_SQR:
        if( soln_list->soln[ndx] == 0.0 )
          break;   /* No change */
        soln_list->soln[ndx] =
          safe_sqrt_D0( soln_list->soln[ndx],not_safe );
        if( *not_safe == safe_ok ) {
          append_soln(soln_list, -soln_list->soln[ndx]);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ABS:
        if( soln_list->soln[ndx] == 0.0 )
          break;   /* No change */
        if( soln_list->soln[ndx] < 0.0 ) {/* impossible */
          remove_soln(soln_list,ndx);
          break;
        }
        /* soln_list->soln[ndx] = soln_list->soln[ndx]; already there */
        append_soln(soln_list, -soln_list->soln[ndx]);
        break;

      case F_HOLD:
        /* soln_list->soln[ndx] = soln_list->soln[ndx]; already there */
        break;

      case F_SQRT:
        if( soln_list->soln[ndx] < 0.0 ||
            (soln_list->soln[ndx] =
             safe_sqr_D0(soln_list->soln[ndx],not_safe),
             *not_safe != safe_ok) ) {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ARCCOS:
        if( 0.0 <= soln_list->soln[ndx] &&
            soln_list->soln[ndx] <= safe_PI ) {
          soln_list->soln[ndx] =
            safe_cos_D0(soln_list->soln[ndx],not_safe);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ARCCOSH:
        if( soln_list->soln[ndx] < 1.0 ||
            (soln_list->soln[ndx] =
             safe_cosh_D0(soln_list->soln[ndx],not_safe),
             *not_safe != safe_ok) ) {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ARCSIN:
        if( -safe_PI/2.0 <= soln_list->soln[ndx] &&
            soln_list->soln[ndx] <= safe_PI/2.0 ) {
          soln_list->soln[ndx] =
            safe_sin_D0(soln_list->soln[ndx],not_safe);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ARCSINH:
        soln_list->soln[ndx] =
          safe_sinh_D0(soln_list->soln[ndx],not_safe);
        if( *not_safe != safe_ok) {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ARCTAN:
        if( -safe_PI/2.0 < soln_list->soln[ndx] &&
            soln_list->soln[ndx] < safe_PI/2.0 ) {
          soln_list->soln[ndx] =
            safe_tan_D0(soln_list->soln[ndx],not_safe);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_ARCTANH:
        if( -1.0 < soln_list->soln[ndx] && soln_list->soln[ndx] < 1.0 ) {
          soln_list->soln[ndx] =
            safe_tanh_D0(soln_list->soln[ndx],not_safe);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_COS:
      case F_SIN:
        if( -1.0 <= soln_list->soln[ndx] && soln_list->soln[ndx] <= 1.0 ) {
          return(FALSE);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_TAN:
        return(FALSE);

      case F_COSH:
        soln_list->soln[ndx] =
          safe_arccosh_D0( soln_list->soln[ndx],not_safe );
        if( *not_safe == safe_ok ) {
          append_soln(soln_list, -soln_list->soln[ndx]);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_SINH:
        soln_list->soln[ndx] =
          safe_arcsinh_D0( soln_list->soln[ndx],not_safe );
        if( *not_safe != safe_ok) {
          remove_soln(soln_list,ndx);
        }
        break;

      case F_TANH:
        soln_list->soln[ndx] =
          safe_arctanh_D0( soln_list->soln[ndx],not_safe );
        if( *not_safe != safe_ok) {
          remove_soln(soln_list,ndx);
        }
        break;

#ifdef HAVE_ERF
      case F_ERF:
        if( -1.0 < soln_list->soln[ndx] && soln_list->soln[ndx] < 1.0 ) {
          soln_list->soln[ndx] =
            safe_erf_inv(soln_list->soln[ndx],not_safe);
        } else {
          remove_soln(soln_list,ndx);
        }
        break;
#endif /* HAVE_ERF */

      case F_CBRT:
        soln_list->soln[ndx] = safe_cube(soln_list->soln[ndx],not_safe);
        if( *not_safe != safe_ok) {
          remove_soln(soln_list,ndx);
        }
        break;

      default:
        FPRINTF(stderr,"ERROR:  (karlexpr) invert_token\n");
        FPRINTF(stderr,"        Unknown function %s.\n",
                FuncName(TermFunc(*term)));
        return(FALSE);


      } /* end switch */
      break;

    case e_power:
    case e_ipower:
      if( side == 0 ) {  /*** Solve x^c = y ***/
        if( value == 0.0 )
          return(FALSE);
        switch( safe_is_int(value,not_safe) ) {
        case 0:   /* Even integer */
          soln_list->soln[ndx] =
            safe_pow_D0(soln_list->soln[ndx],1.0/value,not_safe);
          if( *not_safe == safe_ok ) {
            append_soln(soln_list,-soln_list->soln[ndx]);
          }
          /* else solution removed later */
          break;

        case 1:   /* Odd integer */
          soln_list->soln[ndx] = (soln_list->soln[ndx] >= 0.0)
            ? safe_pow_D0(soln_list->soln[ndx],1.0/value,not_safe)
            : -safe_pow_D0(-soln_list->soln[ndx],1.0/value,not_safe);
          break;

        default:   /* Not an integer */
          soln_list->soln[ndx] =
            safe_pow_D0(soln_list->soln[ndx],1.0/value,not_safe);
          break;
        }
      } else {   /*** Solve c^x = y ***/
        if( value == 0.0 || value == 1.0 )
          return(FALSE);
        soln_list->soln[ndx] =
          safe_div_D0(safe_ln_D0(soln_list->soln[ndx],not_safe),
                      safe_ln_D0(value,not_safe),not_safe);
      }

      if( *not_safe != safe_ok ) {
        remove_soln(soln_list,ndx);
      }
      break;

    case e_real:
    case e_zero:
    case e_int:
      FPRINTF(ASCERR,
              "unexpected error: please notify kt2g@andrew.cmu.edu\n");
      break;
    case e_var:
      ++glob_done;
      return(TRUE);  /*solution found*/

      /* don't know how to deal with the following relation operators.
         they may be binary or unary, but InfixArr_MakeSide never set them. */
    case e_maximize: case e_minimize:
    case e_equal: case e_notequal: case e_less:
    case e_greater: case e_lesseq: case e_greatereq:
    default:
      Asc_Panic(2, NULL, "Unknown term type in RelationInvertToken\n");
      break;
    }
  }
  if(soln_list->length == 0) {
    return(FALSE);
  }
  *term = invert_side;
  return(TRUE);
}

/*
 * RelationInvertTokenTop is baisically a while loop which
 * calls RelationInvertToken.  See RelationInvertToken for
 * information on what this function does.
 */
int RelationInvertTokenTop(struct ds_soln_list *soln_list)
{
  int result;
  struct relation_term *invert_side;
  enum safe_err not_safe = safe_ok;

  assert(glob_rel!=NULL);
  assert(Infix_LhsSide(glob_rel)!=NULL && Infix_RhsSide(glob_rel)!=NULL);


  SetUpInvertTokenTop(&(invert_side),&(soln_list->soln[0]));
  result = 1;
  while(glob_done < 1 && result != 0) {
    result = RelationInvertToken(&(invert_side),soln_list,&not_safe);
  }
  return result;
}

/*************************************************************************/
/*************************Rootfinding Functions***************************/
/*************************************************************************/

/*
 *  --- glob_rel is ASSUMED to be of type e_token. ---
 *  CalcResidGivenValue sets the value of the variable being solved
 *  for (given the varnum) and calculates the residual.  Note that
 *  this function uses the glob_rel which should have been set in
 *  RelationFindRoots (and reduced by SearchEval_Branch).  This
 *  functions takes an excessive number of arguments so it will
 *  look like an ExtEvalFunc to our rootfinder.
 */
static
int CalcResidGivenValue(int *mode, int *m, unsigned long *varnum,
        		double *val, double *u, double *f, double *g)
{
  double res;
  /*  glob_rel = rel;*/
  /*
   *  glob_rel is ASSUMED to be of type e_token.
   */

  (void)mode;  /*  stop gcc whine about unused parameter  */
  (void)u;     /*  stop gcc whine about unused parameter  */
  (void)g;     /*  stop gcc whine about unused parameter  */

  SetRealAtomValue(
      ((struct Instance *)gl_fetch(RelationVarList(glob_rel),*varnum)),
      val[*varnum],
      0
  );
  if (RelationRelop(glob_rel) != e_equal) {
    FPRINTF(ASCERR,"CalcResidGivenValue called with non-equality");
    return 1;
  }
 /* ought to Call GetInstanceRelationRelop here to keep out the boxes.
  * may need to set inst ptr. 
  */

  if(Infix_LhsSide(glob_rel) != NULL) {
    res = RelationBranchEvaluator(Infix_LhsSide(glob_rel));
  } else {
    res = 0.0;
  }
  if(Infix_RhsSide(glob_rel) != NULL) {
    res -= RelationBranchEvaluator(Infix_RhsSide(glob_rel));
  }
  f[*m] = res;
  return 0;
}

/*
 *  RootFind is a distributor to a rootfinding method zbrent.
 *  at present, the rootfind can only handle token relations.
 * if varnum is 0 and status is NULL, frees the internal
 * memory recycle and returns. Currently this is slaved
 * to RelationFindRoots internal reset.
 * This function is not threadsafe.
 */
static
double RootFind(struct relation *rel,
        	double *lower_bound,
        	double *upper_bound,
        	double *nominal,
        	double *tolerance,
        	unsigned long varnum,
                int *status)
{
  double *f = NULL;	/* vector of residuals, borrowed from tmpalloc */
  ExtEvalFunc *func;
  int mode;             /* to pass to the eval func */
  int m = 0;            /* the relation index */ /*a dummy var*/
  int n;                /* the variable index */
  double *x;            /* the x vector -- needed by eval func */
  double *u = NULL;     /* the u vector -- needed by eval func, in which
                         * case it is in trouble,eh?
                         */
  double *g;            /* vector of gradients. part of f malloc */
  int j,fcap;
  struct Instance *var;
  CONST struct gl_list_t *vlist;

  (void)nominal;        /* stop gcc whine about unused parameter */
  if (status==NULL && varnum == 0) {
    return 0.0;
  }

  vlist = RelationVarList(glob_rel);
  n = (int)gl_length(vlist);
  fcap = 2 * n + 1;
  f = tmpalloc_array(fcap,double);
  for (j=0;j < fcap; j++) {
    f[j] = 0.0;
  }
  x = &f[1];
  g = &f[n+1];

  /* This is wasteful: only one is used */
  for (j=0;j<n;j++) {
    var = (struct Instance *)gl_fetch(vlist,(unsigned long)(j+1));
    x[j] = RealAtomValue(var);
  }
  n = (int)varnum;

  /*
   * Get the evaluation function.
   */
  func = (ExtEvalFunc *)CalcResidGivenValue;
  glob_rel = rel;
  return zbrent(func,lower_bound,upper_bound,&(mode),
                &(m),&(n),x,u,f,g,tolerance,status);
}

/*************************************************************************/
/*************************External Function******************************/
/*************************************************************************/

/**
 *** RelationFindRoot WILL find a root if there is one. It is
 *** in charge of trying every trick in the book. It returns
 *** 1 for success and 0 for failure. In general compiler functions
 *** return 0 for success but this function returns 1 for success
 *** because success = 1 is the convention on the solver side.
 *** (we really should make a system wide convention)
 *** A return of -1 indicates a problem such as var not found.
 *** If nsolns > 1 then a list of solutions will be returned.
 **/

/* Note we should recycle the memory used for glob_rel */
double *RelationFindRoots(struct Instance *i,
        		  double lower_bound,
        		  double upper_bound,
        		  double nominal,
        		  double tolerance,
        		  unsigned long *varnum,
        		  int *able,
        		  int *nsolns)
{
  struct relation *rel;
  double sideval;
  enum Expr_enum reltype;
  static struct ds_soln_list soln_list = {0,0,NULL};
  CONST struct gl_list_t *list;

  /* check for recycle shutdown */
  if (i==NULL && varnum == NULL && able == NULL && nsolns == NULL) {
    if (soln_list.soln != NULL) {
      ascfree(soln_list.soln);
      soln_list.soln = NULL;
      soln_list.length = soln_list.capacity = 0;
    }
    RootFind(NULL,NULL,NULL,NULL,NULL,0L,NULL); /*clear brent recycle */
    RelationCreateTmp(0,0,e_nop); /* clear tmprelation recycle */
    return NULL;
  } 
  /* check assertions */
#ifndef NDEBUG
  if( i == NULL ) {
    FPRINTF(ASCERR, "error in RelationFindRoot: NULL instance\n");
    glob_rel = NULL;
    return NULL;
  }
  if (able == NULL){
    FPRINTF(ASCERR,"error in RelationFindRoot: NULL able ptr\n");
    glob_rel = NULL;
    return NULL;
  }
  if (varnum == NULL){
    FPRINTF(ASCERR,"error in RelationFindRoot: NULL varnum\n");
    glob_rel = NULL; 
    return NULL;
  }
  if( InstanceKind(i) != REL_INST ) {
    FPRINTF(ASCERR, "error in RelationFindRoot: not relation\n");
    glob_rel = NULL;
    return NULL;
  }
#endif

  *able = FALSE;
  *nsolns = -1;     /* nsolns will be -1 for a very unhappy rootfinder */
  glob_rel = NULL;
  glob_done = 0;
  soln_list.length = 0; /* reset len to 0. if NULL to start, append mallocs */
  append_soln(&soln_list,0.0);
  rel = (struct relation *)GetInstanceRelation(i, &reltype);
  if( rel == NULL ) {
    FPRINTF(ASCERR, "error in RelationFindRoot: NULL relation\n");
    glob_rel = NULL; return NULL;
  }
  /* here we should switch and handle all types. at present we don't
   * handle anything except e_token
   */
  if( reltype != e_token ) {
    FPRINTF(ASCERR, "error in RelationFindRoot: non-token relation\n");
    glob_rel = NULL; 
    return NULL;
  }

  if (RelationRelop(rel) == e_equal){ 
    glob_rel = RelationTmpTokenCopy(rel);
    assert(glob_rel!=NULL);
    glob_done = 0;
    list = RelationVarList(glob_rel);
    if( *varnum >= 1 && *varnum <= gl_length(list)){
      glob_done = 1;
    }
    if (!glob_done) {
      FPRINTF(ASCERR, "error in FindRoot: var not found\n");
      glob_rel = NULL;
      return NULL;
    }

    glob_varnum = *varnum;
    glob_done = 0;
    assert(Infix_LhsSide(glob_rel) != NULL);
    /* In the following if statements we look for the target variable
     * to the left and right, evaluating all branches without the
     * target.
     */
    if (SearchEval_Branch(Infix_LhsSide(glob_rel)) < 1) {
      sideval = RelationBranchEvaluator(Infix_LhsSide(glob_rel));
      if (finite(sideval)) {
        InsertBranchResult(Infix_LhsSide(glob_rel),sideval);
      } else {
        FPRINTF(ASCERR,"Inequality in RelationFindRoots. Infinite RHS.\n");
        glob_rel = NULL;
        return NULL;
      }
    }
    assert(Infix_RhsSide(glob_rel) != NULL);
    if (SearchEval_Branch(Infix_RhsSide(glob_rel)) < 1) {
        sideval = RelationBranchEvaluator(Infix_RhsSide(glob_rel));
        if (finite(sideval)) {
          InsertBranchResult(Infix_RhsSide(glob_rel),sideval);
        } else {
          FPRINTF(ASCERR,"Inequality in RelationFindRoots. Infinite LHS.\n");
          glob_rel = NULL;
          return NULL;
        }
    }
    if (glob_done < 1) {
      /* RelationInvertToken never found variable */
      glob_done = 0;
      *able = FALSE;
      return soln_list.soln;
    }
    if (glob_done == 1) {
      /* set to 0 so while loop in RelationInvertToken will work */
      glob_done = 0; 
      glob_done = RelationInvertTokenTop(&(soln_list));
    }
    if (glob_done == 1) { /* if still one, token inversions successful */
      glob_done = 0;
      *nsolns= soln_list.length;
      *able = TRUE;
      return soln_list.soln;
    }
    /* CALL ITERATIVE SOLVER */
    *soln_list.soln = RootFind(glob_rel,&(lower_bound),
        		       &(upper_bound),&(nominal),
        		       &(tolerance),
        		       glob_varnum,able);

    glob_done = 0;
    if(*able == 0) { /* Root-Find returns 0 for success*/
      *nsolns = 1;
      *able = TRUE;
    } else {
      *able = FALSE;
    }
    return soln_list.soln;

  }
  FPRINTF(ASCERR,"Inequality in RelationFindRoots. can't find roots.\n");
  *able = FALSE;
  return soln_list.soln;
}




/*
 *  Temporary Functions for testing direct solve.
 *  Remove calls from interface.c when this is removed.
 */
void PrintDirectResult(struct Instance *i)
{
  struct relation *rel;
  enum Expr_enum reltype;
  int num,status,n,nsoln;
  double *soln_list,tolerance = 1e-7;
  unsigned long varnum;
  CONST struct gl_list_t *list;

  if (InstanceKind(i) == REL_INST) {
       rel = (struct relation *)GetInstanceRelation(i, &reltype);
       if (reltype == e_token) {
           list = RelationVarList(rel);
           for(num = 1; num <= (int)gl_length(list);++num) {
               FPRINTF(stderr,"VAR NUMBER %d\n",num);
               status = -1;
               varnum = num;
               soln_list = RelationFindRoots(i,-100,100,1,tolerance,&(varnum),
        				     &(status),&(nsoln));
               for(n = nsoln;n > 0;--n) {
        	   FPRINTF(stderr,"SOLUTION = %g\n",soln_list[n-1]);
               }
           }
       }
   }
}

void PrintDirectSolveSolutions(struct Instance *i)
{
  VisitInstanceTree(i,PrintDirectResult, 0, 0);
  /* reset internal memory recycle */
  RelationFindRoots(NULL,0,0,0,0,NULL,NULL,NULL);
}

struct ctrwubs {
  struct gl_list_t *list;
  unsigned long maxlen;
  int overflowed;
};

static
void CollectShares(struct Instance *i,struct ctrwubs *data)
{
  struct relation *r;
  /* If i ok and relation and token type and built and share not 
   * previously built binary or collected, collect it.
   * Previously built shared have a btable > 0 and < INT_MAX.
   * Collected relations have btable = INT_MAX.
   */
  if (gl_length(data->list) <= data->maxlen) {
    if (i!=NULL && 
        InstanceKind(i) == REL_INST &&
        GetInstanceRelationType(i) == e_token &&
        (r = (struct relation *)GetInstanceRelationOnly(i)) != NULL &&
        RTOKEN(r).btable == 0) {
      gl_append_ptr(data->list,i);
      RTOKEN(r).btable = INT_MAX;
    }
  } else {
    data->overflowed = 1;
  }
}

struct gl_list_t *
CollectTokenRelationsWithUniqueBINlessShares(struct Instance *i,
                                             unsigned long maxlen)
{
  struct ctrwubs data;
  struct Instance *rel;
  struct relation *r;
  unsigned long c;
  data.list = gl_create(maxlen);
  data.overflowed = 0;
  data.maxlen = maxlen;
  if (data.list == NULL) {
    return NULL;
  }
  SilentVisitInstanceTreeTwo(i,(VisitTwoProc)CollectShares,1,0,(void *)&data);   
  if (data.overflowed) {
    for (c = gl_length(data.list); c >0; c--) {
      rel = (struct Instance *)gl_fetch(data.list,c);
      r = (struct relation *)GetInstanceRelationOnly(rel);
      RTOKEN(r).btable = 0;
    }
    return NULL;
  } else {
    return data.list;
  }
}

/*
 *  Utility functions for Ascend
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: old_utils.c,v $
 *  Date last modified: $Date: 1998/01/29 01:04:07 $
 *  Last modified by: $Author: ballan $
 *  Part of Ascend
 *
 *  This file is part of the Ascend Programming System.
 *
 *  Copyright (C) 1990 Thomas Guthrie Epperly, Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph James Zaher
 *  Copyright (C) 1993, 1994 Benjamin Andrew Allan, Joseph James Zaher
 *
 *  The Ascend Programming System is free software; you can redistribute
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
 *  This module defines the dimensionality checking and some other auxillaries
 *  for Ascend.
 */

#define ASC_BUILDING_INTERFACE

#include <ctype.h>
#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <general/list.h>
#include <general/dstring.h>
#include <compiler/compiler.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/expr_types.h>
#include <compiler/sets.h>
#include <compiler/instance_enum.h>
#include <compiler/instance_name.h>
#include <compiler/atomvalue.h>
#include <compiler/parentchild.h>
#include <compiler/symtab.h>
#include <compiler/instance_io.h>
#include <compiler/safe.h>
#include <compiler/interval.h>
#include <compiler/func.h>
#include <compiler/extfunc.h>
#include <compiler/extcall.h>
#include <compiler/setinstval.h>
#include <compiler/exprs.h>
#include <compiler/value_type.h>
#include <compiler/find.h>
#include <compiler/relation_type.h>
#include <compiler/rel_blackbox.h>
#include <compiler/vlist.h>
#include <compiler/relation.h>
#include <compiler/relation_util.h>
#include "old_utils.h"

/*int g_check_dimensions_noisy=1;*/
#define GCDN g_check_dimensions_noisy

static char *write_char(char *str, char *slim, int c)
{
   if( str < slim ) *(str++) = c;
   return(str);
}

static char *write_string(char *str, char *slim, char *s)
{
   for( ; *s != '\0' ; ++s )
      str = write_char(str,slim,*s);
   return(str);
}

#define SIZE_INCREMENT 16
#define ROOM_FOR_INT  11    /* a 32-bit int, that is */
char *asc_make_dimensions(const dim_type *dim)
{
   boolean first;
   char *dimens;
   char *str, *slim;
   int size = 0;

   if( IsWild(dim) ) {
      dimens = ASC_NEW_ARRAY(char,2);
      sprintf(dimens,"*");
      return( dimens );
   }

   str = slim = dimens = NULL;
   while( str==slim ) {
      int n;
      size += SIZE_INCREMENT;
      if( dimens != NULL ) ascfree(dimens);
      str = dimens = ASC_NEW_ARRAY(char,size);
      slim = str + size;
      first = TRUE;
      for( n=0 ; n<NUM_DIMENS ; ++n ) {
  struct fraction frac;
  frac = GetDimFraction(*dim,n);
  if( Numerator(frac) == 0 )
     continue;

  if( !first ) str = write_string(str, slim, " * ");
  str = write_string(str, slim, DimName(n));
  if( Denominator(frac) == 1 ) {
     if( Numerator(frac) != 1 ) {
               char buf[ROOM_FOR_INT];
        sprintf(buf,"%d",(int)Numerator(frac));
        str = write_char(str, slim, '^');
        str = write_string(str, slim, buf);
     }
  } else {
            char buf[ROOM_FOR_INT];
     str = write_string(str, slim, "^(");
     sprintf(buf,"%d",(int)Numerator(frac));
     str = write_string(str, slim, buf);
     str = write_char(str, slim, '/');
     sprintf(buf,"%d",(int)Denominator(frac));
     str = write_string(str, slim, buf);
     str = write_char(str, slim, ')');
  }
  first = FALSE;
      }
   }
   *str = '\0';
   return(dimens);
}
#undef ROOM_FOR_INT
#undef SIZE_INCREMENT

#ifdef THIS_MAY_BE_UNUSED_CODE
/* commenting out unused functions  mthomas.96.09.20 */
/* dim checking stuff invokable at any time. */
static double frac_to_real(struct fraction frac)
{
   return( (double)Numerator(frac) / (double)Denominator(frac) );
}
#endif

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


static int nargs(enum Expr_enum type)
{
   switch(type) {
   case e_int:
   case e_real:
   case e_var:
   case e_zero:
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
      FPRINTF(stderr,"Unknown relation term type.\n");
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
   if( node->type==e_real && node->real_const==0.0 )
      return TRUE;
   return FALSE;
}


static void apply_term_dimensions(CONST struct relation *rel,
                                  struct relation_term *rt,
                                  struct dimnode *first,
                                  struct dimnode *second,
                                  boolean *con,             /* consistent ? */
                                  boolean *wild)            /* wild ? */
{
  enum Expr_enum type;

  switch(type=RelationTermType(rt)) {
  case e_int:
    CopyDimensions(Dimensionless(),&(first->d));
    first->int_const = (short)TermInteger(rt);
    first->type = type;
    break;

  case e_zero:
    CopyDimensions(Dimensionless(),&(first->d));
    first->real_const = TermReal(rt);
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

    case F_HOLD:
    case F_ABS:
      /*  For abs, there is nothing to do, so just break out.
       *  assuming first->d is o.k.
       */
      break;

    case F_EXP:
    case F_LN:
    case F_LNM:
    case F_LOG10:
#ifdef HAVE_ERF
	case F_ERF:
#endif
    case F_SINH:
    case F_COSH:
    case F_TANH:
    case F_ARCSINH:
    case F_ARCCOSH:
    case F_ARCTANH:
      /*
       *  first must now be dimensionless.  It will
       *  end up dimensionless as well.
       */
      if( IsWild(&(first->d)) && !IsZero(first) ) {
        char *name = (char *)FuncName(TermFunc(rt));
        if( !*wild ) *wild = TRUE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(stderr,"        in function %s.\n",name);
        }
      } else if( !IsWild(&(first->d)) &&
                 CmpDimen(&(first->d),Dimensionless()) ) {
        char *name = (char *)FuncName(TermFunc(rt));
        char *dimstring = asc_make_dimensions(&(first->d));
        if( *con ) *con = FALSE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Function %s called\n",name);
          FPRINTF(stderr,"        with dimensions %s.\n",dimstring);
        }
        ascfree(dimstring);
      }
      CopyDimensions(Dimensionless(),&(first->d));
      break;

    case F_SIN:
    case F_COS:
    case F_TAN: {
      /*
       *  first must now be of dimension D_PLANE_ANGLE.
       *  It will then be made dimensionless.
       */
      if( IsWild(&(first->d)) && !IsZero(first) ) {
        char *name = (char *)FuncName(TermFunc(rt));
        if( !*wild ) *wild = TRUE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(stderr,"        in function %s.\n",name);
        }
      } else if( !IsWild(&(first->d)) &&
                 CmpDimen(&(first->d),TrigDimension()) ) {
        char *dimstring = asc_make_dimensions(&(first->d));
        char *name = (char *)FuncName(TermFunc(rt));
        if( *con ) *con = FALSE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Function %s called with\n",name);
          FPRINTF(stderr,"        dimensions %s.\n",dimstring);
        }
        ascfree(dimstring);
      }
      CopyDimensions(Dimensionless(),&(first->d));
      break;
    }

    case F_ARCSIN:
    case F_ARCCOS:
    case F_ARCTAN:
      /*
       *  first must now be dimensionless.  It will
       *  end up with dimension D_PLANE_ANGLE
       */
      if( IsWild(&(first->d)) && !IsZero(first) ) {
        char *name = (char *)FuncName(TermFunc(rt));
        if( !*wild ) *wild = TRUE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(stderr,"        in function %s.\n",name);
        }
      } else if( !IsWild(&(first->d)) &&
                 CmpDimen(&(first->d),Dimensionless()) ) {
        char *dimstring = asc_make_dimensions(&(first->d));
        char *name =(char *) FuncName(TermFunc(rt));
        if( *con ) *con = FALSE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Function %s called with\n",name);
          FPRINTF(stderr,"        dimensions %s.\n",dimstring);
        }
        ascfree(dimstring);
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

  case e_power:
  case e_ipower:
    if( IsWild(&(second->d)) && !IsZero(second) ) {
      if( !*wild ) *wild = TRUE;
      if (GCDN) {
        FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
        FPRINTF(stderr,"        in exponent.\n");
      }
    } else if( !IsWild(&(second->d)) &&
               CmpDimen(&(second->d),Dimensionless()) ) {
      char *dimstring = asc_make_dimensions(&(second->d));
      if( *con ) *con = FALSE;
      if (GCDN) {
        FPRINTF(stderr,"ERROR:  Exponent has dimensions %s.\n",
                dimstring);
      }
      ascfree(dimstring);
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

    default:
      if( IsWild(&(first->d)) && !IsZero(first) ) {
        if( !*wild ) *wild = TRUE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(stderr,"        raised to a non-constant power.\n");
        }
      } else if( !IsWild(&(first->d)) &&
                 CmpDimen(&(first->d),Dimensionless()) ) {
        char *dimstring = asc_make_dimensions(&(first->d));
        if( *con ) *con = FALSE;
        if (GCDN) {
          FPRINTF(stderr,
                  "ERROR:  Dimensions %s are\n",dimstring);
          FPRINTF(stderr,
                  "        raised to a non-constant power.\n");
        }
        ascfree(dimstring);
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
          FPRINTF(stderr,"ERROR:  %s has wild dimensions on\n",
                  type==e_plus ? "Addition":"Subtraction");
          FPRINTF(stderr,"        left and right hand sides.\n");
        }
        first->type = type;
      } else if( !IsWild(&(second->d)) ) {
        /* second not wild */
        if( !*wild ) *wild = TRUE;
        if (GCDN) {
          FPRINTF(stderr,"ERROR:  %s has wild dimensions on\n",
                  type==e_plus ? "Addition":"Subtraction");
          FPRINTF(stderr,"        left hand side.\n");
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
          FPRINTF(stderr,"ERROR:  %s has wild dimensions on\n",
                  type==e_plus ? "Addition":"Subtraction");
          FPRINTF(stderr,"        right hand side.\n");
        }
        first->type = type;
      } else if ( !IsWild(&(second->d)) ) {
        /* second not wild */
        if( CmpDimen(&(first->d),&(second->d)) ) {
          char *dimfirst = asc_make_dimensions(&(first->d));
          char *dimsecond = asc_make_dimensions(&(second->d));
          if( *con ) *con = FALSE;
          if (GCDN) {
            FPRINTF(stderr,"ERROR:  %s has dimensions %s on left\n",
                    type==e_plus ? "Addition":"Subtraction",
                    dimfirst);
            FPRINTF(stderr,"        and dimensions %s on right.\n",
                    dimsecond);
          }
          ascfree(dimfirst);
          ascfree(dimsecond);
        }
        first->type = type;
      }
    }
    break;

  default:
    FPRINTF(stderr,"ERROR:  Unknown relation term type.\n");
    if( *con ) *con = FALSE;
    first->type = type;
    break;
  }
}


int asc_check_dimensions(CONST struct relation *rel, dim_type *dimens)
{
   struct dimnode *stack, *sp;
   boolean consistent = TRUE;
   boolean wild = FALSE;
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
            sp += 1-nargs(RelationTermType(rt));
            apply_term_dimensions(rel,rt,sp-1,sp,&consistent,&wild);
         } /* stack[0].d contains the dimensions of the lhs expression */

         /* Now working on the right-hand_side */
         len = RelationLength(rel,FALSE);
         for( c = 1; c <= len; c++ ) {
            struct relation_term *rt;
            rt = (struct relation_term *) RelationTerm(rel,c,FALSE);
            sp += 1-nargs(RelationTermType(rt));
            apply_term_dimensions(rel,rt,sp-1,sp,&consistent,&wild);
         } /* stack[1].d contains the dimensions of the rhs expression */

         if( IsWild(&(stack[0].d)) || IsWild(&(stack[1].d)) ) {
     if( IsWild(&(stack[0].d)) && !IsZero(&(stack[0])) ) {
        if( !wild ) wild = TRUE;
               if (GCDN) {
          FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(stderr,"        on left hand side.\n");
               }
     }
     if( IsWild(&(stack[1].d)) && !IsZero(&(stack[1])) ) {
        if( !wild ) wild = TRUE;
               if (GCDN) {
          FPRINTF(stderr,"ERROR:  Relation has wild dimensions\n");
          FPRINTF(stderr,"        on right hand side.\n");
               }
     }

         } else if( CmpDimen(&(stack[0].d),&(stack[1].d)) ) {
            char *dimfirst = asc_make_dimensions(&(stack[0].d));
            char *dimsecond = asc_make_dimensions(&(stack[1].d));
            if( consistent ) consistent = FALSE;
            if (GCDN) {
              FPRINTF(stderr,"ERROR:  Relation has dimensions %s on left\n",
   	    dimfirst);
              FPRINTF(stderr,"        and dimensions %s on right.\n",
      dimsecond);
            }
            ascfree(dimfirst);
            ascfree(dimsecond);
         }
         break;

      case e_maximize:
      case e_minimize:
         /* Working on the left-hand-side */
         len = RelationLength(rel,TRUE);
         for( c = 1; c <= len; c++ ) {
            struct relation_term *rt;
            rt = (struct relation_term *) RelationTerm(rel,c,TRUE);
            sp += 1-nargs(RelationTermType(rt));
            apply_term_dimensions(rel,rt,sp-1,sp,&consistent,&wild);
         } /* stack[0].d contains the dimensions of the lhs expression */

  if( IsWild(&(stack[0].d)) && !IsZero(&(stack[0])) ) {
     if( !wild ) wild = TRUE;
            if (GCDN) {
       FPRINTF(stderr,"ERROR:  Objective has wild dimensions.\n");
            }
  }
         break;

      default:
         FPRINTF(stderr,"ERROR:  Unknown relation type.\n");
         if( consistent ) consistent = FALSE;
         break;
   }
   CopyDimensions(&(stack[0].d),dimens);
   ascfree(stack);
   return( consistent && !wild );
}

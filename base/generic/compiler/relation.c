/*
 *  Relation  construction  routines
 *  by Tom Epperly
 *  Created: 1/30/90
 *  Version: $Revision: 1.32 $
 *  Version control file: $RCSfile: relation.c,v $
 *  Date last modified: $Date: 1998/03/17 22:09:24 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *  Copyright (C) 1993, 1994, 1995  Kirk Andre' Abbott
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
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 *
 */

#include <math.h>
#include <stdarg.h>
#include "utilities/ascConfig.h"
#include "utilities/ascMalloc.h"
#include "utilities/ascPanic.h"
#include "general/pool.h"
#include "general/list.h"
#include "general/stack.h"
#include "general/dstring.h"
#include "compiler/compiler.h"
#include "compiler/fractions.h"
#include "compiler/dimen.h"
#include "compiler/functype.h"
#include "compiler/func.h"
#include "compiler/types.h"
#include "compiler/name.h"
#include "compiler/nameio.h"
#include "compiler/instance_enum.h"
#include "compiler/bintoken.h"
#include "compiler/exprs.h"
#include "compiler/exprio.h"
#include "compiler/value_type.h"
#include "compiler/evaluate.h"
#include "compiler/forvars.h"
#include "compiler/find.h"
#include "compiler/sets.h"
#include "compiler/setinstval.h"
#include "compiler/instance_io.h"
#include "compiler/extcall.h"
#include "compiler/relation_type.h"
#include "compiler/relation_util.h"
#include "compiler/rel_common.h"
#include "compiler/temp.h"
#include "compiler/atomvalue.h"
#include "compiler/mathinst.h"
#include "compiler/instquery.h"
#include "compiler/tmpnum.h"
#include "compiler/relation.h"

#ifndef lint
static CONST char RelationModRCSid[] =
	 "$Id: relation.c,v 1.32 1998/03/17 22:09:24 ballan Exp $";
#endif

/*
 * internal form of RelationRelop for lval or rval use.
 */
#define RelRelop(r) ((r)->share->s.relop)

#define SUM 1
#define PROD 0
#ifndef abs
#define abs(a) ( ((a)>0) ? (a) : (-(a)) )
#endif

/*
 * Some global and exported variables.
 */
struct gl_list_t *g_relation_var_list = NULL;

int g_simplify_relations = 1;

int g_ExternalNodeStamps=0; /* incremented each time an new external
			   * statement is seen */

/* fwd declaration */
static union RelationTermUnion
*CopyRelationSide(union RelationTermUnion *, unsigned long);

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static
unsigned long ExprLength(register CONST struct Expr *start,
			 register CONST struct Expr *stop)
{
  register unsigned long result=0;
  while(start!=stop){
    start = NextExpr(start);
    result++;
  }
  return result;
}
#endif /* THIS_IS_AN_UNUSED_FUNCTION */


static
void FigureOutError(struct value_t value,
		    enum relation_errors *err,
		    enum find_errors *ferr)
{
  assert(ValueKind(value)==error_value);
  *err = find_error;
  switch(ErrorValue(value)){
  case type_conflict:
  case dimension_conflict:
  case incorrect_name:
  case incorrect_such_that:
  case empty_choice:
  case empty_intersection:
  case temporary_variable_reused:
    *ferr = impossible_instance;
    break;
  case undefined_value:
    *ferr = undefined_instance;
    break;
  case name_unfound:
    *ferr = unmade_instance;
    break;
  default:
    Asc_Panic(2, NULL, "Unknown error type in FigureOutError.\n");
    break;
  }
}


/*********************************************************************\
  Section for creation and management of relation terms.
  It is cheaper to create relation terms in arrays the size of
  the union than individually because of operating system overhead.

  Lookout, the tokens have unionized: next they'll want a raise.
\*********************************************************************/
/*
 * The define POOL_ALLOCTERM is for people who are pulling terms out
 * of a pool and promise to return them immediately.
 */

static pool_store_t g_term_pool = NULL;
/* A pool_store for 1 expression.
 * It is expected that objective functions will cause the
 * largest expressions.
 * Each time an expression is completed, it will be copied
 * into an array which can be created already knowing
 * its proper size. The array will be naturally in postfix.
 */

#define POOL_ALLOCTERM A_TERM(pool_get_element(g_term_pool))
/* get a token. Token is the size of the RelationTermUnion. */
#ifdef NDEBUG
#define PTINIT(x)
#else
#define PTINIT(x) TermUnionInit(x)
#endif
#define POOL_RESET pool_clear_store(g_term_pool)
/* reset the pool for next expression */

#ifndef NDEBUG
/*
 * this function zeros a termunion ptr contents. tu must not be NULL.
 */
static void TermUnionInit(struct relation_term *tu)
{
  memset((char *)tu,0,sizeof(union RelationTermUnion));
}
#endif

static struct {
  long startcheck;
  size_t len;
  size_t cap;
  struct relation_term **buf;
  unsigned long *termstack;
  unsigned long termstackcap;
  long endcheck;
} g_term_ptrs = {1234567890,0,0,NULL,NULL,0,987654321};

#define TPBUF_RESET (g_term_ptrs.len=0)
/* forget about all the terms in the buffer */


/*
 * Now one can ask why a pool and a buffer both? Couldn't one just
 * run a big buffer? Well, yes, but how big? Growing a buffer of
 * complete tokens can cause some system allocators to behave very
 * poorly. Growing a vector of pointers to tokens is much less
 * likely to cause the allocator headaches.
 *
 * The pool has a good growth mechanism and can handle tokens.
 * Tradeoff: it is slower to copy the final token data into a
 * fixed array from pool pointers than from a buffer monolith.
 */
#define TPBUF_INITSIZE 1000
/* initial token buffer capacity */
#define TPBUF_GROW 1000
/* token buffer growth rate */

#define RP_LEN 5
#if (SIZEOF_VOID_P == 8)
#define RP_WID 41
#else
#define RP_WID 63
#endif
/* retune rpwid if the size of tokens changes dramatically */
#define RP_ELT_SIZE (sizeof(union RelationTermUnion))
#define RP_MORE_ELTS 5
/* Number of slots filled if more elements needed.
   So if the pool grows, it grows by RP_MORE_ELTS*RP_WID elements at a time. */
#define RP_MORE_BARS 508
/* This is the number of pool bar slots to add during expansion.
   not all the slots will be filled immediately. */

/* This function is called at compiler startup time and destroy at shutdown.
   One could also recall these every time there is a delete all types. */
void InitRelInstantiator(void) {
  if (g_term_pool != NULL || g_term_ptrs.buf != NULL) {
    Asc_Panic(2, NULL, "ERROR: InitRelInstantiator called twice.\n");
  }
  g_term_pool =
    pool_create_store(RP_LEN, RP_WID, RP_ELT_SIZE, RP_MORE_ELTS, RP_MORE_BARS);
  if (g_term_pool == NULL) {
    Asc_Panic(2, "InitRelInstantiator",
              "ERROR: InitRelInstantiator unable to allocate pool.\n");
  }
  g_term_ptrs.buf = (struct relation_term **)
	asccalloc(TPBUF_INITSIZE,sizeof(union RelationTermUnion *));
  /* don't let the above cast fool you about what's in the array */
  if (g_term_ptrs.buf == NULL) {
    Asc_Panic(2, "InitRelInstantiator",
              "ERROR: InitRelInstantiator unable to allocate memory.\n");
  }
  g_term_ptrs.len = 0;
  g_term_ptrs.cap = TPBUF_INITSIZE;
  g_term_ptrs.termstackcap = 200;
  g_term_ptrs.termstack =
    (unsigned long *)ascmalloc((sizeof(unsigned long)*200));
  if (g_term_ptrs.termstack == NULL) {
    Asc_Panic(2, "InitRelInstantiator",
              "ERROR: InitRelInstantiator unable to allocate memory.\n");
  }
}

/* this function returns NULL when newcap is 0 or when
 * it is unable to allocate the space requested.
 */
static unsigned long *realloc_term_stack(unsigned long newcap){
  if (!newcap) {
    if (g_term_ptrs.termstackcap !=0) {
      ascfree(g_term_ptrs.termstack);
      g_term_ptrs.termstack = NULL;
      g_term_ptrs.termstackcap = 0;
    }
  } else {
    if (newcap >= g_term_ptrs.termstackcap) { /*less than means currently ok */
      unsigned long *newbuf;
      newbuf = (unsigned long *)
        ascrealloc(g_term_ptrs.termstack,(sizeof(unsigned long)*newcap));
      if (newbuf!=NULL) {
        g_term_ptrs.termstack = newbuf;
        g_term_ptrs.termstackcap = newcap;
      } else {
 	FPRINTF(ASCERR,"Insufficient memory in relation processor\n");
        return NULL;
      }
    }
  }
  return g_term_ptrs.termstack;
}

void DestroyRelInstantiator(void) {
  assert(g_term_ptrs.buf!=NULL);
  assert(g_term_pool!=NULL);
  ascfree(g_term_ptrs.buf);
  g_term_ptrs.buf = NULL;
  g_term_ptrs.cap = g_term_ptrs.len = (size_t)0;
  if (g_term_ptrs.termstackcap != 0) {
    ascfree(g_term_ptrs.termstack);
    g_term_ptrs.termstack = NULL;
    g_term_ptrs.termstackcap = 0;
  }
  pool_destroy_store(g_term_pool);
  g_term_pool = NULL;
}

void ReportRelInstantiator(FILE *f)
{
  assert(g_term_pool!=NULL);
  FPRINTF(f,"RelInstantiator ");
  pool_print_store(f,g_term_pool,0);
  FPRINTF(f,"RelInstantiator buffer capacity: %lu\n",
    (unsigned long)g_term_ptrs.cap);
}

/* The slower expansion process. */
static void ExpandTermBuf(struct relation_term *t) {
  struct relation_term **newbuf;
  newbuf = (struct relation_term **)ascrealloc(g_term_ptrs.buf,
      (sizeof(struct relation_term *)*(g_term_ptrs.cap+TPBUF_GROW)));
  if (newbuf!=NULL) {
    g_term_ptrs.buf = newbuf;
    g_term_ptrs.cap += TPBUF_GROW;
    g_term_ptrs.buf[g_term_ptrs.len] = t;
    g_term_ptrs.len++;
  } else {
    FPRINTF(ASCERR,
            "ERROR: Relation Instantiator unable to allocate memory.\n");
    /* we have ignored the term pointer, but somebody else still has it: pool*/
  }
  return;
}

/* Appends term to buffer. if buffer full and can't expand, forgets term.*/
static void AppendTermBuf(struct relation_term *t) {
  if (g_term_ptrs.len < g_term_ptrs.cap) {
    g_term_ptrs.buf[g_term_ptrs.len++] = t;
  } else {
    ExpandTermBuf(t);
  }
  return;
}

/************************************************************************\
 functions to simplify the postfix token list before final creation
 of the token relation array.
\************************************************************************/

/* returns 1 if term is e_zero, e_real=0.0, or e_int=0 */
static int SimplifyTBIsZero(struct relation_term *arg)
{
  if (RelationTermType(arg)==e_real && R_TERM(arg)->value == 0.0) return 1;
  if (RelationTermType(arg)==e_int && I_TERM(arg)->ivalue == 0) return 1;
  if (RelationTermType(arg)==e_zero) return 1;
  return 0;
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
/* check a termtype, t, for scalarness. return 1 if so, 0 otherwise. */
static int SimplifyTBIsScalar(enum Expr_enum t)
{
  return (t <= TOK_SCALAR_HIGH && t >= TOK_SCALAR_LOW);
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */


/* check a termtype, t, for constantness, return 1 if so, 0 otherwise. */
static int SimplifyTBIsConstant(enum Expr_enum t)
{
  return (t <= TOK_CONSTANT_HIGH && t >= TOK_CONSTANT_LOW);
}

#define ZEROTERM(rtp) SimplifyTBIsZero(rtp)
/* check a term pointer, rtp, for scalarness */
#define SCALARTERM(t) SimplifyTBIsScalar(t)
/* check a termtype, t, for scalarness */
#define CONSTANTTERM(t) SimplifyTBIsConstant(t)
/* check a termtype, t, for constantness */

/*
 * Attempt to simplify unary functions.
 * Returns 1 if arg is not constant.
 * Returns 0 if succeeded, in which case *fn is now morphed to a constant term.
 * Returns -1 if arg value/dimens are inconsistent with function fn.
 * Constant arg with numeric value 0 and wild/no dim are coerced quietly
 * where applicable.
 *
 * Cost: O(1).
 */
static int SimplifyTermBuf_Func(struct relation_term *arg,
				struct relation_term *fn)
{
  CONST dim_type *newdim=NULL;
  double rval;
  /* zero constants */
  if (ZEROTERM(arg)) {
    switch(FuncId(F_TERM(fn)->fptr)) {
    case F_LN:
    case F_LOG10:
    case F_ARCCOSH:
      /* illegal argument. caller will whine. */
      return -1;
    case F_EXP:
    case F_COSH:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        arg->t = e_nop;
        fn->t = e_int;
        I_TERM(fn)->ivalue = 1;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_COS:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),TrigDimension())) {
        arg->t = e_nop;
        fn->t = e_int;
        I_TERM(fn)->ivalue = 1;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_SIN:
    case F_TAN:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),TrigDimension())) {
        arg->t = e_nop;
        fn->t = e_int;
        I_TERM(fn)->ivalue = 0;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
#ifdef HAVE_ERF
    case F_ERF:
#endif
    case F_SINH:
    case F_ARCSINH:
    case F_TANH:
    case F_ARCTANH:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        arg->t = e_nop;
        fn->t = e_int;
        I_TERM(fn)->ivalue = 0; /* dimensionless integer 0 */
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_CUBE:
    {
      newdim = CubeDimension(TermDimensions(arg),1);
      if (newdim != NULL) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = 0.0;
        R_TERM(fn)->dimensions = newdim;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    }
    case F_CBRT:
    {
      newdim = ThirdDimension(TermDimensions(arg),1);
      if (newdim != NULL) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = 0.0;
        R_TERM(fn)->dimensions = newdim;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    }
    case F_SQR:
    {
      newdim = SquareDimension(TermDimensions(arg),1);
      if (newdim != NULL) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = 0.0;
        R_TERM(fn)->dimensions = newdim;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    }
    case F_SQRT:
    {
      newdim = HalfDimension(TermDimensions(arg),1);
      if (newdim != NULL) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = 0.0;
        R_TERM(fn)->dimensions = newdim;
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    }
    case F_ARCSIN:
    case F_ARCTAN:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = 0.0;
        R_TERM(fn)->dimensions = TrigDimension();
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_ARCCOS:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = F_PI_HALF;
        R_TERM(fn)->dimensions = TrigDimension();
        return 0;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_ABS:
    case F_HOLD:
    {
      newdim = TermDimensions(arg);
      if (newdim != NULL) {
        arg->t = e_nop;
        fn->t = e_real;
        R_TERM(fn)->value = 0.0;
        R_TERM(fn)->dimensions = newdim;
        return 0;
      } else {
        return -1; /* dimensional insanity */
      }
    }
    case F_LNM:
      return 1; /* user could change lnm epsilon. can't simplify. */
    default:
      FPRINTF(ASCERR,"Unrecognized function in relation.\n");
      return 1;
    }
  }
  /* nonzero int or real */
  if( (arg->t == e_int) || (arg->t == e_real) ) {
    newdim = NULL;
    if (arg->t == e_int) {
      rval = (double)I_TERM(arg)->ivalue;
    } else {
      rval = R_TERM(arg)->value;
    }
    switch(FuncId(F_TERM(fn)->fptr)) {
    /* things that take any trig arg, return dimensionless */
    case F_SIN:
    case F_COS:
    case F_TAN:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),TrigDimension())) {
        newdim = Dimensionless();
      } else {
        return -1; /* dimensional incompatibility */
      }
      break; /* go to fixup */
    /* things that require arg >= 1, return dimless */
    case F_ARCCOSH:
      if( rval < 1.0 ) return -1;
      /* fall through */
    case F_LN:
    case F_LOG10:
      if( rval < 0.0 ) return -1;
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        newdim = Dimensionless();
      } else {
        return -1; /* dimensional incompatibility */
      }
      break; /* go to fixup */
    /* things that take any exponentiable  arg, return dimensionless */
    case F_EXP:
    case F_SINH:
    case F_COSH:
      if (fabs(rval) > F_LIM_EXP) return -1;
      /* fall through */
    /* things that take any arg, return dimensionless */
    case F_ARCSINH:
    case F_TANH:
#ifdef HAVE_ERG
	case F_ERF:
#endif
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        newdim = Dimensionless();
      } else {
        return -1; /* dimensional incompatibility */
      }
      break;
    case F_ARCTANH:
    /* things that take any arg abs <1, return dimensionless */
      if (fabs(rval) < 1.0 && (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless()))) {
        newdim = Dimensionless();
      } else {
        return -1; /* dimensional incompatibility   or range */
      }
      break;
    case F_CUBE:
    {
      newdim = CubeDimension(TermDimensions(arg),1);
      if (newdim == NULL || fabs(rval) > F_LIM_CUBE) {
        return -1; /* dimensional incompatibility */
      }
    }
      break;
    case F_CBRT:
    {
      newdim = ThirdDimension(TermDimensions(arg),1);
      if (newdim == NULL) {
        return -1; /* dimensional incompatibility , range*/
      }
      break;
    }
    case F_SQR:
    {
      newdim = SquareDimension(TermDimensions(arg),1);
      if (newdim == NULL || fabs(rval) > F_LIM_SQR) {
        return -1; /* dimensional incompatibility , range*/
      }
      break;
    }
    case F_SQRT:
    {
      newdim = HalfDimension(TermDimensions(arg),1);
      if (newdim == NULL || rval < 0.0) {
        return -1; /* dimensional incompatibility or range */
      }
      break;
    }
    /* things that take any trig arg, return dimensionless */
    case F_ARCSIN:
    case F_ARCCOS:
      if ( fabs(rval) <= 1.0 && (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless()))) {
        newdim = TrigDimension();
        break;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_ARCTAN:
      if (IsWild(TermDimensions(arg)) ||
          SameDimen(TermDimensions(arg),Dimensionless())) {
        newdim = TrigDimension();
        break;
      } else {
        return -1; /* dimensional incompatibility */
      }
    case F_ABS:
    case F_HOLD:
      newdim = TermDimensions(arg);
      break;
    case F_LNM:
      return 1; /* user could change lnm epsilon. can't simplify. */
    default:
      FPRINTF(ASCERR,"Unrecognized function in relation.\n");
      return 1;
    }
    rval = FuncEval(TermFunc(A_TERM(fn)),rval);
    if (floor(rval)==ceil(rval) && SameDimen(newdim,Dimensionless()) &&
        fabs(rval) < MAXINTREAL) {
      fn->t = e_int;
      I_TERM(fn)->ivalue = (long)floor(rval);
    } else {
      fn->t = e_real;
      R_TERM(fn)->value = rval;
      R_TERM(fn)->dimensions = newdim;
    }
    return 0;
  }
  return 1;
}

static int ArgsForToken(enum Expr_enum t) {
  switch (t) {
  case e_nop:
  case e_undefined:
  case e_glassbox:
  case e_blackbox:
  case e_opcode:
  case e_token:
  case e_zero:
  case e_real:
  case e_int:
  case e_var:
    return 0;
  case e_uminus:
  case e_func:
    return 1;
  case e_plus:
  case e_minus:
  case e_times:
  case e_divide:
  case e_power:
  case e_ipower:
  case e_notequal:
  case e_equal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    return 2;
  case e_maximize:
  case e_minimize:
    return 1;
  default:
    FPRINTF(ASCERR,"ArgsForToken called with illegal token type.\n");
    return -1;
  }
}

/*
 * first = SimplifyTermBuf_SubExprLimit(ts,b,start,tt)
 * unsigned long CONST *ts;		 current term stack
 * struct relation_term ** CONST b;	 global term ptr array
 * unsigned long start;	 starting index IN STACK ts to find needed args
 * enum Expr_enum tt;	 term type of operator you want the subexpr for
 * long int first;	 term stack position of rightmost arg outside subexpr
 *
 * A little function to find the extent of a postfix subexpression for
 * the args of an operator term in the termstack/termbuf processing.
 * Returns -2 if insanity detected. handles nonoperator tt gracefully (-2).
 *
 * e.g. cos(v1+v2) * v3
 * tt = e_times, ts =>
 *  | V1 | V2 | + | cos | V3 | * |
 *                        ^--------start = 3
 * ^--------first = -1
 *
 * e.g. v1 * (v2 + v3)
 * tt = e_plus, ts =>
 *  | V1 | V2 | V3 | + | * |
 *               ^--------start = 2
 *    ^--------first = 0
 *
 * O(n) n= subexpr length.
 */
static long
SimplifyTermBuf_SubExprLimit(unsigned long CONST *ts,
			struct relation_term ** CONST buf,
			unsigned long start,
			enum Expr_enum tt)
{
  long int first, req_args;

  first = start;
  req_args = ArgsForToken(tt);
  if (first < 0) {
    FPRINTF(ASCERR,"SimplifyTermBuf_SubExpr given malformed subexpression.\n");
  }

  while (first >= 0 && req_args >0) {
    switch(buf[ts[first]]->t) {
    case e_zero:
    case e_real:
    case e_int:
    case e_var:
      req_args--;
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      req_args++;
      break;
    case e_func:
    case e_uminus:
      break;
    default:
      FPRINTF(ASCERR,
        "SimplifyTermBuf_SubExpr found illegal argument type (%d).\n",
        buf[ts[first]]->t);
      return -2;
    }
    first--;
  }
  if (first < -1) {
    FPRINTF(ASCERR,"SimplifyTermBuf_SubExpr found malformed subexpression.\n");
  }
  return first;
}

#ifndef NDEBUG
/* some functions to keep assert happy when simplification is in debug */
static int check_gt0(unsigned long i) {
  assert(i);
  return 1;
}
static int check_gt1(unsigned long i) {
  assert(i>1);
  return 1;
}
#endif

/*
 * A function to simplify the term buffer before copying it into a
 * postfix array. Only mandatory dim checking is performed.
 * Cost: O(n) where n = blen.
 *
 * This function is rather large, but simply structured, because speed
 * is important.
 * This is postfix simplification on the cheap. It could be more aggressive,
 * but only at potentially quadratic expense.
 *
 * int level;
 * struct relation_term ** CONST b;
 * CONST unsigned long blen;
 * They are the original term buffer array and its starting length.
 * b stays constant, not the data in it!
 *
 * (the following level definitions are mostly vapor. see relation.h for true.
 * level is how far to go in simplification. it is cumulative.
 * level 0 = do nothing.
 * level 1 = constant folding
 * level 2 = zero reductions. A*0 = 0/A =0. A^0=1;
 * level 3 = converting division by constants into multiplication
 * level 4 = distributing constants over simple mult. (V*C2)*C1 --> V*C3
 *
 * As a side effect, any e_power term that can be resolved to having
 * an integer exponent is converted to an e_ipower.
 *
 * This function is designed to simplifications wrt constants that
 * are easy to do in postfix. If you want something more clever, you
 * need to dress up things in infix, simplify, and put back to postfix.
 * Better you than me, bud.
 *
 * At present level > 1 is ignored; we always do 1-3, never 4.
 *
 * All this goes on in the termbuf array leaving null pointers behind.
 * We will compact the array and adjust the length before leaving this
 * function, so you don't have to care about len changing.
 * The termbuf pointers are from the pool, so we do not free them
 * as terms are eliminated.
 *
 * Internal doc:
 * Because C optimizers are pretty damned good, we aren't going to
 * play pointer games, we will just play subscript of b games.
 * Note that in flight we create null pointers in the already
 * visited buffer, but we always have an argument immediately
 * to the left (b[i-1]) of operator b[i]. If b[i] binary, its
 * right arg is b[i-1] and its left arg is the first nonnull
 * entry b[j] to the left of b[i-1] (j<i-1).
 *
 * The buffer is in postfix. We have no infix to maintain yet.
 * Abbreviations in comments:
 *  U - unary operator
 *  B - binary operator
 *  P - any operator
 *  V - e_var arg
 *  A - any arg
 *  C - any constant arg (e_int, e_real)
 *  R - e_real arg
 *  I - e_int arg
 *  N - null pointer
 * While in flight:
 | A | A | A | A | A | A | A |  termbuf
 *                     ^------- top = rightmost we've considered (current).
 | S | S | S | 0 |
 *             ^----next = next free location to put an index in termstack
 */
static unsigned long SimplifyTermBuf(int level,
				 register struct relation_term ** CONST b,
				 CONST unsigned long blen)
{
  register unsigned long next;
  register unsigned long *ts; /* term stack, should we need it */
  unsigned long top;
  long last;
  unsigned long right;
  int early = 0, err;
  CONST dim_type *newdim;
  long ival;
  double rval;

  if ( level < 1 || !blen ) {
    realloc_term_stack(0);
    return blen;
  }
  ts = realloc_term_stack(blen);
  /* stack gets used a lot, so make him locally managed, reusable mem */
  if (ts==NULL) return blen;
  /* at any trip through this loop we must be able to guarantee
   * some simple change, or that the buffer is suitable for
   * cleanup and return, so that we can handle the rogue operators,
   * args cleanly.
   */
  /* check that stack doesn't start with operator */
  /* should check that stack doesn't start pos 1 with binary operator */
  switch (b[0]->t) {
  case e_var:
  case e_int:
  case e_real:
  case e_zero:
    break;
  default:
    FPRINTF(ASCERR,"Compiler cannot simplify malformed expression\n");
    return blen;
  }

#ifdef NDEBUG
# define TS_TOP (ts[next-1]) /* term address last pushed */
# define TS_LEFT (ts[next-2])
   /* left hand term address IFF current term is binary and the term at TS_TOP is scalar (not operator) */
# define TS_SHIFTPOP ts[next-2] = ts[next-1]; next-- /* overwrite ts_left with ts_top and pop */
#else
# define TS_TOP (check_gt0(next),ts[next-1]) /* term address last pushed */
# define TS_LEFT (check_gt1(next),ts[next-2]) /* left hand term address IFF current term is binary and the term at TS_TOP is scalar (not operator) */
# define TS_SHIFTPOP assert(next>1); ts[next-2] = ts[next-1]; next-- /* overwrite ts_left with ts_top and pop */
#endif
/* keep the above definitions in sync. only difference should be assert. */

#define TS_PUSH(index) ts[next]=(index); next++ /* add a term to the stack */
#define TS_POP next-- /* backup the stack */
#define TS_POP2 next -= 2 /* backup the stack 2 spots */

  for (next=top=0; top < blen; top++) {
    /* pass through the tokens pointers array */
    if (b[top]==NULL) continue; /* so we can go through again if we like */
    /* each case and nested case should be complete in itself for
       readability. do not use fall throughs */
    switch (b[top]->t) {
    case e_var:
    case e_int:
    case e_real:
    case e_zero:
      TS_PUSH(top);
      break;
    case e_nop:
      b[top] = NULL; /* forget nop */
      break;
    case e_func:
      if ( CONSTANTTERM(b[TS_TOP]->t) ) {
	/* C U -> C' */
        if ( (err = SimplifyTermBuf_Func(b[TS_TOP],b[top]) ) != 0 ) {
          /* not simplified. just push later. whine if needed. */
          if (err < 0) {
            FPRINTF(ASCERR,
              "Can't simplify inconsistent argument to unary function.\n");
          }
        } else {
          b[TS_TOP] = NULL;	/* kill old arg, func term was morphed. */
          TS_POP; 	/* set up to push morphed func in place of arg */
        }
      }
      TS_PUSH(top); /* for all cases in the ifs */
      break;
    case e_uminus:
      switch (b[TS_TOP]->t) {
      case e_int:
        I_TERM(b[TS_TOP])->ivalue = -(I_TERM(b[TS_TOP])->ivalue);
        b[top] = b[TS_TOP]; /* I - => -I */
        b[TS_TOP] = NULL;
        TS_POP;
        TS_PUSH(top);
        break;
      case e_real:
        R_TERM(b[TS_TOP])->value = -(R_TERM(b[TS_TOP])->value);
        b[top] = b[TS_TOP]; /* R - => -R */
        b[TS_TOP] = NULL;
        TS_POP;
        TS_PUSH(top);
        break;
      case e_zero:
        b[top] = b[TS_TOP]; /* -0 = 0 */
        b[TS_TOP] = NULL;
        TS_POP;
        TS_PUSH(top);
        break;
      default:	/* aren't going to distribute or do other funky things */
        TS_PUSH(top);
        break;
      }
      break;

    case e_plus:
      /* A 0 + => NULL NULL A */
      if ( ZEROTERM(b[TS_TOP]) ) {
        /*
         * Note: we really should be checking the dimens of A to match
         * with dimens of 0 if e_real, but we are can't yet.
         */
        b[top] = b[TS_LEFT];	/* overwrite the + with the A */
        b[TS_LEFT] = NULL;	/* null the A old location */
        b[TS_TOP] = NULL;	/* null old location of 0 */
        TS_POP2;
        TS_PUSH(top);
        break;
      }
      switch (b[TS_TOP]->t) {
      case e_var:
        if ( ZEROTERM(b[TS_LEFT]) ) {
          /* 0 V + => NULL NULL V */
          /*
           * Note: we really should be checking the dimens of V to match
           * with dimens of 0 if e_real, but we are don't yet.
           */
          b[TS_LEFT] = NULL; /* null the zero term */
          b[top] = b[TS_TOP]; /* overwrite the + with the V */
          b[TS_TOP] = NULL; /* null old location of V */
          TS_POP2;
          TS_PUSH(top);
        } else {
          TS_PUSH(top);
        }
        break;
      /* 2 constant args? mangle C1 C2 + => C3 of appropriate type,if ok. */
      case e_int:  /* 0 I +, R I +, I I + */
        if ( CONSTANTTERM(b[TS_LEFT]->t) ) {
          /* 2 constant args. mangle C2 I1 + => C3 of appropriate type,if ok.*/
          if (b[TS_LEFT]->t==e_zero) { /* 0 I + */
            b[top] = b[TS_TOP];	/* overwrite the + with the I */
            b[TS_LEFT] = NULL;	/* null the 0 old location */
            b[TS_TOP] = NULL;	/* null old location of I */
            TS_POP2;
            TS_PUSH(top);
            break;
          }
          if (b[TS_LEFT]->t == e_int) { /* I2 I1 + */
            I_TERM(b[TS_TOP])->ivalue += I_TERM(b[TS_LEFT])->ivalue;
            b[top] = b[TS_TOP];   /* overwrite the + with the I1' */
            b[TS_LEFT] = NULL;    /* null the I2 old location */
            b[TS_TOP] = NULL;     /* null old location of I1 */
            TS_POP2;
            TS_PUSH(top);
            break;
          }
          if ( b[TS_LEFT]->t==e_real &&
               ( SameDimen(R_TERM(b[TS_LEFT])->dimensions,Dimensionless()) ||
                 (IsWild(R_TERM(b[TS_LEFT])->dimensions) &&
                  R_TERM(b[TS_LEFT])->value == 0.0)
               )
             ) { /* Ri2(possibly wild 0.0)  I1 + => I1' */
            if (floor(R_TERM(b[TS_LEFT])->value) ==
                  ceil(R_TERM(b[TS_LEFT])->value) &&
                  fabs(R_TERM(b[TS_LEFT])->value) < MAXINTREAL) {
              I_TERM(b[TS_TOP])->ivalue +=
                (long)floor(R_TERM(b[TS_LEFT])->value);
              b[top] = b[TS_TOP];   /* overwrite the + with the I1' */
              b[TS_LEFT] = NULL;    /* null the R2 old location */
              b[TS_TOP] = NULL;     /* null old location of I1 */
              TS_POP2;
              TS_PUSH(top);
              break;
            } else { /* morph + to R' */
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = Dimensionless();
              R_TERM(b[top])->value =
                R_TERM(b[TS_LEFT])->value + (double)I_TERM(b[TS_TOP])->ivalue;
              b[TS_LEFT] = NULL;    /* null the R2 old location */
              b[TS_TOP] = NULL;     /* null old location of I1 */
              TS_POP2;
              TS_PUSH(top);
              break;
            }
          } else {	/* dimensional conflict can't be fixed */
            FPRINTF(ASCERR,
              "Can't simplify dimensionally inconsistent arguments to +.\n");
            TS_PUSH(top);
          }
          break;
        } else { 	/* non C TS_LEFT */
          TS_PUSH(top);
        }
        break;
      case e_real: /* 0 R +, R R +, I R + */
        if ( CONSTANTTERM(b[TS_LEFT]->t) ) {
          /* 2 constant args. mangle C2 R1 + => C3 of appropriate type,if ok.*/
          newdim = CheckDimensionsMatch(TermDimensions(b[TS_TOP]),
                                        TermDimensions(b[TS_LEFT]));
          if (newdim == NULL) {
            FPRINTF(ASCERR,
              "Can't simplify dimensionally inconsistent arguments to +.\n");
            TS_PUSH(top);
            break;
          }
          if (b[TS_LEFT]->t==e_zero) { /* 0 R + */
            b[top] = b[TS_TOP];	/* overwrite the + with the R */
            b[TS_LEFT] = NULL;	/* null the 0 old location */
            b[TS_TOP] = NULL;	/* null old location of R */
            TS_POP2;
            TS_PUSH(top);
            /* if R was wild, it stays wild */
            break;
          }
          if (b[TS_LEFT]->t == e_int) { /* I2 R1 + */
            R_TERM(b[TS_TOP])->value += (double)I_TERM(b[TS_LEFT])->ivalue;
            R_TERM(b[TS_TOP])->dimensions = newdim;
            b[top] = b[TS_TOP];   /* overwrite the + with the R1' */
            b[TS_LEFT] = NULL;    /* null the I2 old location */
            b[TS_TOP] = NULL;     /* null old location of R1 */
            TS_POP2;
            TS_PUSH(top);
            /* if R wild, R becomes dimensionless */
            break;
          }
          if ( b[TS_LEFT]->t==e_real ) { /* R2 R1 + => R1', if R1' whole->I1'*/
            b[top]->t = e_real;
            R_TERM(b[top])->dimensions = newdim;
            R_TERM(b[top])->value =
              R_TERM(b[TS_LEFT])->value + R_TERM(b[TS_TOP])->value;
            b[TS_LEFT] = NULL;    /* null the R2 old location */
            b[TS_TOP] = NULL;     /* null old location of R1 */
            TS_POP2;
            TS_PUSH(top);
            /* if integer valued dimless real, convert to int */
            if (floor(R_TERM(b[top])->value) == ceil(R_TERM(b[top])->value)
                 && SameDimen(R_TERM(b[top])->dimensions,Dimensionless()) &&
                 fabs(R_TERM(b[top])->value) < MAXINTREAL) {
              I_TERM(b[top])->ivalue = (long)R_TERM(b[top])->value;
              b[top]->t = e_int;
            }
            break;
          } else {	/* dimensional conflict can't be fixed */
            FPRINTF(ASCERR,
              "Can't simplify dimensionally inconsistent arguments to +.\n");
            TS_PUSH(top);
          }
          break;
        } else { 	/* non C TS_LEFT */
          TS_PUSH(top);
        }
        break; /* end eplus, right arg is e_real */
      default: /* tstop is not 0, R, I, V */
        TS_PUSH(top);
        break;
      } /* end argtype switch of e_plus */
      break;

    case e_minus:
      /* A 0 - => NULL NULL A */
      if ( ZEROTERM(b[TS_TOP]) ) {
        /*
         * Note: we really should be checking the dimens of A to match
         * with dimens of 0 if e_real, but we are can't yet.
         */
        b[top] = b[TS_LEFT];	/* overwrite the - with the A */
        b[TS_LEFT] = NULL;	/* null the A old location */
        b[TS_TOP] = NULL;	/* null old location of 0 */
        TS_POP2;
        TS_PUSH(top);
        break;
      }
      switch (b[TS_TOP]->t) {
      case e_var:
        if ( ZEROTERM(b[TS_LEFT]) ) {
          /* 0 V - => NULL V uminus */
        /*
         * Note: we really should be checking the dimens of V to match
         * with dimens of 0 if e_real, but we are don't yet.
         */
          b[TS_LEFT] = NULL;	/* null the zero term */
          b[top]->t = e_uminus;	/* morph - to uminus */
          TS_SHIFTPOP;		/* reduce 0 V => V */
          TS_PUSH(top);
        } else {
          TS_PUSH(top);
        }
        break;
      /* 2 constant args? mangle C1 C2 - => C3 of appropriate type,if ok. */
      case e_int:  /* 0 I -, R I -, I I - */
        if ( CONSTANTTERM(b[TS_LEFT]->t) ) {
          /* 2 constant args. mangle C2 I1 - => C3 of appropriate type,if ok.*/
          if (b[TS_LEFT]->t==e_zero) { /* 0 I - */
            b[top] = b[TS_TOP];	/* overwrite the - with -I */
            I_TERM(b[top])->ivalue = -(I_TERM(b[top])->ivalue);
            b[TS_LEFT] = NULL;	/* null the 0 old location */
            b[TS_TOP] = NULL;	/* null old location of I */
            TS_POP2;
            TS_PUSH(top);
            break;
          }
          if (b[TS_LEFT]->t == e_int) { /* I2 I1 - */
            I_TERM(b[TS_TOP])->ivalue =
              I_TERM(b[TS_LEFT])->ivalue - I_TERM(b[TS_TOP])->ivalue;
            b[top] = b[TS_TOP];   /* overwrite the - with the I1' */
            b[TS_LEFT] = NULL;    /* null the I2 old location */
            b[TS_TOP] = NULL;     /* null old location of I1 */
            TS_POP2;
            TS_PUSH(top);
            break;
          }
          if ( b[TS_LEFT]->t==e_real &&
               ( SameDimen(R_TERM(b[TS_LEFT])->dimensions,Dimensionless()) ||
                 (IsWild(R_TERM(b[TS_LEFT])->dimensions) &&
                  R_TERM(b[TS_LEFT])->value == 0.0)
               )
             ) { /* Ri2(possibly wild 0.0)  I1 - => I1' */
            if (floor(R_TERM(b[TS_LEFT])->value) ==
                  ceil(R_TERM(b[TS_LEFT])->value) &&
                fabs(R_TERM(b[TS_LEFT])->value) < MAXINTREAL) {
              I_TERM(b[TS_TOP])->ivalue =
                (long)floor(R_TERM(b[TS_LEFT])->value)
                 - I_TERM(b[TS_TOP])->ivalue;
              b[top] = b[TS_TOP];   /* overwrite the + with the I1' */
              b[TS_LEFT] = NULL;    /* null the R2 old location */
              b[TS_TOP] = NULL;     /* null old location of I1 */
              TS_POP2;
              TS_PUSH(top);
              break;
            } else { /* morph - to R' */
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = Dimensionless();
              R_TERM(b[top])->value =
                R_TERM(b[TS_LEFT])->value - (double)I_TERM(b[TS_TOP])->ivalue;
              b[TS_LEFT] = NULL;    /* null the R2 old location */
              b[TS_TOP] = NULL;     /* null old location of I1 */
              TS_POP2;
              TS_PUSH(top);
              break;
            }
          } else {	/* dimensional conflict can't be fixed */
            FPRINTF(ASCERR,
              "Can't simplify dimensionally inconsistent arguments to -.\n");
            TS_PUSH(top);
          }
          break;
        } else { 	/* non C TS_LEFT */
          TS_PUSH(top);
        }
        break;

      case e_real: /* 0 R -, R R -, I R - */
        if ( CONSTANTTERM(b[TS_LEFT]->t) ) {
          /* 2 constant args. mangle C2 R1 - => C3 of appropriate type,if ok.*/
          newdim = CheckDimensionsMatch(TermDimensions(b[TS_TOP]),
                                        TermDimensions(b[TS_LEFT]));
          if (newdim == NULL) {
            FPRINTF(ASCERR,
              "Can't simplify dimensionally inconsistent arguments to -.\n");
            TS_PUSH(top);
            break;
          }
          if (b[TS_LEFT]->t==e_zero) { /* 0 R - */
            b[top] = b[TS_TOP];	/* overwrite the - with the R */
            R_TERM(b[top])->value = -(R_TERM(b[top])->value);
            b[TS_LEFT] = NULL;	/* null the 0 old location */
            b[TS_TOP] = NULL;	/* null old location of R */
            TS_POP2;
            TS_PUSH(top);
            /* if R was wild, it stays wild */
            break;
          }
          if (b[TS_LEFT]->t == e_int) { /* I2 R1 - */
            R_TERM(b[TS_TOP])->value =
              (double)I_TERM(b[TS_LEFT])->ivalue - R_TERM(b[TS_TOP])->value;
            R_TERM(b[TS_TOP])->dimensions = newdim;
            b[top] = b[TS_TOP];   /* overwrite the - with the R1' */
            b[TS_LEFT] = NULL;    /* null the I2 old location */
            b[TS_TOP] = NULL;     /* null old location of R1 */
            TS_POP2;
            TS_PUSH(top);
            /* if R wild, R becomes dimensionless */
            break;
          }
          if ( b[TS_LEFT]->t==e_real ) { /* R2 R1 - => R1', if R1' whole->I1'*/
            b[top]->t = e_real; 	/* morph - to R */
            R_TERM(b[top])->dimensions = newdim;
            R_TERM(b[top])->value =
              R_TERM(b[TS_LEFT])->value - R_TERM(b[TS_TOP])->value;
            b[TS_LEFT] = NULL;    /* null the R2 old location */
            b[TS_TOP] = NULL;     /* null old location of R1 */
            TS_POP2;
            TS_PUSH(top);
            /* if integer valued dimless real, convert to int */
            if (floor(R_TERM(b[top])->value) == ceil(R_TERM(b[top])->value)
                && SameDimen(R_TERM(b[top])->dimensions,Dimensionless())
                && fabs(R_TERM(b[top])->value) < MAXINTREAL) {
              I_TERM(b[top])->ivalue = (long)R_TERM(b[top])->value;
              b[top]->t = e_int;
            }
            break;
          } else {	/* dimensional conflict can't be fixed */
            FPRINTF(ASCERR,
              "Can't simplify dimensionally inconsistent arguments to -.\n");
            TS_PUSH(top);
          }
          break;
        } else { 	/* non C TS_LEFT */
          TS_PUSH(top);
        }
        break; /* end eminus, right arg is e_real */
      default: /* tstop is not 0, R, I, V */
        TS_PUSH(top);
        break;
      } /* end argtype switch of e_minus */
      break;

    case e_times:
      /* needs completing. only C*C done at present. need A*0 reductions */
      if ( !CONSTANTTERM(b[TS_LEFT]->t) && !CONSTANTTERM(b[TS_TOP]->t) ) {
        /* no constants in sight, go on fast. */
        TS_PUSH(top);
        break;
      } else {
        /* some constants in sight, try things. */
        if (b[TS_LEFT]->t == e_zero || b[TS_TOP]->t == e_zero) {
          /* end of A 0 * and 0 A *  => 0 */
          ival = SimplifyTermBuf_SubExprLimit(ts,b,next-1,e_times);
          if ( ival > -2 ) {
            for (last = next-1; last > ival; last--) {
              b[ts[last]] = NULL; /* kill the subexpression tokens */
            }
            next = ival + 1; /* big stack pop */
            b[top]->t = e_zero;
            R_TERM(b[top])->dimensions = WildDimension();
            R_TERM(b[top])->value = 0.0;
            TS_PUSH(top);
            break;
          } else {
            /* we had an error in subexpression limit search */
            TS_PUSH(top);
            break;
          }
        } /* end of A 0 * and 0 A * */
        /* NOTE: here we should be watching for 0.0 e_real and 0 e_int,
         * but as yet we don't have the dimen derivation utility to
         * check these cases and derive a properly dimensioned e_real 0.
         * We are not going to do a dimensionally incorrect shortcut
         * implementation. BAA 3/96
         */
        if ( CONSTANTTERM(b[TS_LEFT]->t) ) { /* C A * =?=> ?*/
          /* LEFT is now ereal or e_int because it passed the 0 and C tests */
          if ( b[TS_TOP]->t == e_real) { /* C R * => C */
            if ( b[TS_LEFT]->t == e_real ) { /* R R * => R */
              newdim = SumDimensions(R_TERM(b[TS_TOP])->dimensions,
                                     R_TERM(b[TS_LEFT])->dimensions,1);
              if ( newdim == NULL || IsWild(newdim) ) { /* bad dim */
                FPRINTF(ASCERR,
               "Mult. by wild or fractional dimension constant not folded.\n");
                TS_PUSH(top);
                break;
              } else { /* dim ok. morph etimes to be result. */
                rval = R_TERM(b[TS_TOP])->value * R_TERM(b[TS_LEFT])->value;
                /* god help us if this overflows... */
                b[top]->t = e_real;
                R_TERM(b[top])->dimensions = newdim;
                R_TERM(b[top])->value = rval;
                b[TS_TOP] = NULL;
                b[TS_LEFT] = NULL;
                TS_POP2;
                TS_PUSH(top);
                break;
              }
            } else { /* I R * => R */
              rval =
                R_TERM(b[TS_TOP])->value * (double)I_TERM(b[TS_LEFT])->ivalue;
              /* god help us if this overflows... */
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = R_TERM(b[TS_TOP])->dimensions;
              R_TERM(b[top])->value = rval;
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            }
#ifndef NDEBUG
            FPRINTF(ASCERR,"Unexpected error in Simplification (1).\n");
            /* NOT REACHED */
            break;
#endif
          }
          if ( b[TS_TOP]->t == e_int) { /* C I * => C */
            if ( b[TS_LEFT]->t == e_real ) { /* R I * => R */
              rval =
                R_TERM(b[TS_LEFT])->value * (double)I_TERM(b[TS_TOP])->ivalue;
              /* god help us if this overflows... */
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = R_TERM(b[TS_LEFT])->dimensions;
              R_TERM(b[top])->value = rval;
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            } else { /* I I * => I */
              rval = (double)I_TERM(b[TS_TOP])->ivalue *
                     (double)I_TERM(b[TS_LEFT])->ivalue;
              if (fabs(rval) < (double)(LONG_MAX/2)) {/*result safely integer*/
                b[top]->t = e_int;
                I_TERM(b[top])->ivalue =
                  I_TERM(b[TS_TOP])->ivalue * I_TERM(b[TS_LEFT])->ivalue;
                b[TS_TOP] = NULL;
                b[TS_LEFT] = NULL;
                TS_POP2;
                TS_PUSH(top);
                break;
              } else {
                b[top]->t = e_real;
                R_TERM(b[top])->dimensions = Dimensionless();
                R_TERM(b[top])->value = rval;
                b[TS_TOP] = NULL;
                b[TS_LEFT] = NULL;
                TS_POP2;
                TS_PUSH(top);
                break;
              }
            }
#ifndef NDEBUG
            FPRINTF(ASCERR,"Unexpected error in Simplification (2).\n");
            /* NOT REACHED */
            break;
#endif
          }
        } /* end all C A * alternatives */
        /* if here, no simplifications worked,
         * though constants exist.
         */
        TS_PUSH(top);
        break;
      } /* end of case e_times outermost if */
#ifndef NDEBUG
      FPRINTF(ASCERR,"Unexpected error in Simplification (3).\n");
      /* NOT REACHED */
      break;
#endif

    case e_divide: /* note: A1 A2 / postfix => A1/A2 infix */
      /* needs completing only does C/C at present. needs to do 0/A. */
      if ( !CONSTANTTERM(b[TS_LEFT]->t) && !CONSTANTTERM(b[TS_TOP]->t) ) {
        /* no constants in sight, go on fast. */
        TS_PUSH(top);
        break;
      } else {
        /* some constants in sight, try things. */
        if (b[TS_LEFT]->t == e_zero && b[TS_TOP]->t != e_zero) {
          /*  0 A /  => 0 */
          ival = SimplifyTermBuf_SubExprLimit(ts,b,next-1,e_divide);
          if ( ival > -2 ) {
            for (last = next-1; last > ival; last--) {
              b[ts[last]] = NULL; /* kill the subexpression tokens */
            }
            next = ival + 1; /* big stack pop, could be pop2 */
            b[top]->t = e_zero;
            R_TERM(b[top])->dimensions = WildDimension();
            R_TERM(b[top])->value = 0.0;
            TS_PUSH(top);
            break;
          } else {
            /* we had an error in subexpression limit search */
            TS_PUSH(top);
            break;
          }
        } /* end of 0 A / */
        /* NOTE: here we should be watching for 0.0 e_real and 0 e_int,
         * but as yet we don't
         * check these cases and derive a properly dimensioned e_real 0.
         * We are not going to do a dimensionally incorrect shortcut
         * implementation. BAA 3/96
         */
        if ( ZEROTERM(b[TS_TOP]) ) {
          /* trap A/0 out */
          FPRINTF(ASCERR,"Division by constant 0 not simplified.\n");
          top = blen;
          early = 1; /* set flag that we punted. */
          TS_PUSH(top);
          break;
        } /* end of  A/0 out */
        if ( CONSTANTTERM(b[TS_LEFT]->t) ) { /* C A / =?=> ?*/
          /* LEFT is now R or I because it passed the 0 and C tests */
          if ( b[TS_TOP]->t == e_real) { /* C R / => C */
            if ( b[TS_LEFT]->t == e_real ) { /* R R / => R */
              newdim = DiffDimensions(R_TERM(b[TS_LEFT])->dimensions,
                                     R_TERM(b[TS_TOP])->dimensions,1);
              if ( newdim == NULL || IsWild(newdim) ) { /* bad dim */
                FPRINTF(ASCERR,
               "Div. by wild or fractional dimension constant not folded.\n");
                TS_PUSH(top);
                break;
              } else { /* dim ok. morph edivide to be result. */
                rval = R_TERM(b[TS_LEFT])->value / R_TERM(b[TS_TOP])->value;
                /* god help us if this overflows/underflows... */
                b[top]->t = e_real;
                R_TERM(b[top])->dimensions = newdim;
                R_TERM(b[top])->value = rval;
                b[TS_TOP] = NULL;
                b[TS_LEFT] = NULL;
                TS_POP2;
                TS_PUSH(top);
                break;
              }
            } else { /* I R / => R */
              rval =
                ((double)I_TERM(b[TS_LEFT])->ivalue) /R_TERM(b[TS_TOP])->value;
              /* god help us if this overflows... */
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions =
                DiffDimensions(Dimensionless(),
                               R_TERM(b[TS_TOP])->dimensions,0);
                /* diff dimens not checked here because top is dimensionless */
              R_TERM(b[top])->value = rval;
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            }
#ifndef NDEBUG
            FPRINTF(ASCERR,"Unexpected error in Simplification (4).\n");
            /* NOT REACHED */
            break;
#endif
          }
          if ( b[TS_TOP]->t == e_int) { /* C I / => C */
            if ( b[TS_LEFT]->t == e_real ) { /* R I / => R */
              rval =
                R_TERM(b[TS_LEFT])->value / (double)I_TERM(b[TS_TOP])->ivalue;
              /* god help us if this overflows... */
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = R_TERM(b[TS_LEFT])->dimensions;
              R_TERM(b[top])->value = rval;
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            } else { /* I I / => R! Integer division is NOT allowed */
              rval = (double)I_TERM(b[TS_LEFT])->ivalue;
              rval /= (double)I_TERM(b[TS_TOP])->ivalue;
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = Dimensionless();
              R_TERM(b[top])->value = rval;
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            }
#ifndef NDEBUG
            FPRINTF(ASCERR,"Unexpected error in Simplification (5).\n");
            /* NOT REACHED */
            break;
#endif
          }
        } /* end all C A / alternatives */
        if ( CONSTANTTERM(b[TS_TOP]->t) ) { /* A C / => A (1/C) * */
          /* we screened out 0 above, so its int or real */
          if (b[TS_TOP]->t == e_real) { /* A R / => A R * */
            rval = 1/R_TERM(b[TS_TOP])->value;
            /* god help us if this overflows... */
            b[top]->t = e_times; /* morph / to * */
            /* flip dimens */
            R_TERM(b[TS_TOP])->dimensions =
              DiffDimensions(Dimensionless(),R_TERM(b[TS_TOP])->dimensions,0);
            /* diff dimens not checked here because top is dimensionless */
            R_TERM(b[TS_TOP])->value = rval; /* flip value */
            TS_PUSH(top);
            break;
          }
          if (b[TS_TOP]->t == e_int) { /* A I / => A I * */
            rval = 1.0/(double)I_TERM(b[TS_TOP])->ivalue;
            /* god help us if this overflows... */
            b[top]->t = e_times; /* morph / to * */
            /* flip dimens */
            b[TS_TOP]->t = e_real; /* morph int to real */
            R_TERM(b[TS_TOP])->value = rval; /* flip value */
            R_TERM(b[TS_TOP])->dimensions = Dimensionless();
            TS_PUSH(top);
            break;
          }
        } /* end of morphing A/C => A*c' */
        /* if here, no simplifications worked,
         * though constants exist.
         */
        TS_PUSH(top);
        break;
      } /* end of case e_divide outermost if */
      /* NOT REACHED */
#ifndef NDEBUG
      FPRINTF(ASCERR,"Unexpected error in Simplification (6).\n");
      break;
#endif
    case e_power: /* DANGER! WILL ROBINSON, DANGER! possible fall through */
      /* exponents must be dimensionless to make any sense */
      if (b[TS_TOP]->t == e_zero || b[TS_TOP]->t == e_int ||
           (b[TS_TOP]->t == e_real &&
             ( SameDimen(R_TERM(b[TS_TOP])->dimensions,Dimensionless()) ||
               IsWild(R_TERM(b[TS_TOP])->dimensions) ) &&
            floor(R_TERM(b[TS_TOP])->value)==ceil(R_TERM(b[TS_TOP])->value) &&
            fabs(R_TERM(b[TS_TOP])->value) < MAXINTREAL)
        ) { /* big if ipowerable */
        if (b[TS_TOP]->t == e_real) { /* morph real to int */
          b[TS_TOP]->t = e_int;
          I_TERM(b[TS_TOP])->ivalue = (long)R_TERM(b[TS_TOP])->value;
        }
        /* e_zero and e_int are appropriate to ipower and need no morph */
        b[top]->t = e_ipower; /* morph to ipower and fall through */
        /* FALL THROUGH! FALL THROUGH! FALL THROUGH! FALL THROUGH! */
        /* we aren't supposed to allow fall, but this is really the
           most perfect place to do power=>ipower conversion.
           Note that very large exponent values may be impossible later. */
      } else {
        /* still need to code C^R case. A^0 promoted to ipow, not here */
        if ( CONSTANTTERM(b[TS_LEFT]->t) && CONSTANTTERM(b[TS_TOP]->t) ) {
          /* C is either 0, int, or real. R is nonintegral (or damn big) real.
             Because R is not integer, C must be positive and dimensionless,
             and also small enough not to overflow: C > 1 =>
             check for pow(DBL_MAX,1/R) > R */
          if ( !SameDimen(R_TERM(b[TS_TOP])->dimensions,Dimensionless()) &&
               !IsWild(R_TERM(b[TS_TOP])->dimensions) ) {
            FPRINTF(ASCERR,"Illegal dimensioned exponent found in power.\n");
            top=blen;
            early = 1; /* set flag that we punted. */
            break;
          }
          if (b[TS_LEFT]->t == e_zero) { /* 0^R, R!=0 => 1 */
            b[top]->t = e_int;
            I_TERM(b[top])->ivalue = 1;
            b[TS_TOP] = NULL;
            b[TS_LEFT] = NULL;
            TS_POP2;
            TS_PUSH(top);
            break;
            /* end of 0^R */
          } else {
            if (b[TS_LEFT]->t == e_real) { /* R^R */
              if ( !SameDimen(R_TERM(b[TS_LEFT])->dimensions,Dimensionless())
                   && !IsWild(R_TERM(b[TS_LEFT])->dimensions) ) {
                /* can happen on very large exponents */
                FPRINTF(ASCERR,
                  "Illegal dimensioned base raised to nonintegral power.\n");
                top = blen;
                early = 1; /* set flag that we punted. */
                break;
              } else { /* R(dimless)^R , err if R ln(C) > ln(DBL_MAX) */
                if (R_TERM(b[TS_LEFT])->value < 0) {
                  /* can happen on very large exponents */
                  FPRINTF(ASCERR,
                    "Illegal negative base raised to nonintegral power.\n");
                  top = blen;
                  early = 1; /* set flag that we punted. */
                  break;
                }
                if (R_TERM(b[TS_LEFT])->value == 0.0) {
                  /* R!=0, 0^R = 1 */
                  b[top]->t = e_int;
                  I_TERM(b[top])->ivalue = 0;
                  b[TS_TOP] = NULL;
                  b[TS_LEFT] = NULL;
                  TS_POP2;
                  TS_PUSH(top);
                  break;
                }
                if ( R_TERM(b[TS_TOP])->value*log(R_TERM(b[TS_LEFT])->value) >
                  F_LIM_EXP) { /* overflow in result. let solver do it */
                  TS_PUSH(top);
                  break;
                } else {
                  b[top]->t = e_real;
                  R_TERM(b[top])->dimensions = Dimensionless();
                  R_TERM(b[top])->value =
                    pow(R_TERM(b[TS_LEFT])->value,R_TERM(b[TS_TOP])->value);
                  b[TS_TOP] = NULL;
                  b[TS_LEFT] = NULL;
                  TS_POP2;
                  TS_PUSH(top);
                  break;
                }
              }
              /* end of R^R */
            } else { /* I^R */
              if (I_TERM(b[TS_LEFT])->ivalue < 0) {
                /* can happen on very large exponents */
                FPRINTF(ASCERR,
                  "Illegal negative base raised to nonintegral power.\n");
                top = blen;
                early = 1; /* set flag that we punted. */
                break;
              }
              if (I_TERM(b[TS_LEFT])->ivalue == 0) {
                /* R!=0, 0^R = 1 */
                b[top]->t = e_int;
                I_TERM(b[top])->ivalue = 0;
                b[TS_TOP] = NULL;
                b[TS_LEFT] = NULL;
                TS_POP2;
                TS_PUSH(top);
                break;
              }
              if ( R_TERM(b[TS_TOP])->value *
                   log((double)I_TERM(b[TS_LEFT])->ivalue) > F_LIM_EXP) {
                /* overflow in result. let solver do it */
                TS_PUSH(top);
                break;
              } else {
                b[top]->t = e_real;
                R_TERM(b[top])->dimensions = Dimensionless();
                R_TERM(b[top])->value =
                  pow((double)I_TERM(b[TS_LEFT])->ivalue,
                      R_TERM(b[TS_TOP])->value);
                b[TS_TOP] = NULL;
                b[TS_LEFT] = NULL;
                TS_POP2;
                TS_PUSH(top);
                break;
              }
              /* end of  I^R  */
            }
            /* end of I,R ^R */
          }
          /* end of 0,I,R ^R */
        } else {
          TS_PUSH(top);
          /* remaining A^A2 where A2 => R or V or expr */
        }
        /* end of not morphing to ipower */
        break;
      }
      /* FALL THROUGH if morphing to ipower test succeeded */

    case e_ipower:
      if ( ZEROTERM(b[TS_TOP]) ) {
        /* A^0 */
        if ( ZEROTERM(b[TS_LEFT]) ) {
          /* 0^0 */
          FPRINTF(ASCERR,"0 raised to 0 power is undefined.\n");
          top=blen;
          early = 1; /* set flag that we punted. */
          break;
        } else {
          /* A^0 => 1 */
          ival = SimplifyTermBuf_SubExprLimit(ts,b,next-1,e_ipower);
          if ( ival > -2 ) {
            for (last = next-1; last > ival; last--) {
              b[ts[last]] = NULL; /* kill the subexpression tokens */
            }
            next = ival + 1; /* big stack pop */
            b[top]->t = e_int;
            I_TERM(b[top])->ivalue = 1;
            TS_PUSH(top);
            break;
          } else {
            /* we had an error */
            TS_PUSH(top);
            break;
          }
        }
      } else { /* A^I, I!=0, A!=0  => R or I as needed */
        if (CONSTANTTERM(b[TS_LEFT]->t)) { /* C^I */
          if (b[TS_LEFT]->t == e_real) { /* R^I */
            /* err if I*ln(R) > ln(DBL_MAX) */
            if ( I_TERM(b[TS_TOP])->ivalue*log(fabs(R_TERM(b[TS_LEFT])->value))
                 > F_LIM_EXP) { /* overflow in result. let solver do it */
              TS_PUSH(top);
              break;
            } else {
              ival = I_TERM(b[TS_TOP])->ivalue;
              newdim = PowDimension(ival,R_TERM(b[TS_LEFT])->dimensions,1);
              if (newdim==NULL) {
                FPRINTF(ASCERR,
                  "Dimensional overflow in exponentiation of constant.\n");
                TS_PUSH(top);
                break;
              }
              b[top]->t = e_real;
              R_TERM(b[top])->dimensions = newdim;
              R_TERM(b[top])->value =
                asc_ipow(R_TERM(b[TS_LEFT])->value,(int)ival);
                /* cast of ival is accurate if newdim was ok */
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            }
            /* end of R^I */
          } else { /* I^I */
            ival = I_TERM(b[TS_TOP])->ivalue;
            if ( ival * log((double)abs(I_TERM(b[TS_LEFT])->ivalue))
                 > F_LIM_EXP) {
              /* overflow in result. let solver do it */
              TS_PUSH(top);
              break;
            }
            if (abs(ival) < INT_MAX) { /* this could be a little better */
              rval = asc_ipow((double)I_TERM(b[TS_LEFT])->ivalue,
                              (int)I_TERM(b[TS_LEFT])->ivalue);
              if (fabs(rval) > MAXINTREAL || floor(rval)!=ceil(rval) ) {
                b[top]->t = e_real;
                R_TERM(b[top])->dimensions = Dimensionless();
                R_TERM(b[top])->value = rval;
              } else { /* can be an int safely */
                b[top]->t = e_int;
                I_TERM(b[top])->ivalue = (long)rval;
              }
              b[TS_TOP] = NULL;
              b[TS_LEFT] = NULL;
              TS_POP2;
              TS_PUSH(top);
              break;
            } else {
              /* exponent to damn big */
              TS_PUSH(top);
              break;
            }
            /* end of  I^I  */
          } /* end of C^I */
        } else {
          TS_PUSH(top);
          break;
        }
#ifndef NDEBUG
        FPRINTF(ASCERR,"Unexpected error in Simplification (7).\n");
        break; /* NOT REACHED */
#endif
      }
#ifndef NDEBUG
      FPRINTF(ASCERR,"Unexpected error in Simplification (8).\n");
      break; /* NOT REACHED */
#endif
    /* end e_ipower */

   /* all the following are bogus in instantiated tokens at this time. (2/96)
    * e_subexpr,e_const,e_par,
    * e_card,e_choice,e_sum,e_prod,e_union,e_inter,e_in,e_st,
    * e_glassbox,e_blackbox,e_opcode,e_token,
    * e_or,e_and,e_boolean,e_set,e_symbol,
    * e_equal,e_notequal,e_less,e_greater,e_lesseq,e_greatereq,e_not,
    * e_qstring,
    * e_maximize,e_minimize,
    * e_undefined
    */
    default:
      FPRINTF(ASCERR,"Unexpected token in relation simplification.\n");
      FPRINTF(ASCERR,"Returning early.\n");
      top=blen;
      early = 1; /* flag that we punted. */
      break;
    }
  }
  /*
   *  cleanup reduced expression, if needed.
   *  after the for loop, next is now the length of the postfix expression,
   *  or garbage if early is true.
   */
  if (blen <= next) return blen; /* no simplification, oh well. */
  right = 0;
  while (right < blen && b[right] != NULL) right++; /* find first null */
  for(last = right; right < blen; right++) { /* shift nonnulls left */
    if (b[right] != NULL) {
      b[last] = b[right];
      last++;
    }
  }
  if (!early && last != (long)next) {
    FPRINTF(ASCERR,"Confusing token counts in Simplify\n");
  }
  right = last;
  while (last<(long)blen) { /* null remainder, if any, of pointers */
    b[last] = NULL;
    last++;
  }
  return right;
}
/* END SimplifyTermBuf */

struct relation_side_temp {
  unsigned long length;
  union RelationTermUnion *side;
};

static struct relation_term
*InfixArr_MakeSide(CONST struct relation_side_temp *, int *);
/* forward declaration */

/* returns 1 if converting buf is successful
 * returns 0 if buf empty or insufficient memory.
 * The structure tmp given is filled with an array of terms
 * and its length. You must free the array if you decide you
 * don't want it. We don't care how the structure is initialized.
 */
static int ConvertTermBuf(struct relation_side_temp *tmp)
{
  union RelationTermUnion *arr = NULL;
  unsigned long len,c;

  len = SimplifyTermBuf(g_simplify_relations,g_term_ptrs.buf,g_term_ptrs.len);
  if (len < 1) return 0;
  arr = (union RelationTermUnion *)
	ascmalloc(len*sizeof(union RelationTermUnion));
  if (arr==NULL) {
    FPRINTF(ASCERR,"Create Token Relation: Insufficient memory :-(.\n");
    return 0;
  }
  for (c=0; c<len; c++) {
    arr[c] = *(UNION_TERM(g_term_ptrs.buf[c]));
  }
  tmp->side = arr;
  tmp->length = len;
  return 1;
}

/*
 *  usually we want to reset both simultaneously. reset our
 *  pooling and buffering data.
 */
static
void DestroyTermList(void) {
  POOL_RESET;
  TPBUF_RESET;
}

/* create a term from the pool */
static struct relation_term *CreateOpTerm(enum Expr_enum t)
{
  struct relation_term *term;
  term = POOL_ALLOCTERM;
  assert(term!=NULL);
  PTINIT(term);
  term->t = t;
  if (t==e_uminus) {
    U_TERM(term)->left = NULL;
  } else {
    B_TERM(term)->left = NULL;
    B_TERM(term)->right = NULL;
  }
  return term;
}

/* create a term from the pool, inserting it
 * in pointer sorted order on g_relation_var_list.
 * Note that this and ModifyTokenRelationPointers are the
 * only places where the sort
 * order of the var list matters.
 * In fact, in most cases we could equally afford
 * linear search and that would give us repeatability
 * across platforms and runs since the vars will be
 * then encountered in a constant order determined
 * by how the user wrote the equation.
 * Needs consideration, especially in light of
 * potential to improve relation sharing.
 * In particular, we could then easily share
 * in a fine-grained manner those relations with
 * only a single index involved and no internal sums/products,
 * such as f[i] = x[i]*Ftot; in[i].f = out[i].f;
 * x = hold(x);
 * which could be pretty darn common forms.
 */
static struct relation_term *CreateVarTerm(CONST struct Instance *i)
{
  struct relation_term *term;
  unsigned long pos;
  if (0 != (pos = gl_search(g_relation_var_list,i,(CmpFunc)CmpP))) {
    /* find var if already on relations var list */
    term = POOL_ALLOCTERM;
    assert(term!=NULL);
    PTINIT(term);
    term->t = e_var;
    V_TERM(term) -> varnum = pos;
  } else {
    /* or add it to the var list */
    gl_append_ptr(g_relation_var_list,(VOIDPTR)i);
    term = POOL_ALLOCTERM;
    assert(term!=NULL);
    PTINIT(term);
    term->t = e_var;
    V_TERM(term) -> varnum = gl_length(g_relation_var_list);
  }
  return term;
}

/* create a term from the pool */
static struct relation_term *CreateIntegerTerm(long int v)
{
  struct relation_term *term;
  term = POOL_ALLOCTERM;
  assert(term!=NULL);
  PTINIT(term);
  term->t = e_int;
  I_TERM(term) -> ivalue = v;
  return term;
}

/* create a term from the pool */
static struct relation_term *CreateRealTerm(double v, CONST dim_type *dim)
{
  struct relation_term *term;
  term = POOL_ALLOCTERM;
  assert(term!=NULL);
  PTINIT(term);
  term->t = e_real;
  R_TERM(term) -> value = v;
  R_TERM(term) -> dimensions = dim;
  return term;
}

/* create a term from the pool. Zero terms look like real, wild zeros */
static struct relation_term *CreateZeroTerm(void)
{
  struct relation_term *term;
  term = POOL_ALLOCTERM;
  assert(term!=NULL);
  PTINIT(term);
  term->t = e_zero;
  R_TERM(term)->value = 0.0;
  R_TERM(term)->dimensions = WildDimension();
  return term;
}

/* create a term from the pool */
static struct relation_term *CreateFuncTerm(CONST struct Func *f)
{
  struct relation_term *term;
  term = POOL_ALLOCTERM;
  assert(term!=NULL);
  PTINIT(term);
  term->t = e_func;
  F_TERM(term) -> fptr = f;
  F_TERM(term) -> left = NULL;
  return term;
}


#ifdef  THIS_IS_AN_UNUSED_FUNCTION
/* create a term from the pool */
static struct relation_term *CreateNaryTerm(CONST struct Func *f)
{
  struct relation_term *term;
  term = POOL_ALLOCTERM;
  assert(term!=NULL);
  PTINIT(term);
  term->t = e_func;
  N_TERM(term) -> fptr = f;
  N_TERM(term) -> args = NULL;
  return term;
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */


/*
 * This function create and *must* create the memory
 * for the structure and for the union that the structure
 * points to. Too much code depends on the pre-existent
 * of a properly initialized union.
 * If copyunion is crs_NOUNION, the share ptr is init to NULL and user
 * must set refcount,relop after the allocate a UNION or whatever.
 * If copyunion is crs_NEWUNION, share ptr is allocated and configured.
 *
 */
struct relation *CreateRelationStructure(enum Expr_enum relop,int copyunion)
{
  struct relation *newrelation;

  newrelation = (struct relation *)ascmalloc(sizeof(struct relation));
  assert(newrelation!=NULL);

  newrelation->residual = DBL_MAX;
  newrelation->multiplier = DBL_MAX;
  newrelation->nominal = 1.0;
  newrelation->iscond = 0;
  newrelation->vars = NULL;
  newrelation->d =(dim_type *)WildDimension();

  if (copyunion) {
    newrelation->share =
        (union RelationUnion *)ascmalloc(sizeof(union RelationUnion));
    assert(newrelation->share!=NULL);
    RelationRefCount(newrelation) = 0;
    RelRelop(newrelation) = relop;
#if TOKENDOMINANT
    RTOKEN(newrelation).lhs_term = NULL;
    RTOKEN(newrelation).rhs_term = NULL;
    RTOKEN(newrelation).lhs = NULL;
    RTOKEN(newrelation).rhs = NULL;
    RTOKEN(newrelation).lhs_len = 0;
    RTOKEN(newrelation).rhs_len = 0;
    RTOKEN(newrelation).btable = 0;
    RTOKEN(newrelation).bindex = 0;
#else
    memset((char *)(newrelation->share),0,sizeof(union RelationUnion));
#endif
  } else {
    newrelation->share = NULL;
  }
  return newrelation;
}


/*
 **************************************************************************
 * External Procedures Processing.
 *
 * A special note on external relations.
 * External relations behave like relations but they also behave like
 * procedures. As such when they are constructed and invoked they expect
 * a particular ordering of their variables.
 * However there are some operations that can mess up (reduce) the number
 * of incident variables on the incident varlist -- ATSing 2 variables in the
 * *same* relation will do this. BUT we still need to maintain the number
 * of variables in the call to the evaluation routine.
 * Consider the following example:
 * An glassbox relation is constructed as: test1(x[46,2,8,9] ; 2);
 * It *requires* 4 arguements, but its incident var count could be anything
 * from 1 <= n <= 4, depending on how many ATS are done. Unfortunately
 * the ATS could have been done even before we have constructed the relation,
 * so we have to make sure that we check for aliasing.
 **************************************************************************
 */



struct relation *CreateBlackBoxRelation(struct Instance *relinst,
					struct ExternalFunc *efunc,
					struct gl_list_t *arglist,
					struct Instance *subject,
					struct gl_list_t *inputs,
					struct Instance *data)
{
  struct relation *result;
  struct gl_list_t *newarglist;
  struct gl_list_t *newlist;
  struct ExtCallNode *ext;
  struct Instance *var = NULL;
  int *args;
  unsigned long c,len,pos;
  unsigned long n_inputs;

  CONSOLE_DEBUG("CREATING BLACK BOX RELATION");

  n_inputs = gl_length(inputs);
  len = n_inputs + 1;		/* an extra for the output variable. */

  /*
   * Add the input vars, making sure that their incidence
   * is unique, and adjusting the indexing appropriately
   * on the integer args array.
   */

  args = (int *)asccalloc((int)(len+1), sizeof(int));
  newlist = gl_create(len);

  for (c=1;c<=n_inputs;c++) {
    var = (struct Instance *)gl_fetch(inputs,c);
    pos = gl_search(newlist,var,(CmpFunc)CmpP);
    if (pos) {
      FPRINTF(ASCERR,"Incidence for external relation will be inaccurate\n");
      *args++ = (int)pos;
    }
    else{
      gl_append_ptr(newlist,(VOIDPTR)var);
      *args++ = (int)gl_length(newlist);
      AddRelation(subject,relinst);
    }
  }

  /*
   * Add the subject.
   */
  pos = gl_search(newlist,subject,(CmpFunc)CmpP);
  if (pos) {
    FPRINTF(ASCERR,"An input and output variable are the same !!\n");
    *args++ = (int)pos;
  }
  else{
    gl_append_ptr(newlist,(VOIDPTR)subject);		/* add the subject */
    *args++ = (int)gl_length(newlist);
    AddRelation(var,relinst);
  }
  *args = 0;						/* terminate */

  /*
   * Create the BlackBox relation structure. This requires
   * creating a ExtCallNode node.
   */
  newarglist = CopySpecialList(arglist);
  ext = CreateExtCall(efunc,newarglist,subject,data);
  SetExternalCallNodeStamp(ext,g_ExternalNodeStamps);

  /*
   * Now make the main relation structure and put it all
   * together. Then append the necessary lists.
   */
  result = CreateRelationStructure(e_equal,crs_NEWUNION);
  RelationRefCount(result) = 1;
  RBBOX(result).args = args;
  RBBOX(result).ext = ext;
  result->vars = newlist;
  return result;
}


struct relation *CreateGlassBoxRelation(struct Instance *relinst,
					struct ExternalFunc *efunc,
					struct gl_list_t *varlist,
					int index,
					enum Expr_enum relop)
{
  struct relation *result;
  struct Instance *var;
  struct gl_list_t *newlist = NULL;
  int *tmp = NULL, *args = NULL;
  unsigned long len,c,pos;

  len  = gl_length(varlist);
  /*
   * Make the variables aware that they are incident
   * in this relation instance. At the same time set up
   * the args list indexing.
   */
  if (len) {
    tmp = args = (int *)asccalloc((int)(len+1), sizeof(int));
    newlist = gl_create(len);

    for (c=1;c<=len;c++) {
      var = (struct Instance *)gl_fetch(varlist,c);
      pos = gl_search(newlist,var,(CmpFunc)CmpP);
      if (pos) {
	FPRINTF(ASCERR,"Incidence for external relation will be inaccurate\n");
	*tmp++ = (int)pos;
      }
      else{
	gl_append_ptr(newlist,(VOIDPTR)var);
	*tmp++ = (int)gl_length(newlist);
	AddRelation(var,relinst);
      }
    }
  }
  *tmp = 0;						/* terminate */

  /*
   * Create the relation data structure and append the
   * varlist.
   */
  result = CreateRelationStructure(relop,crs_NEWUNION);
  RelationRefCount(result) = 1;
  RGBOX(result).efunc = efunc;
  RGBOX(result).args = args;
  RGBOX(result).nargs = (int)len;
  RGBOX(result).index = index;
  result->vars = newlist;
  return result;
}


/**************************************************************************\
  TokenRelation processing and general expr -> relation check routines.
\**************************************************************************/


static
struct value_t CheckIntegerCoercion(struct value_t v)
{
  if ((ValueKind(v)==real_value) && (RealValue(v)==0.0) &&
      IsWild(RealValueDimensions(v)) ){
    DestroyValue(&v);
    return CreateIntegerValue(0,1); /* assume this is a constant then */
  }
  else return v;
}

static
int ProcessListRange(CONST struct Instance *ref,
		     CONST struct Expr *low,
		     CONST struct Expr *up,
		     int *added,
		     int i,
		     enum relation_errors *err,
		     enum find_errors *ferr)
{
  struct value_t lower,upper;
  struct relation_term *term;
  long lv,uv;
  assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(ref);
  lower = EvaluateExpr(low,NULL,InstanceEvaluateName);
  upper = EvaluateExpr(up,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  lower = CheckIntegerCoercion(lower);
  upper = CheckIntegerCoercion(upper);
  if ((ValueKind(lower)==integer_value)&&(ValueKind(upper)==integer_value)){
    lv = IntegerValue(lower);
    uv = IntegerValue(upper);
    while(lv<=uv){
      term = CreateIntegerTerm(lv);
      AppendTermBuf(term);
      if ((*added)++) {
	switch(i){
	case SUM:
	  term = CreateOpTerm(e_plus);
	  break;
	case PROD:
	  term = CreateOpTerm(e_times);
	  break;
	}
	AppendTermBuf(term);
      }
      lv++;
    }
    return 0;
  }
  else{
    if(ValueKind(lower)==error_value) {
      FigureOutError(lower,err,ferr);
      return 1;
    }
    if(ValueKind(upper)==error_value){
      FigureOutError(upper,err,ferr);
      return 1;
    }
    *err = incorrect_structure;
    FPRINTF(ASCERR,"incorrect_structure in ProcessListRange\n");
    return 1;
  }
}

static
CONST struct Expr *ExprContainsSuchThat(register CONST struct Expr *ex)
{
  while(ex!=NULL){
    if (ExprType(ex)==e_st) return ex;
    ex = NextExpr(ex);
  }
  return ex;
}

/*
 *  Here we give up if vars are not well defined.
 *  At present e_var acceptable ARE:
 *  REAL_ATOM_INSTANCE
 *  Well defined Real and Integer constants.
 *  Everything else is trash.
 *  CreateTermFromInst() and CheckExpr() must have matching semantics.
 */
static
struct relation_term *CreateTermFromInst(struct Instance *inst,
					 struct Instance *rel,
					 enum relation_errors *err)
{
  struct relation_term *term;
  switch(InstanceKind(inst)){
  case REAL_ATOM_INST:
    term = CreateVarTerm(inst);
    AddRelation(inst,rel);
    return term;
  case REAL_CONSTANT_INST:
    if ( AtomAssigned(inst) && !IsWild(RealAtomDims(inst)) ){
      term = CreateRealTerm(RealAtomValue(inst),RealAtomDims(inst));
      return term;
    }
    else{
      if ( IsWild(RealAtomDims(inst)) && AtomAssigned(inst) ) {
	*err = real_value_wild;
      } else {
	*err = real_value_undefined;
      }
      return NULL;
    }
  case INTEGER_CONSTANT_INST:
    if (AtomAssigned(inst)){
      term = CreateIntegerTerm(GetIntegerAtomValue(inst));
      return term;
    }
    else{
      *err = integer_value_undefined;
      return NULL;
    }
  case REAL_INST:
    *err = incorrect_real_inst_type;
    return NULL;
  case INTEGER_ATOM_INST:
  case INTEGER_INST:
    *err = incorrect_integer_inst_type;
    return NULL;
  case SYMBOL_ATOM_INST:
  case SYMBOL_CONSTANT_INST:
  case SYMBOL_INST:
    *err = incorrect_symbol_inst_type;
    return NULL;
  case BOOLEAN_ATOM_INST:
  case BOOLEAN_CONSTANT_INST:
  case BOOLEAN_INST:
    *err = incorrect_boolean_inst_type;
    return NULL;
  default:
    *err = incorrect_inst_type;
    return NULL;
  }
}

/* forward declaration */
static int AppendList( CONST struct Instance *,
		 struct Instance *,
		 CONST struct Set *,
		 int ,
		 enum relation_errors *,
		 enum find_errors *);

static
int ConvertSubExpr(CONST struct Expr *ptr,
		   CONST struct Expr *stop,
		   CONST struct Instance *ref,
		   struct Instance *rel,
		   int *added,
		   int i,
		   enum relation_errors *err,
		   enum find_errors *ferr)
{
  struct relation_term *term = NULL;
  struct gl_list_t *instances;
  unsigned c,len;
  struct Instance *inst;
  struct value_t svalue,cvalue;
  int my_added=0;
  symchar *str;
  CONST struct for_var_t *fvp;	/* for var pointer */
  while (ptr!=stop){
    switch(ExprType(ptr)){
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_uminus:
      term = CreateOpTerm(ExprType(ptr));
      my_added++;
      AppendTermBuf(term);
      break;
    case e_var:
      str = SimpleNameIdPtr(ExprName(ptr));
      if (str&&TempExists(str)){
        cvalue = TempValue(str);
        switch(ValueKind(cvalue)){
        case integer_value:
          term = CreateIntegerTerm(IntegerValue(cvalue));
          my_added++;
          AppendTermBuf(term);
          break;
        default:
          FPRINTF(ASCERR,"Non-integer temporary variable used in expression.\n");
          *err = incorrect_inst_type;
          term = NULL;
          return 1;
        }
      }else if (GetEvaluationForTable() != NULL && str !=NULL &&
	       (fvp=FindForVar(GetEvaluationForTable(),str)) !=NULL ){
        if (GetForKind(fvp)==f_integer){
          term = CreateIntegerTerm(GetForInteger(fvp));
          my_added++;
          AppendTermBuf(term);
        }
        else{
          FPRINTF(ASCERR,
		  "Non-integer FOR variable used in expression.\n");
          *err = incorrect_inst_type;
          return 1;
         }
      }
      else{
	instances = FindInstances(ref,ExprName(ptr),ferr);
	if (instances!=NULL){
	  if (NextExpr(ptr)==stop){ /* possibly multiple instances */
	    len = gl_length(instances);
	    for(c=1;c<=len;c++){
	      inst = (struct Instance *)gl_fetch(instances,c);
	      if ((term=CreateTermFromInst(inst,rel,err))!=NULL){
		AppendTermBuf(term);
		if (my_added++){
		  switch(i){
		  case SUM:
		    term = CreateOpTerm(e_plus);
		    break;
		  case PROD:
		    term = CreateOpTerm(e_times);
		    break;
		  }
		  AppendTermBuf(term);
		}
	      }
	      else{
		gl_destroy(instances);
		return 1;
	      }
	    }
	    gl_destroy(instances);
	  }
	  else{			/* single instance */
	    if (gl_length(instances)==1){
	      inst = (struct Instance *)gl_fetch(instances,1);
	      gl_destroy(instances);
	      if ((term=CreateTermFromInst(inst,rel,err))!=NULL){
		my_added++;
		AppendTermBuf(term);
	      }
	      else
		return 1;
	    }
	    else{
	      gl_destroy(instances);
	      *err = incorrect_structure;
              FPRINTF(ASCERR,"incorrect_structure in ConvertSubExpr 1\n");
	      return 1;
	    }
	  }
	} else{
	  *err = find_error;
	  return 1;
	}
      }
      break;
    case e_int:
      term = CreateIntegerTerm(ExprIValue(ptr));
      my_added++;
      AppendTermBuf(term);
      break;
    case e_zero:
      /* this should never happen here */
      term = CreateZeroTerm();
      my_added++;
      AppendTermBuf(term);
      break;
    case e_real:
      term = CreateRealTerm(ExprRValue(ptr),ExprRDimensions(ptr));
      my_added++;
      AppendTermBuf(term);
      break;
    case e_card:
      assert(GetEvaluationContext() == NULL);
      SetEvaluationContext(ref);
      svalue = EvaluateSet(ExprBuiltinSet(ptr),InstanceEvaluateName);
      SetEvaluationContext(NULL);
      cvalue = CardValues(svalue);
      DestroyValue(&svalue);
      switch(ValueKind(cvalue)){
      case integer_value:
	term = CreateIntegerTerm(IntegerValue(cvalue));
	my_added++;
	AppendTermBuf(term);
	break;
      case error_value:
	FigureOutError(cvalue,err,ferr);
	DestroyValue(&cvalue);
	return 1;
      default:
	FPRINTF(ASCERR,"This message should never occur.\n");
	FPRINTF(ASCERR,"If it does tell %s\n",ASC_BIG_BUGMAIL);
	DestroyValue(&cvalue);
	*err = incorrect_structure;
	return 1;
      }
      DestroyValue(&cvalue);
      break;
    case e_sum:
      my_added++;
      if (AppendList(ref,rel,ExprBuiltinSet(ptr),SUM,err,ferr))
	return 1;
      break;
    case e_prod:
      my_added++;
      if (AppendList(ref,rel,ExprBuiltinSet(ptr),PROD,err,ferr))
	return 1;
      break;
    case e_func:
      term = CreateFuncTerm(ExprFunc(ptr));
      my_added++;
      AppendTermBuf(term);
      break;
    default:
      *err = incorrect_structure;
      FPRINTF(ASCERR,"incorrect_structure in ConvertSubExpr 2\n");
      return 1;

    }
    ptr = NextExpr(ptr);
  }
  if (my_added) {
    if ((*added)++){
      switch(i){
      case SUM:
        term = CreateOpTerm(e_plus);
        break;
      case PROD:
        term = CreateOpTerm(e_times);
        break;
      }
      AppendTermBuf(term);
    }
  }
  return 0;
}

static
int CorrectSuchThat(CONST struct Expr *ex,
		    CONST struct Expr **depth_one,
		    CONST struct Expr **node)
{
  unsigned depth=0;
  CONST struct Expr *previous=NULL;
  while(ex!=NULL){
    switch(ExprType(ex)){
    case e_zero:
    case e_var:
    case e_int:
    case e_real:
    case e_boolean:
    case e_set:
    case e_symbol:
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_inter:
      if ((++depth)==1) *depth_one = ex;
      break;
      /* binary operators */
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_or:
    case e_and:
    case e_in:
    case e_equal:
    case e_notequal:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
      if ((--depth)==1) *depth_one = ex;
      break;
    case e_func:
    case e_uminus:
    case e_not:
      if (depth==1) *depth_one = ex;
      break;
    case e_st:
      if (previous==NULL) return 0; /* error */
      if (NextExpr(ex)!=NULL) return 0;	/* error */
      if (ExprType(previous)!=e_in) return 0; /* error */
      *node = previous;
      return 1;
    case e_minimize:
    case e_maximize:
      Asc_Panic(2, NULL,
                "Maximize and minimize are not allowed in expression.\n"
                "They are only allowed in relations.\n");
      break;
    default:
      Asc_Panic(2, NULL, "Unknown expression node type.\n");
      break;
    }
    previous = ex;
    ex = NextExpr(ex);
  }
  return 0;
}

/* if problem, returns 1. if ok, returns 0 */
static
int DoNameAndSet(CONST struct Expr *ex,
		 CONST struct Expr *stop,
		 CONST struct Instance *ref,
		 symchar **name,
		 struct value_t *value)
{
  if (ExprType(ex)==e_var){
    if ((*name = SimpleNameIdPtr(ExprName(ex)))!=NULL){
      assert(GetEvaluationContext()==NULL);
      SetEvaluationContext(ref);
      *value = EvaluateExpr(NextExpr(ex),stop,InstanceEvaluateName);
      SetEvaluationContext(NULL);
      if (ValueKind(*value)==set_value) return 0;
      DestroyValue(value);
      return 1;
    }
    else return 1;
  }
  else return 1;
}

static
int ConvertSuchThat(CONST struct Expr *ex,
		    CONST struct Instance *ref,
		    struct Instance *rel,
		    int *added,
		    int i,
		    enum relation_errors *err,
		    enum find_errors *ferr)
{
  symchar *tmp_name;
  unsigned long c,len;
  int my_added=0;
  struct value_t iteration_set,tmp_value;
  struct relation_term *term = NULL;
  struct set_t *sptr;
  CONST struct Expr *depth_one,*node;
  if (CorrectSuchThat(ex,&depth_one,&node)){
    if (DoNameAndSet(NextExpr(depth_one),node,ref,&tmp_name,&iteration_set)){
      *err = incorrect_structure;
      FPRINTF(ASCERR,"incorrect_structure in ConvertSuchThat 1\n");
      if (depth_one!=NULL && NextExpr(depth_one)!=NULL) {
        FPRINTF(ASCERR,"such that expression (RPN):\n\t");
        WriteExpr(ASCERR,NextExpr(depth_one));
        FPRINTF(ASCERR,"\n");
      }
      return 1;
    }
    node = NextExpr(depth_one);
    sptr = SetValue(iteration_set);
    switch(SetKind(sptr)){
    case empty_set:
      DestroyValue(&iteration_set);
      return 0;
    case integer_set:
    case string_set:
      if (TempExists(tmp_name)){
	FPRINTF(ASCERR,"Reused temporary variable %s.\n",SCP(tmp_name));
	DestroyValue(&iteration_set);
	*err = incorrect_structure;
	return 1;
      }
      AddTemp(tmp_name);
      len = Cardinality(sptr);
      for(c=1;c<=len;c++) {
	if (SetKind(sptr)==string_set)
	  tmp_value = CreateSymbolValue(FetchStrMember(sptr,c),1);
	else
	  tmp_value = CreateIntegerValue(FetchIntMember(sptr,c),1);
	SetTemp(tmp_name,tmp_value);
	if (ConvertSubExpr(ex,node,ref,rel,&my_added,i,err,ferr)){
	  RemoveTemp(tmp_name);
	  DestroyValue(&tmp_value);
	  DestroyValue(&iteration_set);
	  return 1;
	}
	DestroyValue(&tmp_value);
      }
      if (my_added){
        my_added++;
        if ((*added)++){
          switch(i){
          case SUM:
            term = CreateOpTerm(e_plus);
            break;
          case PROD:
            term = CreateOpTerm(e_times);
            break;
          }
          AppendTermBuf(term);
        }
      }
      RemoveTemp(tmp_name);
      DestroyValue(&iteration_set);
      return 0;
    }
    /*NOTREACHED*/
  }
  else{
    *err = incorrect_structure;
    FPRINTF(ASCERR,"incorrect_structure in ConvertSuchThat 2\n");
    return 1;
  }
  /*NOTREACHED we hope*/
  return 1;
}

static
int ProcessListExpr(CONST struct Instance *ref,
		    struct Instance *rel,
		    CONST struct Expr *ex,
		    int *added,
		    int i,
		    enum relation_errors *err,
		    enum find_errors *ferr)
{
  if (ExprContainsSuchThat(ex)!=NULL){
    return ConvertSuchThat(ex,ref,rel,added,i,err,ferr);
  } else {
    return ConvertSubExpr(ex,NULL,ref,rel,added,i,err,ferr);
  }
}

static int AppendList(CONST struct Instance *ref,
	       struct Instance *rel,
	       CONST struct Set *set,
	       int i,
	       enum relation_errors *err,
	       enum find_errors *ferr)
{
  int added_one=0;		/* becomes true when a term is added */
  struct relation_term *term = NULL;
  while (set!=NULL){
    if (SetType(set)){		/* range of values */
      if (ProcessListRange(ref,GetLowerExpr(set),
			   GetUpperExpr(set),&added_one,i,err,ferr))
	return 1;
    }
    else{			/* single expr */
      if (ProcessListExpr(ref,rel,GetSingleExpr(set),&added_one,
			  i,err,ferr))
	return 1;
    }
    set = NextSet(set);
  }
  if(!added_one){	/* case of the empty set */
    switch(i){
    case SUM:
      term = CreateZeroTerm();
      break;
    case PROD:
      term = CreateRealTerm(1.0,Dimensionless());
      break;
    }
    AppendTermBuf(term);
  }
  return 0;
}

/* nonrecursive, but may call recursive things. returns 1 if ok. 0 if not
 * On a return of 1, newside->arr will be filled and should be deallocated
 * if the user does not want it. a return of 0 means that newside data is
 * invalid.
 * This is the ONLY function that should call DestroyTermList.
 */
static int ConvertExpr(CONST struct Expr *start,
			      CONST struct Expr *stop,
			      struct Instance *ref,
			      struct Instance *rel,
			      enum relation_errors *err,
			      enum find_errors *ferr,
				struct relation_side_temp *newside)
{
  struct gl_list_t *instances;
  struct relation_term *term;
  struct Instance *inst;
  int result;
  symchar *str;
  CONST struct for_var_t *fvp;
  struct value_t svalue,cvalue;
  if (newside==NULL) {
    Asc_Panic(2, NULL, "newside == NULL");
  }
  while(start!=stop){
    switch(ExprType(start)){
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_uminus:
      term = CreateOpTerm(ExprType(start));
      AppendTermBuf(term);
      break;
    case e_var:
      if (GetEvaluationForTable() &&
	  (NULL != (str = SimpleNameIdPtr(ExprName(start)))) &&
	  (NULL != (fvp = FindForVar(GetEvaluationForTable(),str)))) {
	if (GetForKind(fvp)==f_integer){
	  term = CreateIntegerTerm(GetForInteger(fvp));
	  AppendTermBuf(term);
	} else{
	  *err = incorrect_inst_type;
	  DestroyTermList();
	  return 0;
	}
      } else{
	instances = FindInstances(ref,ExprName(start),ferr);
	if (instances!=NULL){
	  if (gl_length(instances)==1){
	    inst = (struct Instance *)gl_fetch(instances,1);
	    gl_destroy(instances);
	    if ((term = CreateTermFromInst(inst,rel,err))!=NULL){
	      AppendTermBuf(term);
	    }
	    else{
	      DestroyTermList();
	      return 0;
	    }
	  } else{
	    *err=incorrect_structure;
            FPRINTF(ASCERR,"incorrect_structure in ConvertExpr 1\n");
	    gl_destroy(instances);
	    DestroyTermList();
	    return 0;
	  }
	} else{
	  *err = find_error;
          if (*ferr == impossible_instance) {
			error_reporter_start(ASC_USER_ERROR,NULL,0);
            FPRINTF(ASCERR,"Impossible name or subscript in '");
            WriteName(ASCERR,ExprName(start));
            FPRINTF(ASCERR,"'");
			error_reporter_end_flush();
          }
	  DestroyTermList();
	  return 0;
	}
      }
      break;
    case e_zero:
      /* this should never happen here */
      term = CreateZeroTerm();
      AppendTermBuf(term);
      break;
    case e_int:
      term = CreateIntegerTerm(ExprIValue(start));
      AppendTermBuf(term);
      break;
    case e_real:
      term = CreateRealTerm(ExprRValue(start),ExprRDimensions(start));
      AppendTermBuf(term);
      break;
    case e_card:
      assert(GetEvaluationContext()==NULL);
      SetEvaluationContext(ref);
      svalue = EvaluateSet(ExprBuiltinSet(start),InstanceEvaluateName);
      SetEvaluationContext(NULL);
      cvalue = CardValues(svalue);
      DestroyValue(&svalue);
      switch(ValueKind(cvalue)){
      case integer_value:
	term = CreateIntegerTerm(IntegerValue(cvalue));
	AppendTermBuf(term);
	break;
      case error_value:
	DestroyTermList();
	FigureOutError(cvalue,err,ferr);
	DestroyValue(&cvalue);
	return 0;
      default:
	FPRINTF(ASCERR,"This message should never occur.\n");
	FPRINTF(ASCERR,"If it does tell %s\n",ASC_BIG_BUGMAIL);
	DestroyValue(&cvalue);
	DestroyTermList();
	*err = incorrect_structure;
	return 0;
      }
      DestroyValue(&cvalue);
      break;
    case e_sum:
      if (AppendList(ref,rel,ExprBuiltinSet(start),SUM,err,ferr)){
	DestroyTermList();
	return 0;
      }
      break;
    case e_prod:
      if (AppendList(ref,rel,ExprBuiltinSet(start),PROD,err,ferr)){
	DestroyTermList();
	return 0;
      }
      break;
    case e_func:
      term = CreateFuncTerm(ExprFunc(start));
      AppendTermBuf(term);
      break;
    default:
      *err = incorrect_structure;
      FPRINTF(ASCERR,"incorrect_structure in ConvertExpr 2\n");
      DestroyTermList();
      return 0;
    }
    start = NextExpr(start);
  }
  result = ConvertTermBuf(newside);
  DestroyTermList();
  return result;
  /* we do not check result here. that is the callers job */
}

static
CONST struct Expr *FindRHS(CONST struct Expr *ex)
{
  CONST struct Expr *rhs = NULL, *previous = NULL;
  unsigned depth=0;
  while(ex!=NULL){
    switch(ExprType(ex)){
    case e_zero:
    case e_var:
    case e_int:
    case e_real:
    case e_boolean:
    case e_set:
    case e_symbol:
    case e_card:
    case e_choice:
    case e_sum:
    case e_prod:
    case e_union:
    case e_inter:
      if ((++depth)==1) rhs = ex;
      break;
      /* binary operators */
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_or:
    case e_and:
    case e_in:
      if ((--depth)==1) rhs = ex;
      break;
    case e_equal:
    case e_notequal:
    case e_less:
    case e_greater:
    case e_lesseq:
    case e_greatereq:
      if (NextExpr(ex)==NULL) {
	return NextExpr(rhs);
      } else {
	return NULL;
      }
    case e_func:
    case e_uminus:
    case e_not:
      if (depth==1) {
        rhs = ex;
      }
      break;
    case e_st:
      Asc_Panic(2, NULL, "Such that expressions are not allowed.\n");
      break;
    case e_minimize:
    case e_maximize:
      Asc_Panic(2, NULL,
                "Maximize and minimize are not allowed in expression.\n"
                "They are only allowed in relations.\n");
      break;
    default:
      Asc_Panic(2, NULL, "Unknown expression node type.\n");
      break;
    }
    previous = ex;
    ex = NextExpr(ex);
  }
  return NULL;
}

/*********************************************************************\
  This code is to support the conversion from postfix to infix.
\*********************************************************************/


#define PopTermStack(stack) \
   ((struct relation_term *)gs_stack_pop((stack)))
#define PushTermStack(stack,term) \
   (gs_stack_push((stack),(char*)(term)))

/*
 * *err = 0 if ok, 1 otherwise. Sets up infix pointers.
 */
static struct relation_term
*InfixArr_MakeSide(CONST struct relation_side_temp *tmp, int *err)
{
  struct relation_term *term = NULL;
  struct relation_term *left;
  long len,count=0;
  struct gs_stack_t *stack;
  enum Expr_enum t;

  *err = 0;
  len = tmp->length;
  stack = gs_stack_create(len);
  while(count < len) {
    term = A_TERM(&(tmp->side[count])); /* aka tmp->side+count */
    switch(t = RelationTermType(term)) {
    case e_var:
    case e_int:
    case e_real:
    case e_zero:
      gs_stack_push(stack,(char *)term);
      break;
    case e_func:
      left = A_TERM(gs_stack_pop(stack));
      F_TERM(term)->left = left;
      gs_stack_push(stack,(char *)term);
      break;
    case e_uminus:
      left = A_TERM(gs_stack_pop(stack));
      U_TERM(term)->left = left;
      gs_stack_push(stack,(char *)term);
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      B_TERM(term)->right = A_TERM(gs_stack_pop(stack));
      B_TERM(term)->left = A_TERM(gs_stack_pop(stack));
      gs_stack_push(stack,(char *)term);
      break;
    default:
      Asc_Panic(2, "MakeInfix",
                "Dont know this type of relation term in MakeInfix\n");
      break;
    }
    count++;
  }
  term = A_TERM(gs_stack_pop(stack));
  if (!gs_stack_empty(stack)) {
    /* ensure that the stack is empty */
    FPRINTF(ASCERR,"stacksize %ld\n",stack->size);
    FPRINTF(ASCERR,"Something screwy with Infix_MakeSide\n");
    *err = 1;
  }
  gs_stack_destroy(stack,0);
  return term;
}

void DoInOrderVisit(struct relation_term *term,
		    struct relation *r,
		    void (*func)(struct relation_term *,
				 struct relation *))
{
  if (term) {
    switch(RelationTermType(term)) {
    case e_zero:
    case e_var:
    case e_int:
    case e_real:
      (*func)(term,r);
      break;
    case e_func:
      DoInOrderVisit(F_TERM(term)->left,r,func);
      (*func)(term,r);
      break;
    case e_uminus:
      DoInOrderVisit(U_TERM(term)->left,r,func);
      (*func)(term,r);
      break;
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      DoInOrderVisit(B_TERM(term)->left,r,func);
      (*func)(term,r);
      DoInOrderVisit(B_TERM(term)->right,r,func);
      break;
    default:
      return;
    }
  }
}

#if 0 /* potential future use */
/* tHis is a recursive deallocation of a term tree.
   It presupposes all terms are independently allocated,
   which at present is true nowhere in the compiler.
   It's a nice little function, though so we'll keep it in case,
   but not compile it in the meantime.
   Token relations term lists are not independently allocated.
*/
void DestroyTermTree(struct relation_term *term)
{
  if (term) {
    switch(term->t) {
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
      DestroyTermTree(B_TERM(term)->left);
      DestroyTermTree(B_TERM(term)->right);
      ascfree((char *)term);
      term = NULL;
      break;
    case e_func:
      DestroyTermTree(F_TERM(term)->left);
      ascfree((char *)term);
      term = NULL;
      break;
    case e_uminus:
      DestroyTermTree(U_TERM(term)->left);
      break;
    case e_zero:
    case e_var:
    case e_int:
    case e_real:
      ascfree((char *)term);
      term = NULL;
      break;
    default:
      FPRINTF(ASCERR,"DestroyTermTree called with unexpected term type\n");
      break;
    }
  }
}
#endif

/*********************************************************************\
  Relation Processing for Instantiation.
\*********************************************************************/
static void DestroyTermSide(struct relation_side_temp *);
void DestroyVarList(struct gl_list_t *, struct Instance *);

struct relation *CreateTokenRelation(struct Instance *reference,
				     struct Instance *relinst,
				     CONST struct Expr *ex,
				     enum relation_errors *err,
				     enum find_errors *ferr)
{
  struct relation *result;
  CONST struct Expr *rhs_ex,*last_ex;
  int lhs,rhs;
  enum Expr_enum relop;
  struct relation_side_temp leftside,rightside;
  assert(reference&&relinst&&ex&&err&&ferr);
  g_relation_var_list = gl_create(20l);
  *err = okay;
  *ferr = correct_instance;
  last_ex = FindLastExpr(ex);
  switch(ExprType(last_ex)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    relop = ExprType(last_ex);
    rhs_ex = FindRHS(ex);
    if (rhs_ex!=NULL){
      lhs = ConvertExpr(ex,rhs_ex,reference,relinst,err,ferr,&leftside);
      if(!lhs) {
        if (g_relation_var_list!=NULL) {
           DestroyVarList(g_relation_var_list,relinst);
	}
	g_relation_var_list = NULL;
	return NULL;
      }
      rhs = ConvertExpr(rhs_ex,last_ex,reference,relinst,err,ferr,&rightside);
      if(!rhs) {
	DestroyTermSide(&leftside);
        if (g_relation_var_list!=NULL) {
           DestroyVarList(g_relation_var_list,relinst);
	}
	g_relation_var_list = NULL;
	return NULL;
      }
    }
    else{
      *err = incorrect_structure;
      FPRINTF(ASCERR,"Error finding relational operator.\n");
      if (g_relation_var_list!=NULL) {
           DestroyVarList(g_relation_var_list,relinst);
      }
      g_relation_var_list = NULL;
      return NULL;
    }
    break;
  case e_maximize:
  case e_minimize:
    relop = ExprType(last_ex);
    rhs = 0;
    lhs=ConvertExpr(ex,last_ex,reference,relinst,err,ferr,&leftside);
    if (!lhs) {
      if (g_relation_var_list!=NULL) {
         DestroyVarList(g_relation_var_list,relinst);
      }
      g_relation_var_list = NULL;
      return NULL;
    }
    break;
  default:
    *err = incorrect_structure;
    ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Error expression missing relational operator.");
    if (g_relation_var_list!=NULL) {
       DestroyVarList(g_relation_var_list,relinst);
    }
    g_relation_var_list = NULL;
    return NULL;
  }
  result = CreateRelationStructure(relop,crs_NEWUNION);
  RelationRefCount(result) = 1;
  if (lhs) { /* always true */
    int status;
    RTOKEN(result).lhs_len = leftside.length;
    RTOKEN(result).lhs = leftside.side;
    RTOKEN(result).lhs_term = InfixArr_MakeSide(&leftside,&status);
#ifndef NDEBUG
    if (status) {
      FPRINTF(ASCERR,"Anomaly in ");
      WriteInstanceName(ASCERR,relinst,NULL);
      FPRINTF(ASCERR," LHS.\n");
    }
#endif
  }
  if (rhs) { /* sometimes true */
    int status;
    RTOKEN(result).rhs_len = rightside.length;
    RTOKEN(result).rhs = rightside.side;
    RTOKEN(result).rhs_term = InfixArr_MakeSide(&rightside,&status);
#ifndef NDEBUG
    if (status) {
      FPRINTF(ASCERR,"Anomaly in ");
      WriteInstanceName(ASCERR,relinst,NULL);
      FPRINTF(ASCERR," RHS.\n");
    }
#endif
  }
  result->vars = g_relation_var_list;
  g_relation_var_list = NULL;
  return result;
}

/**************************************************************************\
  OpCodeRelation processing.
\**************************************************************************/

struct relation *CreateOpCodeRelation(struct Instance *reference,
                                      struct Instance *relinst,
                                      CONST struct Expr *ex,
                                      enum relation_errors *err,
                                      enum find_errors *ferr)
{
  struct relation *result;

  (void)reference;  /*  stop gcc whine about unused parameter  */
  (void)relinst;    /*  stop gcc whine about unused parameter  */
  (void)ex;         /*  stop gcc whine about unused parameter  */
  (void)err;        /*  stop gcc whine about unused parameter  */
  (void)ferr;       /*  stop gcc whine about unused parameter  */

  result = CreateRelationStructure(e_equal,crs_NEWUNION); /* needs a passed in relop */
  RelationRefCount(result) = 1;
  ROPCODE(result).lhs = NULL;
  ROPCODE(result).rhs = NULL;
  ROPCODE(result).args = NULL;
  ROPCODE(result).constants = NULL;
  result->vars = NULL;

  return result;
}

/*
 **************************************************************************
 * Destroy Code.
 *
 * This takes care of destroying the parts of relations.
 * At the same time it ensures that any variables that are
 * incident upon the relations have their relation references
 * removed. This is done using the RemoveRelation function.
 **************************************************************************
 */

static void DestroyTermSide(struct relation_side_temp *temp)
{
  if (temp!=NULL){
    if (temp->side !=NULL) ascfree(temp->side);
  }
  temp->side=NULL;
  temp->length=0L;
}

void DestroyVarList(struct gl_list_t *l, struct Instance *inst)
{
  register struct Instance *ptr;
  register unsigned long c;
  for(c=gl_length(l);c>=1;c--)
    if (NULL != (ptr = (struct Instance *)gl_fetch(l,c)))
      RemoveRelation(ptr,inst);
  gl_destroy(l);
}

void DestroyRelation(struct relation *rel, struct Instance *relinst)
{
  struct ExtCallNode *ext;
  if (rel==NULL) return;
  assert(RelationRefCount(rel));
  if (--(RelationRefCount(rel))==0) {
    switch (GetInstanceRelationType(relinst)) {
    case e_token:
      if (RTOKEN(rel).lhs!=NULL) {
        ascfree(RTOKEN(rel).lhs);
      }
      if (RTOKEN(rel).rhs!=NULL) {
        ascfree(RTOKEN(rel).rhs);
      }
      if (RTOKEN(rel).btable > 0) {
        BinTokenDeleteReference(RTOKEN(rel).btable);
      }
      break;
    case e_opcode:
      if (ROPCODE(rel).lhs) {
        ascfree((char *)ROPCODE(rel).lhs);
      }
      if (ROPCODE(rel).rhs) {
        ascfree((char *)ROPCODE(rel).rhs);
      }
      if (ROPCODE(rel).constants) {
        ascfree((char *)ROPCODE(rel).constants);
      }
      break;
    case e_glassbox:
      if (RGBOX(rel).args) {
        ascfree((char *)RGBOX(rel).args);
      }
      break;
    case e_blackbox:		/* KAA_DEBUG -- double check */
      ext = RBBOX(rel).ext;
      DestroyExtCall(ext,relinst);
      ascfree((char *)RBBOX(rel).ext);
      break;
    default:
      /*NOTREACHED we hope */
      FPRINTF(ASCERR,"Weird relation type passed to DestroyRelation\n");
      break;
    }
    if (rel->share != NULL) {
      ascfree(rel->share);
    }
  }

  if (rel->vars) DestroyVarList(rel->vars,relinst);
  ascfree((char *)rel);
}


/*
 **************************************************************************
 * Variable Maintenance.
 *
 * Relations need to keep a *unique* list of variables incident upon
 * them. This is for the purpose of constructing incidence matrices
 * etc, when solving. However variables move around and also disappear,
 * in particular when being ARE_THE_SAME'd. This code does that variable
 * maintenance.
 *
 * This requires some explanaition. There are a number of cases
 * to consider.
 *
 * 1) the old instance does not exist in the var list -- do nothing.
 *
 * 2) the old instance exists, but the new does not -- store the
 *    the new instance in the slot where the old instance was and
 *    return.
 *
 * 3) the old instance exists, *and* the new instance also exists in
 *    the varlist. This can happen in the case when 2 variables
 *    incident upon a relation are going to be ATS'ed (not wise but
 *    possible.) We need to run down the entire token list in the case
 *    of token relations, or opcode array in the case of opcode relations,
 *    fixing up the indexing. This is expensive and uses the
 *    DeleteAndChange function.
 *
 *  4) the new instance is NULL, which can happen transiently during some
 *     operations. This defaults to case 2).
 **************************************************************************
 */

static
void ChangeTermSide(union RelationTermUnion *side,
		    unsigned long int len,
		    unsigned long int old,
		    unsigned long int new)
{
  register long c;
  register struct relation_term *term;
  for(c=len-1;c>=0;c--){
    term = A_TERM(&(side[c]));
    if (term->t == e_var){
      if (V_TERM(term)->varnum == old)
	V_TERM(term)->varnum = new;
      else
	if (V_TERM(term)->varnum > old) V_TERM(term)->varnum--;
    }
  }
}

/*
 * Delete one of these variable numbers,
 */
static
void DeleteAndChange(struct relation *rel,
		     unsigned long int pos1,
		     unsigned long int pos2)
{
  if (pos1 < pos2) Swap(&pos1,&pos2);
  /* pos1 > pos2 now */
  gl_delete(rel->vars,pos1,0);
  if (RTOKEN(rel).rhs) {
    ChangeTermSide(RTOKEN(rel).rhs,RTOKEN(rel).rhs_len,pos1,pos2);
  }
  if (RTOKEN(rel).lhs) {
    ChangeTermSide(RTOKEN(rel).lhs,RTOKEN(rel).lhs_len,pos1,pos2);
  }
}

static
union RelationUnion *CopyRelationShare(union RelationUnion *ru,
                                       enum Expr_enum type)
{
  struct TokenRelation *result,*src;
  long int delta;

  if (type != e_token) {
    FPRINTF(ASCERR,
            "CopyRelationShare not implemented except for token relations\n");
    return NULL;
  }
  src = (struct TokenRelation *)ru;
  /* yes, the sizeof in the following is correct */
  result = (struct TokenRelation *)ascmalloc(sizeof(union RelationUnion));
  if (result==NULL) {
    Asc_Panic(2, "CopyRelationShare" ,"Insufficient memory.");
    return NULL; /* NOT REACHED */
  }
  result->lhs = CopyRelationSide(src->lhs,src->lhs_len);
  if (result->lhs != NULL) {
    delta = UNION_TERM(src->lhs_term) - src->lhs;
    result->lhs_term = A_TERM(result->lhs + delta);
    result->lhs_len = src->lhs_len;
  } else {
    result->lhs_term = NULL;
    result->lhs_len = 0;
  }

  result->rhs = CopyRelationSide(src->rhs,src->rhs_len);
  if (result->rhs != NULL) {
    delta = UNION_TERM(src->rhs_term) - src->rhs;
    result->rhs_term = A_TERM(result->rhs + delta);
    result->rhs_len = src->rhs_len;
  } else {
    result->rhs_term = NULL;
    result->rhs_len = 0;
  }
  result->relop = src->relop;
  result->ref_count = src->ref_count;

  return (union RelationUnion *)result;
}

void ModifyTokenRelationPointers(struct Instance *relinst,
			         struct relation *rel,
			         CONST struct Instance *old,
			         CONST struct Instance *new)
{
  unsigned long pos,other;

  (void)relinst;    /*  stop gcc whine about unused parameter  */

  assert(rel!=NULL);

  if (old==new) {
    return;
  }
  if (new!=NULL){
    pos = gl_search(rel->vars,old,(CmpFunc)CmpP);
    if (pos != 0) {
      other = gl_search(rel->vars,new,(CmpFunc)CmpP);
      if (other != 0L) {
        if (RelationRefCount(rel) > 1) {
          /* must copy and split off a separate token string
           * so as not to mess up sharer's varlists.
           */
          RelationRefCount(rel)--; /* adjusts the shared data refcount */
          rel->share = CopyRelationShare(rel->share,e_token);
          RelationRefCount(rel) = 1; /* init the new copied data refcount */
        }
	gl_store(rel->vars,pos,(VOIDPTR)new);
	DeleteAndChange(rel,pos,other);		/* case 3 */
      } else {
	gl_store(rel->vars,pos,(char *)new);	/* case 2 */
      }
    } else{					/* case 1 */
      FPRINTF(ASCERR,"Warning ModifyTokenRelationPointers arg not found.\n");
      FPRINTF(ASCERR,"This shouldn't effect your usage at all.\n");
    }
  } else {						/* case 4 */
    pos = gl_search(rel->vars,old,(CmpFunc)CmpP);
    if (pos) {
      gl_store(rel->vars,pos,(VOIDPTR)new);
    }
  }
}

void ModifyGlassBoxRelPointers(struct Instance *relinst,
			       struct relation *rel,
			       CONST struct Instance *old,
			       CONST struct Instance *new)
{
  unsigned long pos,other;

  (void)relinst;    /*  stop gcc whine about unused parameter  */

  assert(rel!=NULL);

  if (old==new) return;
  if (new){
    if (0 != (pos = gl_search(rel->vars,old,(CmpFunc)CmpP))) {
      if (0 != (other = gl_search(rel->vars,new,(CmpFunc)CmpP))){
	gl_store(rel->vars,pos,(VOIDPTR)new);
	FPRINTF(ASCERR,"Incidence for relation is inaccurate\n");
      } else {
	gl_store(rel->vars,pos,(char *)new);	/* case 2 */
      }
    } else {					/* case 1 */
      FPRINTF(ASCERR,"Warning ModifiyRelationPointers not found.\n");
      FPRINTF(ASCERR,"This shouldn't effect your usage at all.\n");
    }
  }
  else						/* case 4 */
    if (0 != (pos = gl_search(rel->vars,old,(CmpFunc)CmpP)))
      gl_store(rel->vars,pos,(VOIDPTR)new);
}

/*********************************************************************\
  This procedure should change all references of "old" in relation
  instance rel to "new. This is similar to ModifyTokenRelationPointers
  but handles the "external variables incident on the relation".
  Remember that variables may be exist more than once in the list, so
  that we have to find ALL occurrences.
\*********************************************************************/
void ModifyBlackBoxRelPointers(struct Instance *relinst,
			       struct relation *rel,
			       CONST struct Instance *old,
			       CONST struct Instance *new)
{
  unsigned long len1,c1,len2,c2;
  struct gl_list_t *branch, *extvars;
  struct Instance *arg;

  (void)relinst;    /*  stop gcc whine about unused parameter  */

  assert(rel!=NULL);
  if (old==new) return;
  extvars = ExternalCallArgList(RBBOX(rel).ext);
  if (extvars==NULL) return;

  len1 = gl_length(extvars);
  if (!len1) return;
  for (c1=1;c1<=len1;c1++){	/* find all occurrences and change them */
    branch = (struct gl_list_t *)gl_fetch(extvars,c1);
    if (branch){
      len2 = gl_length(branch);
      for (c2=1;c2<=len2;c2++){
	arg = (struct Instance *)gl_fetch(branch,c2);
	if (arg==old)
	  gl_store(branch,c2,(VOIDPTR)new);
      }
    }
  }
}


static
int ReturnFromValue(struct value_t value)
{
  assert(ValueKind(value)==error_value);
  switch(ErrorValue(value)){
  case name_unfound:
  case undefined_value: DestroyValue(&value); return 0;
  default: DestroyValue(&value); return 1;
  }
}

/* forward declaration for recursing Check functions of relation */
static int
CheckExpr(CONST struct Instance *ref, CONST struct Expr *start,
  CONST struct Expr *stop, int list);


/**********************************************************************\
  Here we check that vars are well defined, a precondition to FOR
  statements being executed.
  If lists of vars are acceptable (don't know why they would be)
  list should be passed in as 1, otherwise 0.
  At present e_var acceptable ARE:
  REAL_ATOM_INSTANCE
  Well defined Real and Integer constants.
  CreateTermFromInst() and CheckExpr() must have matching semantics.

 Returns: -1 --> OK,
	  0  --> BAD (undefined/unassigned) try again later
	  1  --> incurably BAD
\**********************************************************************/
static int CheckExprVar(CONST struct Instance *ref, CONST struct Name *name,
			int list)
{
  struct gl_list_t *instances;
  symchar *str;
  struct Instance *inst;
  CONST struct for_var_t *fvp;
  enum find_errors err;
  if(NULL != (str = SimpleNameIdPtr(name))){
    if (TempExists(str)) {
      if (ValueKind(TempValue(str))==integer_value) {
	return -1;
      } else {
	return 1;
      }
    }
    if (GetEvaluationForTable() != NULL &&
        (NULL != (fvp=FindForVar(GetEvaluationForTable(),str)))) {
      if (GetForKind(fvp)==f_integer) {
	return -1;
      } else {
	return 1;
      }
    }
  }
  instances = FindInstances(ref,name,&err); /* need noisy version of Find */
  if (instances == NULL){
    switch(err){
    case unmade_instance:
    case undefined_instance: return 0;
    default:
      return 1;
    }
  }
  else{
    if (gl_length(instances)==1) {
      inst = (struct Instance *)gl_fetch(instances,1);
      gl_destroy(instances);
      switch(InstanceKind(inst)){
      case REAL_ATOM_INST:
	return -1;
      case REAL_CONSTANT_INST:
	if (IsWild(RealAtomDims(inst))) {
	  return 0;
	} /* else fall through to check assignment */
      case INTEGER_CONSTANT_INST:
	if (AtomAssigned(inst)) {
	  return -1;
        }
	return 0;
      default: return 1; /* bogus var type found */
      }
    }
    else if (list){
/* this part of the code is most peculiar, and semantics may not
   match it. We need to find out what is the semantics of list. */
      unsigned long c,len;
      len = gl_length(instances);
      for(c=1;c<=len;c++){
	inst = (struct Instance *)gl_fetch(instances,1);
	switch(InstanceKind(inst)){
	case REAL_ATOM_INST:
	  break;
	case REAL_CONSTANT_INST:
	  if (IsWild(RealAtomDims(inst))) {
	    gl_destroy(instances);
	    return 0;
	  } /* else fall through to check assignment */
	case INTEGER_CONSTANT_INST:
	  if (!AtomAssigned(inst)){
	    gl_destroy(instances);
	    return 1;
	  }
	default:
	  gl_destroy(instances);
	  return 0;
	}
      }
      gl_destroy(instances);
      return -1;
    }
    else {
      gl_destroy(instances);
      return 1;
    }
  }
}

static int CheckCard(CONST struct Instance *ref, CONST struct Set *sptr)
{
  struct value_t svalue,cvalue;
  assert(GetEvaluationContext() ==NULL);
  SetEvaluationContext(ref);
  svalue = EvaluateSet(sptr,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  cvalue = CardValues(svalue);
  DestroyValue(&svalue);
  switch(ValueKind(cvalue)){
  case integer_value:
    DestroyValue(&cvalue);
    return 1;
  case error_value:
    return ReturnFromValue(cvalue);
  default:
    DestroyValue(&cvalue);
    return 0;
  }
}

static int CheckLowerAndUpper(CONST struct Instance *ref,
				CONST struct Expr *low,
				CONST struct Expr *up)
{
  struct value_t lower,upper;
  assert(GetEvaluationContext()==NULL);
  SetEvaluationContext(ref);
  lower = EvaluateExpr(low,NULL,InstanceEvaluateName);
  upper = EvaluateExpr(up,NULL,InstanceEvaluateName);
  SetEvaluationContext(NULL);
  lower = CheckIntegerCoercion(lower);
  upper = CheckIntegerCoercion(upper);
  if ((ValueKind(lower)==integer_value)&&(ValueKind(upper)==integer_value)){
    DestroyValue(&lower);
    DestroyValue(&upper);
    return 1;
  }
  else{
    if (ValueKind(lower)==error_value) {
      DestroyValue(&upper);
      return ReturnFromValue(lower);
    }
    else if (ValueKind(upper)==error_value){
      DestroyValue(&lower);
      return ReturnFromValue(upper);
    }
    else{
      DestroyValue(&lower);
      DestroyValue(&upper);
      return 1;
    }
  }
}

/* called only by CheckListExpr. does what? */
static int CheckSuchThat(CONST struct Instance *ref, CONST struct Expr *ex)
{
  CONST struct Expr *depth_one,*node;
  struct set_t *sptr;
  symchar *tmp_name;
  unsigned long c,len;
  struct value_t iteration_set,tmp_value;
  if (CorrectSuchThat(ex,&depth_one,&node)){
    if (!DoNameAndSet(NextExpr(depth_one),node,ref,&tmp_name,&iteration_set)){
      node = NextExpr(depth_one);
      sptr = SetValue(iteration_set);
      switch(SetKind(sptr)){
      case empty_set:
	DestroyValue(&iteration_set);
	return 1;
      case integer_set:
      case string_set:
	if (!TempExists(tmp_name)){
	  AddTemp(tmp_name);
	  len = Cardinality(sptr);
	  for(c=1;c<=len;c++){
	    if (SetKind(sptr)==string_set)
	      tmp_value = CreateSymbolValue(FetchStrMember(sptr,c),1);
	    else
	      tmp_value = CreateIntegerValue(FetchIntMember(sptr,c),1);
	    SetTemp(tmp_name,tmp_value);
	    if (!CheckExpr(ref,ex,node,0)){
	      RemoveTemp(tmp_name);
	      DestroyValue(&tmp_value);
	      DestroyValue(&iteration_set);
	      return 0;
	    }
	    DestroyValue(&tmp_value);
	  }
	  RemoveTemp(tmp_name);
	  DestroyValue(&iteration_set);
	}
      }
      return 1;
    }
    else
      return 0;
  }
  return 1;
}

static int CheckListExpr(CONST struct Instance *ref, CONST struct Expr *ex)
{
  if (ExprContainsSuchThat(ex)!=NULL) {
    return CheckSuchThat(ref,ex);
  } else {
    return CheckExpr(ref,ex,NULL,1);
  }
}

/**********************************************************************\
  here we enforce that sets are well defined.
\**********************************************************************/
static int CheckList(CONST struct Instance *ref, CONST struct Set *sptr)
{
  while(sptr!=NULL){
    if (SetType(sptr)){		/* range of values */
      if (!CheckLowerAndUpper(ref,GetLowerExpr(sptr),GetUpperExpr(sptr)))
	return 0;
    }
    else{			/* single expression */
      if (!CheckListExpr(ref,GetSingleExpr(sptr)))
	return 0;
    }
    sptr = NextSet(sptr);
  }
  return 1;
}

/**********************************************************************\
 CheckExpr(ref, start, stop, list)
 struct Instance *ref; context of the relation instance, ie parent.
 int list; boolean whether list of instances are acceptable
 struct Expr *start, *stop; pointers to the portion of relation this
 checks.
\**********************************************************************/
static int CheckExpr(CONST struct Instance *ref,
			CONST struct Expr *start,
			CONST struct Expr *stop,
			int list)
{
  while (start!=stop){
    switch(ExprType(start)){
    case e_plus:
    case e_minus:
    case e_times:
    case e_divide:
    case e_power:
    case e_ipower:
    case e_uminus:
    case e_zero:
    case e_int:
    case e_real:
    case e_func:
      break;			/* automatically okay! */
    case e_var:
      switch(CheckExprVar(ref,ExprName(start),list)){
      case 0: return 0;
      case 1: return 1;
      }
      break;
    case e_card:
      if (!CheckCard(ref,ExprBuiltinSet(start))) return 0;
      break;
    case e_sum:
      if (!CheckList(ref,ExprBuiltinSet(start))) return 0;
      break;
    case e_prod:
      if (!CheckList(ref,ExprBuiltinSet(start))) return 0;
      break;
    default:
      return 0;
    }
    start = NextExpr(start);
  }
  return 1;
}

/* see header. returns 1 if relation expression is fully instantiable
 * ie all vars exist, and, if need be, properly initialized.
 */
int CheckRelation(CONST struct Instance *reference, CONST struct Expr *ex)
{
  CONST struct Expr *last_ex,*rhs_ex;
  last_ex = FindLastExpr(ex);
  switch(ExprType(last_ex)){
  case e_equal:
  case e_notequal:
  case e_less:
  case e_greater:
  case e_lesseq:
  case e_greatereq:
    rhs_ex = FindRHS(ex);
    if (!CheckExpr(reference,rhs_ex,last_ex,0)) {
      return 0;
    }
    return CheckExpr(reference,ex,rhs_ex,0);
  case e_maximize:
  case e_minimize:
    return CheckExpr(reference,ex,last_ex,0);
  default:
    return 0;
  }
}


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
 * calling this function.
 */
static union RelationTermUnion
*CopyRelationSide(union RelationTermUnion *old, unsigned long len)
{
  struct relation_term *term;
  union RelationTermUnion *arr;
  unsigned long c;
  long int delta;

  if (!old || !len) return NULL;
  arr = (union RelationTermUnion *)
	ascmalloc(len*sizeof(union RelationTermUnion));
  if (arr==NULL) {
    FPRINTF(ASCERR,"CopyTokenRelation: Insufficient memory :-(.\n");
    return NULL;
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
      Asc_Panic(2, NULL, "Unknown term type in CopyRelationSide\n");
      break;
    }
  }
#undef ADJPTR

  return arr;
}


/*
 * This function will *always create a new variables list, from
 * the copylist provided and the tmpnums of the variables
 * in the source instances var list. The copylist variables will be
 * made aware of the destination relation instance. -baa
 */
static
struct gl_list_t *CopyAnonRelationVarList(CONST struct Instance *src_inst,
                                          struct Instance *dest_inst,
				          struct gl_list_t *copylist)
{
  struct Instance *var;
  CONST struct gl_list_t *oldvarlist;
  struct gl_list_t *newvarlist;
  unsigned long len,c;
  assert(copylist!=NULL);

  oldvarlist = RelationVarList(GetInstanceRelationOnly(src_inst));
  len = gl_length(oldvarlist);
  newvarlist = gl_create(len);
  for (c=1;c<=len;c++) {

    var = (struct Instance *)gl_fetch(oldvarlist,c);
    var = (struct Instance *)gl_fetch(copylist,GetTmpNum(var));
    assert(var!=NULL);

    gl_append_ptr(newvarlist,(VOIDPTR)var);
    /* ^^^^^^^^^^^ means copied list is not sorted and future modify
     * calls may lie if they make a sorted assumption.
     */

    AddRelation(var,dest_inst);
  }
  return newvarlist;
}

/*
 * This function will *always create a new variables list, from
 * the variable list provided. The variables will be copied
 * and made aware of the destination relation instance.
 */
static
struct gl_list_t *CopyRelationVarList(struct Instance *dest_inst,
				      struct gl_list_t *copylist)
{
  struct Instance *var;
  struct gl_list_t *newvarlist = NULL;
  unsigned long len,c;
#ifndef NDEBUG
  unsigned long pos;
#endif

  if (copylist) {
    len = gl_length(copylist);
    newvarlist = gl_create(len);
    for (c=1;c<=len;c++) {
      var = (struct Instance *)gl_fetch(copylist,c);
#ifndef NDEBUG
      /* garbage in, garbage out */
      pos = gl_search(newvarlist,var,(CmpFunc)CmpP);
      if (pos) {
	Asc_Panic(2, NULL, "Corrupted variable list in CopyTokenRelation\n");
      }
#endif
      gl_append_ptr(newvarlist,(VOIDPTR)var);
      /* ^^^^^^^^^^^ means copied list is not sorted and future modify
       * calls may lie.
       */
      AddRelation(var,dest_inst);
    }
  } else {
    /* we will always return a varlist, even if empty */
    newvarlist = gl_create(1L);
  }
  return newvarlist;
}

static
struct relation *CopyTokenRelation(CONST struct Instance *src_inst,
				   struct Instance *dest_inst,
				   struct gl_list_t *copylist)
{
  CONST struct relation *src;
  enum Expr_enum type;
  struct relation *result;

  src = GetInstanceRelation(src_inst,&type);
  if (src== NULL) {
    return NULL;
  }

  result = CreateRelationStructure(RelRelop(src),crs_NEWUNION);

  RelationRefCount(result) = 1;

  result->share = CopyRelationShare(src->share,e_token);
  result->vars = CopyRelationVarList(dest_inst,copylist);

/* obsolete. If needed, replace with InfixArr_MakeSide.
 *if (RTOKEN(result).lhs)
 *  RTOKEN(result).lhs_term = Infix_MakeSide(RTOKEN(result).lhs);
 *if (RTOKEN(result).rhs)
 *  RTOKEN(result).rhs_term = Infix_MakeSide(RTOKEN(result).rhs);
 */
  result->d = src->d;
  result->residual = src->residual;
  result->multiplier = src->multiplier;
  result->nominal = src->nominal;
  result->iscond = src->iscond;
  return result;
}


/*
 * This does nothing but copy the local struct relation content
 * and init the target vars to NULL.
 */
static
void CopyRelationHead(struct relation *src,struct relation *target)
{
  target->share = src->share;
  target->residual = src->residual;
  target->multiplier = src->multiplier;
  target->nominal = src->nominal;
  target->iscond = src->iscond;
  target->vars = NULL;
  target->d = src->d;

}

/* see external header. -baa */
struct relation *CopyAnonRelationByReference(CONST struct Instance *src_inst,
                                             struct Instance *dest_inst,
                                             struct gl_list_t *copyvars)
{
  struct relation *src;
  struct relation *result;
  enum Expr_enum type;
  unsigned size;

  src = (struct relation *)GetInstanceRelation(src_inst,&type);
  if (src==NULL)  {
    return NULL;
  }

  result = CreateRelationStructure(RelRelop(src),crs_NOUNION);
  size = sizeof(struct relation);
  CopyRelationHead(src,result);
  /*
   * We now have a verbatim copy. We now need to patch the public
   * stuff. This will vary depending on the type of relation.
   */
  result->vars = CopyAnonRelationVarList(src_inst,dest_inst,copyvars);
  switch (type) {
  case e_token:		/* only need increment the reference count */
  case e_opcode:
  case e_glassbox:	/* Double  check  -- what about the args ?? */
  case e_blackbox:	/* Double  check  -- what about the args ?? */
    RelationRefCount(src)++;
    break;
  default: /*NOTREACHED we hope*/
    break;
  }
  return result;
}

struct relation *CopyRelationByReference(CONST struct Instance *src_inst,
					 struct Instance *dest_inst,
					 struct gl_list_t *copylist)
{
  struct relation *src;
  struct relation *result;
  enum Expr_enum type;
  unsigned size;

  src = (struct relation *)GetInstanceRelation(src_inst,&type);
  if (!src) return NULL;

  result = CreateRelationStructure(RelRelop(src),crs_NOUNION);
  size = sizeof(struct relation);
  ascbcopy(src,result,sizeof(struct relation));
    /* copy everything. Everything includes the pointer to the rel union. */
  /*
   * We now have a verbatim copy. We now need to patch the public
   * stuff. This will vary depending on the type of relation.
   */
  result->vars = CopyRelationVarList(dest_inst,copylist);
  switch (type) {
  case e_token:		/* only need increment the reference count */
  case e_opcode:
  case e_glassbox:	/* Double  check  -- what about the args ?? */
  case e_blackbox:	/* Double  check  -- what about the args ?? */
    RelationRefCount(src)++;
    break;
  default: /*NOTREACHED we hope*/
    break;
  }
  return result;
}

struct relation *CopyRelationToModify(CONST struct Instance *src_inst,
				      struct Instance *dest_inst,
				      struct gl_list_t *copylist)
{
  CONST struct relation *src;
  struct relation *result;
  enum Expr_enum type;

  src = GetInstanceRelation(src_inst,&type);
  if (!src) {
    return NULL;
  }

  switch (type) {
  case e_token:
    result = CopyTokenRelation(src_inst,dest_inst,copylist);
    return result;
  case e_opcode:
    Asc_Panic(2, NULL, "Opcode relation copying not yet supported\n");
    exit(2);/* Needed to keep gcc from whining */
    break;
  case e_glassbox:
    result = CreateGlassBoxRelation(dest_inst, RGBOX(src).efunc,
				    copylist, RGBOX(src).index,
				    RelRelop(src));
    return result;
  case e_blackbox:
    Asc_Panic(2, NULL, "Blackbox relation copying not yet supported\n");
    exit(2);/* Needed to keep gcc from whining */
    break;
  default:
    Asc_Panic(2, NULL, "unknown relation type in CopyRelationToModify\n");
    exit(2);/* Needed to keep gcc from whining */
    break;
  }
}

/*
 * baa
 */
void RelationSetBinTokens(struct Instance *i,int btable, int bindex)
{
  struct relation *rel;
  if (i==NULL || InstanceKind(i) != REL_INST ||
      GetInstanceRelationType(i) != e_token ||
      (rel = (struct relation *)GetInstanceRelationOnly(i)) == NULL) {
    return;
  }
  if (btable != 0 || bindex != 0) {
    if (RTOKEN(rel).btable != INT_MAX) {
      Asc_Panic(2, "RelationSetBinTokens",
                "ERROR: Relation bintokens already set.\n");
    }
  }
  RTOKEN(rel).btable = btable;
  RTOKEN(rel).bindex = bindex;
}

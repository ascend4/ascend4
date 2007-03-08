/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Boyd Safrit
	Copyright (C) 1994 Benjamin Andrew Allan
	Copyright (C) 1994-1995 Benjamin Andrew Allan
	Copyright (C) 1995 Kirk Andre Abbott
	Copyright (C) 2006-2007 Carnegie Mellon University

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
*//** @file
	linsolqr: sparse direct linear solver module

	Implementation Notes:

	9/95: there are many places in linsolqr.c where lower level matrix
	operations have been replaced by higher level ones. These used to
	be flagged as such so that people who wanted to construct newer
	operations from the lowest level mtx operators could see how to
	use the lowlevel operators.

		Later note (JP, 2007): The earlier version of linsolqr, named linsol,
		has been removed completely. There was some comment about it here but 
		it's not here any more.

	10/95: (BAA) I attempted to optimize number_drag through use of
	pointer arithmetic. Testing the resulting code without optimization
	turned on gave about 20% improvement over the original on an alpha
	using the native compiler. However, with optimization on the array
	subscripted version which is left here won by ~40%.
	The results for gcc were similar: pointer 10% better unoptimized,
	subscripted 30% better when -O2.
	For both compilers -O3 was worse! gcc was ~1.8x slower than the
	native cc both optimized and not.
	I have not tested a loop unrolled drag, but I expect unrolling is
	exactly what the optimizer is doing, so why bother?
*//*
	based on linsol
	by Karl Westerberg, Joseph Zaher, Ben Allan. Created: 2/6/90
	and on sqr.pas v1.5, by Boyd Safrit, 1994.
	
	Last in CVS: $Revision: 1.26 $ $Date: 2000/01/25 02:26:58 $ $Author: ballan $
*/

#include "linsolqr.h"
#include "ranki.h"
#include "mtx.h"

#include <math.h>
#include <stdarg.h>
#include <utilities/error.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/mem.h>
#include <utilities/set.h>
#include <general/mathmacros.h>

#include "ranki.h"
#include "ranki2.h"
#include "rankiba2.h"
#include "plainqr.h"

#include "linsolqr_impl.h"

#ifdef __WIN32__
#define copysign _copysign
#endif /* __WIN32__ */

#define LINSOLQR_DEBUG FALSE
/* qr code debugging flag */
#define LINSOL_DEBUG FALSE
/* linsol debugging flag */


/* housekeeping flags */
#define BUILD_DEAD_CODE 0
#undef BUILD_KIRK_CODE


#define RBADEBUG 0
/*	turns on spew that will generate output files
	comparable to and in the same location as for rankiba2
	user specific hardcoded pathnames are involved.
	don't try this at home.
*/

#if RBADEBUG
extern FILE *gscr;
#endif

/*
	timing messages disable flag.
*/
int g_linsolqr_timing = 1;



/*-----------------------------------------------------------------------------
  STRING FUNCTIONS
*/

char *linsolqr_rmethods() {
  static char names[]="Natural, SPK1, TSPK1";
  return names;
}

char *linsolqr_fmethods() {
  static char names[] =
    "SPK1/RANKI,SPK1/RANKI+ROW,Fast-SPK1/RANKI,Fast-SPK1/RANKI+ROW,Fastest-SPK1/MR-RANKI,CondQR,CPQR";
  return names;
}

/** **/
enum reorder_method linsolqr_rmethod_to_enum(char *name) {
  if (strcmp(name,"SPK1")==0) return spk1;
  if (strcmp(name,"TSPK1")==0) return tspk1;
  if (strcmp(name,"Natural")==0) return natural;
  return unknown_r;
}

enum factor_method linsolqr_fmethod_to_enum(char *name) {
  if (strcmp(name,"KIRK-STUFF")==0) return ranki_ka;
  if (strcmp(name,"SPK1/RANKI")==0) return ranki_kw;
  if (strcmp(name,"SPK1/RANKI+ROW")==0) return ranki_jz;
  if (strcmp(name,"Fast-SPK1/RANKI")==0) return ranki_kw2;
  if (strcmp(name,"Fast-SPK1/RANKI+ROW")==0) return ranki_jz2;
  if (strcmp(name,"Fastest-SPK1/MR-RANKI")==0) return ranki_ba2;
  if (strcmp(name,"CondQR")==0) return cond_qr;
  if (strcmp(name,"CPQR")==0) return plain_qr;
  return unknown_f;
}

enum factor_class linsolqr_fmethod_to_fclass(enum factor_method fm)
{
  switch (fm) {
    /* ranki-like things */
    case ranki_kw:
    case ranki_jz:
    case ranki_ba2:
    case ranki_kw2:
    case ranki_jz2:
    case ranki_ka:
      return ranki;
    /* implemented qr things */
    case plain_qr:
      return s_qr;
    /* other stuff unimplemented. all fall through */
    case cond_qr:
    case opt_qr:
    case ls_qr:
    case gauss_ba2:
    case symmetric_lu:
    case unknown_f:
    default:
      return unknown_c;
  }
}

/** **/
char *linsolqr_enum_to_rmethod(enum reorder_method meth) {
	switch (meth) {
		case spk1: return "SPK1";
		case tspk1: return "TSPK1";
		case natural: return "Natural";
		default: return "<unknown reordering method>";
	}
}

char *linsolqr_enum_to_fmethod(enum factor_method meth) {
	switch (meth) {
		case ranki_kw: return "SPK1/RANKI";
		case ranki_jz: return "SPK1/RANKI+ROW";
		case ranki_ba2: return "Fastest-SPK1/MR-RANKI";
		case ranki_kw2: return "Fast-SPK1/RANKI";
		case ranki_jz2: return "Fast-SPK1/RANKI+ROW";
		case ranki_ka: return "KIRK-STUFF";
		case cond_qr: return "CondQR";
		case plain_qr: return "CPQR";
		default: return "<unknown factorization method>";
	}
}

const char *linsolqr_rmethod_description(const enum reorder_method meth) {
	switch (meth) {
		case spk1: return "SPK1 reordering ala Stadtherr";
		case tspk1: return "SPK1 reordering column wise a la Stadtherr";
		case natural: return "Ordering as received from user";
		default: return "<unknown reordering method>";
	}
}

const char *linsolqr_fmethod_description(const enum factor_method meth) {
	switch (meth) {
		case ranki_kw: return "SPK1 reordering with RANKI LU factoring a la Stadtherr";
		case ranki_kw2: return "SPK1 reordering with RANKI LU factoring a la Stadtherr (KW2)";
		case ranki_ba2: return "SPK1 reordering with RANKI LU factoring a la Stadtherr, but fast";
		case ranki_jz: return "SPK1/RANKI LU with pseudo-complete pivoting (JZ)";
		case ranki_jz2: return "SPK1/RANKI LU with pseudo-complete pivoting (JZ2)";
		case ranki_ka: return "KIRK-STUFF/RANKI LU with pseudo-complete pivoting";
		case cond_qr: return "Sparse QR with condition controlled pivoting";
		case plain_qr: return "Sparse QR with column pivoting";
		default: return "<unknown factorization method>";
	}
}

/*-----------------------------------------------------------------------------
  INTERNAL CHECK ROUTINES
*/

/**
	Checks the system handle. Return 0 if ok, 1 otherwise.
*/
int check_system(linsolqr_system_t sys, char *file, int line){
   UNUSED_PARAMETER(file);

   if( ISNULL(sys) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL system handle.");
      return 1;
   }

   switch( sys->integrity ) {
   case OK:
      return 0;
   case DESTROYED:
      error_reporter(ASC_PROG_ERR,file,line,__FUNCTION__,"System was recently destroyed.");
      break;
   default:
      error_reporter(ASC_PROG_ERR,file,line,__FUNCTION__,"System was reused or never created.");
      break;
   }
   return 1;
}

/*-----------------------------------------------------------------------------
  EXTERNAL CALLS (and some of their internals)
*/

/**
	Destroys rhs list.
*/
static void destroy_rhs_list(struct rhs_list *rl){
   while( NOTNULL(rl) ) {
      struct rhs_list *p;
      p = rl;
      rl = rl->next;
      if( NOTNULL(p->varvalue)  )
         ascfree( (POINTER)(p->varvalue) );
      ascfree( (POINTER)p );
   }
}

/**
	Searches for rhs in rhs list, returning it or NULL if not found.
*/
static struct rhs_list *find_rhs(struct rhs_list *rl,real64 *rhs){
   for( ; NOTNULL(rl) ; rl = rl->next )
      if( rl->rhs == rhs )
         break;
   return(rl);
}

/**
	Creates an empty zeroed ludata struct.
	Filled up by ensure_lu_capacity.
*/
static struct lu_auxdata *create_ludata(){
  struct lu_auxdata *d;
  d=ASC_NEW_CLEAR(struct lu_auxdata);
  return d;
}

/**
	Creates an empty zeroed qrdata struct.
	Filled up by ensure_qr_capacity.
*/
static struct qr_auxdata *create_qrdata(){
  struct qr_auxdata *d;
  d=ASC_NEW_CLEAR(struct qr_auxdata);
  return d;
}

/**
	Conditionally destroys a ludata struct and its parts.
*/
static void destroy_ludata(struct lu_auxdata *d) {
  if (NOTNULL(d)) {
    if (NOTNULL(d->pivlist))  ascfree(d->pivlist);
    d->pivlist=NULL;
    if (NOTNULL(d->tmp))  ascfree(d->tmp);
    d->tmp=NULL;
    ascfree(d);
  }
}

/**
	Conditionally destroys a qrdata struct and its parts.
*/
static void destroy_qrdata(struct qr_auxdata *d) {
  if (NOTNULL(d)) {
    if (NOTNULL(d->alpha)) ascfree(d->alpha);
    if (NOTNULL(d->sigma)) ascfree(d->sigma);
    if (NOTNULL(d->tau))   ascfree(d->tau);
    if (NOTNULL(d->hhcol)) ascfree(d->hhcol);
    if (NOTNULL(d->hhrow)) ascfree(d->hhrow);
    if (NOTNULL(d->fill))  ascfree(d->fill);

    ascfree(d);
  }
}

linsolqr_system_t linsolqr_create(){
	linsolqr_system_t sys;

	sys = (linsolqr_system_t)ascmalloc( sizeof(struct linsolqr_header) );
	sys->fclass = unknown_c;
	sys->fmethod = unknown_f;
	sys->rmethod = unknown_r;
	sys->integrity = OK;
	sys->capacity = 0;
	sys->coef = NULL;
	sys->rl = NULL;
	sys->rlength = 0;
	sys->pivot_zero = 1e-12;           /* default value */
	sys->ptol = 0.1;                   /* default value */
	sys->ctol = 0.1;                   /* default value */
	sys->dtol = LINQR_DROP_TOLERANCE;  /* default value */
	sys->factors = NULL;
	sys->inverse = NULL;
	sys->factored = FALSE;
	sys->rowdeps = FALSE;
	sys->coldeps = FALSE;
	sys->rng.low = sys->rng.high = -1;
	sys->reg.row.low = sys->reg.row.high = -1;
	sys->reg.col.low = sys->reg.col.high = -1;
	sys->rank = -1;
	sys->smallest_pivot = MAXDOUBLE;
	sys->qrdata = NULL;
	sys->ludata = NULL;
	return(sys);
}

linsolqr_system_t linsolqr_create_default(){
	linsolqr_system_t L;
	L = linsolqr_create();
	L->fmethod = ranki_ba2;
	L->rmethod = spk1;
	return L;
}

void linsolqr_destroy(linsolqr_system_t sys){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. Not destroyed.");
     return;
   }
   if( NOTNULL(sys->coef) ) {
     CONSOLE_DEBUG("linsolqr contains coef mtx which will NOT be destroyed");
   }
   if( NOTNULL(sys->inverse) )
      mtx_destroy(sys->inverse);
   if( NOTNULL(sys->factors) )
      mtx_destroy(sys->factors);
   destroy_rhs_list(sys->rl);
   destroy_qrdata(sys->qrdata);
   destroy_ludata(sys->ludata);
   sys->integrity = DESTROYED;
   ascfree( (POINTER)sys );
}

void linsolqr_set_matrix(linsolqr_system_t sys,mtx_matrix_t mtx){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Bad linsolqr_system_t found. coef mtx not set.");
     return;
   }
   sys->coef = mtx;
}


void linsolqr_set_region(linsolqr_system_t sys,mtx_region_t region){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. coef mtx not set.");
     return;
   }
   CONSOLE_DEBUG("Region rows=[%d,%d], cols=[%d,%d]",region.row.low,region.row.high,region.col.low,region.col.high);
   sys->reg = region;
}


mtx_matrix_t linsolqr_get_matrix(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->coef );
}

mtx_region_t *linsolqr_get_region(linsolqr_system_t sys){
    return &(sys->reg);
}

void linsolqr_add_rhs(linsolqr_system_t sys,
                      real64 *rhs,
                      boolean transpose
){
   struct rhs_list *rl;

   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. rhs not added.");
     return;
   }
   rl = find_rhs(sys->rl,rhs);
   if( NOTNULL(rl) ) {
      return;
   } else {
     if( NOTNULL(rhs) ) {
       rl = (struct rhs_list *)ascmalloc( sizeof(struct rhs_list) );
       rl->rhs = rhs;
       rl->varvalue = NULL;
       rl->solved = FALSE;
       rl->transpose = transpose;
       rl->next = sys->rl;
       sys->rl = rl;
       ++(sys->rlength);
       if (sys->capacity>0)  {
         rl->varvalue =
          ASC_NEW_ARRAY(real64,sys->capacity);
       }
     }
   }
}

void linsolqr_remove_rhs(linsolqr_system_t sys,real64 *rhs){
   struct rhs_list **q;

   if(CHECK_SYSTEM(sys)) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. RHS not removed.");
     return;
   }
   for( q = &(sys->rl) ; NOTNULL(*q) ; q = &((*q)->next) )
      if( (*q)->rhs == rhs )
         break;
   if( NOTNULL(*q) ) {
      struct rhs_list *p;
      p = *q;
      *q = p->next;
      if( NOTNULL(p->varvalue) )
         ascfree( (POINTER)(p->varvalue) );
      ascfree( (POINTER)p );
      --(sys->rlength);
   } else if( NOTNULL(rhs) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs does not exist.");
   }
}

int32 linsolqr_number_of_rhs(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->rlength );
}

real64 *linsolqr_get_rhs(linsolqr_system_t sys,int n){
   struct rhs_list *rl;
   int count;

   CHECK_SYSTEM(sys);

   count = sys->rlength - 1 - n;
   if( count < 0 )  return(NULL);
   for( rl = sys->rl ; count > 0 && NOTNULL(rl) ; rl = rl->next )
      --count;
   return( ISNULL(rl) ? NULL : rl->rhs );
}

void linsolqr_matrix_was_changed(linsolqr_system_t sys){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. matrix change message ignored.");
     return;
   }
   sys->rowdeps = sys->coldeps = sys->factored = FALSE;
}

void linsolqr_rhs_was_changed(linsolqr_system_t sys, real64 *rhs){
   struct rhs_list *rl;

   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. rhs change ignored.");
     return;
   }
   rl = find_rhs(sys->rl,rhs);
   if( NOTNULL(rl) ) {
      rl->solved = FALSE;
   } else if( NOTNULL(rhs) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs does not exist.");
   }
}

void linsolqr_set_pivot_zero(linsolqr_system_t sys,real64 pivot_zero){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. set_pivot_zero ignored.");
     return;
   }
   if( pivot_zero < D_ZERO ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid pivot zero of %g",pivot_zero);
   } else {
      sys->pivot_zero = pivot_zero;
      linsolqr_matrix_was_changed(sys);
   }
}

real64 linsolqr_pivot_zero(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->pivot_zero );
}

void linsolqr_set_pivot_tolerance(linsolqr_system_t sys, real64 ptol){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. set_pivot_tol ignored.");
     return;
   }
   if( ptol < D_ZERO || ptol > D_ONE ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid pivot tolerance of %g",ptol);
   } else {
      sys->ptol = ptol;
      linsolqr_matrix_was_changed(sys);
   }
}

void linsolqr_set_condition_tolerance(linsolqr_system_t sys, real64 ctol){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. set_condition_tolerance ignored.");
     return;
   }
   if( ctol < D_ZERO || ctol > D_ONE ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid condition tolerance of %g.",ctol);
   } else {
      sys->ctol = ctol;
      linsolqr_matrix_was_changed(sys);
   }
}

void linsolqr_set_drop_tolerance(linsolqr_system_t sys, real64 dtol){
   if(CHECK_SYSTEM(sys)) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"Bad linsolqr_system_t found. set_drop_tolerance ignored.");
     return;
   }
   if( dtol < D_ZERO || dtol > D_ONE ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid drop tolerance of %g.",dtol);
   } else {
      sys->dtol = dtol;
      linsolqr_matrix_was_changed(sys);
   }
}

real64 linsolqr_pivot_tolerance(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->ptol );
}

real64 linsolqr_condition_tolerance(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->ctol );
}

real64 linsolqr_drop_tolerance(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->dtol );
}

extern enum factor_class linsolqr_fclass(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->fclass );
}
extern enum factor_method linsolqr_fmethod(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->fmethod );
}
extern enum reorder_method linsolqr_rmethod(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   return( sys->rmethod );
}

int32 linsolqr_rank(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
   }
   return(sys->rank);
}

real64 linsolqr_smallest_pivot(linsolqr_system_t sys){
   CHECK_SYSTEM(sys);
#if LINSOL_DEBUG
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
   }
#endif
   return(sys->smallest_pivot);
}

/*-----------------------------------------------------------------------------
  Commonly used internal functions for sparse linear solvers based on mtx.
   void ensure_capacity(sys)
   void ensure_lu_capacity(sys)
   void ensure_qr_capacity(sys)
   void forward_substitute(sys,rvec,transpose)
   void backward_substitute(sys,rvec,transpose)
   macro SQR(x)
   int find_pivot_number(vec,len,tol,eps,ivec,rvec,maxi)
*/


/**
	Raises the capacity of the array and returns a new array.
	It is assumed that oldcap < newcap.  vec is destroyed or
	returned as appropriate.
	If !NDEBUG, the vector expanded is also set to 0.
*/
static real64 *raise_capacity(real64 *vec,
                                    int32 oldcap,
                                    int32 newcap
){
  real64 *newvec=NULL;
#ifndef NDEBUG
  int i;
#endif
  if (newcap < oldcap) {
#ifndef NDEBUG
    for (i = 0; i < newcap; i++) {
      newvec[i] = 0.0;
    }
#endif
    return vec;
  }
  if (NOTNULL(vec)) {
    /* don't call realloc on null with newcap 0 or it frees */
    newvec=(real64 *)ascrealloc(vec,(newcap * sizeof(real64)));
  } else {
    newvec=(newcap > 0 ?
      ASC_NEW_ARRAY(real64,newcap ) : NULL);
  }
#ifndef NDEBUG
  for (i = 0; i < newcap; i++) {
    newvec[i] = 0.0;
  }
#endif
  return newvec;
}

/**
	Raises the capacity of the index array and returns a new array.
	It is assumed that oldcap < newcap.  vec is destroyed or
	returned as appropriate. vec returned is zeroed.
	calling with newcap=0 does not force deallocation.
*/
static int32 *raise_qr_int_capacity(int32 *vec,
                                          int32 oldcap,
                                          int32 newcap
){
  int32 *newvec=NULL;
  if (newcap < oldcap)
    return vec;
  if (NOTNULL(vec)) {/* don't call realloc on null with newcap 0 or it frees */
    newvec=(int32 *)ascrealloc(vec,(newcap * sizeof(int32)));
    mtx_zero_int32(vec,newcap);
  } else {
    newvec= (newcap > 0 ? ASC_NEW_ARRAY_CLEAR(int32,newcap) : NULL);
  }
  return newvec;
}

/**
	Raises the capacity of the real array and returns a new array.
	It is assumed that oldcap < newcap.  vec is destroyed or
	returned as appropriate. vec returned is zeroed.
	calling with newcap=0 does not force deallocation.
 **/
static real64 *raise_qr_real_capacity(real64 *vec,
                                       int32 oldcap,
                                       int32 newcap
){
  real64 *newvec=NULL;
  if (newcap < oldcap)
    return vec;
  if (NOTNULL(vec)) {/* don't call realloc on null with newcap 0 or it frees */
    newvec=(real64 *)ascrealloc(vec,(newcap * sizeof(real64)));
    mtx_zero_real64(vec,newcap);
  } else {
    newvec= (newcap > 0 ? ASC_NEW_ARRAY_CLEAR(real64,newcap) : NULL);
  }
  return newvec;
}

static struct qr_fill_t *raise_qr_fill_capacity(struct qr_fill_t *vec,
                                                int32 oldcap,
                                                int32 newcap)
/**
	Raises the capacity of the fill array and returns a new array.
	It is assumed that oldcap < newcap.  vec is destroyed or
	returned as appropriate. vec returned is zeroed.
	calling with newcap=0 does not force deallocation.
 **/
{
  struct qr_fill_t *newvec=NULL;
  if (newcap < oldcap)
    return vec;
  if (NOTNULL(vec)) {/* don't call realloc on null with newcap 0 or it frees */
    newvec=(struct qr_fill_t *)
      ascrealloc(vec,(newcap * sizeof(struct qr_fill_t)));
    mtx_zero_char((char *)vec,newcap*sizeof(struct qr_fill_t));
  }else{
    newvec= (newcap > 0 ? ASC_NEW_ARRAY_CLEAR(struct qr_fill_t, newcap) : NULL);
  }
  return newvec;
}

/**
	ensures that the capacity of all of the solution vectors
	for each rhs is large enough.
	The above implies a malloc if varvalue is null.
	Assumes varvalue are at sys->capacity already.
*/
void ensure_capacity(linsolqr_system_t sys){
   int32 req_cap;
   req_cap = mtx_capacity(sys->coef);

   if( req_cap > sys->capacity ) {
      struct rhs_list *rl;

      for( rl = sys->rl ; NOTNULL(rl) ; rl = rl->next )
         rl->varvalue = raise_capacity(rl->varvalue,sys->capacity,req_cap);
      sys->capacity = req_cap;
   }
}

/**
	ensures that the capacity of all of the ludata vectors.
	The above implies a malloc if vector is null.
	If not null, implies an extension if needed.
 **/
void ensure_lu_capacity(linsolqr_system_t sys){
  int32 req_cap;
  if (!sys || !(sys->coef) || !(sys->ludata)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with NULL pointer");
    return;
  }
  req_cap = mtx_capacity(sys->coef);

  if( req_cap > sys->ludata->cap ) {
    sys->ludata->pivlist =
      raise_capacity(sys->ludata->pivlist,sys->ludata->cap,req_cap);
    sys->ludata->tmp =
      raise_capacity(sys->ludata->tmp,sys->ludata->cap,req_cap);
    sys->ludata->cap = req_cap;
  }
}

/**
	ensures that the capacity of all of the qrdata vectors
	is large enough.
	The above implies a calloc if vector is null.
	If not null, implies an extension and zeroing of the vector.
	Also zeroes the simple elements of the qrdata.
*/
void ensure_qr_capacity(linsolqr_system_t sys){
  int32 req_cap;
  if (!sys || !(sys->coef) || !(sys->qrdata)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with NULL pointer");
    return;
  }
  req_cap = mtx_capacity(sys->coef);

  if( req_cap > sys->qrdata->cap ) {
    sys->qrdata->alpha =
      raise_qr_real_capacity(sys->qrdata->alpha,sys->qrdata->cap,req_cap);
    sys->qrdata->sigma =
      raise_qr_real_capacity(sys->qrdata->sigma,sys->qrdata->cap,req_cap);
    sys->qrdata->tau =
      raise_qr_real_capacity(sys->qrdata->tau,sys->qrdata->cap,req_cap);
    sys->qrdata->hhcol =
      raise_qr_real_capacity(sys->qrdata->hhcol,sys->qrdata->cap,req_cap);
    sys->qrdata->hhrow =
      raise_qr_real_capacity(sys->qrdata->hhrow,sys->qrdata->cap,req_cap);

    sys->qrdata->sp.data =
      raise_qr_real_capacity(sys->qrdata->sp.data,sys->qrdata->cap,req_cap);
    sys->qrdata->sp.idata =
      raise_qr_int_capacity(sys->qrdata->sp.idata,sys->qrdata->cap,req_cap);

    sys->qrdata->fill =
      raise_qr_fill_capacity(sys->qrdata->fill,sys->qrdata->cap,req_cap);

    sys->qrdata->cap = req_cap;
    sys->qrdata->sp.cap = req_cap;
    sys->qrdata->sp.len = 0;
  }
  sys->qrdata->nu=D_ZERO;
  sys->qrdata->anu=D_ZERO;
  sys->qrdata->asubnu=D_ZERO;
}

#ifdef SQR
# undef SQR
#endif
#define SQR(x) ((x)*(x))

/*
	 Search array vec of positive numbers for the first entry, k, which passes
	 (vec[k] >= tol*vecmax) where vecmax is the largest value in vec.
	 vec is an array len long. rvec, ivec are (worst case) len long.
	 *maxi will be set to the index of the first occurence of the largest
	 number in vec which is >= eps.
	 0.0 <= tol <= 1.
	 If tol <=0, no search is done (*maxi = 0, k = 0).
	 Eps is an absolute number which vec values must be >= to before
	 being eligible for the tol test.

	 Best case, highly nonlinear feature:
	   if on entry *maxi = len, we will do a simplified search for the
	   first k which satisfies tol and eps tests, based on the value
	   stored at vec[len-1].

	 We could allocate and deallocate rvec/ivec since they are
	 temporaries, but it is more efficient for caller to manage that.
	 If tol>=1.0, rvec and ivec are ignored and may be NULL.
	 NOTE: we are going on value vec[i], not fabs(vec[i]).
	 Returns k.

	Remember: GIGO.
	Failure modes- if all numbers are <= 0.0, k and *maxi will be returned 0.
	Failure modes- if all numbers are < eps, k and *maxi will be returned 0.

	find_pivot_index, should we implement, would search an int vector.
	
	Hint: if you know you have not increased any values in vec and have not
	changed the value at vec[maxi] before a subsequent search, then call
	with len = previous maxi+1 to reduce the search time.
*/
int32 find_pivot_number(const real64 *vec,
                                     const int32 len,
                                     const real64 tol,
                                     const real64 eps,
                                     int32 * const ivec,
                                     real64 * const rvec,
                                     int32 * const maxi
){
  int32 j,k;
  if (tol <= D_ZERO ) {
    *maxi = 0;
    return 0;
  }
  if (tol >= D_ONE) {
    register real64 biggest = MAX(eps,D_ZERO);
    /* get the biggest, period */
    k = 0;
    for (j=0; j < len; j++) {
      if (vec[j] > biggest) {
        biggest = vec[j];
        k = j;
      }
    }
    *maxi = k;
  } else {
    int32 bigi;
    bigi = 0;
    rvec[0] = eps;
    ivec[0] = 0;
    if (*maxi==len) {
      /* cheap search against eps and tol, no list. */
      register real64 thold;
      thold = tol * vec[len-1];
      if (thold >= eps) {
        for (k = 0; k < len && vec[k] < thold; k++);
      } else {
        for (k = 0; k < len && vec[k] < eps; k++);
      }
      /* adjust maxi to still point at last location, as indicated by len */
      (*maxi)--;
    } else {
      real64 rlast;
      rlast = eps;
      /* create short list */
      for (j=0; j < len; j++) {
        while (j < len && vec[j] <= rlast) j++; /* skip to next max */
        if (j < len) {
          ivec[bigi] = j;
          rvec[bigi] = vec[j];
          bigi++;  /* bigi ends up being len of rvec or, if vec all<= eps, 0 */
          rlast = rvec[bigi-1];
        }
      }
      if (bigi == 0) {
        *maxi = k = 0;
      } else {
        register real64 thold;
        /* Note: if bigi is enormous, we should do a binary search,
           not linear. */
        *maxi = ivec[bigi-1];
        thold = tol * rvec[bigi-1];
        /* get pivot from shortlist, searching backward */
        if (thold >= eps) {
          for (k = bigi-1; k >= 0 && rvec[k] >= thold; k--);
        } else {
          for (k = bigi-1; k >= 0 && rvec[k] >= eps; k--);
        }
        /* translate pivot to vec index */
        k = ivec[k+1];
      }
    }
  }
  return k;
}

static void calc_dependent_rows_ranki1(linsolqr_system_t sys)
{
  mtx_coord_t nz;
  real64 value;
  mtx_range_t colrange;
  mtx_range_t rowrange;
  real64 *lc;
  mtx_matrix_t mtx;

  sys->rowdeps = TRUE;
  if( ( (sys->reg.row.low == sys->rng.low) &&
        ( sys->reg.row.high == sys->rng.low+sys->rank-1 )
      ) || sys->rank==0 )
    return;

  lc = sys->ludata->tmp;
  colrange.low = sys->rng.low;
  colrange.high = colrange.low + sys->rank - 1;
  rowrange.low = sys->rng.high;
  rowrange.high = sys->rng.low+sys->rank;
  mtx=sys->factors;

  nz.row = sys->reg.row.low;
  for( ; nz.row <= sys->reg.row.high; nz.row++ ) {
    if( nz.row == sys->rng.low ) {
      nz.row = rowrange.high-1;
      continue;
    }
    mtx_zero_real64(lc,(sys->capacity));
    mtx_org_row_vec(mtx,nz.row,lc,&colrange);
    if( nz.row < rowrange.high || nz.row > rowrange.low )
      backward_substitute(sys,lc,TRUE);
    forward_substitute(sys,lc,TRUE);
    mtx_clear_row(mtx,nz.row,&colrange);
    for( nz.col=colrange.low; nz.col <= colrange.high; nz.col++ ) {
      value = lc[mtx_col_to_org(mtx,nz.col)];
      if( value != D_ZERO ) mtx_fill_value(mtx,&nz,value);
    }
  }
}


static void calc_dependent_cols_ranki1(linsolqr_system_t sys)
{
  mtx_coord_t nz;
  real64 value;
  mtx_range_t rowrange;
  mtx_range_t colrange;
  real64 *lc;
  mtx_matrix_t mtx;

  sys->coldeps = TRUE;
  if( ( (sys->reg.col.low == sys->rng.low) &&
        ( sys->reg.col.high == sys->rng.low+sys->rank-1 )
      ) || sys->rank==0 )
    return;

  lc = sys->ludata->tmp;
  rowrange.low = sys->rng.low;
  rowrange.high = rowrange.low + sys->rank - 1;
  colrange.high = sys->rng.low+sys->rank;
  colrange.low = sys->rng.high;
  mtx=sys->factors;

  nz.col = sys->reg.col.low;
  for( ; nz.col <= sys->reg.col.high; nz.col++ ) {
    if( nz.col == sys->rng.low ) {
      nz.col = colrange.high-1;
      continue;
    }
    mtx_zero_real64(lc,sys->capacity);
    mtx_org_col_vec(mtx,nz.col,lc,&rowrange);
    if( nz.col < colrange.high || nz.col > colrange.low )
      backward_substitute(sys,lc,FALSE);
    forward_substitute(sys,lc,FALSE);
    mtx_clear_col(mtx,nz.col,&rowrange);
    for( nz.row=rowrange.low; nz.row <= rowrange.high; nz.row++ ) {
        value = lc[mtx_row_to_org(mtx,nz.row)];
      if( value != D_ZERO ) mtx_fill_value(mtx,&nz,value);
    }
  }
}

/**
	Given a matrix and a diagonal range, sets the diagonal elements to 0
	but does not delete them in the range low to high inclusive.
	worst cost= k*(nonzeros in rows of range treated).
	likely cost= k*(nonzeros in column pivoted rows of range treated)
	            since you can smartly put in the diagonal last and find
	            it first on many occasions.
	This is a good bit cheaper than deleting the elements unless one
	expects thousands of solves.
*/
void zero_diagonal_elements(mtx_matrix_t mtx,
                                   int32 low, int32 high){
  mtx_coord_t nz;
  for (nz.row = low; nz.row <= high; nz.row++) {
    nz.col = nz.row;
    mtx_set_value(mtx,&nz,0.0);
  }
}

/**
	Sets the values of unpivoted variables to zero.
 **/
void zero_unpivoted_vars(linsolqr_system_t sys,
				real64 *varvalues,
				boolean transpose
){
   int32 ndx,order;
   mtx_matrix_t mtx;

   mtx = sys->factors;
   order = mtx_order(mtx);
   for( ndx = 0 ; ndx < sys->rng.low ; ++ndx )
     if (transpose)
       varvalues[mtx_col_to_org(mtx,ndx)] = D_ZERO;
     else
       varvalues[mtx_row_to_org(mtx,ndx)] = D_ZERO;

   for( ndx = sys->rng.low + sys->rank ; ndx < order ; ++ndx )
     if (transpose)
       varvalues[mtx_col_to_org(mtx,ndx)] = D_ZERO;
     else
       varvalues[mtx_row_to_org(mtx,ndx)] = D_ZERO;
}

#if LINSOL_DEBUG
static void debug_out_factors(FILE *fp,linsolqr_system_t sys)
/**
	Outputs permutation and values of the nonzero elements in the
	factor matrix square region given by sys->rng.
 **/
{
   mtx_region_t reg;
   reg.row.low=reg.col.low=sys->rng.low;
   reg.row.high=reg.col.high=sys->rng.high;
   mtx_write_region(fp,sys->factors,&reg);
}
#endif /* LINSOL_DEBUG */

/*-----------------------------------------------------------------------------
  Reordering functions for SPK1, and possibly for other schemes to be
  implemented later.
  The stuff here is almost, but not quite, black magic. Don't tinker with it.
*/

struct reorder_list {            /* List of rows/columns and their counts. */
   int32 ndx;
   int32 count;
   struct reorder_list *next;
};

struct reorder_vars {
   mtx_matrix_t mtx;
   mtx_region_t reg;             /* Current active region */
   int32 colhigh;          /* Original value of reg.col.high */
   struct reorder_list *tlist;   /* Array of (enough) list elements */
   int32 *rowcount;        /* rowcount[reg.row.low .. reg.row.high] */
};

static void adjust_row_count(struct reorder_vars *vars,int32 removed_col)
/**
	Adjusts the row counts to account for the (to be) removed column.
 **/
{
   mtx_coord_t nz;
   real64 value;
   nz.col = removed_col;
   nz.row = mtx_FIRST;
   while( value = mtx_next_in_col(vars->mtx,&nz,&(vars->reg.row)),
         nz.row != mtx_LAST )
      --(vars->rowcount[nz.row]);
}

static void assign_row_and_col(struct reorder_vars *vars,
                          int32 row,
                          int32 col)
/**
	Assigns the given row to the given column, moving the row and column
	to the beginning of the active region and removing them (readjusting
	the region).  The row counts are NOT adjusted.  If col == mtx_NONE,
	then no column is assigned and the row is moved to the end of the
	active block instead.  Otherwise, it is assumed that the column
	is active.
 **/
{
   if( col == mtx_NONE ) {
      mtx_swap_rows(vars->mtx,row,vars->reg.row.high);
      vars->rowcount[row] = vars->rowcount[vars->reg.row.high];
      --(vars->reg.row.high);
   } else {
      mtx_swap_rows(vars->mtx,row,vars->reg.row.low);
      vars->rowcount[row] = vars->rowcount[vars->reg.row.low];
      mtx_swap_cols(vars->mtx,col,vars->reg.col.low);
      ++(vars->reg.row.low);
      ++(vars->reg.col.low);
   }
}

static void push_column_on_stack(struct reorder_vars *vars,int32 col)
/**
	Pushes the given column onto the stack.  It is assumed that the
	column is active.  Row counts are adjusted.
 **/
{
   adjust_row_count(vars,col);
   mtx_swap_cols(vars->mtx,col,vars->reg.col.high);
   --(vars->reg.col.high);
}

static int32 pop_column_from_stack(struct reorder_vars *vars)
/**
	Pops the column on the "top" of the stack off of the stack and
	returns the column index, where it now lies in the active region.
	If the stack is empty, mtx_NONE is returned.  Row counts are NOT
	adjusted (this together with a subsequent assignment of this column
	==> no row count adjustment necessary).
 **/
{
   if( vars->reg.col.high < vars->colhigh )
      return(++(vars->reg.col.high));
   else
      return( mtx_NONE );
}

static void assign_null_rows(struct reorder_vars *vars)
/**
	Assigns empty rows, moving them to the assigned region.  It is
	assumed that row counts are correct.  Columns are assigned off the
	stack.
 **/
{
   int32 row;

   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      if( vars->rowcount[row] == 0 )
         assign_row_and_col(vars , row , pop_column_from_stack(vars));
}

static void forward_triangularize(struct reorder_vars *vars)
/**
	Forward triangularizes the region, assigning singleton rows with their
	one and only incident column until there are no more.  The row counts
	must be correct, and they are updated.
 **/
{
   boolean change;
   mtx_coord_t nz;
   real64 value;

   do {
      change = FALSE;
      for( nz.row = vars->reg.row.low ;
          nz.row <= vars->reg.row.high && vars->rowcount[nz.row] != 1;
          ++nz.row ) ;
      if( nz.row <= vars->reg.row.high ) {
         /* found singleton row */
         nz.col = mtx_FIRST; /* this is somehow coming back with nz.col -1 */
         value = mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col));
         adjust_row_count(vars,nz.col);
         assign_row_and_col(vars,nz.row,nz.col);
         change = TRUE;
      }
   } while( change );
}

static int32 select_row(struct reorder_vars *vars)
/**
	Selects a row and returns its index.  It is assumed that there is a
	row.  Row counts must be correct.  vars->tlist will be used.
 **/
{
   int32 min_row_count;
   int32 max_col_count;
   int32 row;
   int32 i, nties=-2;  /* # elements currently defined in vars->tlist */
   int32 sum;
   mtx_coord_t nz;
   real64 value;
   mtx_matrix_t mtx;
   mtx_range_t *colrng, *rowrng;

   /* Set to something > any possible value */
   min_row_count = vars->reg.col.high-vars->reg.col.low+2;
   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      if( vars->rowcount[row] <= min_row_count ) {
         if( vars->rowcount[row] < min_row_count ) {
            min_row_count = vars->rowcount[row];
            nties = 0;
         }
         vars->tlist[nties++].ndx = row;
      }
   /**
   	vars->tlist[0..nties-1] is a list of row numbers which tie for
   	 minimum row count.
    **/

   max_col_count = -1;   /* < any possible value */
   i = 0;
   mtx = vars->mtx;
   colrng=&(vars->reg.col);
   rowrng=&(vars->reg.row);
   while( i < nties ) {

      sum = 0;
      nz.row = vars->tlist[i].ndx;
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(mtx,&nz,colrng),
            nz.col != mtx_LAST )
         sum += mtx_nonzeros_in_col(mtx,nz.col,rowrng);
      if( sum > max_col_count ) {
         max_col_count = sum;
         row = nz.row;
      }
      i++;
   }
   /**
   	Now row contains the row with the minimum row count, which has the
   	greatest total column count of incident columns among all rows with
   	the same (minimum) row count.  Select it.
    **/
   return(row);
}

static void spk1_reorder(struct reorder_vars *vars)
/**
	Reorders the assigned matrix vars->mtx within the specified bounding
	block region vars->reg.  The region is split into 6 subregions during
	reordering:  the rows are divided in two, and the columns divided in
	three.  Initially everything is in the active subregion.  Ultimately,
	everything will be assigned.

	        <-- assigned -->|<-- active-->|<-- on stack -->|
	   ----+----------------+-------------+----------------+
	     a |                |             |                |
	     s |                |             |                |
	     s |                |             |                |
	     i |    (SQUARE)    |             |                |
	     g |                |             |                |
	     n |                |             |                |
	     e |                |             |                |
	     d |                |             |                |
	   ----+----------------+-------------+----------------+
	     a |                |             |                |
	     c |                |    ACTIVE   |                |
	     t |                |    REGION   |                |
	     i |                |             |                |
	     v |                |             |                |
	     e |                |             |                |
	   ----+----------------+-------------+----------------+

	The algorithm is roughly as follows:
	  (1) select a row (by some criterion).
	  (2) push columns incident on that row onto the stack in decreasing
	      order of their length.
	  (3) pop first column off the stack and assign it to the selected
	      row.
	  (4) forward-triangularize (assign singleton rows with their one
	      and only incident column, until no longer possible).

	  (1)-(4) should be repeated until the active subregion becomes empty.

	Everything above was written as though the entire matrix is
	involved.  In reality, only the relevant square region is involved.
*/
{
   int32 row, size;
   int32 *rowcount_array_origin;
   mtx_matrix_t mtx;

   size = MAX(vars->reg.row.high,vars->reg.col.high) + 1;
   vars->tlist = size > 0 ? (struct reorder_list *)
      ascmalloc( size*sizeof(struct reorder_list) ) : NULL;
   vars->rowcount = rowcount_array_origin = size > 0 ? (int32 *)
      ascmalloc( size*sizeof(int32) ) : NULL;
   mtx = vars->mtx;

   vars->colhigh = vars->reg.col.high;
   /* Establish row counts */
   for( row = vars->reg.row.low ; row <= vars->reg.row.high ; ++row )
      vars->rowcount[row] =
         mtx_nonzeros_in_row(mtx,row,&(vars->reg.col));

   while(TRUE) {
      struct reorder_list *head;
      int32 nelts;   /* # elements "allocated" from vars->tlist */
      mtx_coord_t nz;
      real64 value;

      forward_triangularize(vars);
      assign_null_rows(vars);
      if( vars->reg.row.low>vars->reg.row.high ||
         vars->reg.col.low>vars->reg.col.high ) {
         /* Active region is now empty, done */
         if( NOTNULL(vars->tlist) )
            ascfree( vars->tlist );
         if( NOTNULL(rowcount_array_origin) )
            ascfree( rowcount_array_origin );
         return;
      }

      head = NULL;
      nelts = 0;
      nz.row = select_row(vars);
      nz.col = mtx_FIRST;
      while( value = mtx_next_in_row(mtx,&nz,&(vars->reg.col)),
            nz.col != mtx_LAST ) {
         struct reorder_list **q,*p;

         p = &(vars->tlist[nelts++]);
         p->ndx = mtx_col_to_org(mtx,nz.col);
         p->count = mtx_nonzeros_in_col(mtx,nz.col,&(vars->reg.row));
         for( q = &head; *q && (*q)->count > p->count; q = &((*q)->next) );
         p->next = *q;
         *q = p;
      }
      /**
      	We now have a list of columns which intersect the selected row.
      	The list is in order of decreasing column count.
       **/

      /* Push incident columns on stack */
      for( ; NOTNULL(head) ; head = head->next )
         push_column_on_stack(vars,mtx_org_to_col(mtx,head->ndx));

      /* Pop column off stack and assign to selected row */
      assign_row_and_col(vars , nz.row , pop_column_from_stack(vars));
   }
   /*  Not reached. */
}

/*
	end of spk1 stuff
*/

/*-----------------------------------------------------------------------------
 begin of tspk1 stuff
*/

struct creorder_list {            /* List of columns/rows and their counts. */
   int32 ndx;
   int32 count;
   struct creorder_list *next;
};

struct creorder_vars {
   mtx_matrix_t mtx;
   mtx_region_t reg;             /* Current active region */
   int32 rowhigh;          /* Original value of reg.row.high */
   struct creorder_list *tlist;  /* Array of (enough) list elements */
   int32 *colcount;        /* colcount[reg.col.low .. reg.col.high] */
};

static void adjust_col_count(struct creorder_vars *vars,int32 removed_row)
/**
	Adjusts the column counts to account for the (to be) removed row.
 **/
{
   mtx_coord_t nz;
   real64 value;
   nz.row = removed_row;
   nz.col = mtx_FIRST;
   while( value = mtx_next_in_row(vars->mtx,&nz,&(vars->reg.col)),
         nz.col != mtx_LAST )
      --(vars->colcount[nz.col]);
}

static void assign_col_and_row(struct creorder_vars *vars,
                          int32 col,
                          int32 row)
/**
	Assigns the given row to the given column, moving the row and column
	to the beginning of the active region and removing them (readjusting
	the region).  The col counts are NOT adjusted.  If col == mtx_NONE,
	then no column is assigned and the col is moved to the end of the
	active block instead.  Otherwise, it is assumed that the row
	is active.
 **/
{
   if( row == mtx_NONE ) {
      mtx_swap_cols(vars->mtx,col,vars->reg.col.high);
      vars->colcount[col] = vars->colcount[vars->reg.col.high];
      --(vars->reg.col.high);
   } else {
      mtx_swap_cols(vars->mtx,col,vars->reg.col.low);
      vars->colcount[col] = vars->colcount[vars->reg.col.low];
      mtx_swap_rows(vars->mtx,row,vars->reg.row.low);
      ++(vars->reg.col.low);
      ++(vars->reg.row.low);
   }
}

static void push_row_on_stack(struct creorder_vars *vars,int32 row)
/**
	Pushes the given row onto the stack.  It is assumed that the
	row is active.  Col counts are adjusted.
 **/
{
   adjust_col_count(vars,row);
   mtx_swap_rows(vars->mtx,row,vars->reg.row.high);
   --(vars->reg.row.high);
}

static int32 pop_row_from_stack(struct creorder_vars *vars)
/**
	Pops the row on the "top" of the stack off of the stack and
	returns the row index, where it now lies in the active region.
	If the stack is empty, mtx_NONE is returned.  Col counts are NOT
	adjusted (this together with a subsequent assignment of this row
	==> no col count adjustment necessary).
 **/
{
   if( vars->reg.row.high < vars->rowhigh )
      return(++(vars->reg.row.high));
   else
      return( mtx_NONE );
}

static void assign_null_cols(struct creorder_vars *vars)
/**
	Assigns empty cols, moving them to the assigned region.  It is
	assumed that col counts are correct.  Rows are assigned off the
	stack.
 **/
{
   int32 col;

   for( col = vars->reg.col.low ; col <= vars->reg.col.high ; ++col )
      if( vars->colcount[col] == 0 )
         assign_col_and_row(vars , col , pop_row_from_stack(vars));
}

static void cforward_triangularize(struct creorder_vars *vars)
/**
	Forward triangularizes the region, assigning singleton columns with their
	one and only incident row until there are no more.  The column counts
	must be correct, and they are updated.
 **/
{
   boolean change;

   do {
      mtx_coord_t nz;
      real64 value;
      change = FALSE;
      for( nz.col = vars->reg.col.low ;
          nz.col <= vars->reg.col.high && vars->colcount[nz.col] != 1;
          ++nz.col ) ;
      if( nz.col <= vars->reg.col.high ) {
         /* found singleton col */
         nz.row = mtx_FIRST;
         value = mtx_next_in_col(vars->mtx,&nz,&(vars->reg.row));
         adjust_col_count(vars,nz.row);
         assign_col_and_row(vars,nz.col,nz.row);
         change = TRUE;
      }
   } while( change );
}

static int32 select_col(struct creorder_vars *vars)
/**
	Selects a col and returns its index.  It is assumed that there is a
	col.  Col counts must be correct.  vars->tlist will be used.
 **/
{
   int32 min_col_count;
   int32 max_row_count;
   int32 col;
   int32 i, nties=-2;  /* # elements currently defined in vars->tlist */
   int32 sum;
   mtx_coord_t nz;
   real64 value;
   mtx_matrix_t mtx;
   mtx_range_t *colrng, *rowrng;

   /* Set to something > any possible value */
   min_col_count = vars->reg.row.high-vars->reg.row.low+2;
   for( col = vars->reg.col.low ; col <= vars->reg.col.high ; ++col )
      if( vars->colcount[col] <= min_col_count ) {
         if( vars->colcount[col] < min_col_count ) {
            min_col_count = vars->colcount[col];
            nties = 0;
         }
         vars->tlist[nties++].ndx = col;
      }
   /**
   	vars->tlist[0..nties-1] is a list of row numbers which tie for
   	 minimum col count.
    **/

   max_row_count = -1;   /* < any possible value */
   i = 0;
   mtx = vars->mtx;
   rowrng=&(vars->reg.row);
   colrng=&(vars->reg.col);
   while( i < nties ) {

      sum = 0;
      nz.row = mtx_FIRST;
      nz.col = vars->tlist[i].ndx;
      while( value = mtx_next_in_col(mtx,&nz,rowrng),
            nz.row != mtx_LAST )
         sum += mtx_nonzeros_in_row(mtx,nz.row,colrng);
      if( sum > max_row_count ) {
         max_row_count = sum;
         col = nz.col;
      }
      i++;
   }
   /**
   	Now col contains the col with the minimum col count, which has the
   	greatest total row count of incident rows among all cols with
   	the same (minimum) col count.  Select it.
    **/
   return(col);
}

static void tspk1_reorder(struct creorder_vars *vars)
/**
	Transpose the picture and explanation that follows:
	Reorders the assigned matrix vars->mtx within the specified bounding
	block region vars->reg.  The region is split into 6 subregions during
	reordering:  the rows are divided in two, and the columns divided in
	three.  Initially everything is in the active subregion.  Ultimately,
	everything will be assigned.

	        <-- assigned -->|<-- active-->|<-- on stack -->|
	   ----+----------------+-------------+----------------+
	     a |                |             |                |
	     s |                |             |                |
	     s |                |             |                |
	     i |    (SQUARE)    |             |                |
	     g |                |             |                |
	     n |                |             |                |
	     e |                |             |                |
	     d |                |             |                |
	   ----+----------------+-------------+----------------+
	     a |                |             |                |
	     c |                |    ACTIVE   |                |
	     t |                |    REGION   |                |
	     i |                |             |                |
	     v |                |             |                |
	     e |                |             |                |
	   ----+----------------+-------------+----------------+

	The algorithm is roughly as follows:
	  (1) select a row (by some criterion).
	  (2) push columns incident on that row onto the stack in decreasing
	      order of their length.
	  (3) pop first column off the stack and assign it to the selected
	      row.
	  (4) forward-triangularize (assign singleton rows with their one
	      and only incident column, until no longer possible).

	  (1)-(4) should be repeated until the active subregion becomes empty.

	Everything above was written as though the entire matrix is
	involved.  In reality, only the relevant square region is involved.
 **/
{
   int32 col, size;
   int32 *colcount_array_origin;
   mtx_matrix_t mtx;

   size = vars->reg.col.high - vars->reg.col.low + 1;
   size = MAX(size,vars->reg.row.high - vars->reg.row.low + 1);
   vars->tlist = size > 0 ? (struct creorder_list *)
      ascmalloc( size*sizeof(struct creorder_list) ) : NULL;
   colcount_array_origin = size > 0 ? (int32 *)
      ascmalloc( size*sizeof(int32) ) : NULL;
   vars->colcount =
     NOTNULL(colcount_array_origin) ?
       colcount_array_origin - vars->reg.col.low : NULL;
   mtx = vars->mtx;

   vars->rowhigh = vars->reg.row.high;
   /* Establish col counts */
   for( col = vars->reg.col.low ; col <= vars->reg.col.high ; ++col )
      vars->colcount[col] =
         mtx_nonzeros_in_col(mtx,col,&(vars->reg.row));

   while(TRUE) {
      struct creorder_list *head;
      int32 nelts;   /* # elements "allocated" from vars->tlist */
      mtx_coord_t nz;
      real64 value;

      cforward_triangularize(vars);
      assign_null_cols(vars);
      if( vars->reg.col.low > vars->reg.col.high ||
         vars->reg.row.low > vars->reg.row.high ) {
         /* Active region is now empty, done */
         if( NOTNULL(vars->tlist) )
            ascfree( vars->tlist );
         if( NOTNULL(colcount_array_origin) )
            ascfree( colcount_array_origin );
         return;
      }

      head = NULL;
      nelts = 0;
      nz.row = mtx_FIRST;
      nz.col = select_col(vars);
      while( value = mtx_next_in_col(mtx,&nz,&(vars->reg.row)),
            nz.row != mtx_LAST ) {
         struct creorder_list **q,*p;

         p = &(vars->tlist[nelts++]);
         p->ndx = mtx_row_to_org(mtx,nz.row);
         p->count = mtx_nonzeros_in_row(mtx,nz.row,&(vars->reg.col));
         for( q = &head; *q && (*q)->count > p->count; q = &((*q)->next) );
         p->next = *q;
         *q = p;
      }
      /**
      	We now have a list of columns which intersect the selected row.
      	The list is in order of decreasing column count.
       **/

      /* Push incident rows on stack */
      for( ; NOTNULL(head) ; head = head->next )
         push_row_on_stack(vars,mtx_org_to_row(mtx,head->ndx));

      /* Pop row off stack and assign to selected col */
      assign_col_and_row(vars , nz.col , pop_row_from_stack(vars));
   }
   /*  Not reached. */
}

/*
 end of tspk1 stuff
 -----------------------------------------------------------------------------*/

static boolean nonempty_row(mtx_matrix_t mtx,int32 row)
/**
	? row not empty in mtx.
 **/
{
   mtx_coord_t nz;
   real64 value;
   value = mtx_next_in_row(mtx, mtx_coord(&nz,row,mtx_FIRST), mtx_ALL_COLS);
   return( nz.col != mtx_LAST );
}

static boolean nonempty_col(mtx_matrix_t mtx,int32 col)
/**
	? column not empty in mtx.
 **/
{
   mtx_coord_t nz;
   real64 value;
   value = mtx_next_in_col(mtx, mtx_coord(&nz,mtx_FIRST,col), mtx_ALL_ROWS);
   return( nz.row != mtx_LAST );
}

void determine_pivot_range(linsolqr_system_t sys){
   sys->reg.row.low = sys->reg.row.high = -1;
   sys->reg.col.low = sys->reg.col.high = -1;

   for( sys->rng.high=mtx_order(sys->coef) ; --(sys->rng.high) >= 0 ; ) {
      if( nonempty_row(sys->coef,sys->rng.high) &&
         sys->reg.row.high < 0 )
         sys->reg.row.high = sys->rng.high;
      if( nonempty_col(sys->coef,sys->rng.high) &&
         sys->reg.col.high < 0 )
         sys->reg.col.high = sys->rng.high;
      if( nonempty_row(sys->coef,sys->rng.high) &&
         nonempty_col(sys->coef,sys->rng.high) )
         break;
   }

   for( sys->rng.low=0 ; sys->rng.low <= sys->rng.high ; ++(sys->rng.low) ) {
      if( nonempty_row(sys->coef,sys->rng.low) &&
         sys->reg.row.low < 0 )
         sys->reg.row.low = sys->rng.low;
      if( nonempty_col(sys->coef,sys->rng.low) &&
         sys->reg.col.low < 0 )
         sys->reg.col.low = sys->rng.low;
      if( nonempty_row(sys->coef,sys->rng.low) &&
         nonempty_col(sys->coef,sys->rng.low) )
         break;
   }
}

/**
	 Get the largest square confined to the diagonal within the region given
	 and set sys->rng accordingly.
 **/
void square_region(linsolqr_system_t sys,mtx_region_t *region){
      sys->reg = *region;
      sys->rng.low = MAX(region->row.low,region->col.low);
      sys->rng.high = MIN(region->row.high,region->col.high);
}

static int ranki_reorder(linsolqr_system_t sys,mtx_region_t *region)
/**
	The region to reorder is first isolated by truncating the region
	provided to the largest square region confined to the matrix diagonal.
	It is presumed it will contain no empty rows or columns and will
	provide the basis of candidate pivots when factoring.
 **/
{
   struct reorder_vars vars;
   CHECK_SYSTEM(sys);
   if (sys->fclass != ranki) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"called on system with inappropriate factor class");
     return 1;
   }
   if( region == mtx_ENTIRE_MATRIX ) determine_pivot_range(sys);
   else square_region(sys,region);

   vars.mtx = sys->coef;
   vars.reg.row.low = vars.reg.col.low = sys->rng.low;
   vars.reg.row.high = vars.reg.col.high = sys->rng.high;
   spk1_reorder(&vars);
   return 0;
}

static int tranki_reorder(linsolqr_system_t sys,mtx_region_t *region)
/**
	The region to reorder is first isolated by truncating the region
	provided to the largest square region confined to the matrix diagonal.
	It is presumed it will contain no empty rows or columns and will
	provide the basis of candidate pivots when factoring.
 **/
{
   struct creorder_vars vars;
   CHECK_SYSTEM(sys);
   if ( !(sys->fclass==ranki || sys->fclass==s_qr) ) {
     ERROR_REPORTER_HERE(ASC_PROG_ERR,"reorder called on system with inappropriate factor method");
     return 1;
   }
   if( region == mtx_ENTIRE_MATRIX ) determine_pivot_range(sys);
   else square_region(sys,region);

   vars.mtx = sys->coef;
   vars.reg.row.low = vars.reg.col.low = sys->rng.low;
   vars.reg.row.high = vars.reg.col.high = sys->rng.high;
   tspk1_reorder(&vars);
   return 0;
}

/*
  End of reordering functions for SPK1.
*/

/*-----------------------------------------------------------------------------
  RANKI implementation functions.
*/



#ifdef BUILD_KIRK_CODE
/*-----------------------------------------------------------------------------
 * Start of kirk_* routines
 *
 * kirk1_factor:
 * 	This routine is  based on rankikw2_factor, except that it takes
 * 	an additional region, so as to do some *global* pivot restriction.
 *	This is for the case of a single border for the entire matrix.
 */

/*
 * This is just a dummy structure so as to get the size
 * correct. It is *exactly the same size as mtx_linklist,
 * until we can sort out the software engineering issues.
 */
struct dlinklist {
  int index;
  struct dlinklist *prev;
};

static
int kirk1_factor(linsolqr_system_t sys,
		mtx_region_t *A11,
		int kirk_method
){
  mtx_coord_t nz;
  int32 last_row;
  mtx_range_t pivot_candidates;
  real64 *tmp,*tmp_array_origin;
  real64 pivot, *pivots;
  int32 length;
  mtx_matrix_t mtx, upper_mtx;
  real64 maxa;

  int32 *inserted = NULL; /* stuff for the link list */
  mem_store_t eltbuffer = NULL;

#ifdef NOP_DEBUG
  mtx_number_ops = 0;
#endif /* NOP_DEBUG */

  length = sys->rng.high + 1;
  tmp_array_origin = length > 0 ? ASC_NEW_ARRAY_CLEAR(real64,length) : NULL;
  tmp = tmp_array_origin;
  pivots=sys->ludata->pivlist;
  mtx = sys->factors;
  upper_mtx = sys->inverse;
  inserted = ASC_NEW_ARRAY_CLEAR(int32,length);
  eltbuffer = mem_create_store(2,256,sizeof(struct dlinklist),
                               2,256);

  sys->smallest_pivot = MAXDOUBLE;
  last_row = pivot_candidates.high = sys->rng.high;
  for( nz.row = sys->rng.low ; nz.row <= last_row ; ) {

    pivot_candidates.low = nz.col = nz.row;
    pivots[nz.row]=pivot = mtx_value(mtx,&nz);
    pivot = fabs(pivot);
    maxa = mtx_row_max(mtx,&nz,&pivot_candidates,NULL);
    if ((pivot > sys->pivot_zero) && (pivot >= sys->ptol*maxa) &&
        !col_is_a_spike(sys,nz.row)) {
      if (pivot < sys->smallest_pivot)
        sys->smallest_pivot = pivot;
      ++(nz.row);
      continue;
    }
    /* pivots for rows nz.row back to sys->rng->low are stored in pivots */
    /**
    	Row is a spike row or will
    	be when a necessary column
    	exchange occurs.
     **/
    if (kirk_method==1)
      eliminate_row2(mtx,upper_mtx,&(sys->rng),nz.row,tmp,pivots,sys->dtol);
    else{
      mtx_eliminate_row2(mtx,upper_mtx,&(sys->rng),
                         nz.row,tmp,pivots,inserted,eltbuffer);
      mem_clear_store(eltbuffer);
    }
    /* pivot will be largest of those available. get size and value */
    pivot=mtx_row_max(mtx,&nz,&pivot_candidates,&(pivots[nz.row]));
    if( pivot <= sys->pivot_zero ) { /* pivot is an epsilon */
      /*
       * Dependent row, drag to the end. The upper_mtx is a slave
       * of mtx, and will be dragged automatically.
       */
      mtx_drag(mtx,nz.row,last_row);
      number_drag(pivots,nz.row,last_row);
      --last_row;
#ifdef KAA_DEBUG
      ERROR_REPORTER_HERE(ASC_PROG_WARNING"Row %d is dependent with pivot %20.8g",
              nz.row,pivot);
#endif /* KAA_DEBUG */
    } else {
      /* Independent row: nz contains best pivot */
      mtx_swap_cols(mtx,nz.row,nz.col); /* this Fixes U as well */
      /* Move pivot to diagonal */
      mtx_drag(mtx , nz.row , sys->rng.low ); /* this Fix U as well */
      number_drag(pivots,nz.row,sys->rng.low);
      if( pivot < sys->smallest_pivot )
        sys->smallest_pivot = pivot;
      ++(nz.row);
    }
  }
  if( NOTNULL(tmp_array_origin) )
    ascfree( (POINTER)tmp_array_origin );
  if (NOTNULL(inserted)) ascfree((char *)inserted);
  mem_destroy_store(eltbuffer);

#ifdef NOP_DEBUG
  CONSOLE_DEBUG("Number operations = %d",mtx_number_ops);
#endif /* NOP_DEBUG */

  sys->rank = last_row - sys->rng.low + 1;
  return 0; /* this return code needs to be useful */
}
#endif /* BUILD_KIRK_CODE */

#if 0
#include "ascgauss.c"
#endif

/*-----------------------------------------------------------------------------
  CondQR implementation.
*/

static int qr_cmp_fill(struct qr_fill_t *f1, struct qr_fill_t *f2)
{
  if (f1->fill < f2->fill) return -1;
  return (f2->fill > f1->fill);
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static int qr_cmp_cnts(struct qr_fill_t *f1, struct qr_fill_t *f2)
{
  if (f1->cnt < f2->cnt) return -1;
  return (f2->cnt > f1->cnt);
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

static void qr_sort_fill(struct qr_fill_t *fill,mtx_range_t *rng)
/*
  sort a list portion (rng) by order of increasing fill.
*/
{
  struct qr_fill_t *base;
  size_t n;

  base=&(fill[rng->low]);
  n = rng->high - rng->low + 1;
  qsort((void *)base,n,sizeof(struct qr_fill_t),
        (int (*)(const void *,const  void *))qr_cmp_fill);
}

#ifdef THIS_IS_AN_UNUSED_FUNCTION
static void qr_sort_col_cnts(struct qr_fill_t *fill,mtx_range_t *rng)
/*
 * sort a list portion (rng) by order of increasing cnt.
 */
{
  struct qr_fill_t *base;
  size_t n;

  base=&(fill[rng->low]);
  n = 1+ rng->high - rng->low ;
  qsort((void *)base,n,sizeof(struct qr_fill_t),
        (int (*)(const void *,const void*))qr_cmp_cnts);
}
#endif  /* THIS_IS_AN_UNUSED_FUNCTION */

static int qr_fillable_cols(linsolqr_system_t sys,
                            int32 pivot,
                            mtx_range_t *rng)
/*
	It is possible to calculate the exact fill, but this is not done.
	Fill is estimated by Boyd/Art method.
	For exact fill use the overlap as a counter
	instead of just a marker.
	fill(p)  = andcount * fill[p].cnt - SUM(fill[j].overlap |j IN rng)
	where andcount = number of nonpivot columns found with overlaps.
	The performance difference is small on test problems. It comes into
	play when the first few choices (based on sparsity) are all rejected
	for other reasons and the choice is then between several pivots with
	high fillin.
 */
{
  mtx_matrix_t mtx;
  int32 fillable,high,col;
  real64 val1,val2;
  mtx_coord_t coord1,coord2;
  struct qr_fill_t *fill;

  mtx=sys->factors;
  high=rng->high;
  fill=sys->qrdata->fill;
  coord1.col=pivot;
  coord1.row=mtx_FIRST;

  /* for each element in the pivot column */
  while( val1 = mtx_next_in_col(mtx,&coord1,rng),
         coord1.row != mtx_LAST )
    /* traverse row, marking nonzeros which are not in pivot column */
    if (val1!=D_ZERO) {
      coord2.row=coord1.row;
      coord2.col=mtx_FIRST;
      while( val2 = mtx_next_in_row(mtx,&coord2,rng),
             coord2.col != mtx_LAST )
        if (coord2.col!=pivot && val2!=D_ZERO) fill[coord2.col].overlap=1;
        /* can we change so thawt ^^^^^^^^^ goes away? */
    }
  /* END while */
  /* add up fillable columns (and rezero overlap) */
  fillable=0;
  for (col=rng->low; col<=high; col++) {
    fillable +=fill[col].overlap;
    fill[col].overlap=0;
  }

  return fillable;
}

static void qr_get_fill_data(linsolqr_system_t sys,
                             int32 curcol,
                             mtx_range_t *colrange)
/**
	Cook up a list of columns sorted by increasing fill potential.
	Due to the possibility of cancellations and zeros, this must
	be redeveloped every time from scratch.
	On exit, the colrange portion of sys->qrdata->fill will contain
	the sorted list of columns. Pivoting column fill[j].col will
	create estimated fill <= pivoting column fill[j+1].col. The fill
	estimate for fill[j].col is fill[j].fill. The only way to access
	fill counts by column number is linear search.
	
	The Boyd and Art estimate for fill is:
	  for column P being the pivot from a column set indexed over j,
	  FILL(P)= NFC(P) * (NZ(P)-1)
	    where NZ is the number of nonzeros in column P
	    and NFC is the number of columns that P overlaps with, i.e.
	    NFC(P)=SUM[ (sparsity(j) & sparsity(P) > 0) | j!=P]
	This implementation does not count 0.0 into the sparsity pattern.
	
 **/
{
  int32 col;
  struct qr_fill_t *fill;
  mtx_matrix_t mtx;

  fill=sys->qrdata->fill;
  mtx=sys->factors;

  /* count column incidences and set original column addresses in ->col. */
  /* was GET_SPARSE_PATTERN (calc NZ(j))*/
  for (col = curcol ; col <=colrange->high; col++) {
    fill[col].cnt= mtx_numbers_in_col(mtx,col,colrange);
    fill[col].col= col;
  }
  /* sort fill by column number counts.
  qr_sort_col_cnts(fill,colrange);
  Why?*/

  /* calculate estimated FILL(P) for all columns */
  for (col=curcol ; col <=colrange->high; col++)
    if (fill[col].cnt <2) fill[col].fill=0;
    else fill[col].fill=(fill[col].cnt-1)*qr_fillable_cols(sys,col,colrange);

  qr_sort_fill(fill,colrange);
}

static real64 qr_square_compute_alpha(real64 *alpha,
                                            mtx_matrix_t mtx,
                                            mtx_range_t *rng)
/**
	 Computes alphas for columns in a square region indicated by rng.
	 alpha are frobenius norms : sqrt( sum(sqr(aij)|i,j) )
	 as opposed
	 to matrix 2-norm: (the largest eigenvalue of At dot A, see stewart p 180)
	 or the matrix 1 norm: max( sum(abs(aij) | i) |j)
	 or the matrix inf norm: max( sum(abs(aij) | j) |i)
	 Returns the Frobenius norm of the region calculated.
 **/
{
  int32 hilim,col;
  real64 asubnu=D_ZERO,tsqr;
  hilim = rng->high;
  for (col=rng->low; col <= hilim; col++) {
    tsqr=mtx_sum_sqrs_in_col(mtx,col,rng);
    alpha[col]=sqrt(tsqr);
    asubnu+= tsqr;
  }
  return sqrt(asubnu);
}

static boolean qr_min_incr_col(linsolqr_system_t sys,
                               int32 curcol,
                               int32 *newcol,
                               real64 *incr,
                               real64 max)
/**
	qr_min_incr_col(sys,curcol,newcol,&min_incr,fin)
	This function finds the column with minimal value
	(1+sigma[k]**2)/alpha[k]**2 in range curcol->fin.
	Returns FALSE if no nonzero alpha^2 are available.
	If all alpha are zero, the curent set of columns is singular.
 **/
{
  int32 j,tmp;
  real64 tmpval,ts,ta,*sigma,*alpha,stmpval;
  boolean ok_column=FALSE;

  tmp=curcol;
  tmpval=MAXDOUBLE;
  sigma=sys->qrdata->sigma;
  alpha=sys->qrdata->alpha;

/*
 * If alpha is 0, then we have a 0 column.  We don't want to put this into
 * the factorization, so skip it.
 */
  for ( j = curcol; j<= max; j++) {
    ta = alpha[j];
    ts = sigma[j];
    if ( (stmpval=SQR(ta)) != D_ZERO) {
      if ( (stmpval=(1+SQR(ts))/stmpval) < tmpval ) {
        tmpval = stmpval;
        tmp = j;
        ok_column = TRUE;
      }
    }
  }
  *newcol = tmp;
  *incr = tmpval;
  return ok_column;
}

static int32 qr_square_select_col(linsolqr_system_t sys,
                                    int32 curcol,
                                    int32 *newcol,
                                    mtx_range_t *rng)
/*
	qr_square_select_col(sys,col,newcolptr,colrange)
 *
	This function returns the number of nonzeros in the column it picks
	for newcol. Newcol is the column that has the least fill but also
	satisfies the relationship column_incr*ctol <= min_column_incr
	where 0 <= ctol <= 1. The column_incr is the condition increment
	used in Stewart's algorithm to determine the next column to factor in.
	If no satisfactory column is found, col is returned in newcol and the
	return value is 0.
 *
	This is where we enforce pivot selection strategies.
 */
{
  boolean found, ok_min_column;
  int32 c,start,fin,ok_count;
  real64 min_incr,incr,pzero, *alpha;
  struct qr_fill_t *fill;

  *newcol=curcol;
  found=FALSE;
  ok_count=0;
  fin=rng->high;
  alpha=sys->qrdata->alpha;
  pzero=sys->pivot_zero;

  ok_min_column=qr_min_incr_col(sys,curcol,newcol,&min_incr,fin);

  if ( curcol == rng->high && fabs(alpha[curcol]) >= pzero )
    return (int32)ok_min_column; /* wrong return if region rectangle */

  if (ok_min_column) {
    real64 *sigma, ts,ta,ctol;

    sigma=sys->qrdata->sigma;
    fill=sys->qrdata->fill;
    ctol=sys->ctol;

    qr_get_fill_data(sys,curcol,rng); /* GET_FILL_TABLE */

/*
 * We will walk through the remaining columns and see if we can find
 * a column that satifies the relationship: column_incr <= min_incr/ctol.
 * Alpha must satisfy pivot_zero before the condition criterion is checked.
 * If we find such a column then ok_count is set and return this and the
 * column number, otherwise return ok_count = 0, at which time the
 * user will either decrease ctol or examine the system more closely.
 */
    start=curcol;
    while (!found && (start <= fin)) {
      ts=sigma[(c=fill[start].col)];
      ta=alpha[c];
      if (fabs(ta)>=pzero) {
        incr=(1+SQR(ts))/SQR(ta);
        if (ctol*incr <= min_incr) {
          found=TRUE;
          *newcol=c;
          ok_count=fill[start].cnt;
        } else {
          start++;
        }
      } else {
        start++;
      }
    }
  } /* else remaining cols singular and user should give up. */
  return ok_count;
}

static real64 qr_permute(linsolqr_system_t sys,
                               int32 curcol,
                               int32 newcol,
                               mtx_range_t *rng)
/*
	This permutes coef, factors and inverse by swapping curcol and newcol
	and swapping rows such that the largest non zero col element is in the
	position curcol, curcol.
	Swaps are performed in the qrdata vectors as needed.
	It is hoped that the permutations in the coef matrix will be done
	mostly on the first solution in any series of similar linear solves.
	Further, the ordering of the resulting coef matrix will give the
	user some idea of the variables contributing to ill conditioning,
	bad variables being more on the right edge.
	The return value is the new a(curcol,curcol).
 */
{
  real64 tmp, *vec,pivot,cmax;
  int32 i;
  boolean found;
  mtx_coord_t coord;
  mtx_matrix_t mtx;

  i = curcol;
  found = FALSE;
  mtx=sys->factors;

  /* find largest nonzero in column */
  coord.row=mtx_FIRST;
  coord.col=newcol;
  cmax=mtx_col_max(mtx,&coord,rng,&pivot);
  i=coord.row;

  if ( 0 == ( found = (cmax >= sys->pivot_zero) ) ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"column %d has no good pivot. Poor pivot= %g accepted",
      newcol,pivot);
    if (pivot==D_ZERO)  /* how the hell did we get here? */
      CONSOLE_DEBUG("Expect disaster VERY shortly!");
  }

#if (LINSOLQR_DEBUG==FALSE)
  if (i!= curcol && found) {
    mtx_swap_rows(mtx,i,curcol);
    mtx_swap_rows(sys->coef,i,curcol);
    /* slave mtx_swap_rows(sys->inverse,i,curcol); needed?*/
  }
#endif
  if (curcol!=newcol) {
    mtx_swap_cols(sys->coef,curcol,newcol);
    mtx_swap_cols(mtx,curcol,newcol);
    /*slave mtx_swap_cols(sys->inverse,curcol,newcol); */

    vec=sys->qrdata->alpha;
    tmp=vec[newcol];
    vec[newcol]=vec[curcol];
    vec[curcol]=tmp;

    vec=sys->qrdata->sigma;
    tmp=vec[newcol];
    vec[newcol]=vec[curcol];
    vec[curcol]=tmp;
  }
  return pivot;
}

static boolean qr_get_householder(linsolqr_system_t sys,
                                  int32 curcol,
                                  mtx_range_t *rng,
                                  real64 *xdothhcol,
                                  real64 x1)
/**
	 A Householder matrix is H = I - tau hhcol dot Transpose(hhcol)
	 where hhcol is a Householder vector. Here H dot x = -s * e1.
	 We suppose the matrix column (x) being transformed is well-scaled:
	 i.e. no gymnastics to avoid over or underflow are needed.
	
	 Calculate the Householder vector hhcol and coefficient tau(curcol)
	 as follows (Intro to Matrix Computations, Stewart, 1971, p233):
	 (the order of operations has been improved to return hhcol dot x)
	
	  0) alpha = 2norm of curcol, precomputed.
	  1) x1=A(c,c)                      (expected to be nonzero passed in)
	  2) s = alpha = copysign(alpha,x1) (avoid cancellation in xdothhcol)
	     Note A(c,c) is overwritten by u(c)(c).
	     R(c,c) is now -s, (alias -alpha[c].)
	  3) xdothhcoll = s*(x1+s)          (Stewart's pi)
	  4) x1 = x1+s;                     (combined with step 3 in code)
	  5) tau[curcol] = 1/xdothhcol unless xdothhcol==0 in which case tau=0
	                               and we get out of here.
	  6) A(curcol,curcol) = x1          (now Stewart's v1)
	
	 The result is an orthogonal vector, normalized by tau[c].
	 Any time you want to do something with the normalized vector, don't
	 forget to use tau[c] as part of the coefficient for the operation.
	 The useful bit is that only A(c,c) must be changed in the matrix.
	 We change A(c,c) here and stuff xdothhcol with x[] dot hhcol.
	
	 Be sure to watch the signs if updating alpha later: not just the norm.
	 It is best if mtx(curcol,curcol) is the largest element in its column.
	
 ! ! Column curcol will have soft zeros deleted from it before any of the
 ! ! above takes place.
	
	 On an empty or zero curcol this will return FALSE, but this should
	 never have been called with such a column. If FALSE, nothing was
	 done to A and xdothhcol and tau[curcol] are 0.
 **/
{
  real64 s,t;
  real64 *hhcol, *alpha, pivot;
  mtx_matrix_t mtx;
  mtx_coord_t coord;

  hhcol=sys->qrdata->hhcol;
  alpha=sys->qrdata->alpha;
  mtx=sys->factors;

  mtx_del_zr_in_col(mtx,curcol);                  /* kill zeros */
  s=alpha[curcol] = copysign(alpha[curcol],x1);   /* alpha = -Rcc */
  t = s*(x1+=s);                                  /* steps 3,4 */
  /* If, like some gcc compilers, your compiler is too amazingly braindead
     to have copysign in the math library as IEEE recommends, link in the
     math library on your system that DOES follow the recommendation.
     On HPs this is /lib/pa1.1/libm.a or libM.a.
  */

/*
 * If t is 0, then the current column will be effectively
 * empty and we will have a zero pivot.  In this case, we
 * can not use this column so GET_HOUSEHOLDER is FALSE
 * and column is undisturbed, except for having been cleaned of zeros.
 */
  if (t != D_ZERO) {
    *xdothhcol=t;
    sys->qrdata->tau[curcol]=D_ONE/t;
    mtx_set_value(mtx,mtx_coord(&coord,curcol,curcol),x1);
    mtx_cur_col_vec(mtx,curcol,hhcol,rng);
    pivot=fabs(x1);
    if (pivot < sys->smallest_pivot)
      sys->smallest_pivot= pivot;
    return TRUE;
  } else {
    *xdothhcol=D_ZERO;
    return FALSE;
  }
}

static void qr_apply_householder(linsolqr_system_t sys,
                                 int32 curcol,
                                 mtx_range_t *rng,
                                 real64 pivot)
/*
 * Takes the square region A indicated by rng and does a Householder
 * transformation on it in place. Curcol should be the same as rng->low.
 * The first column (curcol, rng->low) is transformed into
 * the orthogonal Householder vector, hhcol, unless this is impossible.
 * The pivot passed in should be the value at sys->factors(curcol,curcol).
 *
 * By construction the curcolth diagonal element of R is the negative of
 * alpha[curcol] on exit.
 * The remaining columns in rng are transformed as follows:
 *
	 calculate hhrow (a row vector) = Transpose(hhcol) dot A
	 then calculate
	   A' = A' - tau(curcol) * hhcol dot hhrow
	 where A' is A sans column curcol.
 *
 * This is equivalent to:                     T
	  A = [ I  - tau(curcol) * hhcol dot hhcol ] dot A = H dot A
 * This will change the sparsity of A, by removing 0s and
 * generating nonzeros.
 * xdothhcol is stuffed with x dot hhcol.
 *
 * Calling this with curcol==-2 will cause it to deallocate any memory
 * it has squirreled away and return without doing anything else.
 *
 */
{
  static int32 *hhrowlist=NULL, listlen=0;
  real64 *hhcol, *hhrow, tauc, value,xdothhcol;
  int32 col,row,collim,rowlim;
  mtx_matrix_t mtx;
  mtx_coord_t coord;
  mtx_range_t tmprng;
  boolean hhrowdense=FALSE;
/*
 * hhcol is the vector calculated in the get_householder routine.
 * hhrow is the product Transpose(hhcol)*mtx.
 * Make sure this leaves hhrow/hhcol zero so we can avoid
 * blanket zeroing on entry.
 */
  if (curcol==-2) {
    if (NOTNULL(hhrowlist)) {
      ascfree(hhrowlist);
      hhrowlist=NULL;
    }
    listlen=0;
    return;
  }
  /* no transform on a final singleton or an impossibility */
  if (curcol==rng->high || rng->high < rng->low ) {
    return;
  }

  if (qr_get_householder(sys,curcol,rng,&xdothhcol,pivot)) {
    hhcol = sys->qrdata->hhcol;
    hhrow = sys->qrdata->hhrow;
    if (listlen <= sys->qrdata->cap) {
      listlen= sys->qrdata->cap+1;
      hhrowlist=
        (int32 *)ascrealloc(hhrowlist,listlen*sizeof(int32));
    }
    rowlim=collim=rng->high;
    mtx=sys->factors;
    tauc=sys->qrdata->tau[curcol];

    /* calculate hhrow = Transpose(hhcol) dot A : 1xN dot NxN -> 1xN */
    tmprng=(*rng);
    for (col = curcol+1; col <= collim; col++) {
      hhrow[col]=mtx_col_dot_full_cur_vec(mtx,col,hhcol,&tmprng,FALSE);
    }

    /* do hhrow to rhss if done simultaneous, but they aren't. */

    /* don't want to transform curcol twice! */
    /* apply transform to remaining columns: A -= tauc* hhcol outerdot hhrow */
    /* for each nonzero element of hhcol. don't need hhrow(curcol) */
    tmprng.low++;
#define MINROWSIZE 2
    /* calculate hhrowlist if needed. */
    if (rowlim-tmprng.low < MINROWSIZE) {
      hhrowdense=FALSE;
      hhrowlist[0]=(-1);
    } else {
      col=0;
      hhrowdense=TRUE;
      for (row=tmprng.low; row<=tmprng.high; row++) {
        if (hhrow[row]!=D_ZERO) {
          hhrowlist[col++]=row;
        }
      }
      hhrowlist[col]=(-1);
      CONSOLE_DEBUG("hhrow density (col %d) %g",
        curcol,(D_ONE*col)/(rowlim-curcol+1));
    }
#undef MINROWSIZE
    coord.row=mtx_FIRST;
    coord.col=curcol;
    /* do transform */
    while( value = mtx_next_in_col(mtx,&coord,rng),
           coord.row != mtx_LAST ) {
      if (hhrowdense) {
        mtx_old_add_row_sparse(mtx,coord.row,hhrow,-tauc*hhcol[coord.row],
                          mtx_ALL_COLS,hhrowlist);
      } else {
        mtx_old_add_row_sparse(mtx,coord.row,hhrow,-tauc*hhcol[coord.row],
                          &tmprng,NULL);
      }
      hhcol[coord.row]=D_ZERO;
    }
    /* do rest of transform to rhss if done simultaneous, but they aren't. */

    /* rezero hhrow*/
    if (hhrowdense) {/* zero with list */
      col=0;
      hhrow[curcol]=0;
      while ( (row=hhrowlist[col++]) >=0 )
        hhrow[row]=0;
    } else {
      mtx_zero_real64(&(hhrow[curcol]), (rowlim - curcol +1));
    }

    /* rezero used part of hhcol and do housekeeping on changed rows */
    coord.row=mtx_FIRST;
    while( value = mtx_next_in_col(mtx,&coord,rng),
           coord.row != mtx_LAST ) {
      mtx_del_zr_in_row(mtx,coord.row);
      /*safe only because we cleaned curcol in get_householder */
    }

  } /* else we have an empty column !!! should never be here*/
}

static void qr_update_inverse_norm(struct qr_auxdata *data,int32 col)
/** name says it all. **/
{
  real64 tmps,tmpa,tmpn;
  tmps= data->sigma[col];
  tmpa= data->alpha[col];
  tmpn= data->nu;
  data->nu=sqrt( SQR(tmpn) + (1+SQR(tmps)) / SQR(tmpa) );
}

/**
	Assumes sys->inverse starts out empty.

	Update the R inverse matrix (columnwise) as follows:
		Scale existing column(col) by 1/R(col,col), add element 1/R(col,col)
		to sys->inverse(col,col).

	Note R(c,c) is not in sys->factors but in the alpha vector since the
	diagonal of R is the (correctly signed) norms of the reduced A(c).
	For each v=incidence in columns col+1 to rng->high in row 'col' of R,
	add -v*column(col) to column(vlocation) of sys->inverse.
*/
static void qr_update_inverse(linsolqr_system_t sys,
                              int32 col,
                              mtx_range_t *rng){
  real64 value,icc, *sigma;
  mtx_matrix_t inv, mtx;
  mtx_range_t newcols;
  mtx_coord_t coord;

  sigma=sys->qrdata->sigma;
  inv=sys->inverse;
  mtx=sys->factors;
  newcols=*rng;

  icc=(-D_ONE)/sys->qrdata->alpha[col]; /* apha[i] == -Rii */
  mtx_mult_col(inv,col,icc,mtx_ALL_ROWS);
  mtx_fill_value(inv,mtx_coord(&coord,col,col),icc);
  sigma[col] = sqrt(mtx_sum_sqrs_in_col(inv,col,mtx_ALL_ROWS));
  coord.col=mtx_FIRST;
  newcols.low=col+1;
  /* this loop will be empty if col == rng->high */
  while( value = mtx_next_in_row(mtx,&coord,&(newcols)),
         coord.col != mtx_LAST ) {
    if (value!=D_ZERO) {
      mtx_add_col(inv,col,coord.col,-value,mtx_ALL_ROWS);
      sigma[coord.col] = sqrt(mtx_sum_sqrs_in_col(inv,coord.col,mtx_ALL_ROWS));
      mtx_del_zr_in_col(inv,coord.col);
    }
  }
}

/**
	QR factorisation

	The QR factorization calculated of the square on the diagonal
	from d.low to d.high takes the form

	Q = H(NR-1)H(NR-2)..H(1),
	R = Q*A  where A is the coef matrix,
	H(i) = I - tau(i)*v(i)*Transpose(v(i)) with d.low <= i < d.high.

	The factors matrix contains the H(i) in the lower triangle,
	while the diagonal of R is stored densely (in col order)
	as -alpha[] and the off-diagonal elements of R are in the
	superdiagonal triangle of factors.
*/
static int condqr_factor(linsolqr_system_t sys){
  mtx_range_t active; /* range within sys->rng yet to be pivoted */
  int32 col,newcol=(-1);
  int32 colfound=0;
  real64 onepivot=D_ZERO;
  struct qr_auxdata *data;
  mtx_matrix_t mtx;
  /* test for region size if needed.*/
  /* do initializations need for factoring or refactoring here */
  data=sys->qrdata;
  active=sys->rng;
  data->nu=D_ZERO;
  mtx=sys->factors;
  /* trash any zeros laying around */
  mtx_del_zr_in_colrange(mtx,&active);

  /* compute starting column norms and matrix norm*/
  data->anu = data->asubnu =
    qr_square_compute_alpha(data->alpha,mtx,&active);

  for (col=active.low;col<=active.high;) {
    colfound=qr_square_select_col(sys,col,&newcol,&active);
    if (!colfound) {
      /* jump out of the loop: remaining cols singular. */
      break;
    }
    ++(sys->rank);
    onepivot=qr_permute(sys,col,newcol,&active);
    if (colfound>1) { /* non-singleton */
      qr_apply_householder(sys,col,&active,onepivot);
    } else {
      /**
       ** singletons cheap: Anew=permuted Aold, no fill.
       ** tau(col)= 0.0
       ** alpha(col) = -Anew(col,col) [alpha(i)= -Rii, remember?]
       **/
      data->alpha[col]=(-onepivot);
      data->tau[col]=D_ZERO;       /* should be defaulted, but just in case */
    }
    qr_update_inverse(sys,col,&(sys->rng));
    qr_update_inverse_norm(data,col);
    active.low=(++col);
    /* this makes the final alpha correct, though the final sigma may be off */
    data->asubnu = qr_square_compute_alpha(data->alpha,sys->factors,&active);
  }

  if (!colfound) {
    /* handle setup for singularity, if any */
    return 1;
  }
  return 0;
}

#if LINSOLQR_DEBUG
/*
	qr_apply_inverse(sys,rhs,transpose):
	 convert rhs to c in place.
	 This is generally a bad idea when qr_backward_substitute is available,
	 but this can be used to test the production of Rinvers used in
	 calculating the condition number for CondQR.

	 transpose=FALSE
	 c=Rinverse.rhs
	 could as easily be named upper_tri_plus_diag_dot_org_row_vec.
	 (assuming independent columns/rows 1 to rank)
	 for i=1 to rank
	   c(i)=row(i,(i..rank)) dot rhs(i..rank)
	 endfor
	 transpose=TRUE
	 not implemented.
*/
static void qr_apply_inverse(linsolqr_system_t sys,
                             real64 *arr,
                             boolean transpose
){
  mtx_range_t dot_rng;
  mtx_matrix_t mtx;

  dot_rng.high = sys->rng.low +sys->rank-1;
  mtx=sys->inverse;
  if (transpose) { /* array indexed by original col number */
  } else {
    /* array indexed by original row number */
    for (dot_rng.low = sys->rng.low;
         dot_rng.low <= dot_rng.high; dot_rng.low++) {
      arr[mtx_row_to_org(mtx,dot_rng.low)]=
        mtx_row_dot_full_org_vec(mtx,dot_rng.low,arr,&dot_rng,TRUE);
    }
  }
}
#endif

/**
	convert rhs to c in place (only stored u of H= I-tau u dot Transpose(u)

	transpose=FALSE
	  c=Q.rhs.
	  (assuming independent columns/rows 1 to rank)
	  for j= 1 to rank (apply H(j) to rhs, HH foward elim)
	    if (tau(j)!= 0)
	      w=tau(j)* (Transpose(u(j)) dot c)
	      if (w!=0)
	        c -= w*u(j)
	      endif
	    endif
	  endfor

	transpose=TRUE
	  Solve Transpose(R).c=rhs.  (given R in untransposed form)
	  0<=k<r ==> x(k) = [c(k) - R((0..k-1),k) dot x(0..k-1)]/R(k,k)
*/
static void qr_forward_eliminate(linsolqr_system_t sys,
                                 real64 *arr,
                                 boolean transpose
){
  mtx_range_t dot_rng;
  real64 sum;
  mtx_matrix_t mtx;

  dot_rng.high = sys->rng.low + sys->rank -1;
  mtx = sys->factors;

  if (transpose) { /* ok */
    /* arr is indexed by original column number */
    int32 dotlim,col;
    real64 *diag;
    register int32 org_col;

    diag=sys->qrdata->alpha;
    dot_rng.low= sys->rng.low;
    dotlim=dot_rng.low+sys->rank;
    /* 0<=k<r */
    for( col=dot_rng.low; col <dotlim; ++col) {
      /* rows of transpose are cols of R */
      dot_rng.high=col-1;
      /* sum = R((0..k-1),k) dot x(0..k-1) */
      sum = mtx_col_dot_full_org_vec(mtx,col,arr,&dot_rng,TRUE);
      org_col =  mtx_col_to_org(mtx,col);
      /* arr[org_col] = (arr[org_col] - sum)/ -diag[k]; */
      arr[org_col]= (sum - arr[org_col])/diag[col];
    }
  } else {
    /* arr is indexed by original row number */
    real64 *tau;
    tau=sys->qrdata->tau;

    for( dot_rng.low = sys->rng.low;
         dot_rng.low <= dot_rng.high;
         dot_rng.low++) {
      if (tau[dot_rng.low]!=D_ZERO) {
        sum=mtx_col_dot_full_org_vec(mtx,dot_rng.low,arr,&dot_rng,FALSE);
        if (sum!=D_ZERO) {
          mtx_org_vec_add_col(mtx,arr,dot_rng.low,
                              -sum*tau[dot_rng.low],&dot_rng,FALSE);
        }
      }
    }
  }
}

/**
	qr_backward_substitute(sys,rhs,transpose):
	It is assumed that the R (or Q) part of sys->factors is computed.
	This function converts rhs to c in place by solving one of the
	following:

	transpose = FALSE               transpose = TRUE
	  R.c = rhs                       Q.c = rhs

	The following formulae hold:
	(for rank=r, upper left is R(0,0) transpose= FALSE
	   r>k>=0 --> c(k) = [rhs(k) - R(k,(k+1..r-1)) dot c(k+1..r-1)] / R(k,k)
	   -R(k,k) is assumed to be in sys->qrdata->alpha[k]
	or
	(for rank=r, upper left is Q(0,0) transpose= TRUE
	  c=Transpose(Q).rhs ==>
	  for k = rank..1
	    c = H(k).rhs = rhs - tau*(Transpose(uk) dot rhs) *uk
	    rhs <-- c
	  endfor
*/
static void qr_backward_substitute(linsolqr_system_t sys, real64 *arr,
                                   boolean transpose
){
  mtx_range_t dot_rng;
  real64 sum;
  mtx_matrix_t mtx;
  int32 dotlim;

  dot_rng.high= sys->rng.low+sys->rank -1; /* ultimate pivoted row/col */
  dotlim=sys->rng.low;                     /* upleft corner row/col */
  mtx=sys->factors;

  if (transpose) {
    /* arr is indexed by original column number */
    real64 *tau;
    tau=sys->qrdata->tau;

/***    for k = rank..1
	    c = H(k).rhs = rhs - tau*(Transpose(uk) dot rhs) *uk
	    rhs <-- c
	  endfor
*/
    for (dot_rng.low=dot_rng.high-1; dot_rng.low >=dotlim; dot_rng.low--) {
    /* H(rank) is I for square, nonsingular systems. for sing. sys this
       code is wrong.
    */
      if (tau[dot_rng.low]!=D_ZERO) {
        sum=mtx_col_dot_full_org_vec(mtx,dot_rng.low,arr,&dot_rng,TRUE);
        if (sum !=D_ZERO) {
          mtx_org_vec_add_col(mtx,arr,dot_rng.low,-sum*tau[dot_rng.low],
                              &dot_rng,TRUE);
        }
      }
    }
  } else {
    int32 org_row,row;
    real64 *diag;

    diag=sys->qrdata->alpha;

    /* r >k>=0 we are working backwards through the pivoted rows.     */
    /* dot_rng is stuff to the right of the current pivot, (row,row). */

    for( row = dot_rng.high; row >=dotlim; --row) {
      dot_rng.low=row+1; /* our dot left edge is just after the pivot */

      /* sum = R(k,(k+1..r-1)) dot c(k+1..r-1) */
      sum= mtx_row_dot_full_org_vec(mtx,row,arr,&dot_rng,TRUE);
      org_row= mtx_row_to_org(mtx,row);

      /* c(k) = [rhs(k) -sum] /R(k,k) */
      /* arr[org_row] = (arr[org_row] - sum) / -diag[row]; */
      arr[org_row] = (sum - arr[org_row])/diag[row];
    }
  }
}

/**
	The region to factor is first isolated by truncating the region
	provided to the largest square region confined to the matrix diagonal.
	It is presumed it will contain no empty rows or columns and that it has
	been previously reordered using linsolqr_reorder(sys,region,tspk1).
	on exit, sys->coef, sys->factors, and sys->inverse will have been
	permuted identically by solution process.
*/
static int condqr_entry(linsolqr_system_t sys,mtx_region_t *region){
   struct rhs_list *rl;
   boolean rank_deficient;

   UNUSED_PARAMETER(region);

   CHECK_SYSTEM(sys);
   if( sys->factored )
      return 0;
   if( sys->fmethod!=cond_qr )
      return 1;
   if(ISNULL(sys->qrdata))
      return 1;

/*
   if (region == mtx_ENTIRE_MATRIX)
     determine_pivot_range(sus);
   else
     square_region(sys,region);
*/

   if( NOTNULL(sys->factors) )
      mtx_destroy(sys->factors);
   if( NOTNULL(sys->inverse)  )
      mtx_destroy(sys->inverse);
   sys->factors = mtx_copy_region(sys->coef, mtx_region(&(sys->qrdata->facreg),
                                             sys->rng.low,sys->rng.high,
                                             sys->rng.low,sys->rng.high));
   sys->inverse = mtx_create_slave(sys->factors);
   sys->rank = 0;
   sys->smallest_pivot = MAXDOUBLE;
   for( rl = sys->rl ; NOTNULL(rl)  ; rl = rl->next )
      rl->solved = FALSE;
   ensure_capacity(sys);
   ensure_qr_capacity(sys);

   rank_deficient=condqr_factor(sys);
   if (rank_deficient) {
#if LINSOLQR_DEBUG
     int j;
#endif
     ERROR_REPORTER_HERE(ASC_PROG_WARNING,"condqr found rank %d of %d",sys->rank,
     sys->rng.high-sys->rng.low+1);
#if LINSOLQR_DEBUG
     CONSOLE_DEBUG("alpha vec:(curcol,val)");
       for (j=sys->qrdata->facreg.col.low;
            j<= sys->qrdata->facreg.col.high; j++)
         CONSOLE_DEBUG("alpha[%d] = %.8g",j,sys->qrdata->alpha[j]);
     CONSOLE_DEBUG("tau vec:(curcol,val)");
       for (j=sys->qrdata->facreg.col.low;
            j<= sys->qrdata->facreg.col.high; j++)
         CONSOLE_DEBUG("tau[%d] = %.8g",j,sys->qrdata->tau[j]);
#endif
/* asssumes lu. not autodone anyway. see the other protocol.
   calc_dependent_rows_ranki1(sys);
   calc_dependent_cols_ranki1(sys);
*/
   }
   sys->factored = TRUE;
   return 0;
}

/**
	 Solve a previously qr factorized matrix with a rhs b.
	 If b is not transposed (is org row ordered):
	 c:=Q.b, then solve R.x=c for x.
 **/
static int condqr_solve(linsolqr_system_t sys,struct rhs_list *rl)
{
  qr_forward_eliminate(sys,rl->varvalue,rl->transpose);
  qr_backward_substitute(sys,rl->varvalue,rl->transpose);
  /* doesn't the following destroy the least squares solution? */
  zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
  return 0;
}
/*
  End of CondQR implementation.
*/

/*----------------------------------------------------------------------------*/

/*
	include CPQR implementation. Note that this is VERY VERY bad style
	and needs to be fixed immediately by splitting up the linsolqr file.

	(said by someone about 12 years ago -- JP 2007)
*/
#if 0
#include "plainqr.c"
               /* ^-- gawd that is so naughty */
#endif

/*-----------------------------------------------------------------------------
  more external calls, likely to distribute over the method of reorder
  or factor.
*/

int linsolqr_prep(linsolqr_system_t sys,enum factor_class fclass)
{
/* this will get a little more involved if sacrifice options come along */
  if(CHECK_SYSTEM(sys)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with bad linsolqr_system.");
    return 1;
  }
  switch (fclass) {
  case ranki:
    if (ISNULL(sys->ludata))
      sys->ludata=create_ludata();
    destroy_qrdata(sys->qrdata);
    sys->qrdata=NULL;
    break;
  case s_qr:
    if (ISNULL(sys->qrdata))
    sys->qrdata=create_qrdata();
    destroy_ludata(sys->ludata);
    sys->ludata=NULL;
    break;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"called with unsupported factor class");
    sys->fclass=unknown_c;
    return 1;
  }
  sys->fclass = fclass;
  return 0;
}

/**
	The region to reorder is handled according to the method specified.
	This function distributes to reorder_$method and sets the method for
	factoring. If reorder fails, method is set unknown.
*/
int linsolqr_reorder(linsolqr_system_t sys,mtx_region_t *region,
                     enum reorder_method method
){
   int reostatus=0;
   CHECK_SYSTEM(sys);
   sys->rmethod=method; /* set method of this call */
   switch (method) {
   case spk1:
      reostatus=ranki_reorder(sys,region);
      break;
   case tspk1:
      reostatus=tranki_reorder(sys,region);
      break;
   case natural:
      square_region(sys,region);
      break;
   default:
      reostatus=1;
      break;
   }
   if (reostatus) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error %d in reordering with %s",reostatus,
        linsolqr_enum_to_rmethod(method));
      sys->rmethod=unknown_r;
   }
   return reostatus;
}

/**
	The region to factor is determined by the factorization method in use.
	It is presumed it will contain no empty rows or columns and that it has
	been previously reordered using linsolqr_reorder.

	This function distributes to $method_entry. $method_entry is
	responsible for any and all checking and allocation that needs
	to be done and then calling a factorization routine and updating
	status flags. The return from $method_entry is returned.
*/
int linsolqr_factor(linsolqr_system_t sys, enum factor_method fmeth){
  int facstatus=0;

  CHECK_SYSTEM(sys);
  if (fmeth != sys->fmethod) {
    linsolqr_matrix_was_changed(sys);
    if (sys->fclass != linsolqr_fmethod_to_fclass(fmeth)) {
      return 1;
    } else {
      sys->fmethod = fmeth;
    }
  }
  if (sys->factored)
    return facstatus;
  switch (sys->fmethod) {
  case ranki_kw:
  case ranki_jz:
    facstatus = ranki_entry(sys,&(sys->reg));
    break;
  case ranki_ba2:
  case ranki_kw2:
  case ranki_jz2:
    facstatus = ranki2_entry(sys,&(sys->reg));
    break;
#ifdef BUILD_KIRK_CODE
  case ranki_ka:
    facstatus = kirk1_factor(sys,&(sys->reg),2);
    break;
#endif /* BUILD_KIRK_CODE */
  case cond_qr:
    facstatus = condqr_entry(sys,&(sys->reg));
    break;
   case plain_qr:
    facstatus = cpqr_entry(sys,&(sys->reg)); /* BAA. check reg */
    break;
  default:
    facstatus = 1;
    break;
  }
  if (facstatus) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error %d in factoring with %s",facstatus,
            linsolqr_enum_to_fmethod(sys->fmethod));
  }
  return facstatus;
}

int linsolqr_get_pivot_sets(linsolqr_system_t sys
		,unsigned *org_rowpivots
		,unsigned *org_colpivots
){
   int32 ndx;

   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
#if LINSOL_DEBUG
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"system not factored yet.");
#endif
      return 0;
   }
   for( ndx = sys->rng.low ; ndx < sys->rng.low + sys->rank ; ++ndx ) {
      set_change_member(org_rowpivots,mtx_row_to_org(sys->factors,ndx),TRUE);
      set_change_member(org_colpivots,mtx_col_to_org(sys->factors,ndx),TRUE);
   }
   return 1;
}

mtx_sparse_t *linsolqr_unpivoted_rows(linsolqr_system_t sys)
{
  mtx_sparse_t *ret = NULL;
  int32 k=0,defect,ndx;

  CHECK_SYSTEM(sys);
  if( !sys->factored  || ISNULL(sys->ludata) || ISNULL(sys->ludata->pivlist)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
    return ret;
  }
  defect = sys->rng.high - (sys->rng.low + sys->rank -1);
  if (defect <= 0) return ret; /* actually, should never be negative. */
  ret = mtx_create_sparse(defect);
  if (ISNULL(ret)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"mtx_sparse_t create failed.");
    return ret;
  }
  ret->len = defect;
  for( ndx = sys->rng.low + sys->rank; ndx <= sys->rng.high ; ndx++ ) {
    ret->idata[k] = mtx_row_to_org(sys->factors,ndx);
    ret->data[k] = sys->ludata->pivlist[ndx];
    k++;
  }
  return ret;
}

mtx_sparse_t *linsolqr_unpivoted_cols(linsolqr_system_t sys){
  mtx_sparse_t *ret = NULL;
  int32 k=0,defect,ndx;

  CHECK_SYSTEM(sys);
  if( !sys->factored  || ISNULL(sys->ludata) || ISNULL(sys->ludata->pivlist)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
    return ret;
  }
  defect = sys->rng.high - (sys->rng.low + sys->rank -1);
  if (defect <= 0) return ret; /* actually, should never be negative. */
  ret = mtx_create_sparse(defect);
  if (ISNULL(ret)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"mtx_sparse_t create failed.");
    return ret;
  }
  ret->len = defect;
  for( ndx = sys->rng.low + sys->rank; ndx <= sys->rng.high ; ndx++ ) {
    ret->idata[k] = mtx_col_to_org(sys->factors,ndx);
    ret->data[k] = sys->ludata->pivlist[ndx];
    k++;
  }
  return ret;
}


mtx_sparse_t *linsolqr_pivoted_rows(linsolqr_system_t sys){
  mtx_sparse_t *ret = NULL;
  int32 k=0,ndx;

  CHECK_SYSTEM(sys);
  if( !sys->factored  || ISNULL(sys->ludata) || ISNULL(sys->ludata->pivlist)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
    return ret;
  }
  if (sys->rank < 0 ) return ret; /* actually, should never be negative. */
  ret = (mtx_sparse_t *)ascmalloc(sizeof(mtx_sparse_t));
  if (ISNULL(ret)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory");
    return ret;
  }
  ret->cap = sys->rank;
  ret->len = sys->rank;
  ret->data = (real64 *)ascmalloc(sizeof(real64)*sys->rank);
  ret->idata = (int32 *)ascmalloc(sizeof(int32)*sys->rank);
  if (ISNULL(ret->data) || ISNULL(ret->idata)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    mtx_destroy_sparse(ret);
    ret = NULL;
    return ret;
  }
  for( ndx = sys->rng.low; ndx <= sys->rng.low + sys->rank - 1; ndx++ ) {
    ret->idata[k] = mtx_row_to_org(sys->factors,ndx);
    ret->data[k] = sys->ludata->pivlist[ndx];
    k++;
  }
  return ret;
}


mtx_sparse_t *linsolqr_pivoted_cols(linsolqr_system_t sys){
  mtx_sparse_t *ret = NULL;
  int32 k=0,ndx;

  CHECK_SYSTEM(sys);
  if( !sys->factored  || ISNULL(sys->ludata) || ISNULL(sys->ludata->pivlist)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
    return ret;
  }
  if (sys->rank <= 0) return ret; /* actually, should never be negative. */
  ret = (mtx_sparse_t *)ascmalloc(sizeof(mtx_sparse_t));
  if (ISNULL(ret)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    return ret;
  }
  ret->cap = sys->rank;
  ret->len = sys->rank;
  ret->data = (real64 *)ascmalloc(sizeof(real64)*sys->rank);
  ret->idata = (int32 *)ascmalloc(sizeof(int32)*sys->rank);
  if (ISNULL(ret->data) || ISNULL(ret->idata)) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
    mtx_destroy_sparse(ret);
    ret = NULL;
    return ret;
  }
  for( ndx = sys->rng.low; ndx <= sys->rng.low + sys->rank - 1; ndx++ ) {
    ret->idata[k] = mtx_col_to_org(sys->factors,ndx);
    ret->data[k] = sys->ludata->pivlist[ndx];
    k++;
  }
  return ret;
}


#define org_row_to_org_col(sys,org_row) \
   mtx_col_to_org((sys)->factors,mtx_org_to_row((sys)->factors,(org_row)))
#define org_col_to_org_row(sys,org_col) \
   mtx_row_to_org((sys)->factors,mtx_org_to_col((sys)->factors,(org_col)))

int32 linsolqr_org_row_to_org_col(linsolqr_system_t sys,
                                        int32 org_row
){
   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
   }
   return( org_row_to_org_col(sys,org_row) );
}

int32 linsolqr_org_col_to_org_row(linsolqr_system_t sys,
                                        int32 org_col)
{
  CHECK_SYSTEM(sys);
  if( !sys->factored ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
  }
  return( org_col_to_org_row(sys,org_col) );
}

/* 
	this is a distributor to the appropriate functions.  This function
	should probably have more arguments in the future.
*/
void linsolqr_calc_row_dependencies(linsolqr_system_t sys){
  CHECK_SYSTEM(sys);
  if( !sys->factored ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
  }
  switch (sys->fmethod) {
  case ranki_kw:
  case ranki_jz:
    calc_dependent_rows_ranki1(sys);
    break;
  case ranki_ba2:
  case ranki_kw2:
  case ranki_jz2:
    calc_dependent_rows_ranki2(sys);
    break;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,
      "Don't know how to calculate for method %s."
      ,linsolqr_enum_to_fmethod(sys->fmethod)
    );
  }
  return;
}

void linsolqr_calc_col_dependencies(linsolqr_system_t sys){
  if( !sys->factored ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
  }
  switch (sys->fmethod) {
  case ranki_kw:
  case ranki_jz:
    calc_dependent_cols_ranki1(sys);
    break;
  case ranki_ba2:
  case ranki_kw2:
  case ranki_jz2:
    calc_dependent_cols_ranki2(sys);
    break;
  default:
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Don't know how to calculate for method %s.",
            linsolqr_enum_to_fmethod(sys->fmethod));
  }
  return;
}

mtx_sparse_t *linsolqr_row_dependence_coefs(linsolqr_system_t sys, int32 org){
  mtx_coord_t nz;
  mtx_sparse_t *ret=NULL;
  int32 k;
  mtx_range_t cols;
  mtx_matrix_t mtx;

  CHECK_SYSTEM(sys);
  if( !sys->factored ) {
#if LINSOL_DEBUG
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
#endif
    return NULL;
  }

  if( !sys->rowdeps ) {
#if LINSOL_DEBUG
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Dependency not yet calculated.");
#endif
    return NULL;
  }

  mtx = sys->factors;
  nz.row = mtx_org_to_row(mtx,org);
  /* checks for independence, in-block-ness */
  if( ((nz.row <= sys->rng.low+sys->rank-1) && (nz.row >= sys->rng.low)) ||
       nz.row < sys->rng.low || nz.row > sys->rng.high) {
    return NULL;
  }

  /* get data */
  cols.low = sys->rng.low;
  cols.high = sys->rng.low + sys->rank -1;
  ret = mtx_create_sparse(MAX(sys->reg.row.high - sys->reg.row.low + 1,
                                  sys->reg.row.high - sys->reg.row.low + 1));
  ret = mtx_cur_row_sparse(mtx,nz.row,ret,&cols,mtx_IGNORE_ZEROES);
  if (ISNULL(ret)) return ret; /* a very weird event! */

  /* reindex it to org rows */
  for (k=0; k < ret->len; k++) {
    ret->idata[k] = mtx_row_to_org(mtx,ret->idata[k]);
  }

  return ret;
}

mtx_sparse_t *linsolqr_col_dependence_coefs(linsolqr_system_t sys, int32 org){
  mtx_coord_t nz;
  mtx_sparse_t *ret=NULL;
  int32 k;
  mtx_range_t rows;
  mtx_matrix_t mtx;

  CHECK_SYSTEM(sys);
  if( !sys->factored ) {
#if LINSOL_DEBUG
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
#endif
    return NULL;
  }

  if( !sys->rowdeps ) {
#if LINSOL_DEBUG
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Dependency not yet calculated.");
#endif
    return NULL;
  }

  mtx = sys->factors;
  nz.col = mtx_org_to_col(mtx,org);
  /* checks for independence, in-block-ness */
  if( ((nz.col <= sys->rng.low+sys->rank-1) && (nz.col >= sys->rng.low)) ||
       nz.col < sys->rng.low || nz.col > sys->rng.high) {
    return NULL;
  }

  /* get data */
  rows.low = sys->rng.low;
  rows.high = sys->rng.low + sys->rank -1;
  ret = mtx_create_sparse(MAX(sys->reg.row.high - sys->reg.row.low + 1,
                                  sys->reg.row.high - sys->reg.row.low + 1));
  ret = mtx_cur_col_sparse(mtx,nz.col,ret,&rows,mtx_IGNORE_ZEROES);

  if (ISNULL(ret)) return ret; /* a very weird event! */

  /* reindex it to org cols */
  for (k=0; k < ret->len; k++) {
    ret->idata[k] = mtx_col_to_org(mtx,ret->idata[k]);
  }

  return ret;
}

real64 linsolqr_org_row_dependency(linsolqr_system_t sys,
		int32 dep,
		int32 ind
){
   mtx_coord_t nz;

   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
#if LINSOL_DEBUG
      FERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet. Returning 0.");
#endif
      return(D_ZERO);
   }

   nz.col = mtx_org_to_row(sys->factors,ind);
   if( (sys->rng.low > nz.col) || (nz.col > sys->rng.low+sys->rank-1) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Original row %ld is not independent. Returning 0.",
              (long)ind);
      return(D_ZERO);
   }

   nz.row = mtx_org_to_row(sys->factors,dep);
   if( (nz.row <= sys->rng.low+sys->rank-1) && (nz.row >= sys->rng.low) )
      return( ind == dep ? D_ONE : D_ZERO );

   return(mtx_value(sys->factors,&nz));
}

real64 linsolqr_org_col_dependency(linsolqr_system_t sys,
                                         int32 dep,
                                         int32 ind)
{
   mtx_coord_t nz;

   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
#if LINSOL_DEBUG
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet. Returning 0.");
      return(D_ZERO);
#endif
   }

   nz.row = mtx_org_to_col(sys->factors,ind);
   if( (sys->rng.low > nz.row) || (nz.row > sys->rng.low+sys->rank-1) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Original col %ld is not independent. Returning 0.",
              (long)ind);
      return(D_ZERO);
   }

   nz.col = mtx_org_to_col(sys->factors,dep);
   if( (nz.col <= sys->rng.low+sys->rank-1) && (nz.col >= sys->rng.low) )
      return( ind == dep ? D_ONE : D_ZERO );

   return(mtx_value(sys->factors,&nz));
}


/**
	Assuming the bounding block region of the matrix has been previously
	factored, the specified rhs can then be applied.
	
	Application is specific to the method in question.
	
	SPK1/RANKI and variants:
	 If rhs has transpose==FALSE
	    A x = U L x = rhs.  Define c := L x, solve U c = rhs and L x = c.
	
	 or
	 If rhs has transpose==TRUE
	     T     T T                       T          T             T
	    A x = L U x = rhs.  Define c := U x, solve L c = rhs and U x = c.
	
	 The variables associated with any of the unpivoted original rows
	 and columns are assigned the value of zero by convention.
	 The diagonal has the element of L, the diag of U 1 by construction.
	
	CondQR:
	    Define Q :=PROD( H(i) | i in rank...1 ) (note H(rank)= I)
	    R = Q.A                                 (note Q^-1 = Q^T)
	                                            (why? H^T=H=H^-1)
	
	 If rhs has transpose==FALSE
	    A.x=rhs.  Define c := Q.rhs, then solve R.x = c.
	    Notes: If A is singular, x will be the least squares solution.
	 or
	 If rhs has transpose==TRUE
	    Notes: If A is singular, what the heck does solving the transpose
	    mean?
	
	         -1      T        T     T   T    T
	    A = Q  .R = Q .R, so A  = (Q .R)  = R .Q, then:
	
	     T      T                                   T
	    A .x = R .Q.x = rhs. Define Q.x = c. Solve R .c = rhs for c.
	
	     -1        -1           T
	    Q  .Q.x = Q  .c => x = Q .c = PROD( H(i) | i in 1...rank ).c.
	
	 The variables associated with any of the unpivoted original rows
	 and collumns are assigned the value of zero by convention.
	 The diagonal has the element of H, the diag of R is stored elsewhere.
	
	The *_solve functions called from here are expected to zero or
	leave a least squares value set in the solution vector for unpivoted
	  relations/variables.
 **/
int linsolqr_solve(linsolqr_system_t sys,real64 *rhs){
   struct rhs_list *rl;
   int solstatus=0;

   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
   }
   rl = find_rhs(sys->rl,rhs);
   if( NOTNULL(rl) ) {
      if( rl->solved )
         return 0;
      if( ISNULL(rl->varvalue) ) { /* rhs wasn't around at factor time */
        rl->varvalue=(real64 *)
          ascmalloc(sys->capacity*sizeof(real64));
      }
      mem_copy_cast(rl->rhs,rl->varvalue,sys->capacity*sizeof(real64));
      switch (sys->fmethod) {
      case ranki_kw:
      case ranki_jz:
         solstatus = ranki_solve(sys,rl);
         break;
      case ranki_ba2:
      case ranki_kw2:
      case ranki_jz2:
         solstatus = ranki2_solve(sys,rl);
         break;
      case cond_qr:
         solstatus = condqr_solve(sys,rl);
         break;
      case plain_qr:
         solstatus = cpqr_solve(sys,rl);
         break;
      default:
         solstatus=1;
         break;
      }
      if (!solstatus) {
         rl->solved = TRUE;
      } else {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error %d in solving with %s",solstatus,
          linsolqr_enum_to_fmethod(sys->fmethod));
      }
   } else {
     if( NOTNULL(rhs) ) {
       ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not found on list.");
     } else {
       ERROR_REPORTER_HERE(ASC_PROG_ERR,"No rhs exist at all.");
     }
   }
   return solstatus;
}
/** Some notes for internal consumption on the ordering of rl->varvalue.
	baa. 1-10-95

	On a matrix permuted during solution:

	If rl is not a transpose:
	    At the beginning of solution, rl->varvalue[eqn] is the residual of
	    the equation to be solved in original matrix row number eqn (M).
	    After solution, rl->varvalue[eqn] is the value of the variable
	    pivoted under in that equation.

	    To get the value of the variable
	    corresponding to original column N, you must find column P,
	    the current column that N became during solution. The pivoting
	    happened at A(currow,curcol) = A(P,P). Now take row P and find
	    the equation M (original row number).
	    The value of variable(N) is stored in varvalue(M).

	    The macros org_row_to_org_col and org_col_to_org_row
	    take care if translating between org_cols(N) and org_rows(M).

	If rl is a transpose:
		Well, you figure it out.
 **/

real64 linsolqr_var_value(linsolqr_system_t sys,
                                real64 *rhs,
                                int32 ndx)
{
   struct rhs_list *rl;

   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
      return D_ZERO;
   }
   rl = find_rhs(sys->rl,rhs);
   if( NOTNULL(rl) ) {
      if( !(rl->solved) ) {
         ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not solved yet.");
         return D_ZERO;
      }
      if( rl->transpose )
         /* ndx is an original row index */
         return( rl->varvalue[org_row_to_org_col(sys,ndx)] );
      else
         /* ndx is an original column index */
         return( rl->varvalue[org_col_to_org_row(sys,ndx)] );
   } else {
     if( NOTNULL(rhs) ) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs does not exist.");
        return D_ZERO;
     }
   }
   ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL rhs given does not exist, idiot (yeah, nice one, computer). Returning 0.");
   return D_ZERO;
}

boolean linsolqr_copy_solution(linsolqr_system_t sys, real64 *rhs,
                               real64 *vector)
{
  struct rhs_list *rl;
  real64 *varvalue;
  int ndx,size;

  CHECK_SYSTEM(sys);
  if( !sys->factored ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
    return TRUE;
  }
  rl = find_rhs(sys->rl,rhs);
  if( NOTNULL(rl) ) {
    if( !(rl->solved) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not solved yet.");
      return TRUE;
    }
    if( ISNULL(rl->varvalue) ) { /* shouldn't be possible! */
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs solved, but solution missing!");
      return TRUE;
    }

    size = sys->capacity;
    varvalue = rl->varvalue;
    if( rl->transpose ) {       /* ndx is an original row index */
      for (ndx = 0;ndx < size;ndx++) {
        vector[ndx] = varvalue[org_row_to_org_col(sys,ndx)];
      }
    } else {             /* ndx is an original column index */
      for (ndx = 0;ndx < size;ndx++) {
        vector[ndx] = varvalue[org_col_to_org_row(sys,ndx)];
      }
    }
  } else if( NOTNULL(rhs) ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not found.");
    return TRUE;
  }
  return FALSE;
}

real64 linsolqr_eqn_residual(linsolqr_system_t sys,
                                   real64 *rhs,
                                   int32 ndx)
{
   struct rhs_list *rl;
   mtx_coord_t nz;
   real64 value;
   real64 lhs;
   mtx_matrix_t mtx;

   CHECK_SYSTEM(sys);
   mtx = sys->coef;
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
      return (real64)MAXDOUBLE;
   }
   rl = find_rhs(sys->rl,rhs);
   if( NOTNULL(rl) ) {
      real64 *varvalue, *eqnrhs;
      varvalue=rl->varvalue;
      eqnrhs=rl->rhs;
      lhs = D_ZERO;
      if( !(rl->solved) ) {
         ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not solved yet.");
         return (real64)MAXDOUBLE;
      }
      if (rl->transpose) {
         /* ndx is an original column index */
         /* rl->varvalue is indexed peculiarly. see above */
         nz.col = mtx_org_to_col(mtx,ndx);
         nz.row = mtx_FIRST;
         while( value = mtx_next_in_col(mtx,&nz,&(sys->reg.row)),
               nz.row != mtx_LAST )
            lhs += value *
               varvalue[org_row_to_org_col(sys,mtx_row_to_org(mtx,nz.row))];
         return( lhs - eqnrhs[ndx] );
      } else {
         /* ndx is an original row index */
         /* rl->varvalue is indexed as described above */
         /* there isn't much to speed this up except move the
            functionality down to mtx, and it doesn't belong there */
         nz.row = mtx_org_to_row(mtx,ndx);
         nz.col = mtx_FIRST;
         while( value = mtx_next_in_row(mtx,&nz,&(sys->reg.col)),
               nz.col != mtx_LAST )
            lhs += value *
              varvalue[org_col_to_org_row(sys,mtx_col_to_org(mtx,nz.col))];
         return( lhs - eqnrhs[ndx] );
      }
   } else {
      if( NOTNULL(rhs) ) {
         ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs does not exist.");
         return (real64)MAXDOUBLE;
      }
   }
   ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL rhs does not exist, idiot.");
   return (real64)MAXDOUBLE;
}

boolean linsolqr_calc_residual(linsolqr_system_t sys,
                               real64 *rhs,
                               real64 *vec)
{
  struct rhs_list *rl;
  mtx_matrix_t mtx;
  int32 ndx;

  CHECK_SYSTEM(sys);
  mtx = sys->coef;
  if( !sys->factored ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
    return TRUE;
  }
  rl = find_rhs(sys->rl,rhs);
  if( NOTNULL(rl) ) {
    int32 curcol,hicol,currow,hirow;
    real64 *orgvars=NULL;                /* scratch work space */
    real64 *varvalue, *eqnrhs;

    if( !(rl->solved) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not solved yet.");
      return TRUE;
    }
    if( ISNULL(rl->varvalue) ) { /* shouldn't be possible! */
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs solved, but solution missing!");
      return TRUE;
    }

    orgvars = sys->capacity > 0 ? ASC_NEW_ARRAY_CLEAR(real64,sys->capacity) : NULL;
    varvalue=rl->varvalue;
    eqnrhs=rl->rhs;
    hicol=sys->reg.col.high;
    hirow=sys->reg.row.high;
    /* ndx is an original index */
    /* rl->varvalue is indexed as described above */
    /* copy varvalue into an appropriately permuted array
       and use a regular mtx operator to do the dots */

    if (rl->transpose) {
      /* get solution vec into dottable form */
      for (currow=sys->reg.row.low; currow<=hirow; currow++) {
        ndx = mtx_row_to_org(mtx,currow);
        orgvars[ndx]=varvalue[org_row_to_org_col(sys,ndx)];
      }
      /* dot it over the interesting cols. */
      for ( curcol=sys->reg.col.low; curcol<= hicol; curcol++) {
        ndx = mtx_col_to_org(mtx,curcol);
        vec[ndx] =
          mtx_col_dot_full_org_vec(mtx,curcol,orgvars,mtx_ALL_ROWS,FALSE)
          - eqnrhs[ndx];
      }
    } else {
      /* get solution vec into dottable form */
      for (curcol=sys->reg.col.low; curcol<=hicol; curcol++) {
        ndx = mtx_col_to_org(mtx,curcol);
        orgvars[ndx]=varvalue[org_col_to_org_row(sys,ndx)];
      }
      /* dot it over the interesting rows. */
      for ( currow=sys->reg.row.low; currow<= hirow; currow++) {
        ndx = mtx_row_to_org(mtx,currow);
        vec[ndx] =
          mtx_row_dot_full_org_vec(mtx,currow,orgvars,mtx_ALL_COLS,FALSE)
          - eqnrhs[ndx];
      }
    }
    if (NOTNULL(orgvars)) ascfree(orgvars);
    return FALSE;
  } else {
    if( NOTNULL(rhs) ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs does not exist.");
      return TRUE;
    }
  }
  ERROR_REPORTER_HERE(ASC_PROG_ERR,"NULL rhs does not exist, idiot.");
  return TRUE;
}

void linsolqr_free_reused_mem() {
  qr_apply_householder(NULL,-2,NULL,D_ZERO);
  reset_elimination_data(0,0); /* clear rankiba2 */
}

size_t linsolqr_size(linsolqr_system_t sys)
{
  size_t s = 0;
  struct rhs_list *rl;

  if (ISNULL(sys)) return 0;
  if (CHECK_SYSTEM(sys)) return 0;
  s += sizeof(struct linsolqr_header);
  if (NOTNULL(sys->ludata)) {
    s += sizeof(struct lu_auxdata);
    if (NOTNULL(sys->ludata->pivlist))
      s += sizeof(real64) * sys->ludata->cap;
    if (NOTNULL(sys->ludata->tmp))
      s += sizeof(real64) * sys->ludata->cap;
  }
  if (NOTNULL(sys->qrdata)) {
    s += sizeof(struct qr_auxdata);
    if (NOTNULL(sys->qrdata->alpha))
      s += sizeof(real64) * sys->qrdata->cap;
    if (NOTNULL(sys->qrdata->sigma))
      s += sizeof(real64) * sys->qrdata->cap;
    if (NOTNULL(sys->qrdata->tau))
      s += sizeof(real64) * sys->qrdata->cap;
    if (NOTNULL(sys->qrdata->hhcol))
      s += sizeof(real64) * sys->qrdata->cap;
    if (NOTNULL(sys->qrdata->hhrow))
      s += sizeof(real64) * sys->qrdata->cap;
    if (NOTNULL(sys->qrdata->fill))
      s += sizeof(struct qr_fill_t) * sys->qrdata->cap;
  }
  rl = sys->rl;
  while( NOTNULL(rl) ) {
    s += sizeof(struct rhs_list);
    if (NOTNULL(rl->varvalue))
      s += sizeof(real64) * sys->capacity;
    rl = rl->next;
    /* note if there is some screwup, the varvalue may be shorter in reality
       than sys->capacity. we are ignoring user data given */
  }
  switch (sys->fclass) {
    case ranki:
      if (NOTNULL(sys->factors)) s += mtx_size(sys->factors);
      /* this works because sys->inverse is either NULL or a slave for
         the current ranki family: kw jz kw2 jz2 */
    break;
    case s_qr: /* this will change from 9/95 if qr redone */
      if (NOTNULL(sys->factors)) s += mtx_size(sys->factors);
      if (NOTNULL(sys->inverse)) s += mtx_size(sys->inverse);
    break;
    default: /* this is a best guess if we don't know what we are. */
      if (NOTNULL(sys->factors)) s += mtx_size(sys->factors);
      if (NOTNULL(sys->inverse)) s += mtx_size(sys->inverse);
    break;
  }
  /* we are ignoring sys->coef */
  return s;
}

mtx_matrix_t linsolqr_get_factors(linsolqr_system_t sys)
{
  return sys->factors;
}
mtx_matrix_t linsolqr_get_inverse(linsolqr_system_t sys)
{
  return sys->inverse;
}

/*------------------------------------------------------------------------------
  Ken's (Ken Tyner, I presume -- JP) Playground for NGSlv linear functions
*/

/* this function needs to return some value in all cases */
int linsolqr_setup_ngslv(linsolqr_system_t sys,
                         real64 *rhs,
                         mtx_range_t *un_p_rng,
                         real64 *tmpvec)
{
   struct rhs_list *rl;
   int status=0,k;

   CHECK_SYSTEM(sys);
   if( !sys->factored ) {
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"System not factored yet.");
   }
   rl = find_rhs(sys->rl,rhs);
   if( NOTNULL(rl) ) {
      if( rl->solved )
         return 0;
      if( ISNULL(rl->varvalue) ) { /* rhs wasn't around at factor time */
        rl->varvalue=(real64 *)
          ascmalloc(sys->capacity*sizeof(real64));
      }
      mem_copy_cast(rl->rhs,rl->varvalue,sys->capacity*sizeof(real64));
      /* save B2 section of rhs in lower section of tmpvec */

/* KHACK: think about the following data transfer a bit more */
/* OK it looks like zero_unpivoted_vars uses row_to_org */
/* also remove dereferencing */
      for (k = un_p_rng->low; k <= un_p_rng->high; k++){
          tmpvec[mtx_row_to_org(sys->factors,k)] = rl->varvalue[mtx_row_to_org(sys->factors,k)];
      }
      zero_unpivoted_vars(sys,rl->varvalue,rl->transpose);
      backward_substitute2(sys,rl->varvalue,rl->transpose);
      forward_substitute2(sys,rl->varvalue,rl->transpose);
      /* we now have inv(L11)*inv(U11)*B1 stored in rl->varvalue */

      status=1;
   } else {
     if( NOTNULL(rhs) ) {
       ERROR_REPORTER_HERE(ASC_PROG_ERR,"Rhs not found on list.");
     } else {
       ERROR_REPORTER_HERE(ASC_PROG_ERR,"No rhs exist at all.");
     }
   }
   return 0;  /* Function had no return statement.  Added this line.  OK? */
}

real64 *linsolqr_get_varvalue(linsolqr_system_t sys,int n){
   struct rhs_list *rl;
   int count;

   CHECK_SYSTEM(sys);

   count = sys->rlength - 1 - n;
   if( count < 0 )  return(NULL);
   for( rl = sys->rl ; count > 0 && NOTNULL(rl) ; rl = rl->next )
      --count;
   return( ISNULL(rl) ? NULL : rl->varvalue );
}

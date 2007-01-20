/*
	SLV: Ascend Nonlinear Solver
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005 The ASCEND developers

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
	This file is part of the SLV solver.
*/

/** @file 
	General C  utility routines for slv class interfaces. Abstracted from
	slvX.c January 1995. Ben Allan.
*/

#include "slv_common.h"

#include <math.h>

#include <utilities/config.h>
#ifdef ASC_SIGNAL_TRAPS
# include <utilities/ascSignal.h>
#endif

#include <general/mathmacros.h>
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <utilities/mem.h>

/* if libasc.a running around, the following: */
#if SLV_INSTANCES
#include <compiler/compiler.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/functype.h>
#include <compiler/func.h>
#include "relman.h"
#include "logrelman.h"
#else
#ifdef NDEBUG
#define ascnint(a) (((int) (a)>=0.0 ? floor((a) + 0.5) : -floor(0.5 - (a))))
#else
#define ascnint(a) ascnintF(a)
#endif /* NDEBUG */
#endif /* instances */

#include "var.h"
#include "discrete.h"

#define SAFE_FIX_ME 0

/* vector math stuff moved to mtx_vector.c */

/**
 ***  General input/output routines
 ***  -----------------------------
 ***     fp = MIF(sys)
 ***     fp = LIF(sys)
 ***     slv_print_var_name(out,sys,var)
 ***     slv_print_rel_name(out,sys,rel)
 ***     slv_print_dis_name(out,sys,dvar)
 ***     slv_print_logrel_name(out,sys,lrel)
 ***  NOT yet implemented correctly:
 ***     slv_print_obj_name(out,obj)
 ***     slv_print_var_sindex(out,var)
 ***     slv_print_rel_sindex(out,rel)
 ***     slv_print_dis_sindex(out,dvar)
 ***     slv_print_logrel_sindex(out,lrel)
 ***     slv_print_obj_index(out,obj)
 **/

/**
 ***  Returns fp if fp!=NULL, or a file pointer
 ***  open to nul device if fp == NULL.
 **/
FILE *slv_get_output_file(FILE *fp)
{
   static FILE *nuldev = NULL;
#ifndef __WIN32__
   const char fname[] = "/dev/null";
#else
   const char fname[] = "nul";
#endif

   if( fp == NULL ) {
      if(nuldev == NULL)
         if( (nuldev = fopen(fname,"w")) == NULL ) {
            FPRINTF(stderr,"ERROR:  slv_get_output_file\n");
            FPRINTF(stderr,"        Unable to open %s.\n",fname);
         }
      fp = nuldev;
   }
   return(fp);
}
#define MIF(sys) slv_get_output_file( (sys)->p.output.more_important )
#define LIF(sys) slv_get_output_file( (sys)->p.output.less_important )

#if SLV_INSTANCES

void slv_print_var_name( FILE *out,slv_system_t sys, struct var_variable *var)
{
   char *name = NULL;
   if (out == NULL || sys == NULL || var == NULL) return;
   name = var_make_name(sys,var);
   if( *name == '?' ) FPRINTF(out,"%d",var_sindex(var));
   else FPRINTF(out,"%s",name);
   if (name) ascfree(name);
}

void slv_print_rel_name( FILE *out, slv_system_t sys, struct rel_relation *rel)
{
   char *name;
   name = rel_make_name(sys,rel);
   if( *name == '?' ) FPRINTF(out,"%d",rel_sindex(rel));
   else FPRINTF(out,"%s",name);
   ascfree(name);
}

void slv_print_dis_name( FILE *out, slv_system_t sys, struct dis_discrete *dvar)
{
   char *name=NULL;
   if (out == NULL || sys == NULL || dvar == NULL) return;
   name = dis_make_name(sys,dvar);
   if( *name == '?' ) FPRINTF(out,"%d",dis_sindex(dvar));
   else FPRINTF(out,"%s",name);
   if (name) ascfree(name);
}

void slv_print_logrel_name( FILE *out, slv_system_t sys,
                            struct logrel_relation *lrel)
{
   char *name;
   name = logrel_make_name(sys,lrel);
   if( *name == '?' ) FPRINTF(out,"%d",logrel_sindex(lrel));
   else FPRINTF(out,"%s",name);
   ascfree(name);
}

#ifdef NEWSTUFF
void slv_print_obj_name(FILE *out, obj_objective_t obj)
{
   char *name;
   name = obj_make_name(obj);
   if( *name == '?' ) FPRINTF(out,"%d",obj_index(obj));
   else FPRINTF(out,"%s",name);
   ascfree(name);
}
#endif

void slv_print_var_sindex(FILE *out, struct var_variable *var)
{
   FPRINTF(out,"%d",var_sindex(var));
}

void slv_print_rel_sindex(FILE *out, struct rel_relation *rel)
{
   FPRINTF(out,"%d",rel_sindex(rel));
}

void slv_print_dis_sindex(FILE *out, struct dis_discrete *dvar)
{
   FPRINTF(out,"%d",dis_sindex(dvar));
}

void slv_print_logrel_sindex(FILE *out, struct logrel_relation *lrel)
{
   FPRINTF(out,"%d",logrel_sindex(lrel));
}

#ifdef NEWSTUFF
void slv_print_obj_index(FILE *out, obj_objective_t obj)
{
   FPRINTF(out,"%d",obj_index(obj));
}
#endif

#define destroy_array(p)  \
   if( (p) != NULL ) ascfree((p))

/**
 ***  Attempt to directly solve the given relation (equality constraint) for
 ***  the given variable, leaving the others fixed.  Returns an integer
 ***  signifying the status as one of the following three:
 ***
 ***     0  ==>  Unable to determine anything, possibly due to fp exception.
 ***     1  ==>  Solution found, variable value set to this solution.
 ***    -1  ==>  No solution found.
 ***
 ***  The variable bounds will be upheld, unless they are to be ignored.
 **/
int slv_direct_solve(slv_system_t server, struct rel_relation *rel,
                     struct var_variable *var, FILE *fp,
                     real64 epsilon, int ignore_bounds, int scaled)
{
  int32 able, status;
  int nsolns, allsolns;
  real64 *slist, save;

  slist = relman_directly_solve_new(rel,var,&able,&nsolns,epsilon);
  if( !able ) {
    return(0);
  }
  allsolns = nsolns;
  while( --nsolns >= 0 ) {
    if( ignore_bounds ) {
      var_set_value(var,slist[nsolns]);
      break;
    }
    if( var_lower_bound(var) > slist[nsolns] ) {
      save = var_value(var);
      var_set_value(var,var_lower_bound(var));

#ifdef ASC_SIGNAL_TRAPS
      Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif

      (void)relman_eval(rel,&status,SAFE_FIX_ME);

#ifdef ASC_SIGNAL_TRAPS
      Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif

      if (scaled) {
        if( relman_calc_satisfied_scaled(rel,epsilon) ) break;
      } else {
        if( relman_calc_satisfied(rel,epsilon) ) break;
      }
      var_set_value(var,save);
    } else if( var_upper_bound(var) < slist[nsolns] ) {
      save = var_value(var);
      var_set_value(var,var_upper_bound(var));
#ifdef ASC_SIGNAL_TRAPS
      Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif
      (void)relman_eval(rel,&status,SAFE_FIX_ME);
#ifdef ASC_SIGNAL_TRAPS
      Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif
      if (scaled) {
        if( relman_calc_satisfied_scaled(rel,epsilon) ) break;
      } else {
        if( relman_calc_satisfied(rel,epsilon) ) break;
      }
      var_set_value(var,save);
    } else {
      var_set_value(var,slist[nsolns]);
#ifdef ASC_SIGNAL_TRAPS
      Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif
      (void)relman_eval(rel,&status,SAFE_FIX_ME);
#ifdef ASC_SIGNAL_TRAPS
      Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif
      break;
    }
  }
  if (nsolns<0 && allsolns>0 && fp !=NULL) {
   /* dump the rejected solutions to give the user a clue */
	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERROR);
    FPRINTF(ASCERR,"Solution(s) for '");
    var_write_name(server,var,ASCERR);
    FPRINTF(ASCERR,"' in equation '");
    rel_write_name(server,rel,ASCERR);
    FPRINTF(ASCERR,"' out of bounds.\n");
    for (--allsolns; allsolns >= 0; allsolns--)  {
      FPRINTF(ASCERR,"Rejected solution = %.18g\n",slist[allsolns]);
    }
	error_reporter_end_flush();
  }
  /* destroy_array(slist); do not do this */
  return( nsolns >= 0 ? 1 : -1 );
}


/**
 ***  Attempt to directly solve the given relation (equality constraint) for
 ***  the given variable, leaving the others fixed.  Returns an integer
 ***  signifying the status as one of the following three:
 ***
 ***     0  ==>  Unable to determine anything. Bad logrelation or dvar
 ***     1  ==>  Solution found, discrete variable value set to this solution.
 ***     2  ==>  More than one solution, Do not modify the current value
 ***             of the variable and display an error message.
 ***    -1  ==>  No solution found.
 **/
int slv_direct_log_solve(slv_system_t server, struct logrel_relation *lrel,
                         struct dis_discrete *dvar, FILE *fp, int perturb,
                         struct gl_list_t *insts)
{
  int32 able;
  int32 nsolns, c;
  int32 *slist;

  (void)fp;

  slist = logrelman_directly_solve(lrel,dvar,&able,&nsolns,perturb,insts);
  if( !able ) return(0);
  if(nsolns == -1) return (-1);

  if (nsolns == 1) {
    dis_set_boolean_value(dvar,slist[1]);
    logrel_set_residual(lrel,TRUE);
    destroy_array(slist);
    return 1;
  } else {
    ERROR_REPORTER_START_NOLINE(ASC_PROG_ERROR);
    FPRINTF(ASCERR,"Ignoring potential solutions for discrete variable '");
    dis_write_name(server,dvar,ASCERR);
    FPRINTF(ASCERR,"' in equation '");
    logrel_write_name(server,lrel,ASCERR);
    FPRINTF(ASCERR,"'. ");
    for (c = nsolns; c >= 1; c--)  {
      FPRINTF(ASCERR,"Rejected solution: %d \n",slist[c]);
    }
	error_reporter_end_flush();

    destroy_array(slist); /* should we have to do this? */
    return 2;
  }
}

#endif    /* SLV_INSTANCES */

int32 **slv_lnkmap_from_mtx(mtx_matrix_t mtx, mtx_region_t *clientregion)
{
  int32 **map, *data, rl, order;
  real64 val;
  mtx_coord_t coord;
  mtx_range_t range;
  mtx_region_t region;

  if (mtx == NULL) {
    FPRINTF(stderr,"Warning: slv_lnkmap_from_mtx called with null mtx\n");
    return NULL;
  }

  order = mtx_order(mtx);
  if (clientregion != NULL) {
    if ((clientregion->row.low < 0) ||
        (clientregion->row.high > (order-1)) ||
        (clientregion->col.low < 0) ||
        (clientregion->col.high > (order-1))) {
      FPRINTF(stderr,"Warning: slv_lnkmap_from_mtx called with null or invalid region\n");
      return NULL;
    }
    mtx_region(&region, clientregion->row.low, clientregion->row.high, 
                        clientregion->col.low, clientregion->col.high);
  } else {
    mtx_region(&region, 0, order-1, 0, order-1);
  }

  range.low = region.col.low;
  range.high = region.col.high;

  data = (int32 *)ascmalloc((order+2*mtx_nonzeros_in_region(mtx, &region))*sizeof(int32));
  if (NULL == data) {
    return NULL;
  }
  map = (int32 **)ascmalloc(order*sizeof(int32 *));
  if (NULL == map) {
    ascfree(data);
    return NULL;
  }
  for (coord.row=0 ; coord.row<region.row.low ; ++coord.row) {
    map[coord.row] = data;
    data[0] = 0;
    ++data;
  }
  for(coord.row=region.row.low ; coord.row <= region.row.high ; coord.row ++) {
    rl = mtx_nonzeros_in_row(mtx, coord.row, &range);
    map[coord.row] = data;
    data[0] = rl;
    data++;
    coord.col = mtx_FIRST;
    while( val = mtx_next_in_row(mtx, &coord, &range),
           coord.col != mtx_LAST) {
      data[0] = coord.col;
      data[1] = (int32)ascnint(val);
      data += 2;
    }
  }
  for (coord.row=(region.row.high+1) ; coord.row<order ; ++coord.row) {
    map[coord.row] = data;
    data[0] = 0;
    ++data;
  }
  return map;
}

int32 **slv_create_lnkmap(int m, int n, int len, int *hi, int *hj) 
{
  mtx_matrix_t mtx;
  mtx_coord_t coord;
  int32 i, **map;

  mtx = mtx_create();
  if (NULL == mtx) {
    FPRINTF(stderr,"Warning: slv_create_lnkmap called with null mtx\n");
    return NULL;
  }
  mtx_set_order(mtx,MAX(m,n));
  for (i=0 ; i<len ; i++) {
    if ((hi[i] >=  m) || (hj[i] >= n)) {
      FPRINTF(stderr,"Warning: index out of range in slv_create_lnkmap\n");
      mtx_destroy(mtx);
      return NULL;
    }
    coord.row = hi[i];
    coord.col = hj[i];
    mtx_fill_value(mtx,&coord,(real64)i);
  }
  map = slv_lnkmap_from_mtx(mtx,mtx_ENTIRE_MATRIX);
  mtx_destroy(mtx);
  return map;
}


void slv_destroy_lnkmap(int32 **map) 
{
  if (NOTNULL(map)) {
    ascfree(map[0]);
    ascfree(map);
  }
}

void slv_write_lnkmap(FILE *fp, int32 m, int32 **map) 
{
  int32 i,j,nv, *v;
  if (ISNULL(map)) {
    FPRINTF(stderr,"slv_write_lnkmap: Cannot write NULL lnkmap\n");
    return;
  }
  for(i=0; i<m; i++) {
    if (NOTNULL(map[i])) {
      v=map[i];
      nv=v[0];
      v++;
      FPRINTF(fp,"Row %d len = %d\n",i,nv);
      for (j=0;j<nv;j++) {
        FPRINTF(fp,"  Col %d, lnk location %d\n",v[0],v[1]);
        v+=2;
      }
    }
  }
}


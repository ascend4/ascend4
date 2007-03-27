/*	ASCEND modelling environment
	Copyright (C) 2006, 2007 Carnegie Mellon University

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
	Conditional Modeling Solver (CMSlv) module.
*//*
	Conditional Modeling Solver
	by Vicente Rico-Ramirez
	Last in CVS: $Revision: 1.22 $ $Date: 2000/01/25 02:27:58 $ $Author: ballan $
*/

#include "slv9.h"

#include <math.h>

#include <utilities/config.h>
#include <utilities/ascConfig.h>
#include <utilities/ascSignal.h>
#include <utilities/ascMalloc.h>
#include <general/tm_time.h>
#include <utilities/mem.h>
#include <general/list.h>
#include <general/mathmacros.h>

#include <linear/mtx_reorder.h>

#include <system/calc.h>
#include <system/relman.h>
#include <system/logrelman.h>
#include <system/bndman.h>
#include <system/slv_stdcalls.h>
#include <system/cond_config.h>

#include "slvDOF.h"

#ifdef ASC_WITH_CONOPT
# include "conopt.h"
#else
# define MAX_INT MAXINT
# define MAX_REAL MAXDOUBLE
# define CONOPT_BOUNDLIMIT 1e12
#endif

#if !defined(STATIC_CMSLV) && !defined(DYNAMIC_CMSLV)
int slv9_register(SlvFunctionsT *f){
  UNUSED_PARAMETER(f);

  ERROR_REPORTER_HERE(ASC_PROG_ERR,"CMSlv was not built in this copy of ASCEND");
  return 1;
}
#else /* either STATIC_CMSLV or DYNAMIC_CMSLV is defined */

#ifdef DYNAMIC_CMSLV
/* do dynamic loading stuff.   yeah, right */

#else /* - - - - following is used if STATIC_CMSLV is defined - - - - */

/*
 * definitions to enable/disable the output of partial results in
 * the solution of a problem
 */
#define DEBUG FALSE
#define SHOW_LOGICAL_DETAILS FALSE
#define SHOW_BOUNDARY_ANALYSIS_DETAILS FALSE
#define SHOW_OPTIMIZATION_DETAILS FALSE
#define SHOW_BISECTION_DETAILS FALSE
#define SHOW_LINEAR_SEARCH_DETAILS FALSE
#define SHOW_LAGRANGE_DETAILS FALSE
#define DEBUG_CONSISTENCY FALSE
#define TEST_CONSISTENCY FALSE
#define USE_CONSISTENCY FALSE

/*
 * system definitions
 */
#define SLV9(s) ((slv9_system_t)(s))
#define SERVER (sys->slv)
#define slv9_PA_SIZE 26 /* MUST INCREMENT WHEN ADDING PARAMETERS */
#define LOGSOLVER_OPTION_PTR (sys->parm_array[0])
#define LOGSOLVER_OPTION  ((*(char **)LOGSOLVER_OPTION_PTR))
#define NONLISOLVER_OPTION_PTR (sys->parm_array[1])
#define NONLISOLVER_OPTION  ((*(char **)NONLISOLVER_OPTION_PTR))
#define OPTSOLVER_OPTION_PTR (sys->parm_array[2])
#define OPTSOLVER_OPTION  ((*(char **)OPTSOLVER_OPTION_PTR))
#define TIME_LIMIT_PTR (sys->parm_array[3])
#define TIME_LIMIT     ((*(int32 *)TIME_LIMIT_PTR))
#define ITER_LIMIT_PTR (sys->parm_array[4])
#define ITER_LIMIT     ((*(int32 *)ITER_LIMIT_PTR))
#define ITER_BIS_LIMIT_PTR (sys->parm_array[5])
#define ITER_BIS_LIMIT  ((*(int32 *)ITER_BIS_LIMIT_PTR))
#define TOO_SMALL_PTR (sys->parm_array[6])
#define TOO_SMALL     ((*(real64 *)TOO_SMALL_PTR))
#define LINEAR_SEARCH_FACTOR_PTR (sys->parm_array[7])
#define LINEAR_SEARCH_FACTOR  ((*(real64 *)LINEAR_SEARCH_FACTOR_PTR))
#define SHOW_MORE_IMPT_PTR (sys->parm_array[8])
#define SHOW_MORE_IMPT     ((*(int32 *)SHOW_MORE_IMPT_PTR))
#define SHOW_LESS_IMPT_PTR (sys->parm_array[9])
#define SHOW_LESS_IMPT     ((*(int32 *)SHOW_LESS_IMPT_PTR))
#define AUTO_RESOLVE_PTR (sys->parm_array[10])
#define AUTO_RESOLVE     ((*(int32 *)AUTO_RESOLVE_PTR))
#define UNDEFINED_PTR (sys->parm_array[11])
#define UNDEFINED  ((*(real64 *)UNDEFINED_PTR))
#define DOMLIM_PTR (sys->parm_array[12])
#define DOMLIM     ((*(int32 *)DOMLIM_PTR))
#define OPT_ITER_LIMIT_PTR (sys->parm_array[13])
#define OPT_ITER_LIMIT     ((*(int32 *)OPT_ITER_LIMIT_PTR))
#define INFINITY_PTR (sys->parm_array[14])
#define ASC_INFINITY  ((*(real64 *)INFINITY_PTR))
#define OBJ_TOL_PTR (sys->parm_array[15])
#define OBJ_TOL  ((*(real64 *)OBJ_TOL_PTR))
#define RTMAXJ_PTR (sys->parm_array[16])
#define RTMAXJ     ((*(real64 *)RTMAXJ_PTR))
#define RHO_PTR (sys->parm_array[17])
#define RHO     ((*(real64 *)RHO_PTR))


/*
 * Client tokens of the different solvers: Conditional, Optimizer,
 * Nonlinear, Logical. We will switch from one client token to
 * another as the solution process occurs.
 */
#define NUMBER_OF_CLIENTS  4
SlvClientToken  token[NUMBER_OF_CLIENTS];
int32 solver_index[NUMBER_OF_CLIENTS];

/*
 * indeces in arrays token and solver_index
 */
#define CONDITIONAL_SOLVER  0
#define LOGICAL_SOLVER      1
#define NONLINEAR_SOLVER    2
#define OPTIMIZATION_SOLVER 3

/*
 * Do we have an optimization problem ?.  Global variable initialized
 * to 0 (not optimizing)
 */
static int32 g_optimizing = 0;

#if USE_CONSISTENCY
/*
 * number of subregion visited during the solution of the conditional
 * model.
 */
static int32 g_subregions_visited;

#endif /* USE_CONSISTENCY */


/* auxiliar structures */
struct boolean_values {
  int32            *pre_val;    /* previous values of dis_discrete */
  int32            *cur_val;    /* current values of dis_discrete  */
};

struct matching_cases {
  int32            *case_list;      /* list of cases */
  int32            ncases;          /* number of cases */
  int32            diff_subregion;  /* subregion ? */
};

struct real_values {
  real64           *pre_values;     /* previous values of var_variables */
  real64           *cur_values;     /* current values of var_variables */
};

struct opt_vector {
  real64 *element;                  /* elements in colum of matrix */
};

struct opt_matrix {
  struct opt_vector *cols;          /* columns in matrix */
};

struct subregionID {
  unsigned long    ID_number;
  int32            *bool_values;
};

struct ds_subregion_list {
   int32 length,capacity;
   struct subregionID *sub_stack;
};

struct ds_subregions_visited {
   int32 length,capacity;
   unsigned long *visited;
};

/*
 * This solver's data structure (CMSlv)
 */
struct slv9_system_structure {

  /*
   *  Problem definition
   */
  slv_system_t   	 slv;           /* slv_system_t back-link */

  struct rel_relation    *obj;          /* Objective function: NULL = none */
  struct var_variable    **vlist;       /* Variable list (NULL terminated) */
  struct rel_relation    **rlist;       /* Relation list (NULL terminated) */
  struct dis_discrete    **dvlist;      /* Dis vars list (NULL terminated) */
  struct logrel_relation **lrlist;      /* Logrels list(NULL terminated)*/
  struct bnd_boundary    **blist;       /* Variable list (NULL terminated) */
  struct var_variable    **mvlist;
  struct dis_discrete    **mdvlist;     /* We will not touch the masters list,
					 * but they can provide very useful
					 * information to the conditional
					 * solver since the master index does
					 * not change.
                                         */

  /*
   * for optimization at boundaries
   */
  struct opt_matrix      *coeff_matrix; /* Matrix for optimization problem */
  struct opt_vector      *opt_var_values; /* Values of vars in opt problem */
  int32                  subregions;    /* number of subregions at cur bnd */
  mtx_matrix_t           lin_mtx;       /* Matrix to define the linear system
					 * for calculation of the lagrange
					 * multipliers
					 */
  /*
   * For search consistency analysis
   */
   struct ds_subregion_list subregion_list;
                                        /*
					 * Information about the subregions
					 * visited during the solution of the
					 * conditional model
					 */
   struct ds_subregions_visited subregions_visited;
                                        /*
					 * ID number of the subregions
					 * visited
					 */
   int32            *bool_mindex;       /* master indices of boolean vars in
					 * the problem associated with WHENs
					 */
   int32 need_consistency_analysis;    /* Is the consistency analysis needed */


  /*
   *  Solver information
   */
  int32                  integrity;     /* Has the system been created ? */
  int32                  presolved;     /* Has the system been presolved ? */
  slv_parameters_t       p;             /* Parameters */
  slv_status_t           s;             /* Status (as of iteration end) */
  int32                  cap;           /* Order of matrix/vectors */
  int32                  rank;          /* Symbolic rank of problem */
  int32                  vused;         /* Free and incident variables */
  int32                  vtot;          /* length of varlist */
  int32                  mvtot;         /* length of master varlist */
  int32                  rused;         /* Included relations */
  int32                  rtot;          /* length of rellist */
  real64                 clock;         /* CPU time */
  int32                  nliter;        /* iterations in nonlinear solver */

  void *parm_array[slv9_PA_SIZE];     /* array of pointers to param values */
  struct slv_parameter pa[slv9_PA_SIZE]; /* &pa[0] => sys->p.parms */

#ifdef ASC_WITH_CONOPT
  /*
   *  Data for optimizer at boundaries (CONOPT)
   */
  struct conopt_data con;
#endif
};


/*
 *  Integrity checks
 *  ----------------
 *     check_system(sys)
 */

#define OK        ((int32)813029392)
#define DESTROYED ((int32)103289182)

/*
 *  Checks sys for NULL and for integrity.
 */
static
int check_system(slv9_system_t sys){
  if(sys == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) check_system\n");
    FPRINTF(ASCERR,"        NULL system handle.\n");
    return 1;
  }

  switch( sys->integrity ) {
  case OK:
    return 0;
  case DESTROYED:
    FPRINTF(ASCERR,"ERROR:  (slv9) check_system\n");
    FPRINTF(ASCERR,"        System was recently destroyed.\n");
    return 1;
  default:
    FPRINTF(ASCERR,"ERROR:  (slv9) check_system\n");
    FPRINTF(ASCERR,"        System reused or never allocated.\n");
    return 1;
  }
}


/*
 *  General input/output routines
 *  -----------------------------
 *     print_var_name(out,sys,var)
 */
#define print_var_name(a,b,c) slv_print_var_name((a),(b)->slv,(c))


/*
 *  Array operations
 *  ----------------
 *     destroy_array(p)
 *     create_array(len,type)
 *     create_zero_array(len,type)
 */
#define destroy_array(p)  \
   if((p) != NULL ) ascfree((p))
#define create_array(len,type)  \
   ((len) > 0 ? (type *)ascmalloc((len)*sizeof(type)) : NULL)
#define create_zero_array(len,type)  \
   ((len) > 0 ? (type *)asccalloc((len),sizeof(type)) : NULL)


/*
 *  Search Consistency Analysis during iterative process
 *  ---------------------------------------------------------
 *  The caller of this functions is in charge of
 *  defining the extension of the analysis by passing an integer
 *  which will tell us if 1) the analysis consider only the current
 *  and the previous alternatives or 2) the analysis consider all the
 *  alternatives visited at current the state of the solution
 *  algorithm.
 */

  /*
   * Handling dynamic allocation of the structural information
   */

#define alloc_array(nelts,type)   \
   ((nelts) > 0 ? (type *)ascmalloc((nelts)*sizeof(type)) : NULL)
#define copy_nums(from,too,nnums)  \
   asc_memcpy((from),(too),(nnums)*sizeof(int32))
#define copy_subregions(from,too,nsubs)  \
   asc_memcpy((from),(too),(nsubs)*sizeof(struct subregionID))

#if TEST_CONSISTENCY
/*
 *  Appends the subregion_visited into the list
 */
static
void append_subregion(struct ds_subregion_list *sl,
		struct subregionID sub
){
  if(sl->length == sl->capacity ) {
    int32 newcap;
    struct subregionID *newlist;

    newcap = sl->capacity + 10;
    newlist = alloc_array(newcap,struct subregionID);
    copy_subregions((char *)sl->sub_stack,(char *)newlist,sl->length);
    if(sl->sub_stack != NULL ) {
      ascfree(sl->sub_stack);
    }
    sl->sub_stack = newlist;
    sl->capacity = newcap;
  }

  sl->sub_stack[sl->length++] = sub;
}

/*
 *  Appends the subregion_visited into the list
 */
static
void append_sub_visited(struct ds_subregions_visited *sv,
		unsigned long sub_visited
){
  if(sv->length == sv->capacity ) {
    int32 newcap;
    unsigned long *newlist;

    newcap = sv->capacity + 10;
    newlist = alloc_array(newcap,unsigned long);
    copy_nums((char *)sv->visited,(char *)newlist,sv->length);
    if(sv->visited != NULL ) {
      ascfree(sv->visited);
    }
    sv->visited = newlist;
    sv->capacity = newcap;
  }

  sv->visited[sv->length++] = sub_visited;
}

static
unsigned long powoftwo (int32 expo){
  unsigned long val;
  int32 c;

  val = 1;
  for (c=1; c<= expo; c++) {
    val = val * 2;
  }

  return val;
}


/*
 * Storage information (boolean values) about a subregion so that
 * we can visit it later for interactive strucutral analysis
 */
static
void ID_and_storage_subregion_information(slv_system_t server,
		SlvClientToken asys
){
  slv9_system_t sys;
  struct dis_discrete **bvlist;
  struct dis_discrete *cur_dis;
  struct ds_subregion_list *sl;
  struct ds_subregions_visited *sv;
  struct subregionID *sub;
  dis_filter_t dfilter;
  unsigned long val, visited, sID;
  int32 d, numdvs, numdvf, dcount;
  int32 len, s, found;

  sys = SLV9(asys);
  check_system(sys);

  bvlist = sys->mdvlist;
  if(bvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR: ID_and_storage_subregion_information.\n");
    FPRINTF(ASCERR,"        Master discrete var list was never set.\n");
    return;
  }
  numdvs = slv_get_num_master_dvars(server);

  dfilter.matchbits = (DIS_INWHEN | DIS_BOOLEAN);
  dfilter.matchvalue = (DIS_INWHEN | DIS_BOOLEAN);
  numdvf = slv_count_master_dvars(server,&dfilter);

  if(numdvf > 0) {
    sub = (struct subregionID *)(ascmalloc(sizeof(struct subregionID)));
    sub->bool_values = (int32 *)(ascmalloc(numdvf*sizeof(int32)));
  }else{
    FPRINTF(ASCERR,"ERROR: ID_and_storage_subregion_information.\n");
    FPRINTF(ASCERR,"       No boolean variables in the problem \n");
    return;
  }

  dcount = 0;
  val = 0;
  for (d=0; d<numdvs; d++) {
    cur_dis = bvlist[d];
    if(dis_apply_filter(cur_dis,&dfilter)) {
      sub->bool_values[dcount] = dis_value(cur_dis);
      dcount++;
      if(sub->bool_values[dcount - 1] == 1) {
        val = val + powoftwo(numdvf - dcount);
      }
    }
  }
  if((val == 0 ) && (numdvf > 0) ) {
    val = powoftwo(numdvf);
  }
  sub->ID_number = val;
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"ID of alternative is %ul \n", val);
#endif /* DEBUG_CONSISTENCY */
  visited = val;
  found = 0;
  len = sys->subregions_visited.length;
  if(len > 0) {
    for (s=0; s<len; s++) {
      sID = sys->subregions_visited.visited[s];
      if(visited == sID) {
        found = 1;
        break;
      }
    }
  }

  sv = &(sys->subregions_visited);
  append_sub_visited(sv,visited);

  if(found == 0) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"Saving alternative\n");
#endif /* DEBUG_CONSISTENCY */
    sl = &(sys->subregion_list);
    append_subregion(sl,(*sub));
  }else{
    destroy_array(sub->bool_values);
    ascfree(sub);
  }

  return;
}
#endif

/*
 * Destroys subregion information
 */
static
void destroy_subregion_information(SlvClientToken asys){
  slv9_system_t sys;
  struct subregionID *sub;
  int32 lens, s;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->subregions_visited.visited != NULL) {
    destroy_array(sys->subregions_visited.visited);
  }

  lens = sys->subregion_list.length;
  if(lens != 0) {
    for (s=0; s<lens; s++) {
      sub = &(sys->subregion_list.sub_stack[s]);
      if(sub->bool_values != NULL) {
        destroy_array(sub->bool_values);
      }
    }
  }

  if(sys->subregion_list.sub_stack != NULL) {
    destroy_array(sys->subregion_list.sub_stack);
  }

  if(sys->bool_mindex != NULL) {
    destroy_array(sys->bool_mindex);
  }
}


#if 0 /** unused function eligible_set_for_neighboring_subregions */
/* might be used if DEBUG_CONSISTENCY on */

/*
 * Storing original values of boolean variables
 */
static void store_original_bool_values(struct gl_list_t *bollist,
			                 struct boolean_values *bval)
{
  struct dis_discrete *dvar;
  int32 d, dlen;

  dlen = gl_length(bollist);
  bval->pre_val = create_array(dlen,int32);
  bval->cur_val = create_array(dlen,int32);
  for (d=1; d<=dlen; d++){
    dvar = (struct dis_discrete *)gl_fetch(bollist,d);
    bval->cur_val[d-1] = dis_value(dvar);
    bval->pre_val[d-1] = dis_previous_value(dvar);
  }
}

/*
 * Restoring original values of boolean variables
 */
static void restore_original_bool_values(struct gl_list_t *bollist,
			                   struct boolean_values *bval)
{
  struct dis_discrete *dvar;
  int32 d, dlen;

  dlen = gl_length(bollist);
  for (d=1; d<=dlen; d++){
    dvar = (struct dis_discrete *)gl_fetch(bollist,d);
    dis_set_boolean_value(dvar,bval->cur_val[d-1]);
    dis_set_value(dvar,bval->cur_val[d-1]);
    dis_set_previous_value(dvar,bval->pre_val[d-1]);
  }
  destroy_array(bval->cur_val);
  destroy_array(bval->pre_val);
}

#endif /* if 0 */

/*
 * the first element of cur_cases is in position one. The result is
 * the same array, but ordered and starting in position zero
 */
static
void cases_reorder(int32 *cur_cases, int32 *correct_cases, int32 ncases){
  int32 cur_case,pos=0,tmp_num,c,ind;

  for (c=1; c<=ncases; c++) {
    tmp_num = 0;
    for (ind=1; ind<=ncases; ind++) {
      cur_case = cur_cases[ind];
      if(tmp_num < cur_case) {
        pos = ind;
        tmp_num  = cur_case;
      }
    }
    cur_cases[pos] = 0;
    correct_cases[ncases-c] = tmp_num;
  }

  return;
}

#if 0 /** unused function eligible_set_for_neighboring_subregions */
/* might appear if debug_consistency is true. */
/*
 * Restoring orignal configuration of the system
 */
static void restore_configuration(slv_system_t server,
				  struct gl_list_t *bollist)

{
  int32 *cur_cases, *correct_cases;
  int32 ncases;

   cur_cases = cases_matching(bollist,&ncases);
   correct_cases = create_array(ncases,int32);
   cases_reorder(cur_cases,correct_cases,ncases);
   set_active_rels_in_subregion(server,correct_cases,ncases,bollist);
   set_active_vars_in_subregion(server);
   destroy_array(cur_cases);
   destroy_array(correct_cases);
}


/*
 * Get the list of boolean variables in the problem which are
 * associated with a WHEN
 */
static struct gl_list_t *get_list_of_booleans(slv_system_t server,
					      SlvClientToken asys)
{
  slv9_system_t sys;
  struct dis_discrete **bvlist;
  struct dis_discrete *cur_dis;
  struct gl_list_t *boolvars;
  dis_filter_t dfilter;
  int32 numdvf, numdvs, d, dcount;

  sys = SLV9(asys);
  check_system(sys);

  bvlist = sys->mdvlist;
  if(bvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) get_list_of_booleans.\n");
    FPRINTF(ASCERR,"        Master discrete var list was never set.\n");
    return NULL;
  }
  numdvs = slv_get_num_master_dvars(server);

  dfilter.matchbits = (DIS_INWHEN | DIS_BOOLEAN);
  dfilter.matchvalue = (DIS_INWHEN | DIS_BOOLEAN);
  numdvf = slv_count_master_dvars(server,&dfilter);

  if(numdvf == 0) {
    FPRINTF(ASCERR,"ERROR: (slv9) get_list_of_booleans.\n");
    FPRINTF(ASCERR,"       No boolean variables in the problem \n");
    return NULL;
  }

  sys->bool_mindex = (int32 *)(ascmalloc(numdvf*sizeof(int32)));
  boolvars = gl_create(numdvf);

  dcount = 0;
  for (d=0; d<numdvs; d++) {
    cur_dis = bvlist[d];
    if(dis_apply_filter(cur_dis,&(dfilter))) {
      gl_append_ptr(boolvars,cur_dis);
      sys->bool_mindex[dcount] = d;
      dcount++;
    }
  }

  return boolvars;
}

#endif /* 0*/

/*
 * Get the eligible var list for each alternative
 * Return:
 * 1 means everything went right
 * 0 means the analysis has failed with the current parititioning
 * -1 means a memory problem has occurred
 */
static
int32 get_eligible_set(slv_system_t server,struct gl_list_t *disvars,
		int32 *terminate
){
  struct var_variable **vslist;
  struct var_variable **vmlist;
  struct var_variable *mvar, *svar;
  var_filter_t vfilter;
  int32 *cur_cases;
  int32 *correct_cases;
  int32 *vars;
  int32 v, count=0, ind;
  int32 ncases;
  int32 mnum;
  int32 status,dof;

  vslist = slv_get_solvers_var_list(server);
  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    var_set_eligible_in_subregion(mvar,FALSE);
  }

  cur_cases = cases_matching(disvars,&ncases);
  correct_cases = create_array(ncases,int32);
  cases_reorder(cur_cases,correct_cases,ncases);
  set_active_rels_in_subregion(server,correct_cases,ncases,disvars);
  set_active_vars_in_subregion(server);
  destroy_array(cur_cases);
  destroy_array(correct_cases);

#if DEBUG_CONSISTENCY
  FPRINTF(ASCERR,"Analyzing alternative:\n");
#endif /* DEBUG_CONSISTENCY */

  if(!slvDOF_status(server,(&status),(&dof))) {
   FPRINTF(ASCERR,"ERROR in combinatorial search\n");
   FPRINTF(ASCERR,"Combinatorial search aborted\n");
   return -1;
  }else{
    if(status == 3) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"Alternative is structurally singular\n");
#endif /* DEBUG_CONSISTENCY */
      (*terminate) = 0;
      return 0;
    }else{
      if(status == 4) {
#if DEBUG_CONSISTENCY
         FPRINTF(ASCERR,"Alternative is overspecified\n");
#endif /* DEBUG_CONSISTENCY */
         (*terminate) = 0;
         return 0;
      }
    }
  }

  if(status == 1) {
    (*terminate) = 0;
#if DEBUG_CONSISTENCY
    FPRINTF(ASCERR,"Alternative has % d degrees of freedom.\n", dof);

#endif /* DEBUG_CONSISTENCY */
    if(slvDOF_eligible(server,&(vars))) {
      count = 0;
      while (vars[count] != -1) {
        ind = vars[count];
        svar = vslist[ind];
        v = var_mindex(svar);
        mvar = vmlist[v];
        var_set_eligible_in_subregion(mvar,TRUE);
        count++;
      }
      destroy_array(vars);
    }
    if(dof > count) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,
              "Alternative does not have enough number of eligible vars\n");
#endif /* DEBUG_CONSISTENCY */
      return 0;
    }
  }

  if(status == 2) {
#if DEBUG_CONSISTENCY
    FPRINTF(ASCERR,"Alternative is square.\n");
#endif /* DEBUG_CONSISTENCY */
  }

  vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR
		       | VAR_ELIGIBLE_IN_SUBREGION);
  vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);

  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    if(var_apply_filter(mvar,&vfilter)) {
      var_set_eligible(mvar,FALSE);
    }
    var_set_eligible_in_subregion(mvar,FALSE);
  }

  return 1;
}

/*
 * Get the eligible set of variables for each of the alternatives generated
 * by modifying the values of the boolean variables with the values stored
 * during the solution process
 * Return:
 * 1 means everything went right
 * 0 means the analysis has failed with the current partitioning
 * -1 means a memory problem or wierdness has occurred
 */
static
int32 do_search_alternatives(slv_system_t server, SlvClientToken asys,
		struct gl_list_t *disvars,
		int32 *terminate, int32 all_sub
){
  slv9_system_t sys;
  struct dis_discrete *cur_dis;
  struct subregionID *sub;
  int32 *values = NULL;
  int32 dlen, test;
  int32 lens, lenv, v, s, d;
  int32 result;
  unsigned long visited, vID;


  sys = SLV9(asys);

  dlen = gl_length(disvars);
  lenv = sys->subregions_visited.length;
  lens = sys->subregion_list.length;

  if(all_sub == 0) { /* current and previous subregion */
    for (v=lenv-2; v<lenv; v++) {
      test = 0;
      vID = sys->subregions_visited.visited[v];
      for (s=lens-1; s>=0; s--) {
        sub = &(sys->subregion_list.sub_stack[s]);
        visited = sub->ID_number;
        if(vID == visited) {
          values = sub->bool_values;
          test = 1;
          FPRINTF(ASCERR,"s = %d \n",s);
          break;
	}
      }

      if(test == 0) {
        FPRINTF(ASCERR,"ERROR:  (slv9) do_search_alternatives \n");
        FPRINTF(ASCERR,"         subregion not found \n");
        return -1;
      }
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"Alternative = %ul \n", vID);
#endif /* DEBUG_CONSISTENCY */
      for (d=0; d<dlen; d++) {
	assert(values != NULL); /* if null, test was 0 above and we returned, in theory */
        cur_dis = (struct dis_discrete *)(gl_fetch(disvars,d+1));
	 if(values[d] == 1) {
           dis_set_boolean_value(cur_dis,TRUE);
	 }else{
           dis_set_boolean_value(cur_dis,FALSE);
	 }
      }
      result = get_eligible_set(server,disvars,terminate);
      if(result != 1) {
        return result;
      }
    }

  }else{ /* all visited subregions */

    for (s=lens-1; s>=0; s--) {
      sub = &(sys->subregion_list.sub_stack[s]);
      values = sub->bool_values;
      vID = sub->ID_number;
#if DEBUG_CONSISTENCY
    FPRINTF(ASCERR,"Alternative = %ul \n", vID);
#endif /* DEBUG_CONSISTENCY */
      for (d=0; d<dlen; d++) {
        cur_dis = (struct dis_discrete *)(gl_fetch(disvars,d+1));
        if(values[d] == 1) {
          dis_set_boolean_value(cur_dis,TRUE);
	}else{
          dis_set_boolean_value(cur_dis,FALSE);
	}
      }
      result = get_eligible_set(server,disvars,terminate);
      if(result != 1) {
        return result;
      }
    }
  }

  return 1;
}


/*
 * Perform consistency analysis for the visited/current-previous subregions.
 * If all_subs is 1, the analysis takes in account all of the subregions
 * visited by the solution algorithm at the current point if the solution
 * procedure. If all_subs is 0, the analysis is only for the current
 * and previous subregion.
 */
static
int32 consistency(slv_system_t server, SlvClientToken asys,
		struct gl_list_t *bollist,
		int32 all_subs, int32 *terminate
){
  slv9_system_t sys;
  struct var_variable **vmlist;
  struct var_variable *mvar;
  var_filter_t vfilter;
  int32 *globeli = NULL;
  int32 dlen;
  int32 mnum, v, elnum;
  int32 result;
  int32 iter;

  sys = SLV9(asys);
  check_system(sys);

  /*
   * Initializing eligible bit for variables
   */
  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    var_set_eligible(mvar,TRUE);
  }

  dlen = gl_length(bollist);

#if DEBUG_CONSISTENCY
        FPRINTF(ASCERR,"S e a r c h i n g \n");
#endif /* DEBUG_CONSISTENCY */
  result = do_search_alternatives(server,asys,bollist,terminate,all_subs);

  if(result != 1) {
#if DEBUG_CONSISTENCY
    FPRINTF(ASCERR,"returning failed search after S e a r c h \n");
#endif /* DEBUG_CONSISTENCY */
    return result;
  }

  /*
   * Getting the "globally" eligible variables
   */
  vfilter.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE | VAR_FIXED);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE);
  elnum = slv_count_master_vars(server,&vfilter);

  if(elnum > 0) {
    globeli = ASC_NEW_ARRAY(int32,elnum);
    elnum = 0;
    for (v=0; v<mnum; v++) {
      mvar = vmlist[v];
      if(var_apply_filter(mvar,&vfilter)) {
#if DEBUG_CONSISTENCY
        FPRINTF(ASCERR,"Eligible index = %d \n",v);
#endif /* DEBUG_CONSISTENCY */
        globeli[elnum] = v;
        elnum++;
      }
    }
  }

  /*
   * Recursively analysis
   */

  if((*terminate) == 1) {
    if(elnum != 0) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"All alternatives are square but the \n");
      FPRINTF(ASCERR,"Eligible set is not null\n");
#endif /* DEBUG_CONSISTENCY */
      destroy_array(globeli);
    }
    return 1;
  }else{
    if(elnum == 0) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"No globally eligible variables to be fixed.\n");
#endif /* DEBUG_CONSISTENCY */
      return 0;
    }

    for (v=0; v<elnum; v++) {
      iter = 1;
      mvar = vmlist[globeli[v]];
      var_set_fixed(mvar,TRUE);
      var_set_potentially_fixed(mvar,TRUE);
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"Fixing index = %d \n",globeli[v]);
      FPRINTF(ASCERR,"N e s t e d   S e a r c h \n");
#endif /* DEBUG_CONSISTENCY */
      result = consistency(server,asys,bollist,all_subs,&iter);

      if(result != 1) {
#if DEBUG_CONSISTENCY
        FPRINTF(ASCERR,"%d eliminated\n",globeli[v]);
#endif /* DEBUG_CONSISTENCY */
        var_set_fixed(mvar,FALSE);
        var_set_potentially_fixed(mvar,FALSE);
        continue;
      }else{
        if(iter == 1) {
          (*terminate) = 1;
#if DEBUG_CONSISTENCY
          FPRINTF(ASCERR,"%d Acepted \n",globeli[v]);
#endif /* DEBUG_CONSISTENCY */
          destroy_array(globeli);
          return 1;
        }else{
          var_set_fixed(mvar,FALSE);
          var_set_potentially_fixed(mvar,FALSE);
          continue;
        }
      }
    }
    destroy_array(globeli);
#if DEBUG_CONSISTENCY
    FPRINTF(ASCERR,"returning 0 after nested search\n");
#endif /* DEBUG_CONSISTENCY */
    return 0;
  }
}

#if 0 /** unused function eligible_set_for_neighboring_subregions */
/* might appear if debug_consistency is true. */

/*
 * Get a set of globally eligible variables. Eligible for all the subregions
 * visited, or for the previous and current subregions.
 */
static int32 get_globally_eligible(slv_system_t server, SlvClientToken asys,
			           struct gl_list_t *bollist,
			           int32 all_subs, int32 **eliset)
{
  slv9_system_t sys;
  struct var_variable **vmlist;
  struct var_variable *mvar;
  var_filter_t vfilter;
  int32 dlen;
  int32 mnum, v, elnum;
  int32 terminate;
  int32 result;

  sys = SLV9(asys);
  check_system(sys);
  /*
   * Initializing eligible bit for variables
   */
  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    var_set_eligible(mvar,TRUE);
  }

  dlen = gl_length(bollist);

  /*
   * initializing
   */
  *eliset = NULL;
  terminate = 1;

#if DEBUG_CONSISTENCY
        FPRINTF(ASCERR,"S e a r c h i n g \n");
#endif /* DEBUG_CONSISTENCY */
  result = do_search_alternatives(server,asys,bollist,&terminate,all_subs);

  if(result != 1) {
    if(terminate == 0) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"ERROR: some alternatives are either singular or\n");
      FPRINTF(ASCERR,"overspecified. All the alternatives have to be\n");
      FPRINTF(ASCERR,
	      "either square or underspecified to complete the analysis\n");
#endif /* DEBUG_CONSISTENCY */
    }
    return 0;
  }

  /*
   * Getting the "globally" eligible variables
   */
  vfilter.matchbits = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE | VAR_FIXED);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ELIGIBLE);
  elnum = slv_count_master_vars(server,&vfilter);

  *eliset = (int32 *)ascmalloc((elnum+1)*sizeof(int32));
  elnum = 0;
  for (v=0; v<mnum; v++) {
    mvar = vmlist[v];
    if(var_apply_filter(mvar,&vfilter)) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"Eligible index = %d \n",v);
      FPRINTF(ASCERR,"Variable : \n");
      print_var_name(ASCERR,sys,mvar);
#endif /* DEBUG_CONSISTENCY */
      (*eliset)[elnum] = v;
      elnum++;
    }
  }
  (*eliset)[elnum] = -1;

  if(elnum == 0) {
    if(terminate == 0) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,
	      "Some alternatives are underspecified, but there does\n");
      FPRINTF(ASCERR,"not exist a set of eligible variables consistent \n");
      FPRINTF(ASCERR,"with all the alternatives\n");
#endif /* DEBUG_CONSISTENCY */
    }else{
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"All alternatives are already square\n");
#endif /* DEBUG_CONSISTENCY */
    }
    return 0;
  }else{
    if(terminate == 1) {
#if DEBUG_CONSISTENCY
      FPRINTF(ASCERR,"All alternatives are square but the \n");
      FPRINTF(ASCERR,"Eligible set is not null\n");
#endif /* DEBUG_CONSISTENCY */
    }
  }
  return 1;
}



/*
 * Store and restore values of the boolean variables of the problem
 * and calls for the the set of globally eligible variables.If all_subs
 * is 1, the analysis takes in account all of the subregions visited
 * by the solution algorithm at the current point if the solution
 * procedure. If all_subs is 0, the analysis is only for the current
 * and previous subregion.
 */
static
int32 consistent_eligible_set_for_subregions(slv_system_t server,
		SlvClientToken asys,
		int32 **vlist,
		int32 all_subs
){
  struct gl_list_t *blist;
  struct boolean_values bval;
  int32 result;

  if(server==NULL || vlist == NULL) {
    FPRINTF(ASCERR,
	    "consistent_eligible_set_for_subregions called with NULL.\n");
    return 0;
  }

  blist = get_list_of_booleans(server,asys);

  if((blist == NULL) || (gl_length(blist) == 0) ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) consistent_eligible_set_for_subregions \n");
    FPRINTF(ASCERR,"        List of boolean vars could not be found\n");
    return 0;
  }

  store_original_bool_values(blist,&(bval));
  result = get_globally_eligible(server,asys,blist,all_subs,vlist);

  restore_original_bool_values(blist,&(bval));
  restore_configuration(server,blist);
  gl_destroy(blist);

  if(result == 1) {
    return 1;
  }else{
    return 0;
  }

}

/*
 * Store and restore values of the boolean variables of the problem
 * and calls for the consistency analysis of the subregions.If all_subs
 * is 1, the analysis takes in account all of the subregions visited
 * by the solution algorithm at the current point if the solution
 * procedure. If all_subs is 0, the analysis is only for the current
 * and previous subregion.
 */
static
int32 analyze_subregions(slv_system_t server,SlvClientToken asys,
		int32 **vlist, int32 all_subs
){
  slv9_system_t sys;
  struct var_variable ** vmlist;
  struct var_variable *mvar;
  struct gl_list_t *blist;
  struct boolean_values bval;
  var_filter_t vfilter;
  int32 mnum, elnum, v;
  int32 result;
  int32 terminate;

  sys = SLV9(asys);
  check_system(sys);

  if(server==NULL || vlist == NULL) {
    FPRINTF(ASCERR,"(slv9) analyze_subregions called with NULL.\n");
    return 0;
  }

  blist = get_list_of_booleans(server,asys);
  if((blist == NULL) || (gl_length(blist) == 0) ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) analyze_subregions \n");
    FPRINTF(ASCERR,"        List of boolean vars could not be found\n");
    return 0;
  }

  store_original_bool_values(blist,&(bval));
 /*
  * initializing
  */
  terminate = 1;
  (*vlist) = NULL;

  vmlist = slv_get_master_var_list(server);
  mnum = slv_get_num_master_vars(server);

  vfilter.matchbits = (VAR_POTENTIALLY_FIXED);
  vfilter.matchvalue = (VAR_POTENTIALLY_FIXED);

  result = consistency(server,asys,blist,all_subs,&terminate);

  if(result == 1) {
  /*
   * Getting the set of eligible variables
   */
    elnum = slv_count_master_vars(server,&vfilter);
    *vlist = (int32 *)ascmalloc((elnum+1)*sizeof(int32));
    elnum = 0;
    for (v=0; v<mnum; v++) {
      mvar = vmlist[v];
      if(var_apply_filter(mvar,&vfilter)) {
        var_set_fixed(mvar,FALSE);
        var_set_potentially_fixed(mvar,FALSE);
#if DEBUG_CONSISTENCY
        FPRINTF(ASCERR,"Variable in consistent set: \n");
        print_var_name(ASCERR,sys,mvar);
#endif /* DEBUG_CONSISTENCY */
        (*vlist)[elnum] = v;
        elnum++;
      }
    }
    (*vlist)[elnum] = -1;

    restore_original_bool_values(blist,&(bval));
    restore_configuration(server,blist);
    gl_destroy(blist);
    return 1;
  }else{
    for (v=0; v<mnum; v++) {
      mvar = vmlist[v];
      if(var_apply_filter(mvar,&vfilter)) {
        var_set_fixed(mvar,FALSE);
        var_set_potentially_fixed(mvar,FALSE);
      }
    }
    restore_original_bool_values(blist,&(bval));
    restore_configuration(server,blist);
    gl_destroy(blist);
    return 0;
  }
}


/*
 * Finds the globally eligible set of variables only for the current and
 * previous subregions
 */
static
int32 eligible_set_for_neighboring_subregions(slv_system_t server,
		SlvClientToken asys,
		int32 **vlist
){
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->mdvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) eligible_set_for_neighboring_subregions\n");
    FPRINTF(ASCERR,"        Discrete Variable list was never set.\n");
    return 0;
  }

  if(!(sys->need_consistency_analysis)) {
    FPRINTF(ASCERR,"Globally eligible set not necessary\n");
    FPRINTF(ASCERR,"All the subregions have the same structure \n");
    return 0;
  }

  if(consistent_eligible_set_for_subregions(server,asys,vlist,0)) {
    return 1;
  }

  return 0;
}


/*
 * Perform the consistency analysis algorithm only for the current and
 * previous subregions
 */
static
int32 consistency_for_neighboring_subregions(slv_system_t server,
		SlvClientToken asys,
		int32 **vlist
){
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->mdvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) consistency_for_neighboring_subregions\n");
    FPRINTF(ASCERR,"        Discrete Variable list was never set.\n");
    return 0;
  }

  if(!(sys->need_consistency_analysis)) {
    FPRINTF(ASCERR,"consistency_analysis is not required\n");
    FPRINTF(ASCERR,"All the subregions have the same structure \n");
    return 0;
  }

  if(analyze_subregions(server,asys,vlist,0)) {
    return 1;
  }

  return 0;
}



/*
 * Consistency analysis for visisted subregions. This function
 * gets the subregions that the solution algorithm has visited and
 * analyzes them.
 */
static
int32 eligible_set_for_subregions(slv_system_t server,
		SlvClientToken asys,
		int32 **vlist
){
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->mdvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) eligible_set_for_subregions \n");
    FPRINTF(ASCERR,"        Discrete Variable list was never set.\n");
    return 0;
  }

  if(!(sys->need_consistency_analysis)) {
    FPRINTF(ASCERR,"Globally eligible set not necessary \n");
    FPRINTF(ASCERR,"All the subregions have the same structure \n");
    return 0;
  }

  if(consistent_eligible_set_for_subregions(server,asys,vlist,1)) {
    return 1;
  }

  return 0;
}


/*
 * Consistency analysis for visisted subregions. This function
 * gets the subregions that the solution algorithm has visited and
 * analyzes them.
 */
static
int32 consistency_analysis_for_subregions(slv_system_t server,
		SlvClientToken asys,
		int32 **vlist
){
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->mdvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) consistency_analysis_for_subregions\n");
    FPRINTF(ASCERR,"        Discrete Variable list was never set.\n");
    return 0;
  }

  if(!(sys->need_consistency_analysis)) {
    FPRINTF(ASCERR,"consistency_analysis is not required\n");
    FPRINTF(ASCERR,"All the subregions have the same structure \n");
    return 0;
  }

  if(analyze_subregions(server,asys,vlist,1)) {
    return 1;
  }

  return 0;
}

#endif /*#if 0 unused functions */


/*
 *  Handling of solution of the Logical Equations
 *  ---------------------------------------------------------
 *  This is made this way because it is a process which will be
 *  required very often.
 */

/*
 * Solution of the logical relations encountered in the system based on
 * the current values of the discrete variables.
 */
static
void solve_logical_relations(slv_system_t server){
  slv_set_client_token(server,token[LOGICAL_SOLVER]);
  slv_set_solver_index(server,solver_index[LOGICAL_SOLVER]);
  slv_presolve(server);
#if SHOW_LOGICAL_DETAILS
  FPRINTF(ASCERR,"Solving Logical Relations\n");
#endif /* SHOW_LOGICAL_DETAILS  */
  slv_solve(server);
}



/*
 * Handling the modification of parameters in external solvers
 * ---------------------------------------------------------
 */

/*
 * different types of parameter values
 */
union param_value {
  int32 i;
  real64 r;
  int32 b;
  char *c;
};


/*
 * Setting the value of a parameter in a subsidiary solver
 */
static
void set_param_in_solver(slv_system_t server, int32 solver,
		enum parm_type types, char *param,
		union param_value *value
){
  slv_parameters_t p;
  int32 len,length;

  slv_set_client_token(server,token[solver]);
  slv_set_solver_index(server,solver_index[solver]);
  slv_get_parameters(server,&p);
  length = p.num_parms;
  for (len = 0; len < length; len++) {
    if(p.parms[len].type == types) {
      switch (p.parms[len].type) {
        case bool_parm:
          if(strcmp(param,p.parms[len].name) == 0) {
            p.parms[len].info.b.value = value->b;
          }
          break;
        case real_parm:
          if(strcmp(param,p.parms[len].name) == 0) {
            p.parms[len].info.r.value = value->r;
          }
          break;
        case char_parm:
          if(strcmp(param,p.parms[len].name) == 0) {
            p.parms[len].info.c.value = value->c;
          }
          break;
        case int_parm:
          if(strcmp(param,p.parms[len].name) == 0) {
            p.parms[len].info.i.value = value->i;
          }
          break;
        default:
          break;
      }
    }
  }
  return;
}


/*
 *  Analysis of Discrete Variables
 *  -------------------------------
 */

/*
 * Compare current values of the discrete variables with their previous values
 * in order to know if some of them have changed.
 */
static
int32 some_dis_vars_changed(slv_system_t server, SlvClientToken asys){
  struct dis_discrete **dv, *cur_dis;
  int32 numdvs, ind;
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->dvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) some_dis_vars_changed\n");
    FPRINTF(ASCERR,"         Discrete variable list was never set.\n");
    return 0;
  }

  dv = sys->dvlist;
  numdvs = slv_get_num_solvers_dvars(server);
  for( ind = 0; ind < numdvs; ind++ ) {
    cur_dis = dv[ind];
#if SHOW_LOGICAL_DETAILS
    FPRINTF(ASCERR,"Boundary index = %d \n",ind);
    FPRINTF(ASCERR,"Current Value = %d\n",dis_value(cur_dis));
    FPRINTF(ASCERR,"Previous Value = %d\n",dis_previous_value(cur_dis));
#endif /* SHOW_LOGICAL_DETAILS */
    if((dis_kind(cur_dis)==e_dis_boolean_t ) && dis_inwhen(cur_dis) ) {
      if(dis_value(cur_dis) != dis_previous_value(cur_dis)) {
        return 1;
      }
    }
  }
  return 0;
}

/*
 * Compare the original value of a discrete boolean variable (before
 * perturbation of boundaries) with its values after a solution
 * of the logical relations with some perturbed values for boundaries.
 * If those values are different, the bit VAL_MODIFIED is set to
 * TRUE. This will give us the boolean variable which will change as a
 * consequence of a boundary crossing.
 */
static
void search_for_modified_dvars(struct dis_discrete **dv,
		int32 numdvs,
		struct boolean_values *bval
){
  struct dis_discrete *cur_dis;
  int32 d;
  int32 orig_value;

  for (d=0; d<numdvs; d++) {
    cur_dis = dv[d];
    if(dis_inwhen(cur_dis) && dis_boolean(cur_dis)) {
      orig_value = bval->cur_val[d];
      if(orig_value != dis_value(cur_dis)) {
	dis_set_val_modified(cur_dis,TRUE);
      }
    }
  }
}


/*
 *  Analysis of Boundaries
 *  ----------------------------
 */

/*
 * Evaluates the current status (satisfied? , at zero?) of a boundary
 * and sets its flags accordingly. At the same time, it updates the
 * residual of the relation included in the boundary (see
 * bndman_calc_satisfied).
 */

static
void update_boundaries(slv_system_t server, SlvClientToken asys){
  struct bnd_boundary **bp;
  int32 numbnds, ind, value;
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->blist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) update_boundaries.\n");
    FPRINTF(ASCERR,"         Boundary list was never set.\n");
    return;
  }

  bp = sys->blist;
  numbnds = slv_get_num_solvers_bnds(server);

  for( ind = 0; ind < numbnds; ++ind ) {
    value = bnd_status_cur(bp[ind]);
    bnd_set_pre_status(bp[ind],value);
    value = bndman_calc_satisfied(bp[ind]);
    bnd_set_cur_status(bp[ind],value);
    if((bnd_status_cur(bp[ind]) != bnd_status_pre(bp[ind])) &&
        bnd_kind(bp[ind]) == e_bnd_rel  && !bnd_at_zero(bp[ind])) {
      bnd_set_crossed(bp[ind],TRUE);
    }else{
      bnd_set_crossed(bp[ind],FALSE);
    }
    if(bnd_kind(bp[ind]) == e_bnd_rel) {
      value = bndman_calc_at_zero(bp[ind]);
      bnd_set_at_zero(bp[ind],value);
    }else{
      bnd_set_at_zero(bp[ind],FALSE);
    }
  }
}


/*
 * Look for some boundary with the CROSSED bit active. If this boundary
 * is used in some logical relation, the function returns 1, else returns 0
 */
static
int32 some_boundaries_crossed(slv_system_t server, SlvClientToken asys){
  struct bnd_boundary **bp, *cur_bnd;
  int32 numbnds, ind;
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->blist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) some_boundaries_crossed\n");
    FPRINTF(ASCERR,"         Boundary list was never set.\n");
    return 0;
  }

  bp = sys->blist;
  numbnds = slv_get_num_solvers_bnds(server);
  for( ind = 0; ind < numbnds; ++ind ) {
    cur_bnd = bp[ind];
    if(bnd_crossed(cur_bnd) && bnd_in_logrel(cur_bnd)) {
      return 1;
    }
  }
  return 0;
}

/*
 * Look for some boundary with the AT_ZERO bit active.If this boundary
 * is used in some logical relation, the function returns 1, else returns 0
 */
static
int32 some_boundaries_at_zero(slv_system_t server, SlvClientToken asys){
  struct bnd_boundary **bp, *cur_bnd;
  int32 numbnds, ind;
  slv9_system_t sys;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->blist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) some_boundaries_at_zero\n");
    FPRINTF(ASCERR,"         Boundary list was never set.\n");
    return 0;
  }

  bp = sys->blist;
  numbnds = slv_get_num_solvers_bnds(server);
  for( ind = 0; ind < numbnds; ++ind ) {
    cur_bnd = bp[ind];
    if(bnd_at_zero(cur_bnd) && bnd_in_logrel(cur_bnd)) {
      return 1;
    }
  }
  return 0;
}

/*
 * Perform the combinatorial perturbation of the boundaries which are
 * at their zero. That means: We are going to perform a combinatorial
 * search, changing the truth value of a SATISFIED term (for the
 * specified boundaries) ON and OFF, and finding the boolean variables
 * affected for those changes in value of the SATISFIED terms.
 */
static
void do_perturbation_combinations(slv_system_t server,
		struct boolean_values *bval,
		struct bnd_boundary **bp,
		struct dis_discrete **dv,
		int32 numdvs,int32 *bndatzero,
		int32 ind, int32 numbndf
){
  slv_status_t status;
  int32 indpo;

  if(ind<(numbndf-1)) {
    indpo = ind + 1;
    bnd_set_perturb(bp[bndatzero[ind]],TRUE);
    do_perturbation_combinations(server,bval,bp,dv,numdvs,
				 bndatzero,indpo,numbndf);
    bnd_set_perturb(bp[bndatzero[ind]],FALSE);
    do_perturbation_combinations(server,bval,bp,dv,numdvs,
				 bndatzero,indpo,numbndf);
  }else{
    if(ind < numbndf) {
      bnd_set_perturb(bp[bndatzero[ind]],TRUE);
      solve_logical_relations(server);
      slv_get_status(server,&status);
      if(!status.converged) {
        FPRINTF(ASCERR,"WARNING: \n");
        FPRINTF(ASCERR,"(slv9) do_perturbation_combinations\n");
        FPRINTF(ASCERR," Not convergence in logical solver \n");
      }else{
        search_for_modified_dvars(dv,numdvs,bval);
      }
      bnd_set_perturb(bp[bndatzero[ind]],FALSE);
      solve_logical_relations(server);
      slv_get_status(server,&status);
      if(!status.converged) {
        FPRINTF(ASCERR,"WARNING: \n");
        FPRINTF(ASCERR,"(slv9) do_perturbation_combinations\n");
        FPRINTF(ASCERR," Not convergence in logical solver \n");
      }else{
        search_for_modified_dvars(dv,numdvs,bval);
      }
    }else{
      FPRINTF(ASCERR,"ERROR:  (slv9) do_perturbation_combinations\n");
      FPRINTF(ASCERR,"         Wrong boundary index as argument\n");
    }
  }
  return;
}


/*
 * Perform the combinatorial search of the subregions. That means:
 * We perform a combinatorial search, changing the value of the
 * discrete variables (given in disvars) TRUE and FALSE, and
 * finding which cases (in the WHENs) applies for each of the
 * combinations.
 */
static
void do_dvar_values_combinations(struct gl_list_t *disvars,
		struct matching_cases *cases,
		int numdvf, int d,
		int *pos_cases
){
  struct dis_discrete *cur_dis;
  int32 *cur_cases;
  int32 ncases, dpo;

  if(d < numdvf) {
    dpo = d + 1;
    cur_dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    dis_set_boolean_value(cur_dis,TRUE);
    do_dvar_values_combinations(disvars,cases,numdvf,dpo,pos_cases);
    dis_set_boolean_value(cur_dis,FALSE);
    do_dvar_values_combinations(disvars,cases,numdvf,dpo,pos_cases);
  }else{
    if(d == numdvf) {
      cur_dis = (struct dis_discrete *)(gl_fetch(disvars,d));
      dis_set_boolean_value(cur_dis,TRUE);
      cur_cases = cases_matching(disvars,&ncases);
      cases[(*pos_cases)].case_list = cur_cases;
      cases[(*pos_cases)].ncases = ncases;
      cases[(*pos_cases)].diff_subregion = 1;
      (*pos_cases)++;
      dis_set_boolean_value(cur_dis,FALSE);
      cur_cases = cases_matching(disvars,&ncases);
      cases[(*pos_cases)].case_list = cur_cases;
      cases[(*pos_cases)].ncases = ncases;
      cases[(*pos_cases)].diff_subregion = 1;
      (*pos_cases)++;
    }else{
      FPRINTF(ASCERR,"ERROR: (slv9) do_dvar_values_combinations\n");
      FPRINTF(ASCERR,"        Wrong discrete var index as argument\n");
    }
  }
  return;
}


/*
 * Orders of the elements of each array of cases,
 * so that we can compare them easier.
 */
static
void order_case(int32 *case_list, int32 *newcaselist, int ncases){
  int32 cur_case,pos=0,tmp_num,c,ind;

  for (c=1; c<=ncases; c++) {
    tmp_num = 0;
    for (ind=1; ind<=ncases; ind++) {
      cur_case = case_list[ind];
      if(tmp_num < cur_case) {
        pos = ind;
        tmp_num  = cur_case;
      }
    }
    case_list[pos] = 0;
    newcaselist[ncases-c] = tmp_num;
  }
}



/*
 * Calls for the ordering of the elements of each array of cases,
 * so that we can compare them easier.
 */
static
void order_cases(struct matching_cases *cases,int pos_cases){
  int32 *caselist;
  int32 cur_ncase,c;
  int32 *newcaselist;

  for (c=0; c<pos_cases;c++) {
    caselist  = cases[c].case_list;
    cur_ncase = cases[c].ncases;
    if(cur_ncase > 1) {
      newcaselist = create_array(cur_ncase,int32);
      order_case(caselist,newcaselist,cur_ncase);
      cases[c].case_list = newcaselist;
      destroy_array(caselist);
    }else{
      if(cur_ncase == 1) {
      newcaselist = create_array(1,int32);
      newcaselist[0] = caselist[1];
      cases[c].case_list = newcaselist;
      destroy_array(caselist);
      }
    }
  }

}



/*
 * Compare two arrays of cases (integer numbers). It returns 1 if they are
 * equal, else it returns 0.
 */
static
int32 compare_case(int32 *cur_set, int32 *comp_set, int cur_ncases){
  int32 cur_case, comp_case, ind;

  for (ind=0; ind<cur_ncases; ind++) {
    cur_case = cur_set[ind];
    comp_case = comp_set[ind];
    if(cur_case != comp_case) {
      return 0;
    }
  }
  return 1;
}


/*
 * Compare the arrays of cases so that we can find the number of
 * different alternatives (subregions)
 */
static
void compare_cases(struct matching_cases *cases,int pos_cases){
  int32 *cur_set, *comp_set, cur_ncases, comp_ncases;
  int32 c,d;

  for (c=0; c<pos_cases; c++) {
    cur_set = cases[c].case_list;
    cur_ncases = cases[c].ncases;
    if(cur_ncases == 0) {
      cases[c].diff_subregion = 0;
      continue;
    }
    for(d=0; d<c; d++) {
      comp_set = cases[d].case_list;
      comp_ncases = cases[d].ncases;
      if(cur_ncases != comp_ncases) {
        continue;
      }else{
        if(compare_case(cur_set,comp_set,cur_ncases)) {
          cases[c].diff_subregion = 0;
          break;
	}
      }
    }
  }
}


/*
 * Finds if my current point lies at a "real" boundary. By "real"
 * I mean a boundary which really causes a change in the
 * configuration. It returns 0 if the boundary at zero does not
 * affect the configuration. If the configuration is affected,
 * this function will find the number of subregions existing
 * for the current point as well as the cases (in WHENs) corresponding
 * to each of the subregions. At the end, the number of subregions is
 * n_subregions and the cases applying for each of them is stored
 * in the structure subregions.
 */
static
int32 at_a_boundary(slv_system_t server, SlvClientToken asys,
		int32 *n_subregions,
		struct matching_cases **subregions,
		int32 *cur_subregion,
		struct gl_list_t *disvars
){
  slv9_system_t sys;
  struct bnd_boundary **bp, *cur_bnd;
  struct dis_discrete **dv, *cur_dis;
  struct boolean_values bval;
  dis_filter_t dfilter;
  bnd_filter_t bfilter;
  struct matching_cases *cases;
  int32 *bndatzero;
  int32 *dvarmodified;
  int32 *cur_cases;
  int32 *caselist, *newcaselist;
  int32 numbnds, numbndf, b, ind;
  int32 numdvs, numdvf, d;
  int32 cur_ncases, assign_cur_sub;
  int32 pos_cases, comb;
  char *param;
  union param_value u;

  sys = SLV9(asys);
  check_system(sys);

  if(sys->blist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) at_a_boundary\n");
    FPRINTF(ASCERR,"         Boundary list was never set.\n");
    return 0;
  }

  if(sys->dvlist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) at_a_boundary\n");
    FPRINTF(ASCERR,"         Discrete Variable list was never set.\n");
    return 0;
  }

  if(!some_boundaries_at_zero(server,asys)) {
    return 0;
  }

  bp = sys->blist;
  numbnds = slv_get_num_solvers_bnds(server);
  bfilter.matchbits = (BND_AT_ZERO);
  bfilter.matchvalue = (BND_AT_ZERO);
  numbndf = slv_count_solvers_bnds(server,&bfilter);
  bndatzero = create_array(numbndf,int32);
  ind = 0;
  for (b=0; b<numbnds; b++) {
    cur_bnd = bp[b];
    bnd_set_perturb(cur_bnd,FALSE);
    if(bnd_at_zero(cur_bnd)) {
#if SHOW_BOUNDARY_ANALYSIS_DETAILS
    FPRINTF(ASCERR,"boundary at zero = %d\n",b);
#endif /* SHOW_BOUNDARY_ANALYSIS_DETAILS */
      bndatzero[ind] = b;
      ind++;
    }
  }

  dv = sys->dvlist;
  numdvs = slv_get_num_solvers_dvars(server);
  bval.cur_val = create_array(numdvs,int32);
  bval.pre_val = create_array(numdvs,int32);

  for (d=0; d<numdvs; d++) {
    cur_dis = dv[d];
    dis_set_val_modified(cur_dis,FALSE);
    bval.cur_val[d] = dis_value(cur_dis);
    bval.pre_val[d] = dis_previous_value(cur_dis);
  }

#if SHOW_BOUNDARY_ANALYSIS_DETAILS
  FPRINTF(ASCERR,"Executing combinatorial perturbation of boundaries\n");
#endif /* SHOW_BOUNDARY_ANALYSIS_DETAILS */

  /*
   * Setting the value of the perturbation mode flag in the logical solver
   * to 1.
   * PERTURB_BOUNDARY is a boolean parameter of the logical solver
   * LRSlv. This parameter tells the solver whether it should change
   * the truth value of the SATISFIED terms or not (only for the
   * boundaries specified). This trick is important while finding
   * the number of subregions around a/several boundary(ies).
   */
  param = "perturbboundaries";
  u.b = 1;
  set_param_in_solver(server,LOGICAL_SOLVER,bool_parm,param,&u);

  ind = 0;
  do_perturbation_combinations(server,&(bval),bp,dv,numdvs,
			       bndatzero,ind,numbndf);
  /*
   * Setting the value of the perturbation mode flag in the logical solver
   * to 0.
   */
  u.b = 0;
  set_param_in_solver(server,LOGICAL_SOLVER,bool_parm,param,&u);

  destroy_array(bndatzero);

  dfilter.matchbits = (DIS_VAL_MODIFIED);
  dfilter.matchvalue = (DIS_VAL_MODIFIED);
  numdvf = slv_count_solvers_dvars(server,&dfilter);

  if(numdvf == 0) {
    FPRINTF(ASCERR,"Not really at a boundary\n");
    for (d=0; d<numdvs; d++) {
      cur_dis = dv[d];
      dis_set_boolean_value(cur_dis,bval.cur_val[d]);
      dis_set_value(cur_dis,bval.cur_val[d]);
      dis_set_previous_value(cur_dis,bval.pre_val[d]);
    }
    destroy_array(bval.cur_val);
    destroy_array(bval.pre_val);
    return 0;
  }

  dvarmodified = create_array(numdvf,int32);
  ind = 0;
  for (d=0; d<numdvs; d++) {
    cur_dis = dv[d];
    if(dis_val_modified(cur_dis)) {
      dvarmodified[ind] = d;
      gl_append_ptr(disvars,cur_dis);
      dis_set_val_modified(cur_dis,FALSE);
      ind++;
    }
  }

  for (d=0; d<numdvs; d++) {
    cur_dis = dv[d];
    dis_set_boolean_value(cur_dis,bval.cur_val[d]);
    dis_set_value(cur_dis,bval.cur_val[d]);
    dis_set_previous_value(cur_dis,bval.pre_val[d]);
  }

  pos_cases = 1;
  for (d = 1; d<=numdvf; d++) {
    pos_cases = pos_cases * 2;
  }

  cases = (struct matching_cases *)
                 (ascmalloc((pos_cases)*sizeof(struct matching_cases)));

#if SHOW_BOUNDARY_ANALYSIS_DETAILS
  FPRINTF(ASCERR,"Executing combinatorial search for subregions \n");
#endif /* SHOW_BOUNDARY_ANALYSIS_DETAILS */

  d = 1;
  comb = 0;
  do_dvar_values_combinations(disvars,cases,numdvf,d,&(comb));

  order_cases(cases,pos_cases);
  compare_cases(cases,pos_cases);

  (*n_subregions) = 0;
  for(comb=0; comb<pos_cases;comb++) {
    if(cases[comb].diff_subregion) {
      (*n_subregions)++;
    }
  }

  if((*n_subregions)==0) {
    FPRINTF(ASCERR,"ERROR: at least one subregion must be found\n");
    for (d=0; d<numdvs; d++) {
      cur_dis = dv[d];
      dis_set_boolean_value(cur_dis,bval.cur_val[d]);
      dis_set_value(cur_dis,bval.cur_val[d]);
      dis_set_previous_value(cur_dis,bval.pre_val[d]);
    }
    destroy_array(bval.cur_val);
    destroy_array(bval.pre_val);
    for(comb=0; comb<pos_cases;comb++) {
      destroy_array(cases[comb].case_list);
    }
    destroy_array(cases);
    return 0;
  }

  if((*n_subregions)==1) {
    FPRINTF(ASCERR,"Not really at a boundary\n");
    for (d=0; d<numdvs; d++) {
      cur_dis = dv[d];
      dis_set_boolean_value(cur_dis,bval.cur_val[d]);
      dis_set_value(cur_dis,bval.cur_val[d]);
      dis_set_previous_value(cur_dis,bval.pre_val[d]);
    }
    destroy_array(bval.cur_val);
    destroy_array(bval.pre_val);
    for(comb=0; comb<pos_cases;comb++) {
      destroy_array(cases[comb].case_list);
    }
    destroy_array(cases);
    return 0;
  }

  if((*n_subregions) > 0) {
    (*subregions) = (struct matching_cases *)
                 (ascmalloc(((*n_subregions))*sizeof(struct matching_cases)));
    (*n_subregions) = 0;
    for(comb=0; comb<pos_cases;comb++) {
      if(cases[comb].diff_subregion) {
        (*subregions)[(*n_subregions)].case_list = cases[comb].case_list;
        cases[comb].case_list = NULL;
        (*subregions)[(*n_subregions)].ncases = cases[comb].ncases;
        cases[comb].ncases = 0;
        (*subregions)[(*n_subregions)].diff_subregion = 1;
        (*n_subregions)++;
      }
    }
  }

  for(comb=0; comb<pos_cases;comb++) {
    destroy_array(cases[comb].case_list);
  }
  destroy_array(cases);


  assign_cur_sub = 0;
  /*
   * Finding the subregion corresponding to the "original" configuration
   */
  for (d=0; d<numdvs; d++) {
    cur_dis = dv[d];
    dis_set_boolean_value(cur_dis,bval.cur_val[d]);
    dis_set_value(cur_dis,bval.cur_val[d]);
    dis_set_previous_value(cur_dis,bval.pre_val[d]);
  }
  cur_cases = cases_matching(disvars,&cur_ncases);
  caselist = cur_cases;
  if(cur_ncases > 1) {
    newcaselist = create_array(cur_ncases,int32);
    order_case(caselist,newcaselist,cur_ncases);
    cur_cases = newcaselist;
    destroy_array(caselist);
  }else{
    if(cur_ncases == 1) {
      newcaselist = create_array(1,int32);
      newcaselist[0] = caselist[1];
      cur_cases = newcaselist;
      destroy_array(caselist);
    }
  }
  for(comb=0; comb<(*n_subregions);comb++) {
    if((*subregions)[comb].ncases == cur_ncases) {
      if(compare_case((*subregions)[comb].case_list,cur_cases,cur_ncases)) {
        (*cur_subregion) = comb;
        assign_cur_sub = 1;
        break;
      }
    }
  }

  if(!assign_cur_sub) {
    FPRINTF(ASCERR,"PANIC: original configuration not found\n");
  }

  destroy_array(cur_cases);
  destroy_array(dvarmodified);
  destroy_array(bval.cur_val);
  destroy_array(bval.pre_val);
  return 1;
}


/*
 * If some boundary(ies) has been crossed in the iterative scheme,
 * this function finds the boundary crossed (the first one, if many).
 * It returns the factor (less than 1) by which the step length has
 * to be multiplied son that the new point will lie precisely
 * at that boundary. This factor is found by using the method of
 * bisection
 */
static
real64 return_to_first_boundary(slv_system_t server,
		SlvClientToken asys,
		struct real_values *rvalues,
		var_filter_t *vfilter
){
  slv9_system_t sys;
  struct bnd_boundary **bp, *cur_bnd;
  struct var_variable **incidences, **bnd_incidences;
  struct var_variable *cur_var;
  bnd_filter_t bfilter;
  struct boolean_values bval;
  real64 factor=0.0,fup,flo,newvalue;
  int32 *bndcrossed;
  int32 *inc_vars;
  int32 count,n_incidences,inc,conv_flag,still_crossed;
  int32 numbnds,numbndf,b,ind;
  int32 iter,n_iterations;
  FILE *lif;

  sys = SLV9(asys);
  check_system(sys);
  lif = LIF(sys);

  if(sys->blist == NULL ) {
    FPRINTF(ASCERR,"ERROR:  (slv9) return_to_first_boundary\n");
    FPRINTF(ASCERR,"         Boundary list was never set.\n");
    return 1.0;
  }

  if(!some_boundaries_crossed(server,asys)) {
    return 1.0;
  }

  bp = sys->blist;
  numbnds = slv_get_num_solvers_bnds(server);
  bfilter.matchbits = (BND_CROSSED);
  bfilter.matchvalue = (BND_CROSSED);
  numbndf = slv_count_solvers_bnds(server,&bfilter);
  bndcrossed = create_array(numbndf,int32);
  bval.cur_val = create_array(numbndf,int32);
  bval.pre_val = create_array(numbndf,int32);
  ind = 0;
  for (b=0; b<numbnds; b++) {
    cur_bnd = bp[b];
    if(bnd_crossed(cur_bnd)) {
      bndcrossed[ind] = b;
      bval.cur_val[ind] = bnd_status_cur(cur_bnd);
      bval.pre_val[ind] = bnd_status_pre(cur_bnd);
      ind++;
    }
  }

  count = 0;
  for (b=0; b<numbndf; b++) {
    cur_bnd = bp[bndcrossed[b]];
    n_incidences = bnd_n_real_incidences(cur_bnd);
    count = count + n_incidences;
  }

  incidences = (struct var_variable **)
               ( ascmalloc((count)*sizeof(struct var_variable *)));
  inc_vars = create_array(count,int32);
  count = 0;
  for (b=0; b<numbndf; b++) {
    cur_bnd = bp[bndcrossed[b]];
    bnd_incidences = bnd_real_incidence(cur_bnd);
    n_incidences = bnd_n_real_incidences(cur_bnd);
#if SHOW_BOUNDARY_ANALYSIS_DETAILS
    FPRINTF(lif,"boundary crossed = %d\n",bndcrossed[b]);
    FPRINTF(lif,"previous boundary status = %d\n",bval.pre_val[b]);
    FPRINTF(lif,"current boundary status = %d\n",bval.cur_val[b]);
#endif /* SHOW_BOUNDARY_ANALYSIS_DETAILS */
    for (inc=0; inc<n_incidences; inc++) {
      incidences[count] = bnd_incidences[inc];
      inc_vars[count] = var_mindex(incidences[count]);
      count++;
    }
  }

  /* bisection to find first boundary crossed */
  fup = 1.0;
  flo = 0.0;
  conv_flag = 0;
  iter = 0;

/*
 * Maximum number of bisection iterations. This must be modified
 * so that it becomes a parameter to be defined by the user
 */
  n_iterations = ITER_BIS_LIMIT;

#if SHOW_BOUNDARY_ANALYSIS_DETAILS
    for (inc=0; inc<count; inc++) {
      cur_var = incidences[inc];
      if(var_apply_filter(cur_var,vfilter)) {
        FPRINTF(lif,"Variable ");
        print_var_name(lif,sys,cur_var); PUTC('\n',lif);
        FPRINTF(lif,
                "previous value = %f\n",rvalues->pre_values[inc_vars[inc]]);
        FPRINTF(lif,"current value = %f\n",rvalues->cur_values[inc_vars[inc]]);
      }
    }
#endif /* SHOW_BOUNDARY_ANALYSIS_DETAILS */

  while (conv_flag == 0) {
    iter++;
    if(iter>n_iterations) {
      FPRINTF(ASCERR,"ERROR:  (slv9) return_to_first_boundary\n");
      FPRINTF(ASCERR,"Could not find the first boundary crossed \n");
      FPRINTF(ASCERR,"Returning the last factor calculated\n");
      break;
    }
    still_crossed = 0;
    factor = ( fup + flo ) / 2.0;
#if SHOW_BISECTION_DETAILS
    FPRINTF(lif,"fup = %f\n",fup);
    FPRINTF(lif,"flo = %f\n",flo);
    FPRINTF(lif,"factor = %f\n",factor);
#endif /* SHOW_BISECTION_DETAILS */
    for (inc=0; inc<count; inc++) {
      cur_var = incidences[inc];
      if(var_apply_filter(cur_var,vfilter)) {
        newvalue  = rvalues->pre_values[inc_vars[inc]] + factor *
                    ( rvalues->cur_values[inc_vars[inc]] -
                    rvalues->pre_values[inc_vars[inc]] );
        var_set_value(cur_var,newvalue);
#if SHOW_BISECTION_DETAILS
        FPRINTF(lif,"Variable ");
        print_var_name(lif,sys,cur_var); PUTC('\n',lif);
        FPRINTF(lif,"value after factor = %f\n",newvalue);
#endif /* SHOW_BISECTION_DETAILS */
      }
    }

    update_boundaries(server,asys);
    for (b=0; b<numbndf; b++) {
      cur_bnd = bp[bndcrossed[b]];
#if SHOW_BISECTION_DETAILS
      FPRINTF(lif,"previous status = %d\n", bval.pre_val[b]);
      FPRINTF(lif,"status aftert factor = %d\n",bnd_status_cur(cur_bnd));
#endif /* SHOW_BISECTION_DETAILS */
      if(bnd_status_cur(cur_bnd) != bval.pre_val[b] ) {
        still_crossed = 1;
      }
    }
#if SHOW_BISECTION_DETAILS
      FPRINTF(lif,"still_crossed = %d\n",still_crossed);
#endif /* SHOW_BISECTION_DETAILS */
    if(still_crossed) {
      fup = factor;
    }else{
      flo = factor;
      for (b=0; b<numbndf; b++) {
        cur_bnd = bp[bndcrossed[b]];
        bnd_set_pre_status(cur_bnd,bval.pre_val[b]);
        bnd_set_cur_status(cur_bnd,bval.pre_val[b]);
        if(bnd_at_zero(cur_bnd)) {
#if SHOW_BOUNDARY_ANALYSIS_DETAILS
          FPRINTF(ASCERR,"boundary at zero = %d\n",bndcrossed[b]);
          FPRINTF(lif,"factor = %f\n",factor);
          for (inc=0; inc<count; inc++) {
            cur_var = incidences[inc];
            if(var_apply_filter(cur_var,vfilter)) {
            FPRINTF(lif,"Variable ");
            print_var_name(lif,sys,cur_var); PUTC('\n',lif);
            FPRINTF(lif,"value after factor = %f\n",var_value(cur_var));
            }
          }
#endif  /* SHOW_BOUNDARY_ANALYSIS_DETAILS */
          conv_flag = 1;
        }
      }
    }
  }
  destroy_array(bndcrossed);
  destroy_array(inc_vars);
  destroy_array(bval.cur_val);
  destroy_array(bval.pre_val);
  destroy_array(incidences);

  return factor;
}


/*
 *  Storing values of real variables.
 *  ---------------------------------
 *
 *  We use the master list of variables since its order does not change
 *  and it is given by the master index. We do not touch the master list,
 *  we only use its order.
 */

/*
 * Store the values of the var_variables before a newton-like iteration
 */
static
void store_real_pre_values(slv_system_t server,
		struct real_values *rvalues
){
  struct var_variable **master;
  struct var_variable *var;
  int v, vlen;

  master = slv_get_master_var_list(server);
  vlen = slv_get_num_master_vars(server);

  rvalues->pre_values = create_array(vlen,real64);

  for (v=0; v<vlen; v++) {
    var = master[v];
    rvalues->pre_values[v] = var_value(var);
  }
}

/*
 * Store the values of the var_variables after a newton-like iteration
 */
static
void store_real_cur_values(slv_system_t server,
		struct real_values *rvalues
){
  struct var_variable **master;
  struct var_variable *var;
  int v, vlen;

  master = slv_get_master_var_list(server);
  vlen = slv_get_num_master_vars(server);

  rvalues->cur_values = create_array(vlen,real64);

  for (v=0; v<vlen; v++) {
     var = master[v];
     rvalues->cur_values[v] = var_value(var);
  }
}

/*
 * After the length of the step has been modified so that the current point
 * lies at a boundary, the values of all the variables is updated so that
 * they all reduce the length of their step by the same factor.
 */
static
void update_real_var_values(slv_system_t server,
		struct real_values *rvalues,
		var_filter_t *vfilter, real64 factor
){
  struct var_variable **master;
  struct var_variable *var;
  real64 newvalue;
  int v, vlen;

  master = slv_get_master_var_list(server);
  vlen = slv_get_num_master_vars(server);

  for (v=0; v<vlen; v++) {
    var = master[v];
    if(var_apply_filter(var,vfilter)) {
      newvalue = rvalues->pre_values[v] +
                factor * (rvalues->cur_values[v] - rvalues->pre_values[v]);
      var_set_value(var,newvalue);
    }
  }
  destroy_array(rvalues->cur_values);
  destroy_array(rvalues->pre_values);
}


/*
 * Set the flagbit NONBASIC for all the variables in the list
 * to the value passed as argument
 */
static
void set_nonbasic_status_in_var_list(slv_system_t server,
		uint32 value
){
  struct var_variable **master;
  struct var_variable *var;
  int v, vlen;

  master = slv_get_master_var_list(server);
  vlen = slv_get_num_master_vars(server);

  for (v=0; v<vlen; v++) {
    var = master[v];
    var_set_nonbasic(var,value);
  }
}


/*
 * After the length of the step has been modified so that the current point
 * lies at a boundary, the residuals of the equations are updated.
 */
static void update_relations_residuals(slv_system_t server)
{
  struct rel_relation **master;
  struct rel_relation *rel;
  rel_filter_t rfilter;
  real64 resid;
  int32 r, rlen, status;

  master = slv_get_master_rel_list(server);
  rlen = slv_get_num_master_rels(server);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY);
  rfilter.matchvalue = (REL_INCLUDED | REL_EQUALITY);

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif

  for (r=0; r<rlen; r++) {
    rel = master[r];
    if(rel_apply_filter(rel,&rfilter)) {
      resid = relman_eval(rel,&status,1);
    }
  }
#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif

}


#ifdef ASC_WITH_CONOPT
/*------------------------------------------------------------------------------
  CALLBACK ROUTINES FOR CONOPT
*/

/*
 * COIRMS Based on the information provided in Coispz, CONOPT will
 * allocate the number of vectors into which the user can define
 * the details of the model. The details of the model are defined
 * here.
 *
 * COIRMS(lower, curr, upper, vsta, type,rhs, fv, esta, colsta,
 * rowno, value, nlflag, n, m, nz, usrmem)
 *
 * lower - lower bounds on the variables
 * curr  - intial values of the variables
 * upper - upper bounds on the variables
 * vsta  - initial status of the variable(o nonbasic, 1 basic)
 * type  - types of equations (0 equality,1 greater,2 less)
 * rhs   - values of the right hand sides
 * fv    - sum of the nonlinear terms in the initial point
 * esta  - initial status of the slack in the constraint (nonbasic,basic)
 * colsta- start of column pointers
 * rowno - row or equation numbers of the nonzeros
 * value - values of the jacobian elements
 * nlflag- nonlinearity flags(0 nonzero constant,1 varying)
 * n     - number of variables
 * m     - number of constraints
 * nz    - number of jacobian elements
 * usrmem- user memory defined by conopt
 */
static
int COI_CALL slv9_conopt_readmatrix(
		double *lower, double *curr, double *upper
		, int *vsta,  int *type, double *rhs
		, int *esta,  int *colsta, int *rowno
		, double *value, int *nlflag, int *n, int *m, int *nz
		, double *usrmem
){
  slv9_system_t sys;
  struct var_variable *var;
  struct var_variable **varlist;
  struct opt_matrix *coeff_matrix;
  real64 /*obj_val,*/ deriv;
  real64 nominal, up, low, uplow;
  int32 num_var, n_subregions, c, num_eqns;
  int32 numnz, eq;
  int32 count, totvar;
  double limit;

  static var_filter_t vfilter = {
      VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR | VAR_FIXED
     ,VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR | 0
  };

  UNUSED_PARAMETER(vsta);
  UNUSED_PARAMETER(esta);

  sys = (slv9_system_t)usrmem;
  n_subregions = sys->subregions;
  coeff_matrix = sys->coeff_matrix;
  num_var = (*n) - n_subregions;
  num_eqns = num_var + 1;

  varlist = sys->mvlist;
  totvar = sys->mvtot;
 
  /* fetch the configured bound from solver parameters */
  limit = ASC_INFINITY;

  /* CONSOLE_DEBUG("Got limit value of %g",limit); */

  /*
   * Variables: Current Value, lower value and upper value. Note that for
   * this problem the variables are the vector of steps dx. So we "invent"
   * approximations for the bounds based on the bounds of the real
   * variables of the problem. The use of the parameter ASC_INFINITY is
   * Hack to keep CONOPT from complaining for large bounds.
   */

  count = 0;
  for (c=0; c<totvar; c++) {
    var = varlist[c];
    if(var_apply_filter(var,&vfilter)) {
      nominal = var_nominal(var);
      low = var_lower_bound(var);
      up = var_upper_bound(var);
      uplow = fabs( up - low);

      if(-uplow > -limit){
	      lower[count] = -uplow;
      }else{
          lower[count] = -0.5*limit;
          /* CONSOLE_DEBUG("Reducing lower bound limit for var %d to %e",count,lower[count]); */
      }

      if(uplow < limit){
          upper[count] = uplow;
      }else{
          upper[count] = 0.5*limit;
          /* CONSOLE_DEBUG("Reducing upper bound limit for var %d to %e",count,upper[count]); */
      }

      curr[count] = 0.5 * nominal;
      count++;
    }
  }
  /* alphas for each subregion */
  for (c=count; c<(*n); c++) {
      lower[c] = 0.0;
      upper[c] = 1.0;
      curr[c] =  1.0;
  }

  /*CONSOLE_DEBUG("ALL BOUNDS:");
  for(c=0;c<(*n);++c){
    fprintf(stderr,"%d: lower = %g, upper = %g\n",c,lower[c],upper[c]);
  }*/

  /*
   * vsta not in use since STATOK, ipsz[14], is zero
   */

  /*
   * All the equations, but the last row (which is the objective), are
   * equalities.
   */
  for (c = 0; c < (*m); c++) {
    type[c] = 0;
  }
  type[(*m)-1] = 3;


  /*
   * RHS. It is zero for all the equations except the summation of
   * alphas, whose RHS is one.
   */
  for (c = 0; c < (*m); c++) {
    rhs[c] = 0;
  }
  rhs[(*m)-2] = 1.0;

#ifdef DISUSED_CONOPT_PARAMETER
  /*
   * fv =0 for all linear relations. For the objective is the two
   * norm
   */
  for (c = 0; c < (*m); c++) {
    fv[c] = 0;
  }
  obj_val = 0.0;
  for (c = 0; c<num_var; c++) {
    obj_val = obj_val + (curr[c] * curr[c]);
  }
  fv[(*m)-1] = obj_val;
#endif

  /*
   * esta not used since STATOK is zero
   */


  /*
   * For the following parameters, it is important ot see that:
   * 1) The values for the rows and nonzeros that conopt wants start
   *    with 1, not with 0.
   * 2) The indeces of the arrays that we are using in the C side start
   *    with 0.
   */

  /*
   * colsta
   */

  for (c=0; c<num_var; c++) {
    colsta[c] = 2 * c;
  }

  for (c=num_var; c<(*n); c++) {
    colsta[c] = 2 * num_var + num_eqns * (c - num_var);
  }

  colsta[*n] = *nz; /** @TODO check this */

  /*
	rowno, value and nlflag can be done in same loop. The use of the
	parameter RTMAXJ is really a Hack to keep CONOPT from complaining
	about large derivatives
  */

  numnz = 0;
  for (c=0; c<num_var; c++) {
    rowno[numnz] = c;
    nlflag[numnz] = 0;
    value[numnz] = -1;
    numnz++;
    rowno[numnz] = *m - 1;
    nlflag[numnz] = 1;
    numnz++;
  }

  for (c=num_var; c<(*n); c++) {
    numnz = 2 * num_var + num_eqns * (c - num_var);
    for(eq = 0; eq<num_eqns-1; eq++) {
      rowno[numnz] = eq;
      nlflag[numnz] = 0;
      deriv = -1.0 * (coeff_matrix->cols[c - num_var].element[eq]);
      if(deriv > RTMAXJ ) {
        deriv = 0.5 * RTMAXJ;
      }else{
        if(deriv < -RTMAXJ ) {
          deriv = -0.5*RTMAXJ;
        }
      }
      value[numnz] = deriv;
      numnz++;
    }
    rowno[numnz] = num_eqns - 1;
    nlflag[numnz] = 0;
    value[numnz] = 1.0;
  }

  return 0;
}

#if 0 /* not in API any more */
/*
 * COIFBL Defines the nonlinearities of the model by returning
 * numerical values. It works on a block of rows during each call.
 * COIFBL( x, g, otn, nto, from, to, jac, stcl, rnum, cnum, nl, strw,
 *         llen, indx, mode, errcnt, n, m, n1, m1, nz, usrmem)
 *
 * x     - punt of evaluation provided by conopt
 * g     - vector of function values
 * otn   - old to new permutation vector
 * nto   - new to old permutation vector
 * from  - range in permutation
 * to    - range in permutation
 * jac   - vector of jacobian values.
 *         The following are vectors defining the jacobian structure
 * stcl  - start of column pointers
 * rnum  - row numbers
 * cnum  - column numbers
 * nl    - nonlinearity flags
 * strw  - start row pointers
 * llen  - count of linear jacobian elements
 * indx  - pointers from the row-wise representation
 * mode  - indicator of mode of evaluation
 * errcnt- number of function evaluation errors
 * n     - umber of variables
 * m     - number of constraints
 * n1    - n+1
 * m1    - m+1
 * nz    - number of jacobian elements
 * usrmem- user memory defined by conopt
 */
static void slv9_coifbl(real64 *x, real64 *g, int32 *otn, int32 *nto,
			int32 *from,  int32 *to, real64 *jac, int32 *stcl,
			int32 *rnum, int32 *cnum,  int32 *nl, int32 *strw,
                        int32 *llen, int32 *indx, int32 *mode,  int32 *errcnt,
			int32 *n, int32 *m, int32 *n1, int32 *m1,
	                int32 *nz, real64 *usrmem)
{
  /* non defined for this solver */

  /* stop gcc whining about unused parameter */
  (void)x;  (void)g;  (void)otn;  (void)nto;  (void)from;  (void)to;
  (void)jac;  (void)stcl;  (void)rnum;  (void)cnum;  (void)nl;  (void)strw;
  (void)llen;  (void)indx;  (void)mode;  (void)errcnt;  (void)n; (void)m;
  (void)n1;  (void)m1;  (void)nz;
  (void)usrmem;

  return;
}
#endif

/*
 * COIFDE Defines the nonlinearities of the model by returning
 * numerical values. It works on one row or equation at a time
 * COIFDE(x, g, jac, rowno, jcnm, mode, errcnt, newpt, n, nj, usrmem)
 *
 * x      - punt of evaluation provided by conopt
 * g      - function value
 * jac    - jacobian values
 * rowno  - number of the row for which nonlinearities will be eval
 * jcnm   - list of column number fon the NL nonzeros
 * mode   - indicator of mode of evaluation
 * errcnt - sum of number of func evaluation errors thus far
 * newpt  - new point indicator
 * nj     - number of nonlinear nonzero jacobian elements
 * n      - number of variables
 * usrmem - user memory
 *
 * For the optimization problem at a boundary, this subroutine will
 * be called only of the objective function, constraint number m.
 *
 */
static
int COI_CALL slv9_conopt_fdeval(
		double *x, double *g, double *jac
		, int *rowno, int *jcnm, int *mode, int *ignerr
		, int *errcnt, int *newpt, int *n, int *nj
		, double *usrmem
){
  slv9_system_t sys;
  int32 num_vars, v;
  real64 obj, deriv;

  UNUSED_PARAMETER(jcnm);
  UNUSED_PARAMETER(errcnt);
  UNUSED_PARAMETER(newpt);
  UNUSED_PARAMETER(n);
  UNUSED_PARAMETER(nj);

  sys = (slv9_system_t)usrmem;
  num_vars = sys->con.n - sys->subregions;

  if(*mode == 1 || *mode == 3) {
    if(*rowno == sys->con.m - 1){
      obj = 0.0;
      for (v=0; v<num_vars; v++) {
        obj = obj + (x[v] * x[v]);
      }
      *g = obj;
    }else{
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Wrong number of constraints");
      return 1;
    }
  }

  /*
   * The use of the  parameter RTMAXJ is really a Hack to keep CONOPT
   * from complaining about large derivatives.
   */

  if(*mode == 2 || *mode == 3) {
    if(*rowno == sys->con.m - 1){
      for (v=0; v<num_vars; v++) {
        deriv = 2.0 * x[v];
        if(deriv > RTMAXJ ) {
          deriv = 0.5*RTMAXJ;
        }else{
          if(deriv < -RTMAXJ ) {
            deriv = -0.5*RTMAXJ;
          }
        }
        jac[v] = deriv;
      }
    }else{
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Wrong number of constraints");
	  return 1;
    }
  }

  return 0;
}


/*
 * COISTA Pass the solution from CONOPT to the modeler. It returns
 * completion status
 * COISTA(modsta, solsts, iter, objval, usrmem)
 *
 * modsta - model status
 * solsta - solver status
 * iter   - number of iterations
 * objval - objective value
 * usrmem - user memory
 */
static
int COI_CALL slv9_conopt_status(int *modsta, int *solsta, int *iter
		, double *objval, double *usrmem
){
  slv9_system_t sys;

  sys = (slv9_system_t)usrmem;

  sys->con.modsta = *modsta;
  sys->con.solsta = *solsta;
  sys->con.iter = *iter;
  sys->con.obj = *objval;

  return 0;
}

/**
	CONOPT error message reporting
*/
int COI_CALL slv9_conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
){
	slv9_system_t sys;
	char *varname=NULL;
	struct var_variable **vp;

	sys = (slv9_system_t)USRMEM;


	if(*COLNO!=-1){
		vp=sys->mvlist;
		vp = vp + *COLNO;
		assert(*vp!=NULL);
		varname= var_make_name(SERVER,*vp);
	}

	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
	if(*ROWNO == -1){
	    FPRINTF(ASCERR,"Variable %d (Maybe it's '%s'): ",*COLNO,varname);
		ASC_FREE(varname);
	}else if(*COLNO == -1 ){
	    FPRINTF(ASCERR,"Relation %d: ",*ROWNO);
	}else{
	    FPRINTF(ASCERR,"Variable %d (Maybe it's '%s') appearing in relation %d: ",*COLNO,varname,*ROWNO);
		ASC_FREE(varname);
	}
	FPRINTF(ASCERR,"%*s", *MSGLEN, MSG);
	error_reporter_end_flush();
	return 0;
}


/*
 * COIRS Pass the solution from CONOPT to the modeler. It returns
 * solution values
 * COIRS(val, xmar, xbas, xsta, yval, ymar, ybas, ysta, n, m, usrmem)
 *
 * xval   - the solution values of the variables
 * xmar   - corresponding marginal values
 * xbas   - basis indicator for column (at bound, basic, nonbasic)
 * xsta   - status of column (normal, nonoptimal, infeasible,unbounded)
 * yval   - values of the left hand side in all the rows
 * ymar   - corresponding marginal values
 * ybas   - basis indicator for row
 * ysta   - status of row
 * n      - number of variables
 * m      - number of constraints
 * usrmem - user memory
 */
static
int COI_CALL slv9_conopt_solution(double *xval, double *xmar, int *xbas, int *xsta,
		double *yval, double *ymar, int *ybas, int * ysta,
		int *n, int *m, double *usrmem
){
  slv9_system_t sys;
  struct opt_vector *opt_var_values;
  int32 c;
  real64 value;

  UNUSED_PARAMETER(xmar);UNUSED_PARAMETER(xbas);UNUSED_PARAMETER(xsta);
  UNUSED_PARAMETER(yval);UNUSED_PARAMETER(ymar);UNUSED_PARAMETER(ybas);
  UNUSED_PARAMETER(ysta);UNUSED_PARAMETER(m);

  sys = (slv9_system_t)usrmem;
  opt_var_values = sys->opt_var_values;

  for (c = 0; c < (*n); c++) {
    value = xval[c];
    opt_var_values->element[c] = value;
  }

  return 0;
}

#if 0
/*
 * COIUSZ communicates and update of an existing model to CONOPT
 * COIUSZ(nintg, ipsz, nreal, rpsz, usrmem)
 *
 * nintg - number of positions in ipsz
 * ipsz  - array describing problem size and options
 * nreal - number of positions in rpsz
 * rpsz  - array of reals describing problem size and options
 * usrmem- user memory
 */
static void slv9_coiusz(int32 *nintg, int32 *ipsz, int32 *nreal, real64 *rpsz,
	                real64 *usrmem)
{
  /* non defined for this solver */

  /*
   * stop gcc whining about unused parameter
   */
  (void)nintg;  (void)ipsz;   (void)nreal;  (void)rpsz;
  (void)usrmem;

  return;
}
#endif

/*
 * COIOPT communicates non-default option values to CONOPT
 * COIOPT(name, rval, ival, lval, usrmem)
 * name   - the name of a CONOPT CR-cell defined by the modeler
 * rval   - the value to be assigned to name if the cells contains a real
 * ival   - the value to be assigned to name if the cells contains an int
 * lval   - the value to be assigned to name if the cells contains a log value
 * usrmem - user memory
 */
static
int COI_CALL slv9_conopt_option(
		int *NCALL, double *rval, int *ival, int *logical
	    , double *usrmem, char *name, int lenname
){
  slv9_system_t sys;
  sys = (slv9_system_t)usrmem;

  UNUSED_PARAMETER(logical);

  name = memset(name,' ',8);
  while (sys->con.opt_count < slv9_PA_SIZE) {
    if(strlen(sys->p.parms[sys->con.opt_count].interface_label) == 6) {
      if(strncmp(sys->p.parms[sys->con.opt_count].interface_label,
                  "R",1) == 0) {
	name = strncpy(name, sys->p.parms[sys->con.opt_count]. /* . break */
                                  interface_label,6);
	*rval = sys->p.parms[sys->con.opt_count].info.r.value;
	sys->con.opt_count++;
	return 0;
      } else if(strncmp(sys->p.parms[sys->con.opt_count]. /* . break */
                             interface_label,"L",1) == 0) {
	name = strncpy(name,sys->p.parms[sys->con.opt_count]. /* . break */
                                 interface_label,6);
	*ival = sys->p.parms[sys->con.opt_count].info.i.value;
	sys->con.opt_count++;
	return 0;
      }
    }
    sys->con.opt_count++;
  }

  /* sending blank to quit iterative calling */
  name = memset(name,' ',8);
  return 0;
}

#if 0 /* see slv_conopt_iterate */
/*
 * COIPSZ communicates the model size and structure to CONOPT
 * COIPSZ(nintgr, ipsz, nreal, rpsz, usrmem)
 *
 * ningtr - number of positions in ipsz
 * ipsz   - array describing problem size and options
 * nreal  - number of positions in rpsz
 * rpsz   - array of reals describing problem size and options
 * usrmem - user memory
 */
static void slv9_coipsz(int32 *nintg, int32 *ipsz, int32 *nreal, real64 *rpsz,
	                real64 *usrmem)
{
  slv9_system_t sys;

  /*
   * stop gcc whining about unused parameter
   */
  (void)nintg;  (void)nreal;

  sys = (slv9_system_t)usrmem;
  /*
   * Integer array
   */
  ipsz[F2C(1)] = sys->con.n;     /* variables */
  ipsz[F2C(2)] = sys->con.m;     /* constraints including objective */
  ipsz[F2C(3)] = sys->con.nz;    /* non zeros in Jacobian */
  ipsz[F2C(4)] = sys->con.nz - (sys->con.m - 2); /* linear nonzeros */
  ipsz[F2C(5)] = sys->con.m - 2; /* nonlinear nonzeros */
  ipsz[F2C(6)] = -1;             /* direction of optimization min */
  ipsz[F2C(7)] = sys->con.m;     /* objective will be last row     */
  ipsz[F2C(8)] = OPT_ITER_LIMIT; /* iteration limit */
  ipsz[F2C(9)] = DOMLIM;         /* max number of error in func evals */
  ipsz[F2C(10)] = 0;             /* output to file */
  ipsz[F2C(11)] = 1;             /* progress info to screen */
  ipsz[F2C(12)] = 1;             /* correct value of func in coirms */
  ipsz[F2C(13)] = 0;             /* not correct value of jacs in coirms */
  ipsz[F2C(14)] = 0;             /* status not known by modeler */
  ipsz[F2C(15)] = 0;             /* function value include only NL terms */
  ipsz[F2C(16)] = 1;             /* Objective is a constraint */
  ipsz[F2C(17)] = 0;             /* sorted order for jacobian */
  ipsz[F2C(18)] = 0;             /* append the log file after restarts */
  ipsz[F2C(19)] = 0;             /* one subroutine call to coirms */
  ipsz[F2C(20)] = 0;             /* eval subroutine is coifde */
  ipsz[F2C(21)] = 0;             /* no debugging of derivatives */
  ipsz[F2C(22)] = 0;             /* coifde not called for linear eqns */
  /*
   * skipping remainder of ipsz which are fortran io parameters
   */

  /*
   * Real array
   */
  rpsz[F2C(1)] = ASC_INFINITY;       /* infinity */
  rpsz[F2C(2)] = -ASC_INFINITY;      /* -infinity */
  rpsz[F2C(3)] = UNDEFINED;      /* undefined */
  rpsz[F2C(6)] = 0;              /* work space allocated by conopt */
  rpsz[F2C(7)] = TIME_LIMIT;     /* resource limit (time) */
  rpsz[F2C(8)] = 1;              /* initial value for vars if none given */


}
#endif


/**
	Perform CONOPT solution. For the details of what this does in the larger
	context of CMSlv, read (???)

	@TODO document this.

	@see conopt.h
*/
static
void slv_conopt_iterate(slv9_system_t sys){

  if(sys->con.cntvect == NULL){
	sys->con.cntvect = ASC_NEW_ARRAY(int,COIDEF_Size());
  }

  COIDEF_Ini(sys->con.cntvect);

  /*
	We pass pointer to sys as usrmem data.
	Cast back to slv9_system_t to access the information required
  */
  COIDEF_UsrMem(sys->con.cntvect,(double *)sys);

  COIDEF_NumVar(sys->con.cntvect, &(sys->con.n));
  COIDEF_NumCon(sys->con.cntvect, &(sys->con.m)); /* include the obj fn */
  COIDEF_NumNZ(sys->con.cntvect, &(sys->con.nz));
  COIDEF_NumNlNz(sys->con.cntvect, &(sys->con.nlnz));
  COIDEF_OptDir(sys->con.cntvect, &(sys->con.optdir));

  COIDEF_ObjCon(sys->con.cntvect, &(sys->con.objcon)); /* objective will be last row     */
  COIDEF_Base(sys->con.cntvect, &(sys->con.base));
  COIDEF_ErrLim(sys->con.cntvect, &(DOMLIM));
  COIDEF_ItLim(sys->con.cntvect, &(OPT_ITER_LIMIT));

  COIDEF_ReadMatrix(sys->con.cntvect, &slv9_conopt_readmatrix);
  COIDEF_FDEval(sys->con.cntvect, &slv9_conopt_fdeval);
  COIDEF_Option(sys->con.cntvect, &slv9_conopt_option);
  COIDEF_Solution(sys->con.cntvect, &slv9_conopt_solution);
  COIDEF_Status(sys->con.cntvect, &slv9_conopt_status);
  COIDEF_Message(sys->con.cntvect, &asc_conopt_message);
  COIDEF_ErrMsg(sys->con.cntvect, &slv9_conopt_errmsg);
  COIDEF_Progress(sys->con.cntvect, &asc_conopt_progress);

  /** @TODO implement the following options as well... */
#if 0
  ipsz[F2C(10)] = 0;             /* output to file */
  ipsz[F2C(11)] = 1;             /* progress info to screen */
  ipsz[F2C(12)] = 1;             /* correct value of func in coirms */
  ipsz[F2C(13)] = 0;             /* not correct value of jacs in coirms */
  ipsz[F2C(14)] = 0;             /* status not known by modeler */
  ipsz[F2C(15)] = 0;             /* function value include only NL terms */
  ipsz[F2C(16)] = 1;             /* Objective is a constraint */
  ipsz[F2C(17)] = 0;             /* sorted order for jacobian */
  ipsz[F2C(18)] = 0;             /* append the log file after restarts */
  ipsz[F2C(19)] = 0;             /* one subroutine call to coirms */
  ipsz[F2C(20)] = 0;             /* eval subroutine is coifde */
  ipsz[F2C(21)] = 0;             /* no debugging of derivatives */
  ipsz[F2C(22)] = 0;             /* coifde not called for linear eqns */
  /*
   * skipping remainder of ipsz which are fortran io parameters
   */

  /*
   * Real array
   */
  rpsz[F2C(1)] = ASC_INFINITY;       /* infinity */
  rpsz[F2C(2)] = -ASC_INFINITY;      /* -infinity */
  rpsz[F2C(3)] = UNDEFINED;      /* undefined */
  rpsz[F2C(6)] = 0;              /* work space allocated by conopt */
  rpsz[F2C(7)] = TIME_LIMIT;     /* resource limit (time) */
  rpsz[F2C(8)] = 1;              /* initial value for vars if none given */
#endif

/*
 * reset count on coiopt calls
 */
  sys->con.opt_count = 0;

  /*
   * do not keep model in memory after solution
   */
  sys->con.kept = 0;

  COI_Solve(sys->con.cntvect);
  /* conopt_start(&(sys->con.kept), usrmem, &(sys->con.lwork),
	       sys->con.work, &(sys->con.maxusd), &(sys->con.curusd)); */

  /*
   * We assume that we get convergence in optimization problem at
   * boundary
   */
  sys->con.optimized = 1;
}

#endif /* ASC_WITH_CONOPT  */

/*-------------------end of conopt callbacks----------------------------------*/


/*
 * Creates an array of columns (containing an array of real elements
 * each) to storage the linear coefficient matrix of the optimization problem.
 * It also creates the arrays of reals required to storage the values
 * of the gradients of a subregion, which change depending on whether the
 * problem is a simulation or an optimization.
 */
static
void create_opt_matrix_and_vectors(int32 num_opt_eqns,
		int32 n_subregions,
		struct opt_matrix *coeff_matrix,
		struct opt_vector *opt_var_values,
		struct opt_vector *invariant,
		struct opt_vector *variant,
		struct opt_vector *gradient,
		struct opt_matrix *multipliers
){
  int32  c;
  int32 num_vars;

  num_vars = num_opt_eqns - 1 + n_subregions;

  coeff_matrix->cols = ASC_NEW_ARRAY(struct opt_vector,n_subregions);

  if(g_optimizing) {
    multipliers->cols = ASC_NEW_ARRAY(struct opt_vector,n_subregions);
  }

  for (c=0; c<n_subregions; c++) {
    coeff_matrix->cols[c].element = ASC_NEW_ARRAY(real64,num_opt_eqns);
  }
  opt_var_values->element = ASC_NEW_ARRAY(real64,num_vars);

  if(g_optimizing) {
    gradient->element = ASC_NEW_ARRAY(real64,num_opt_eqns);
  }else{
    invariant->element = ASC_NEW_ARRAY(real64,num_opt_eqns);
    variant->element = ASC_NEW_ARRAY(real64,num_opt_eqns);
  }
}


/*
 * destroy the arrays created to storage the gradients for the optimization
 * problem
 */
static
void destroy_opt_matrix_and_vectors(int32 n_subregions,
		struct opt_matrix *coeff_matrix,
		struct opt_vector *opt_var_values,
		struct opt_vector *invariant,
		struct opt_vector *variant,
		struct opt_vector *gradient,
		struct opt_matrix *multipliers
){
  int32  c;
  for (c=0; c<n_subregions; c++) {
    destroy_array(coeff_matrix->cols[c].element);
    if(g_optimizing) {
      destroy_array(multipliers->cols[c].element);
    }
  }
  destroy_array(coeff_matrix->cols);
  destroy_array(opt_var_values->element);
  if(g_optimizing) {
    destroy_array(multipliers->cols);
    destroy_array(gradient->element);
  }else{
    destroy_array(invariant->element);
    destroy_array(variant->element);
  }
}



/*
 * Set Factorization Options
 */
static
void set_factor_options (linsolqr_system_t lsys){
  linsolqr_prep(lsys,linsolqr_fmethod_to_fclass(ranki_ba2));
  linsolqr_set_pivot_zero(lsys, 1e-12);
  linsolqr_set_drop_tolerance(lsys,1e-16);
  linsolqr_set_pivot_tolerance(lsys, 0.1);
  linsolqr_set_condition_tolerance(lsys, 0.1);
}


/*
 * Calculating the Lagrange Multipliers for each subregion
 *
 * We are assuming here that the matrix is structurally nonsingular
 * and than the rank is equal to the number of rows in the matrix.
 * Much more efficient checking must be done.
 */
static
void get_multipliers(SlvClientToken asys,
		int32 subregion,
		int32 nrel,
		real64 *grad_obj,
		struct opt_matrix *multipliers
){
  slv9_system_t sys;
  linsolqr_system_t lsys;
  mtx_region_t  *newblocks, *oneblock;
  int32 rank;
  int32 c, cr, len, row;
  real64 *weights;
  real64 summ;

  sys = SLV9(asys);
  check_system(sys);

  mtx_output_assign(sys->lin_mtx,nrel,nrel);
  rank = mtx_symbolic_rank(sys->lin_mtx);
  mtx_partition(sys->lin_mtx);
  len = mtx_number_of_blocks(sys->lin_mtx);
  newblocks = ASC_NEW_ARRAY(mtx_region_t,len);
  if(newblocks == NULL) {
    mtx_destroy(sys->lin_mtx);
    return;
  }
  for (c = 0 ; c < len; c++) {
    mtx_block(sys->lin_mtx,c,&(newblocks[c]));
  }
  for (c = 0 ; c < len; c++) {
    mtx_reorder(sys->lin_mtx,&(newblocks[c]),mtx_SPK1);
  }

  /* unifying block  */
  oneblock = (mtx_region_t *)ascmalloc(sizeof(mtx_region_t));
  oneblock->row.low = oneblock->col.low = 0;
  oneblock->row.high = nrel-1;
  oneblock->col.high = nrel-1;

  /*
   * Scaling of the linear system
   */

  /*
   *Calculating weights
   */
  weights = ASC_NEW_ARRAY(real64,nrel);
  for (row=0; row<nrel; row++) {
    summ = mtx_sum_sqrs_in_row(sys->lin_mtx,row,&(oneblock->col));
    if(summ <= 0.0) {
      weights[row] = 1.0;
    }else{
      weights[row] = 1.0 / sqrt(summ);
    }
#if DEBUG
     FPRINTF(ASCERR," weight of row %d = %f \n",row,summ);
#endif /* DEBUG */
  }

  /*
   * Dividing rows by weights
   */
  for (row=0; row<nrel; row++) {
    mtx_mult_row(sys->lin_mtx,row,weights[row],&(oneblock->col));
  }

  /*
   * dividing rhs
   */
  for (row=0; row<nrel; row++) {
    grad_obj[mtx_row_to_org(sys->lin_mtx,row)] =
                    grad_obj[mtx_row_to_org(sys->lin_mtx,row)] * weights[row];
  }

  /*
   * End of scaling
   */

  lsys = linsolqr_create();
  linsolqr_set_matrix(lsys,sys->lin_mtx);

  for (cr=0; cr<nrel; cr++) {
    multipliers->cols[subregion].element[cr] = 0.0;
  }

  set_factor_options(lsys);
  /* rhs for multipliers */
  linsolqr_add_rhs(lsys,grad_obj,FALSE);
  linsolqr_set_region(lsys,*oneblock);
  linsolqr_factor(lsys,ranki_ba2);
  linsolqr_solve(lsys,grad_obj);
  for (cr=0; cr<nrel; cr++) {
    multipliers->cols[subregion].element[cr] = linsolqr_var_value
            (lsys,grad_obj,cr);
#if SHOW_LAGRANGE_DETAILS
    FPRINTF(ASCERR, " Row = %d \n",cr);
    FPRINTF(ASCERR,
         "Multiplier = %f \n",multipliers->cols[subregion].element[cr]);
#endif /*  SHOW_LAGRANGE_DETAILS  */
  }
  linsolqr_set_matrix(lsys,NULL);
  mtx_destroy(sys->lin_mtx);
  linsolqr_destroy(lsys);
  destroy_array(newblocks);
  destroy_array(weights);
  ascfree(oneblock);
}


/*
 * Calculate the invariant part of the gradients of the subregions
 */
static
void get_gradient_in_subregion(slv_system_t server,
		SlvClientToken asys,
		int32 subregion,
		int32 num_opt_eqns,
		struct opt_vector *gradient,
		struct opt_matrix *multipliers
){
  slv9_system_t sys;
  struct rel_relation **rlist;
  struct var_variable **vlist;
  struct rel_relation *rel;
  struct var_variable *var;
  var_filter_t vfilter;
  rel_filter_t rfilter;
  mtx_coord_t coord;
  real64 *tmp_value;
  real64 *derivatives, resid;
  real64 *grad_obj, *func_val;
  real64 *f_red_grad;
  struct opt_matrix rel_red_grad;
  int32 *variables_master, *variables_solver, count;
  int32 nvar, nrel, ntotvar, ntotrel;
  int32 countrel,countvar,cr,cv,len,vind;
  int32 nvnb, countnbv;
  FILE *lif;

  sys = SLV9(asys);
  check_system(sys);
  lif = LIF(sys);

  rlist = slv_get_master_rel_list(server);
  vlist = slv_get_master_var_list(server);
  ntotvar = slv_get_num_master_vars(server);
  ntotrel = slv_get_num_master_rels(server);
  tmp_value = ASC_NEW_ARRAY(real64,ntotvar);

  vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_NONBASIC
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);
  nvar = slv_count_master_vars(server,&vfilter);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY | REL_ACTIVE );
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY | REL_ACTIVE );
  nrel = slv_count_master_rels(server,&rfilter);

  if(nrel != nvar) {
    FPRINTF(ASCERR," nrel = %d\n",nrel);
    FPRINTF(ASCERR," nvar = %d\n",nvar);
    FPRINTF(ASCERR,
            "PANIC: number relations does not match number of variables\n");
  }

  /*
   * residual of the relations in the subregion
   */
  func_val = ASC_NEW_ARRAY(real64,nrel);

  /*
   * Lagrange Multipliers of the subregion
   */
  multipliers->cols[subregion].element =
                    (real64 *)(ascmalloc(nrel*sizeof(real64)));
  /*
   * Gradients of the objective function
   */
  grad_obj = ASC_NEW_ARRAY(real64,nvar);

  /*
   * Matrix for solving linear system
   */
  sys->lin_mtx = mtx_create();
  mtx_set_order(sys->lin_mtx,nrel);

  /*
   * Counting nonbasic variables
   */
  vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT | VAR_NONBASIC
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_NONBASIC
	               | VAR_SVAR);
  nvnb = slv_count_master_vars(server,&vfilter);

  /*
   * Information for reduced gradient
   */
  f_red_grad = ASC_NEW_ARRAY(real64,nvnb);
  rel_red_grad.cols = (struct opt_vector *)
                      (ascmalloc(nvnb*sizeof(struct opt_vector)));
  for (cv=0; cv<nvnb; cv++) {
    rel_red_grad.cols[cv].element =
                    (real64 *)(ascmalloc(nrel*sizeof(real64)));
  }

  /*
   * Setting every sindex to -1
   */
  for (cv=0; cv<ntotvar; cv++) {
    var_set_sindex(vlist[cv],-1);
  }
  for (cr=0; cr<ntotrel; cr++) {
    rel_set_sindex(rlist[cr],-1);
  }

  /*
   * Initializing values
   */
  for (cv=0; cv<ntotvar;cv++) {
    tmp_value[cv] = 0.0;
  }
  for (cv=0; cv<num_opt_eqns;cv++) {
    gradient->element[cv] = 0.0;
  }

  for (cr=0; cr<nrel; cr++) {
    func_val[cr] = 0.0;
    multipliers->cols[subregion].element[cr] = 0.0;
  }

  for (cv=0; cv<nvnb; cv++) {
    f_red_grad[cv] = 0.0;
    for (cr=0; cr<nrel; cr++) {
      rel_red_grad.cols[cv].element[cr] = 0.0;
    }
  }

  for (cv=0; cv<nvar; cv++) {
    grad_obj[cv] = 0.0;
  }

  /*
   * Calculate Values
   */
  vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);

  /*
   * List of relations
   */
  countrel = 0;
  countvar = 0;
  countnbv = 0;
  for (cr=0; cr<ntotrel; cr++) {
    rel = rlist[cr];
    if(rel_apply_filter(rel,&rfilter)) {
      rel_set_sindex(rel,countrel);
      coord.col = rel_sindex(rel);
      len = rel_n_incidences(rel);
      variables_master = ASC_NEW_ARRAY(int32,len);
      variables_solver = ASC_NEW_ARRAY(int32,len);
      derivatives = ASC_NEW_ARRAY(real64,len);
      relman_diff_grad(rel,&vfilter,derivatives,variables_master,
		       variables_solver,&count,&resid,1);
      func_val[countrel] = resid;
#if SHOW_LAGRANGE_DETAILS
            FPRINTF(ASCERR,"Equation = %d \n",coord.col);
            FPRINTF(ASCERR,"Residual = %f \n",resid);
#endif /*  SHOW_LAGRANGE_DETAILS  */
      for (cv=0; cv<count;cv++) {
        var = vlist[variables_master[cv]];
        if(!var_nonbasic(var)) {
          tmp_value[variables_master[cv]] = tmp_value[variables_master[cv]] +
                                            derivatives[cv] * RHO * resid;
          if(var_active(var)) {
            coord.row = var_sindex(var);
            if(coord.row == -1) {
              var_set_sindex(var,countvar);
              coord.row = countvar;
              countvar++;
	    }
            assert(coord.col >= 0 && coord.col < mtx_order(sys->lin_mtx));
#if SHOW_LAGRANGE_DETAILS
            FPRINTF(ASCERR,"Coordinate row = %d \n",coord.row);
            FPRINTF(ASCERR,"Coordinate col = %d \n",coord.col);
            FPRINTF(ASCERR,"Derivative = %f \n",derivatives[cv]);
#endif /*  SHOW_LAGRANGE_DETAILS  */
            mtx_fill_org_value(sys->lin_mtx,&coord,derivatives[cv]);
	  }
	}else{
          if(var_sindex(var)== -1) {
            var_set_sindex(var,countnbv);
            countnbv++;
	  }
          rel_red_grad.cols[var_sindex(var)].element[countrel] =
	                                                   derivatives[cv];
#if SHOW_LAGRANGE_DETAILS
          FPRINTF(lif,"Nonbasic Variable ");
          print_var_name(lif,sys,var); PUTC('\n',lif);
          FPRINTF(ASCERR,"Derivative = %f \n",derivatives[cv]);
#endif /*  SHOW_LAGRANGE_DETAILS  */
	}
      }
      destroy_array(variables_master);
      destroy_array(variables_solver);
      destroy_array(derivatives);
      countrel++;
    }
  }

  /*
   * Objective function
   */
  rel = sys->obj;
  len = rel_n_incidences(rel);
  variables_master = ASC_NEW_ARRAY(int32,len);
  variables_solver = ASC_NEW_ARRAY(int32,len);
  derivatives = ASC_NEW_ARRAY(real64,len);
  relman_diff_grad(rel,&vfilter,derivatives,variables_master,
		    variables_solver,&count,&resid,1);
  for (cv=0; cv<count;cv++) {
    var = vlist[variables_master[cv]];
    if(!var_nonbasic(var)) {
#if SHOW_LAGRANGE_DETAILS
      FPRINTF(ASCERR,"Objective row = %d \n",var_sindex(var));
      FPRINTF(ASCERR,"Derivative = %f \n",derivatives[cv]);
#endif /*  SHOW_LAGRANGE_DETAILS  */
      grad_obj[var_sindex(var)] = -1.0 * derivatives[cv];
    }else{
#if SHOW_LAGRANGE_DETAILS
      FPRINTF(ASCERR,"Non Basic Variable = %d \n",var_sindex(var));
      FPRINTF(ASCERR,"Derivative in Objective = %f \n",derivatives[cv]);
#endif /*  SHOW_LAGRANGE_DETAILS  */
      f_red_grad[var_sindex(var)] = derivatives[cv] ;
    }
  }
  destroy_array(variables_master);
  destroy_array(variables_solver);
  destroy_array(derivatives);

  /*
   * Solving Linear System
   */
  get_multipliers(asys,subregion,nrel,grad_obj,multipliers);

  countvar = 0;
  for (cv = 0; cv<ntotvar; cv++) {
    var = vlist[cv];
    if(var_apply_filter(var,&vfilter)) {
      if(!var_nonbasic(var)) {
        gradient->element[countvar] = tmp_value[cv];
        countvar++;
      }else{
	vind = var_sindex(var);
        if((vind != -1) && (vind < nvnb) ) {
          gradient->element[countvar] = f_red_grad[vind];
          for (cr=0; cr<nrel; cr++) {
            gradient->element[countvar] = gradient->element[countvar] +
	      ( rel_red_grad.cols[vind].element[cr] *
	        ( multipliers->cols[subregion].element[cr] +
	          ( RHO * func_val[cr] ) ) );
	  }
          countvar++;
	}
      }
    }
  }
  gradient->element[countvar] = 1.0;

  destroy_array(tmp_value);
  destroy_array(func_val);
  destroy_array(grad_obj);
  destroy_array(f_red_grad);
  for (cv=0; cv<nvnb; cv++) {
    destroy_array(rel_red_grad.cols[cv].element);
  }
  destroy_array(rel_red_grad.cols);

  if(countrel != nrel) {
    FPRINTF(ASCERR,"PANIC: number of invariant relations does not match\n");
  }

  if(countvar != ( num_opt_eqns - 1)) {
    FPRINTF(ASCERR,"PANIC: number of variables does not match at boundary\n");
  }
  /*
  if(countvar != nvar) {
    FPRINTF(ASCERR,"PANIC: number of variables does not match at boundary\n");
  }
  */
}


/*
 * Calculate the invariant part of the norm of the objective function
 */
static
real64 get_augmented_function_in_subregion(slv_system_t server,
					SlvClientToken asys,
					int32 subregion,
				        struct opt_matrix *multipliers)
{
  slv9_system_t sys;
  struct rel_relation **rlist;
  struct rel_relation *rel;
  rel_filter_t rfilter;
  real64 resid, sqrnorm;
  int32 status;
  int32 nrel,ntotrel;
  int32 countrel,cr;

  sys = SLV9(asys);
  check_system(sys);

  rlist = slv_get_master_rel_list(server);
  ntotrel = slv_get_num_master_rels(server);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE);
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE);
  nrel = slv_count_master_rels(server,&rfilter);

  sqrnorm = 0.0;
  countrel = 0;

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif

  for (cr=0; cr<ntotrel; cr++) {
    rel = rlist[cr];
    if(rel_apply_filter(rel,&rfilter)) {
      resid = relman_eval(rel, &status, 1);
      sqrnorm = sqrnorm + (resid *
                           ( multipliers->cols[subregion].element[countrel] +
			     ( (RHO/2) *resid ) ) );
      countrel++;
    }
  }
  rel = sys->obj;
  resid = relman_eval(rel, &status, 1);

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif

  sqrnorm = sqrnorm + resid;

  if(countrel != nrel) {
    FPRINTF(ASCERR,"PANIC: number of invariant relations does not match\n");
  }
  return sqrnorm;
}


/*
 * Calculate the invariant part of the gradients of the subregions
 */
static
void get_invariant_of_gradient_in_subregions(slv_system_t server,
		int32 num_opt_eqns,
		struct opt_vector *invariant
){
  struct rel_relation **rlist;
  struct var_variable **vlist;
  struct rel_relation *rel;
  var_filter_t vfilter;
  rel_filter_t rfilter;
  real64 *tmp_value;
  real64 *derivatives, resid;
  int32 *variables, *varsindex, count;
  int32 nvar, nrel, ntotvar, ntotrel;
  int32 countrel,countvar,cr,cv,len;

  rlist = slv_get_master_rel_list(server);
  vlist = slv_get_master_var_list(server);
  ntotvar = slv_get_num_master_vars(server);
  ntotrel = slv_get_num_master_rels(server);
  tmp_value = ASC_NEW_ARRAY(real64,ntotvar);

  vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
  nvar = slv_count_master_vars(server,&vfilter);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_INVARIANT);
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_INVARIANT);
  nrel = slv_count_master_rels(server,&rfilter);

  for (cv=0; cv<ntotvar;cv++) {
    tmp_value[cv] = 0.0;
  }

  for (cv=0; cv<num_opt_eqns;cv++) {
    invariant->element[cv] = 0.0;
  }

  countrel = 0;
  for (cr=0; cr<ntotrel; cr++) {
    rel = rlist[cr];
    if(rel_apply_filter(rel,&rfilter)) {
      len = rel_n_incidences(rel);
      variables = ASC_NEW_ARRAY(int32,len);
      derivatives = ASC_NEW_ARRAY(real64,len);
      varsindex = ASC_NEW_ARRAY(int32,len);
      relman_diff_grad(rel,&vfilter,derivatives,variables,varsindex,
		       &count,&resid,1);
      for (cv=0; cv<count;cv++) {
        tmp_value[variables[cv]] = tmp_value[variables[cv]] +
                                   derivatives[cv] * resid;
      }
      destroy_array(variables);
      destroy_array(varsindex);
      destroy_array(derivatives);
      countrel++;
    }
  }

  countvar = 0;
  for (cv = 0; cv<ntotvar; cv++) {
    if(var_apply_filter(vlist[cv],&vfilter)) {
      invariant->element[countvar] = tmp_value[cv];
      countvar++;
    }
  }
  invariant->element[countvar] = 1.0;
  destroy_array(tmp_value);

  if(countrel != nrel) {
    FPRINTF(ASCERR,"PANIC: number of invariant relations does not match\n");
  }

  if(countvar != ( num_opt_eqns - 1)) {
    FPRINTF(ASCERR,"PANIC: number of variables does not match at boundary\n");
  }

  if(countvar != nvar) {
    FPRINTF(ASCERR,"PANIC: number of variables does not match at boundary\n");
  }

}


/*
 * Calculate the variant part of the gradients for the current subregion
 */
static
void get_variant_of_gradient_in_subregion(slv_system_t server,
		int32 num_opt_eqns,
		struct opt_vector *variant
){
  struct rel_relation **rlist;
  struct var_variable **vlist;
  struct rel_relation *rel;
  var_filter_t vfilter;
  rel_filter_t rfilter;
  real64 *tmp_value;
  real64 *derivatives, resid;
  int32 *variables, *varsindex, count;
  int32 nvar, nrel, ntotvar, ntotrel;
  int32 countrel,countvar,cr,cv,len;

  rlist = slv_get_master_rel_list(server);
  vlist = slv_get_master_var_list(server);
  ntotvar = slv_get_num_master_vars(server);
  ntotrel = slv_get_num_master_rels(server);
  tmp_value = ASC_NEW_ARRAY(real64,ntotvar);

  vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
  nvar = slv_count_master_vars(server,&vfilter);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_IN_CUR_SUBREGION);
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_IN_CUR_SUBREGION);
  nrel = slv_count_master_rels(server,&rfilter);

  for (cv=0; cv<ntotvar;cv++) {
    tmp_value[cv] = 0.0;
  }

  for (cv=0; cv<num_opt_eqns;cv++) {
    variant->element[cv] = 0.0;
  }

  countrel = 0;
  for (cr=0; cr<ntotrel; cr++) {
    rel = rlist[cr];
    if(rel_apply_filter(rel,&rfilter)) {
      len = rel_n_incidences(rel);
      variables = ASC_NEW_ARRAY(int32,len);
      derivatives = ASC_NEW_ARRAY(real64,len);
      varsindex = ASC_NEW_ARRAY(int32,len);
      relman_diff_grad(rel,&vfilter,derivatives,variables,varsindex,
		       &count,&resid,1);
      for (cv=0; cv<count;cv++) {
        tmp_value[variables[cv]] = tmp_value[variables[cv]] +
                                   derivatives[cv] * resid;
      }
      destroy_array(variables);
      destroy_array(varsindex);
      destroy_array(derivatives);
      countrel++;
    }
  }

  countvar = 0;
  for (cv = 0; cv<ntotvar; cv++) {
    if(var_apply_filter(vlist[cv],&vfilter)) {
      variant->element[countvar] = tmp_value[cv];
      countvar++;
    }
  }
  variant->element[countvar] = 0.0;
  destroy_array(tmp_value);

  if(countrel != nrel) {
    FPRINTF(ASCERR,"PANIC: number of variant relations does not match\n");
  }

  if(countvar != ( num_opt_eqns - 1)) {
    FPRINTF(ASCERR,"PANIC: number of variables does not match at boundary\n");
  }

  if(countvar != nvar) {
    FPRINTF(ASCERR,"PANIC: number of variables does not match at boundary\n");
  }

}

/*
 * Calculate the invariant part of the norm of the objective function
 */
static
real64 get_invariant_of_obj_norm_in_subregions(slv_system_t server){
  struct rel_relation **rlist;
  struct rel_relation *rel;
  rel_filter_t rfilter;
  real64 resid, sqrnorm;
  int32 status;
  int32 nrel,ntotrel;
  int32 countrel,cr;

  rlist = slv_get_master_rel_list(server);
  ntotrel = slv_get_num_master_rels(server);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_INVARIANT);
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_INVARIANT);
  nrel = slv_count_master_rels(server,&rfilter);

  sqrnorm = 0.0;
  countrel = 0;

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif

  for (cr=0; cr<ntotrel; cr++) {
    rel = rlist[cr];
    if(rel_apply_filter(rel,&rfilter)) {
      resid = relman_eval(rel, &status, 1);
      sqrnorm = sqrnorm + (resid * resid);
      countrel++;
    }
  }

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif

  if(countrel != nrel) {
    FPRINTF(ASCERR,"PANIC: number of invariant relations does not match\n");
  }
  return sqrnorm;
}


/*
 * Calculate the variant part of the norm of the objective function for a
 * particular subregion
 */
static
real64 get_variant_of_obj_norm_in_subregion(slv_system_t server){
  struct rel_relation **rlist;
  struct rel_relation *rel;
  rel_filter_t rfilter;
  real64 resid, sqrnorm;
  int32 status;
  int32 nrel,ntotrel;
  int32 countrel,cr;

  rlist = slv_get_master_rel_list(server);
  ntotrel = slv_get_num_master_rels(server);

  rfilter.matchbits = (REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_IN_CUR_SUBREGION);
  rfilter.matchvalue =(REL_INCLUDED | REL_EQUALITY
		       | REL_ACTIVE | REL_IN_CUR_SUBREGION);
  nrel = slv_count_master_rels(server,&rfilter);

  sqrnorm = 0.0;
  countrel = 0;

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif

  for (cr=0; cr<ntotrel; cr++) {
    rel = rlist[cr];
    if(rel_apply_filter(rel,&rfilter)) {
      resid = relman_eval(rel, &status, 1);
      sqrnorm = sqrnorm + (resid * resid);
      countrel++;
    }
  }

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif

  if(countrel != nrel) {
    FPRINTF(ASCERR,"PANIC: number of variant relations does not match\n");
  }
  return sqrnorm;
}


/*
 * Fill a column of the coefficient matrix used for the optimization
 * problem at a boundary
 */
static
void fill_opt_matrix_cols_with_vectors(int32 num_opt_eqns, int32 n,
		struct opt_matrix *coeff_matrix,
		struct opt_vector *invariant,
		struct opt_vector *variant,
		struct opt_vector *gradient
){
  int32 num_eqn;
  real64 norm2;

  norm2 = 0.0;
  for(num_eqn=0; num_eqn<num_opt_eqns; num_eqn++) {
    if(g_optimizing) {
      coeff_matrix->cols[n].element[num_eqn] = gradient->element[num_eqn];
    }else{
      coeff_matrix->cols[n].element[num_eqn] = invariant->element[num_eqn] +
                                                 variant->element[num_eqn];
    }
    if(num_eqn < (num_opt_eqns - 1) ) {
#if SHOW_OPTIMIZATION_DETAILS
      if(g_optimizing) {
        FPRINTF(ASCERR," gradient = %f \n", gradient->element[num_eqn]);
      }else{
        FPRINTF(ASCERR," variant = %f \n", variant->element[num_eqn]);
        FPRINTF(ASCERR," invariant = %f \n", invariant->element[num_eqn]);
      }
#endif /* SHOW_OPTIMIZATION_DETAILS */
     norm2 = norm2 + ( coeff_matrix->cols[n].element[num_eqn] *
                      coeff_matrix->cols[n].element[num_eqn] );
    }
  }

  norm2 = sqrt(norm2);

  for(num_eqn=0; num_eqn<num_opt_eqns-1; num_eqn++) {

#if SHOW_OPTIMIZATION_DETAILS
    FPRINTF(ASCERR," coefficient before normalize = %f \n",
	    coeff_matrix->cols[n].element[num_eqn]);
#endif /* SHOW_OPTIMIZATION_DETAILS */
    coeff_matrix->cols[n].element[num_eqn] =
                    coeff_matrix->cols[n].element[num_eqn] / norm2;
#if SHOW_OPTIMIZATION_DETAILS
    FPRINTF(ASCERR," coefficient = %f \n",
	    coeff_matrix->cols[n].element[num_eqn]);
#endif /* SHOW_OPTIMIZATION_DETAILS */
  }
}



/*
 * Analyzes the result of the optimization problem.
 * Adds to each var value the var step given by the optimization problem
 * at a boundary.
 * It projects a variable to its bounds if required.
 */
static
void apply_optimization_step(slv_system_t server, SlvClientToken asys,
		int32 n_subregions,
		struct opt_vector *values,
		real64 factor,
		struct real_values *rvalues
){
  slv9_system_t sys;
  struct var_variable **vlist;
  struct var_variable *var;
  var_filter_t vfilter;
  int32 totvars, num_vars, num_tot, c, count;
  real64 nominal, up, low, pre_val;
  real64 value, dx, test_value,norm2;
  FILE *lif;

  sys = SLV9(asys);
  check_system(sys);
  lif = LIF(sys);

  vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
  vlist = slv_get_master_var_list(server);
  num_vars = slv_count_master_vars(server,&vfilter);
  totvars = slv_get_num_master_vars(server);

  count = 0;
  norm2 = 0.0;
  for (c=0; c<totvars; c++) {
    var = vlist[c];
    if(var_apply_filter(var,&vfilter)) {
      norm2 = norm2 + ( values->element[count] * values->element[count] );
      count++;
    }
  }
  norm2  = sqrt(norm2);

  count = 0;
  for (c=0; c<totvars; c++) {
    var = vlist[c];
    pre_val = rvalues->pre_values[c];
    if(var_apply_filter(var,&vfilter)) {
      nominal = var_nominal(var);
      low = var_lower_bound(var);
      up = var_upper_bound(var);
      dx = factor * ( values->element[count] / norm2 );
#if SHOW_OPTIMIZATION_DETAILS
      FPRINTF(lif,"Variable ");
      print_var_name(lif,sys,var); PUTC('\n',lif);
      FPRINTF(lif,"dx = %f\n",dx);
#endif /* SHOW_OPTIMIZATION_DETAILS */
      test_value = pre_val + dx;
      if((test_value < low) || (test_value > up) ) {
        if(test_value < low) {
          value = low;
          if(SHOW_LESS_IMPT) {
             FPRINTF(lif,"%-40s ---> ",
                         "    Variable projected to lower bound");
             print_var_name(lif,sys,var); PUTC('\n',lif);
          }
	}else{
          value = up;
          if(SHOW_LESS_IMPT) {
             FPRINTF(lif,"%-40s ---> ",
                         "    Variable projected to upper bound");
             print_var_name(lif,sys,var); PUTC('\n',lif);
          }
	}
      }else{
        value = test_value;
      }
      var_set_value(var,value);
#if SHOW_OPTIMIZATION_DETAILS
             FPRINTF(lif,"value = %f\n",value);
#endif /* SHOW_OPTIMIZATION_DETAILS */
      count++;
    }
  }
  /*
   * num_tot is to stop gcc whining about unused parameters
   */
  num_tot = num_vars+n_subregions;

#if DEBUG
  for(c=count; c<num_tot; c++) {
    FPRINTF(ASCERR," coefficient of subregion %d = %f \n",
	    c-count+1,values->element[c]);
  }
#endif /* DEBUG */
}


/*
 * Creates the problem at a boundary and call the appropriate CONOPT
 * subroutines to perform the optimization problem.
 */
static
int32 optimize_at_boundary(slv_system_t server, SlvClientToken asys,
		int32 *n_subregions,
		struct matching_cases *subregions,
		int32 *cur_subregion,
		struct gl_list_t *disvars,
		struct real_values *rvalues
){
  slv9_system_t sys;
  struct rel_relation **rlist;
  struct var_variable **vlist;
  struct opt_matrix coeff_matrix;
  struct opt_vector opt_var_values;
  struct opt_vector invariant_vect_values;
  struct opt_vector variant_vect_values;
  struct opt_vector gradient;
  struct opt_matrix multipliers;
  var_filter_t vfilter;
  int32 num_vars,num_opt_eqns, num_opt_vars;
  int32 n, return_value, niter;
  int32 global_decrease, red_step;
  real64 obj_val=0.0, factor;
  real64 invnorm=0.0, *varnorm, *testnorm;
  int32 ntotvar, ntotrel, cr, cv;
  int32 *var_ind, *rel_ind;

#if SHOW_OPTIMIZATION_DETAILS
  int32 nc;  /* stop gcc whining about unused variables */
#endif

  sys = SLV9(asys);
  check_system(sys);

  rlist = slv_get_master_rel_list(server);
  vlist = slv_get_master_var_list(server);
  ntotvar = slv_get_num_master_vars(server);
  ntotrel = slv_get_num_master_rels(server);
  /*
   * keep current sindex of variables and relations
   */
  var_ind = ASC_NEW_ARRAY(int32,ntotvar);
  rel_ind = ASC_NEW_ARRAY(int32,ntotrel);
  for (cv=0; cv<ntotvar; cv++) {
    var_ind[cv] = var_sindex(vlist[cv]);
  }
  for (cr=0; cr<ntotrel; cr++) {
    rel_ind[cr] = rel_sindex(rlist[cr]);
  }


  set_active_vars_at_bnd(server,disvars);
  vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
  num_vars = slv_count_master_vars(server,&vfilter);
  num_opt_eqns = num_vars + 1;
  num_opt_vars = num_vars + (*n_subregions);

  create_opt_matrix_and_vectors(num_opt_eqns,(*n_subregions),&coeff_matrix,
				&opt_var_values,&invariant_vect_values,
				&variant_vect_values,&gradient,&multipliers);

  identify_invariant_rels_at_bnd(server,disvars);

  if(!g_optimizing) {
    get_invariant_of_gradient_in_subregions(server,num_opt_eqns,
					    &invariant_vect_values);
  }

  for (n=0;n<(*n_subregions);n++) {
#if SHOW_OPTIMIZATION_DETAILS
    FPRINTF(ASCERR, "subregion = %d \n",n+1);
    for (nc=0; nc<subregions[n].ncases; nc++) {
      FPRINTF(ASCERR, "case %d = %d \n",nc+1,subregions[n].case_list[nc]);
    }
#endif /* SHOW_OPTIMIZATION_DETAILS */
    set_active_rels_in_subregion(server,subregions[n].case_list,
				 subregions[n].ncases,disvars);
    set_active_vars_in_subregion(server);
    identify_variant_rels_in_subregion(server);
    if(g_optimizing) {
      get_gradient_in_subregion(server,asys,n,num_opt_eqns,
			        &gradient,&multipliers);
    }else{
      get_variant_of_gradient_in_subregion(server,num_opt_eqns,
					   &variant_vect_values);
    }
    fill_opt_matrix_cols_with_vectors(num_opt_eqns,n,&coeff_matrix,
				&invariant_vect_values,&variant_vect_values,
				&gradient);
  }

  sys->coeff_matrix = &coeff_matrix;
  sys->opt_var_values = &opt_var_values;
  sys->subregions = (*n_subregions);

#ifdef ASC_WITH_CONOPT
  /* CONOPT parameters */
  sys->con.n = num_opt_vars;
  sys->con.m = num_opt_eqns + 1;  /*including objective function */
  sys->con.objcon = num_opt_eqns; /* last row is the objective fn */
  sys->con.nz = (num_opt_eqns * sys->subregions) + 2 * num_vars;
  /* sys->con.nlnz = sys->con.nz - (num_opt_eqns - 1); */
  sys->con.nlnz = num_opt_vars - sys->subregions;
  sys->con.base = 0; /* C calling convention */
  sys->con.optdir = -1; /* minimisation */

  CONSOLE_DEBUG("%d vars, %d rows",sys->con.n,sys->con.m);
  CONSOLE_DEBUG("objective constraint: %d",sys->con.objcon);
  CONSOLE_DEBUG("nonzeros: %d",sys->con.nz);
  CONSOLE_DEBUG("nonlinear nonzeros: %d",sys->con.nlnz);

  /* Perform optimisation using CONOPT */
  slv_conopt_iterate(sys);
  obj_val = sys->con.obj;

#if DEBUG
  FPRINTF(ASCERR," objective function = %f \n",obj_val);
#endif /* DEBUG */

#endif /* ASC_WITH_CONOPT */

  /*
   * Analyze and apply CONOPT step
   */

  if(fabs(obj_val) > OBJ_TOL) {

    return_value = 1;

    varnorm = (real64 *)ascmalloc((*n_subregions)*sizeof(real64));

    identify_invariant_rels_at_bnd(server,disvars);

    if(!g_optimizing) {
      invnorm = get_invariant_of_obj_norm_in_subregions(server);
    }

#if SHOW_LINEAR_SEARCH_DETAILS
    FPRINTF(ASCERR,"Norms of subregions before gradient step:\n");
#endif /*  SHOW_LINEAR_SEARCH_DETAILS */

    for (n=0;n<(*n_subregions);n++) {
      varnorm[n] = 0.0;
      set_active_rels_in_subregion(server,subregions[n].case_list,
				   subregions[n].ncases,disvars);
      set_active_vars_in_subregion(server);
      identify_variant_rels_in_subregion(server);
      if(g_optimizing) {
        varnorm[n] = get_augmented_function_in_subregion(server,asys,n,
						        &multipliers);
      }else{
        varnorm[n] = get_variant_of_obj_norm_in_subregion(server);
        varnorm[n] = varnorm[n] + invnorm;
        varnorm[n] = sqrt(varnorm[n]);
      }
#if SHOW_LINEAR_SEARCH_DETAILS
      FPRINTF(ASCERR,"Norm of subregion %d = %f \n",n+1,varnorm[n]);
#endif /*  SHOW_LINEAR_SEARCH_DETAILS */
    }

    global_decrease = 0;
    niter = 0;
    factor = LINEAR_SEARCH_FACTOR;

#if SHOW_LINEAR_SEARCH_DETAILS
    FPRINTF(ASCERR,"Initial factor in linear search = %f \n",factor);
#endif /*  SHOW_LINEAR_SEARCH_DETAILS */

    testnorm = (real64 *)ascmalloc((*n_subregions)*sizeof(real64));

    while (global_decrease == 0) {
      niter++;
      if(niter > ITER_BIS_LIMIT) {
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Could not reduce the residuals of all the neighboring subregions.");
        return_value = 0;
        break;
      }

      if((factor*factor*obj_val) < OBJ_TOL) {
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Could not reduce the residuals of all the neighboring subregions.");
        return_value = 0;
        break;
      }

      apply_optimization_step(server,asys,*n_subregions,
			      sys->opt_var_values,factor,rvalues);
      identify_invariant_rels_at_bnd(server,disvars);
      if(!g_optimizing) {
        invnorm = get_invariant_of_obj_norm_in_subregions(server);
      }

      for (n=0;n<(*n_subregions);n++) {
        testnorm[n] = 0.0;
        set_active_rels_in_subregion(server,subregions[n].case_list,
				     subregions[n].ncases,disvars);
        identify_variant_rels_in_subregion(server);

        if(g_optimizing) {
          testnorm[n] = get_augmented_function_in_subregion(server,asys,n,
							    &multipliers);
	}else{
          testnorm[n] = get_variant_of_obj_norm_in_subregion(server);
          testnorm[n] = testnorm[n] + invnorm;
          testnorm[n] = sqrt(testnorm[n]);
	}
      }

      red_step = 0;
      for (n=0;n<(*n_subregions);n++) {
        if(testnorm[n] > varnorm[n]) {
          factor = 0.5 * factor;
          red_step = 1;
#if SHOW_LINEAR_SEARCH_DETAILS
          FPRINTF(ASCERR,"Subregion %d :\n",n+1);
          FPRINTF(ASCERR,"Norm after gradient step > Norm  before step\n");
          FPRINTF(ASCERR," %f > %f\n",testnorm[n],varnorm[n]);
          FPRINTF(ASCERR,"New factor = %f \n",factor);
#endif /*  SHOW_LINEAR_SEARCH_DETAILS */
          break;
	}
      }

      if(!red_step) {
        global_decrease = 1;
#if SHOW_LINEAR_SEARCH_DETAILS
        FPRINTF(ASCERR,"factor accepted \n");
        FPRINTF(ASCERR,"factor in linear search = %f \n",factor);
        FPRINTF(ASCERR,"\n");
#endif /*  SHOW_LINEAR_SEARCH_DETAILS */
      }
    }
   /*
    * destroy arrays containing the two norm of the subregion
    */
    destroy_array(varnorm);
    destroy_array(testnorm);
  }else{
    return_value = 0;
  }

  /*
   * Returning to initial configuration
   */
  set_active_rels_in_subregion(server,subregions[(*cur_subregion)].case_list,
			       subregions[(*cur_subregion)].ncases,disvars);
  set_active_vars_in_subregion(server);
  identify_variant_rels_in_subregion(server);

  /*
   * Assigning initial value of sindex for variables and relations
   */
  for (cv=0; cv<ntotvar; cv++) {
    var_set_sindex(vlist[cv],var_ind[cv]);
  }
  for (cr=0; cr<ntotrel; cr++) {
    rel_set_sindex(rlist[cr],rel_ind[cr]);
  }

  /*
   * destroy matrix, arrays of reals containing gradients, the
   * list of cases for each subregion and the array subregion.
   */

  destroy_opt_matrix_and_vectors((*n_subregions),&coeff_matrix,
                                 &opt_var_values,&invariant_vect_values,
				 &variant_vect_values,&gradient,
				 &multipliers);
  sys->coeff_matrix = NULL;
  if((*n_subregions) > 0) {
    for(n=0; n<(*n_subregions);n++) {
      destroy_array(subregions[n].case_list);
    }
  }
  destroy_array(subregions);
  destroy_array(var_ind);
  destroy_array(rel_ind);
  return return_value;
}


/*
 *  Iteration begin/end routines
 *  ----------------------------
 *     iteration_begins(sys)
 *     iteration_ends(sys)
 */

/*
 *  Prepares sys for entering an iteration, increasing the iteration counts
 *  and starting the clock.
 */
static
void iteration_begins(slv9_system_t sys){
   sys->clock = tm_cpu_time();
   ++(sys->s.block.iteration);
   ++(sys->s.iteration);
   if(SHOW_LESS_IMPT&& (sys->s.block.current_size >1 )) {
     FPRINTF(LIF(sys),"\n%-40s ---> %d\n",
             "Iteration", sys->s.block.iteration);
     FPRINTF(LIF(sys),"%-40s ---> %d\n",
             "Total iteration", sys->s.iteration);
   }
}

/*
 *  Prepares sys for exiting an iteration, stopping the clock and recording
 *  the cpu time.
 */
static
void iteration_ends( slv9_system_t sys){
   double cpu_elapsed;   /* elapsed this iteration */

   cpu_elapsed = (double)(tm_cpu_time() - sys->clock);
   sys->s.block.cpu_elapsed += cpu_elapsed;
   sys->s.cpu_elapsed += cpu_elapsed;
   if(SHOW_LESS_IMPT && (sys->s.block.current_size >1 )) {
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Elapsed time", sys->s.block.cpu_elapsed);
     FPRINTF(LIF(sys),"%-40s ---> %g\n",
            "Total elapsed time", sys->s.cpu_elapsed);
   }
}


/*
 *  Updates the solver status.
 */
static
void update_status( slv9_system_t sys){
  boolean unsuccessful;

  if(!sys->s.converged ) {
    sys->s.time_limit_exceeded = (sys->s.block.cpu_elapsed >= TIME_LIMIT);
    sys->s.iteration_limit_exceeded = (sys->s.block.iteration >= ITER_LIMIT);
   }

  unsuccessful = sys->s.diverged || sys->s.inconsistent ||
     sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded;

  sys->s.ready_to_solve = !unsuccessful && !sys->s.converged;
  sys->s.ok = !unsuccessful && sys->s.calc_ok && !sys->s.struct_singular;
}


/*
 *  Updates the value of the flag unsuccessful based on the information
 *  of the nonlinear solver (square or optimizer)
 */
static
boolean update_unsuccessful( slv9_system_t sys, slv_status_t *status){
  boolean unsuccessful;

  sys->s.time_limit_exceeded = (sys->s.block.cpu_elapsed >= TIME_LIMIT);
  sys->s.iteration_limit_exceeded = (sys->s.block.iteration >= ITER_LIMIT);

   unsuccessful = status->diverged || status->inconsistent ||
      sys->s.iteration_limit_exceeded || sys->s.time_limit_exceeded;

   return unsuccessful;
}


/*
 *  Updates structural information
 */
static
void update_struct_info( slv9_system_t sys, slv_status_t *status){
  sys->s.over_defined = status->over_defined;
  sys->s.under_defined = status->under_defined;
  sys->s.struct_singular = status->struct_singular;
}


/*
 * Updates the values of the block information in the conditional
 * solver (main) based on the information of the nonlinear solver (slave:
 * square solver or optimizer).
 * We definitely have to find a better way of communicating status
 * among solvers. I think the structure of the slv_status would have to be
 * modified accordingly to the need of each solver, however, the GUI is
 * completely dependent of the current structure, so I did not modify
 * that structure at all.
 */
static
void update_real_status(slv_status_t *main, slv_status_t *slave, int32 niter){
    main->block.number_of = slave->block.number_of;
    main->costsize = 1+slave->block.number_of;
    main->block.residual = slave->block.residual;
    main->block.current_size = slave->block.current_size;
    main->block.current_block = slave->block.current_block;
    if(niter ==1 ) {
      main->block.iteration =  slave->block.iteration;
    }
    main->block.previous_total_size = slave->block.previous_total_size;
}

/*
 *  Parameters assignment
 *  ----------------------------
 */
static
int32 slv9_get_default_parameters(slv_system_t server,
		SlvClientToken asys,
		slv_parameters_t *parameters
){
  slv9_system_t sys = NULL;
  union parm_arg lo,hi,val;
  struct slv_parameter *new_parms = NULL;
  int32 make_macros = 0;
  static char *logical_names[] = {
    "LRSlv"
  };
  static char *nonlinear_names[] = {
    "QRSlv"
  };
  static char *optimization_names[] = {
    "CONOPT"
  };

  if(server != NULL && asys != NULL) {
    sys = SLV9(asys);
    make_macros = 1;
  }

  if(parameters->parms == NULL) {
   /* an external client wants our parameter list.
     * an instance of slv9_system_structure has this pointer
     * already set in slv9_create
     */
    new_parms = (struct slv_parameter *)
    ascmalloc((slv9_PA_SIZE)*sizeof(struct slv_parameter));
    if(new_parms == NULL) {
      return -1;
    }

    parameters->parms = new_parms;
    parameters->dynamic_parms = 1;
  }
  parameters->num_parms = 0;

  /* begin defining parameters */

  slv_define_parm(parameters, char_parm,
	       "logsolvers", "logical solver", "logical solver",
	       U_p_string(val,logical_names[0]),
	       U_p_strings(lo,logical_names),
	       U_p_int(hi,sizeof(logical_names)/sizeof(char *)),1);
  SLV_CPARM_MACRO(LOGSOLVER_OPTION_PTR,parameters);

  slv_define_parm(parameters, char_parm,
	       "nlsolvers", "nonlinear solver", "nonlinear solver",
	       U_p_string(val,nonlinear_names[0]),
	       U_p_strings(lo,nonlinear_names),
	       U_p_int(hi,sizeof(nonlinear_names)/sizeof(char *)),1);
  SLV_CPARM_MACRO(NONLISOLVER_OPTION_PTR,parameters);

  slv_define_parm(parameters, char_parm,
	       "optsolvers", "optimization solver", "optimization solver",
	       U_p_string(val,optimization_names[0]),
	       U_p_strings(lo,optimization_names),
	       U_p_int(hi,sizeof(optimization_names)/sizeof(char *)),1);
  SLV_CPARM_MACRO(OPTSOLVER_OPTION_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "timelimit", "time limit (CPU sec/block)",
               "time limit (CPU sec/block)",
	       U_p_int(val,1500),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(TIME_LIMIT_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "iterationlimit", "max iterations/block",
                "max iterations/block",
	       U_p_int(val, 30),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(ITER_LIMIT_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	          "iterationbislimit",
		  "max iterations in bisection for boundaries",
                  "max iterations in bisection for boundaries",
	          U_p_int(val, 50),U_p_int(lo, 1),U_p_int(hi,20000),1);
  SLV_IPARM_MACRO(ITER_BIS_LIMIT_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "toosmall", "default for zero nominal",
               "default for zero nominal",
	       U_p_real(val, 1e-8),U_p_real(lo, 1e-12),U_p_real(hi,1.0), 1);
  SLV_RPARM_MACRO(TOO_SMALL_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "linearfactor", "initial factor for linear search",
               "initial factor for linear search",
	       U_p_real(val, 0.01),U_p_real(lo, 1e-6),U_p_real(hi,1.0), 1);
  SLV_RPARM_MACRO(LINEAR_SEARCH_FACTOR_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "showmoreimportant", "showmoreimportant", "showmoreimportant",
	       U_p_bool(val,1),U_p_bool(lo,0),U_p_bool(hi,1),-1);
  SLV_BPARM_MACRO(SHOW_MORE_IMPT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "showlessimportant", "detailed solving info",
               "detailed solving info",
	       U_p_bool(val, 0),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(SHOW_LESS_IMPT_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "autoresolve", "auto-resolve", "auto-resolve",
	       U_p_bool(val,1),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(AUTO_RESOLVE_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "rho", "penalty parameter for optimization",
	       "penalty parameter",
	       U_p_real(val,1),U_p_real(lo, 0),U_p_real(hi,10e100), 3);
  SLV_RPARM_MACRO(RHO_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "undefined", "real considered as undefined by optimizer",
               "real considered as undefined",
	       U_p_real(val, 1.2e20),U_p_real(lo, 0),U_p_real(hi,1.5e20), 3);
  SLV_RPARM_MACRO(UNDEFINED_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	          "errlim",
                  "maximum number of function errors in optimizer",
                  "limit on function evaluation errors",
	          U_p_int(val,6),U_p_int(lo,0),U_p_int(hi,MAX_INT),3);
  SLV_IPARM_MACRO(DOMLIM_PTR,parameters);

  slv_define_parm(parameters, int_parm,
	       "optiterlimit", "LFITER",
	       "maximum number of iterations for optimizer",
	       U_p_int(val, 100),U_p_int(lo, 1),U_p_int(hi,MAX_INT),3);
  SLV_IPARM_MACRO(OPT_ITER_LIMIT_PTR,parameters);

  ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Default value of RTMAX = %g",CONOPT_BOUNDLIMIT);

  slv_define_parm(parameters, real_parm,
	       "infinity","RTMAXV","internal value of infinity",
	       U_p_real(val,CONOPT_BOUNDLIMIT),U_p_real(lo,10),U_p_real(hi,MAX_REAL),3);
  SLV_RPARM_MACRO(INFINITY_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "objtol","RTOBJR",
               "relative objective tolerance in optimization step",
	       U_p_real(val,1e-13),U_p_real(lo,0),U_p_real(hi,1),3);
  SLV_RPARM_MACRO(OBJ_TOL_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "maxjac","RTMAXJ",
	       "maximum derivative in optimization step"
	       ,U_p_real(val,1e5),U_p_real(lo,10),U_p_real(hi,MAX_REAL),3);
  SLV_RPARM_MACRO(RTMAXJ_PTR,parameters);

  slv_define_parm(parameters, real_parm,
	       "hessian_ub","RTMXJ2",
               "upper bound on 2nd derivatives in optimization step",
	       U_p_real(val,1e4),U_p_real(lo,0),U_p_real(hi,MAX_REAL),3);

  slv_define_parm(parameters, real_parm,
	       "maxfeastol", "RTNWMA",
	       "max residual considered feasible in optimization step",
	       U_p_real(val, 1e-3),U_p_real(lo, 1e-13),U_p_real(hi,10e10),3);

  slv_define_parm(parameters, real_parm,
	       "minfeastol", "RTNWMI",
               "residuals below this considered feasible in optimization step",
	       U_p_real(val, 4e-10),U_p_real(lo, 1e-20),U_p_real(hi,10e10),3);

  slv_define_parm(parameters, real_parm,
	       "oneDsearch","RTONED",
	       "accuracy of one dimensional search in optimization step"
	       ,U_p_real(val,0.2),U_p_real(lo,0.1),U_p_real(hi,0.7),3);

  slv_define_parm(parameters, real_parm,
	       "stepmult","RVSTLM",
               "steplength multiplier in optimization step",
	       U_p_real(val,4),U_p_real(lo,0),U_p_real(hi,MAX_REAL),3);

  slv_define_parm(parameters, real_parm,
	       "pivottol","RTPIVA",
               "absolute pivot tolerance in optimization step",
	       U_p_real(val,1e-7),U_p_real(lo,1e-15),U_p_real(hi,1),3);

  slv_define_parm(parameters, real_parm,
	       "pivtolrel","RTPIVR",
               "relative pivot tolerance in optimization step",
	       U_p_real(val,0.05),U_p_real(lo,0),U_p_real(hi,1),3);

  slv_define_parm(parameters, real_parm,
	       "opttol","RTREDG",
               "optimality tolerance in optimization step",
	       U_p_real(val,2e-5),U_p_real(lo,0),U_p_real(hi,MAX_REAL),3);

  return 1;
}

/*
 *  External routines
 *  -----------------
 */

/*
 * Create the tokens for the nonlinear solver and the logical solver.
 * The token of the conditional solver will be assigned until the
 * end slv9_create, which calls this function. Regarding the optimizer,
 * we use CONOPT in two different ways.  We are using only calls
 * for solving optmization at aboundary, but we can also use a token
 * created by slv8.c if the problem is itself an optimization problem.
 * The vars and rels for the optimization problem at the boundary do
 * not correspond to the vars of the slv, and therefore we have to
 * create the data and calculate the gradients and residuals on the
 * fly. In order to check for the existence of CONOPT, we look
 * for the registration number of slv8.c.  Here we are assuming that
 * slv8 was registred only if CONOPT is available.
 *
 * This function will return 0 if successful. If some of the solvers
 * required by the nonlinear, logical or optimization steps are not
 * available, the function will return 1, and the system will not be
 * created.
 */
static
int32 get_solvers_tokens(  slv9_system_t sys, slv_system_t server){
  int32 newsolver;
  int32 num_log_reg, num_nl_reg, num_opt_reg, num_cond_reg;
  int32 num_solvers,si;
  char *param;
  union param_value u;

  num_solvers = slv_number_of_solvers;
  /*
   * Initally all registration indeces equal to -1
   */
  num_log_reg = -1;
  num_nl_reg = -1;
  num_opt_reg = -1;
  num_cond_reg = -1;

  /* registration number of logical solver  */
  for (si=0; si < num_solvers; si++) {
    if(strcmp(LOGSOLVER_OPTION,slv_solver_name(si)) == 0) {
      num_log_reg = si;
      break;
    }
  }

  /* registration number of nonlinear solver  */
  for (si=0; si < num_solvers; si++) {
    if(strcmp(NONLISOLVER_OPTION,slv_solver_name(si)) == 0) {
      num_nl_reg = si;
      break;
    }
  }

  /*
   * registration number of optimization solver.
   */
  for (si=0; si < num_solvers; si++) {
    if(strcmp(OPTSOLVER_OPTION,slv_solver_name(si)) == 0) {
      num_opt_reg = si;
      break;
    }
  }

  /* registration number of current conditional solver  */
  for (si=0; si < num_solvers; si++) {
    if(strcmp("CMSlv",slv_solver_name(si)) == 0) {
      num_cond_reg = si;
      break;
    }
  }

  /*
   * Check if all the solver required are available. If they are not,
   * this solver's system will not be created.
   */
  if(num_log_reg == -1 ) {
    FPRINTF(ASCERR,"Solver %s not available\n",LOGSOLVER_OPTION);
    return 1;
  }

  if(num_nl_reg == -1 ) {
    FPRINTF(ASCERR,"Solver %s not available\n",NONLISOLVER_OPTION);
    return 1;
  }

  if(num_opt_reg == -1 ) {
    FPRINTF(ASCERR,"Solver %s not available\n",OPTSOLVER_OPTION);
    return 1;
  }

  if(num_cond_reg == -1 ) {
    FPRINTF(ASCERR,"Solver CMSlv was not registered\n");
    return 1;
  }

  /*
   * Create solver tokens
   */
  solver_index[CONDITIONAL_SOLVER] = num_cond_reg;
  newsolver = slv_switch_solver(server,num_log_reg);
  token[LOGICAL_SOLVER] = slv_get_client_token(server);
  solver_index[LOGICAL_SOLVER] = slv_get_selected_solver(server);
  newsolver = slv_switch_solver(server,num_nl_reg);
  token[NONLINEAR_SOLVER] = slv_get_client_token(server);
  solver_index[NONLINEAR_SOLVER] = slv_get_selected_solver(server);
  newsolver = slv_switch_solver(server,num_opt_reg);
  token[OPTIMIZATION_SOLVER] = slv_get_client_token(server);
  solver_index[OPTIMIZATION_SOLVER] = slv_get_selected_solver(server);


  /*
   * Disabling the partition mode flag in the non linear solver.
   * PARTITION is a boolean parameter of the nonlinear solver
   * QRSlv. This parameter tells the solver whether it should block
   * partition or not.
   * As long as we do not have a special subroutine to partition
   * conditional models, this option should be disabled while using
   * the conditional solver CMSlv
   */
  param = "partition";
  u.b = 0;
  set_param_in_solver(server,NONLINEAR_SOLVER,bool_parm,param,&u);


  /*
   * Setting the value of the number of iterations in the optimizer.
   * For us, the optimizer is a blackbox. The only way of asking the
   * values of the variables at each iteration is to stop the optimizer
   * at each iteration by assigning the number of iterations equal to 1.
   * We have seen though  that, because CONOPT work in four different
   * phases, we need to assign a number of iterations a little bit bigger,
   * so that CONOPT is able to determine optimality if we are
   * already at the solution.
   */
  param = "iterationlimit";
  u.i = 20;
  set_param_in_solver(server,OPTIMIZATION_SOLVER,int_parm,param,&u);

  /*
   * Maximum number of subsequent iterations in nonlinear solver.
   * We will give a big value to this parameter, since we really do
   * not care about the limits in the number of iterations in the
   * nonlinear solver, what we care about here is in the number
   * of iterations controlled by CMSlv.
   */
  param = "iterationlimit";
  u.i = 150;
  set_param_in_solver(server,NONLINEAR_SOLVER,int_parm,param,&u);

  return 0;
}



static
SlvClientToken slv9_create(slv_system_t server, int *statusindex){
  slv9_system_t sys;

  sys = (slv9_system_t)asccalloc(1, sizeof(struct slv9_system_structure) );
  if(sys==NULL) {
    *statusindex = 1;
    return sys;
  }
  SERVER = server;
  sys->p.parms = sys->pa;
  sys->p.dynamic_parms = 0;
  slv9_get_default_parameters(server,(SlvClientToken)sys,&(sys->p));
  sys->integrity = OK;
  sys->presolved = 0;
  sys->need_consistency_analysis = slv_need_consistency(server);
  sys->nliter = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;
  sys->p.whose = (*statusindex);
  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.costsize = 0;
  sys->s.cost = NULL; /*redundant, but sanity preserving */
  sys->vlist = slv_get_solvers_var_list(server);
  sys->mvlist = slv_get_master_var_list(server); /* read only */
  sys->rlist = slv_get_solvers_rel_list(server);
  sys->dvlist = slv_get_solvers_dvar_list(server);
  sys->mdvlist = slv_get_master_dvar_list(server);
  sys->lrlist = slv_get_solvers_logrel_list(server);
  sys->blist = slv_get_solvers_bnd_list(server);
  sys->obj = slv_get_obj_relation(server);
  sys->rtot = slv_get_num_solvers_rels(server);
  sys->vtot = slv_get_num_solvers_vars(server);
  sys->mvtot = slv_get_num_master_vars(server);
  sys->coeff_matrix = NULL;
  sys->opt_var_values = NULL;
  if(sys->vlist == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv called with no variables.");
    *statusindex = -2;
    return NULL;
  }
  if(sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv called with no relations or objective.\n");
    *statusindex = -1;
    return NULL;
  }
  if(sys->dvlist == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv called with no discrete variables.\n");
    *statusindex = -2;
    return NULL;
  }
  if(sys->lrlist == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv called with no logrelations.\n");
    *statusindex = -1;
    return NULL;
  }
  if(sys->blist == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv called with no boundaries.\n");
    *statusindex = -2;
    return NULL;
  }
  slv_check_var_initialization(server);
  slv_check_dvar_initialization(server);
  slv_bnd_initialization(server);

  if(get_solvers_tokens(sys,server)) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Solver(s) required by CMSlv were not registered. System cannot be created.");
    *statusindex = -1;
    return NULL;
  }

  *statusindex = 0;
  token[CONDITIONAL_SOLVER] = (SlvClientToken)sys;
  return((SlvClientToken)sys);
}



static
int slv9_eligible_solver(slv_system_t server){
  const char *msg;
  if(!slv_get_num_solvers_rels(server)){
	msg = "No relations were found";
  }else if(!slv_get_num_solvers_logrels(server)){
	msg = "Model must contain at least one logical relation";
  }else if(!slv_get_num_solvers_bnds(server)){
	msg = "Model must contain at least one boundary";
  }else{
    return TRUE;
  }

  ERROR_REPORTER_HERE(ASC_USER_ERROR
	,"CMSlv not elegible for this model: %s",msg
  );
  return FALSE;
}

static
void slv9_get_parameters(slv_system_t server, SlvClientToken asys,
		slv_parameters_t *parameters
){
  slv9_system_t sys;
  (void) server;
  sys = SLV9(asys);
  if(check_system(sys)) return;
  mem_copy_cast(&(sys->p),parameters,sizeof(slv_parameters_t));
}

static
void slv9_set_parameters(slv_system_t server, SlvClientToken asys,
		slv_parameters_t *parameters
){
  slv9_system_t sys;
  (void) server;
  sys = SLV9(asys);
  if(check_system(sys)) return;
  mem_copy_cast(parameters,&(sys->p),sizeof(slv_parameters_t));
}

static
int slv9_get_status(slv_system_t server, SlvClientToken asys,
		slv_status_t *status
){
	slv9_system_t sys;
	(void) server;
	sys = SLV9(asys);
	if(check_system(sys)) return 1;
	mem_copy_cast(&(sys->s),status,sizeof(slv_status_t));
	return 0;
}

static
void slv9_dump_internals(slv_system_t server,
		SlvClientToken sys,int level
){
  check_system(sys);
  (void) server;
  if(level > 0) {
    FPRINTF(ASCERR,"ERROR:  (slv9) slv9_dump_internals\n");
    FPRINTF(ASCERR,"         slv9 does not dump its internals.\n");
  }
}


/*
 * Set to zero the fields of the array cost
 */
static
void reset_cost(struct slv_block_cost *cost,int32 costsize){
  int32 ci;

  for( ci = 0; ci < costsize; ++ci ) {
    cost[ci].size = 0;
    cost[ci].iterations = 0;
    cost[ci].funcs = 0;
    cost[ci].jacs = 0;
    cost[ci].functime = 0;
    cost[ci].jactime = 0;
    cost[ci].time = 0;
    cost[ci].resid = 0;
  }
}

/*
 * Update the values for the array cost of the conditional solver
 * based on the value obtained from the nonlinear solver (square or
 * optimizer).
 * We definitely have to find a better way of communicating status
 * among solvers. I think the structure of the slv_status would have to be
 * modified accordingly to the need of each solver, however, the GUI is
 * completely dependent of the current structure, so I did not modify
 * that structure at all.
 */
static
void update_cost(struct slv_block_cost *cost, slv_status_t *status,
		int32 current_block, int32 previous_block
){
  int32 ci;

  if(current_block >=0) {
    ci=current_block;
    cost[current_block].size = status->block.current_size;
    cost[current_block].iterations	= status->block.iteration;
    cost[current_block].funcs = status->block.funcs;
    cost[current_block].jacs = status->block.jacs;
    cost[current_block].functime = status->block.functime;
    cost[current_block].jactime = status->block.jactime;
    cost[current_block].time = status->block.cpu_elapsed;
    cost[current_block].resid = status->block.residual;
    if(previous_block != -1 && previous_block != current_block) {
      cost[previous_block].size	= status->cost[previous_block].size;
      cost[previous_block].iterations=status->cost[previous_block].iterations;
      cost[previous_block].funcs = status->cost[previous_block].funcs;
      cost[previous_block].jacs	= status->cost[previous_block].jacs;
      cost[previous_block].functime = status->cost[previous_block].functime;
      cost[previous_block].jactime = status->cost[previous_block].jactime;
      cost[previous_block].time	= status->cost[previous_block].time;
      cost[previous_block].resid = status->cost[previous_block].resid;
    }
  }
}

static
int32 is_an_optimization_problem(slv_system_t server,
		SlvClientToken asys
){
  slv9_system_t sys;
  slv_status_t status;
  mtx_matrix_t Jacobian;
  dof_t *dofdata;
  var_filter_t vfilter;
  int32 optimizing;

  sys = SLV9(asys);

  /* count free and incident vars */
  vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  vfilter.matchvalue = (VAR_INCIDENT | VAR_SVAR | VAR_ACTIVE);
  sys->vused = slv_count_solvers_vars(server,&vfilter);

  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  slv_presolve(server);

  Jacobian = slv_get_sys_mtx(server);
  dofdata = slv_get_dofdata(server);
  sys->rank = dofdata->structural_rank;

  /* Initialize Status */
  slv_get_status(server,&status);
  optimizing = sys->obj ? (sys->vused - sys->rank) : 0;
  update_struct_info(sys,&status);
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  return optimizing;
}

static
int slv9_presolve(slv_system_t server, SlvClientToken asys){
  slv9_system_t sys;
  struct var_variable **vp;
  struct rel_relation **rp;
  int32 cap, ind;

  sys = SLV9(asys);
  iteration_begins(sys);
  check_system(sys);
  if(sys->vlist == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Variable list was never set.");
    return 1;
  }
  if(sys->rlist == NULL && sys->obj == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Relation list and objective never set.");
    return 2;
  }

  cap = slv_get_num_solvers_rels(server);
  sys->cap = slv_get_num_solvers_vars(server);
  sys->cap = MAX(sys->cap,cap);

  vp=sys->vlist;
  for( ind = 0; ind < sys->vtot; ++ind ) {
    var_set_in_block(vp[ind],FALSE);
  }

  rp=sys->rlist;
  for( ind = 0; ind < sys->rtot; ++ind ) {
    rel_set_in_block(rp[ind],FALSE);
    rel_set_satisfied(rp[ind],FALSE);
  }

  /*
   * Information about subregions
   *
   */
  sys->subregion_list.length = 0 ;
  sys->subregion_list.capacity = 0;
  sys->subregion_list.sub_stack = NULL;

  sys->subregions_visited.length = 0 ;
  sys->subregions_visited.capacity = 0;
  sys->subregions_visited.visited = NULL;

  sys->presolved = 1;

  /*
   * Assume initially that all the variables are basic
   */
  set_nonbasic_status_in_var_list(server,FALSE);

  /*
   * Sets value of global variable
   */
  g_optimizing = is_an_optimization_problem(server,asys);

  sys->s.block.current_reordered_block = -2;
  /* Reset status */
  sys->s.iteration = 0;
  sys->nliter = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.block.previous_total_size = 0;
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;

  update_status(sys);
  iteration_ends(sys);

  return 0;
}

static
int slv9_resolve(slv_system_t server, SlvClientToken asys){
  struct var_variable **vp;
  struct rel_relation **rp;
  slv9_system_t sys;

  sys = SLV9(asys);
  (void) server;
  check_system(sys);

  for( vp = sys->vlist ; *vp != NULL ; ++vp ) {
    var_set_in_block(*vp,FALSE);
  }
  for( rp = sys->rlist ; *rp != NULL ; ++rp ) {
    rel_set_in_block(*rp,FALSE);
    rel_set_satisfied(*rp,FALSE);
  }

  /* Reset status */
  sys->nliter = 0;
  sys->s.iteration = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.block.previous_total_size = 0;

  /* go to first unconverged block */
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;

  update_status(sys);

  return 0;
}

static
int slv9_iterate(slv_system_t server, SlvClientToken asys){
  slv9_system_t sys;
  slv_status_t status;
  struct matching_cases *subregions;
  struct real_values rvalues;
  var_filter_t vfilter;
  struct gl_list_t *disvars;
  real64 factor;
  int32 n_subregions, cur_subregion;
  int32 previous_block;
  boolean unsuccessful;
  int32 system_was_reanalyzed;
#if TEST_CONSISTENCY
  int32 *test= NULL;
#endif /* TEST_CONSISTENCY */
  FILE *mif;
  FILE *lif;

  sys = SLV9(asys);
  mif = MIF(sys);
  lif = LIF(sys);

  if(server == NULL || sys==NULL) return 1;
  if(check_system(SLV9(sys))) return 2;
  if(!sys->s.ready_to_solve ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not ready to solve.");
    return 3;
  }

  unsuccessful = FALSE;
  iteration_begins(sys);
  system_was_reanalyzed = 0;
  disvars = gl_create(1L);
  /*
   * If the current point is at a boundary, perform optimization step
   * at boundary. If the problem is an optimization problem, then it is
   * required to analyze the system first, in order to investigate which
   * variables are dependent and which are independent. That information
   * is not available before iterating with the optimizer, so, an iteration
   * with the oprimizer is required before the analysis at the boundary.
   */
  if((!g_optimizing || (sys->nliter > 0))
      && at_a_boundary(
        server
        ,asys,&(n_subregions),&(subregions),&(cur_subregion), disvars
      )
  ){
    slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
    slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
    store_real_pre_values(server,&(rvalues));
    ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Solving Optimization Problem at boundary...\n");
    if(optimize_at_boundary(server,asys,&(n_subregions),
                            subregions,&(cur_subregion),disvars,&(rvalues))){
      store_real_cur_values(server,&(rvalues));
      update_boundaries(server,asys);
      if(some_boundaries_crossed(server,asys)) {
        vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
			     | VAR_SVAR | VAR_FIXED);
        vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Boundary(ies) crossed. Returning to boundary first crossed...\n");
        factor = return_to_first_boundary(server,asys,&rvalues,&vfilter);
        update_real_var_values(server,&rvalues,&vfilter,factor);
        update_boundaries(server,asys);
        update_relations_residuals(server);
      }else{
        destroy_array(rvalues.cur_values);
        destroy_array(rvalues.pre_values);
      }
      update_status(sys);
    }else{
      destroy_array(rvalues.pre_values);
      sys->s.converged  = TRUE;
      sys->s.ready_to_solve = FALSE;
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"No progress can be achieved: solution at current boundary.");
      slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
      slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
    }
    gl_destroy(disvars);
    disvars = NULL;
    iteration_ends(sys);
    return 0; /* is there an error here? */
  }else{
    /* solve logical relations */
    solve_logical_relations(server);
    slv_get_status(server,&status);
    sys->s.converged  = status.converged;
    if(!sys->s.converged ) {
      unsuccessful = update_unsuccessful(sys,&status);
      if(unsuccessful) {
        sys->s.ready_to_solve = !unsuccessful;
        ERROR_REPORTER_HERE(ASC_PROG_ERR,"Non-convergence in logical solver.");
        slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
        slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
        gl_destroy(disvars);
        disvars = NULL;
        iteration_ends(sys);
        return 4;
      }
    }
    /*
     * reconfigure the system if necessary
     */
    if(some_dis_vars_changed(server,asys) ) {
      reanalyze_solver_lists(server);
      update_relations_residuals(server);
      system_was_reanalyzed = 1;
    }

    /*
     * Nonlinear solution technique
     */
    if(g_optimizing) {
    /*
     * SetUp Optimizer
     */
      slv_set_client_token(server,token[OPTIMIZATION_SOLVER]);
      slv_set_solver_index(server,solver_index[OPTIMIZATION_SOLVER]);
      store_real_pre_values(server,&(rvalues));
      set_nonbasic_status_in_var_list(server,FALSE);
      (sys->nliter)++;
      if(sys->nliter == 1  || system_was_reanalyzed ==1) {
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Iterating with Optimizer...");
        slv_presolve(server);
        slv_get_status(server,&status);
        update_real_status(&(sys->s),&status,0);
        if(sys->s.cost) {
          destroy_array(sys->s.cost);
        }
        sys->s.cost =
	          create_zero_array(sys->s.costsize,struct slv_block_cost);
        reset_cost(sys->s.cost,sys->s.costsize);
      }else{
        slv_get_status(server,&status);
        update_struct_info(sys,&status);
        if(status.converged) {
          slv_presolve(server);
          update_real_status(&(sys->s),&status,0);
          if(sys->s.cost) {
            destroy_array(sys->s.cost);
          }
          sys->s.cost =
	            create_zero_array(sys->s.costsize,struct slv_block_cost);
          reset_cost(sys->s.cost,sys->s.costsize);
        }else{
          if(!status.ready_to_solve) {
            slv_resolve(server);
          }
        }
      }
    }else{
    /*
     * SetUp nonlinear solver
     */
      slv_set_client_token(server,token[NONLINEAR_SOLVER]);
      slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
      store_real_pre_values(server,&(rvalues));
      (sys->nliter)++;
      if(sys->nliter == 1  || system_was_reanalyzed ==1) {
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Iterating with Non Linear solver...\n");
        slv_presolve(server);
        slv_get_status(server,&status);
        update_struct_info(sys,&status);
        update_real_status(&(sys->s),&status,sys->nliter);
        if(sys->s.cost) {
          destroy_array(sys->s.cost);
        }
        sys->s.cost =
	            create_zero_array(sys->s.costsize,struct slv_block_cost);
        reset_cost(sys->s.cost,sys->s.costsize);
#if TEST_CONSISTENCY
        ID_and_storage_subregion_information(server,asys);
        CONSOLE_DEBUG("New region, Iteration = %d\n",sys->s.block.iteration);
#endif /* TEST_CONSISTENCY  */
      }
      slv_get_status(server,&status);
      update_struct_info(sys,&status);
      if(status.converged) {
        slv_presolve(server);
        update_real_status(&(sys->s),&status,0);
        if(sys->s.cost) {
          destroy_array(sys->s.cost);
        }
        sys->s.cost =
	            create_zero_array(sys->s.costsize,struct slv_block_cost);
        reset_cost(sys->s.cost,sys->s.costsize);
      }
    }
    /*
      Iteration steps common to optimizer and nonlinear solver
    */
    previous_block = sys->s.block.current_block;
    slv_iterate(server);
    store_real_cur_values(server,&(rvalues));
    update_boundaries(server,asys);
    slv_get_status(server,&status);
    sys->s.converged  = status.converged;
    /*
      The following statement was added 4/2
    */
    update_struct_info(sys,&status);
    update_real_status(&(sys->s),&status,0);
    update_cost(sys->s.cost,&status,
                sys->s.block.current_block,previous_block);
    if(!sys->s.converged || some_boundaries_crossed(server,asys) ) {
      sys->s.converged = FALSE;
      sys->s.ready_to_solve = !sys->s.converged;
      if(some_boundaries_crossed(server,asys)) {
        vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT
			     | VAR_SVAR | VAR_FIXED);
        vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Boundary(ies) crossed. Returning to boundary first crossed...\n");
        factor = return_to_first_boundary(server,asys,&rvalues,&vfilter);
        update_real_var_values(server,&rvalues,&vfilter,factor);
        update_boundaries(server,asys);
        update_relations_residuals(server);
      }else{
        destroy_array(rvalues.cur_values);
        destroy_array(rvalues.pre_values);
      }
      unsuccessful = update_unsuccessful(sys,&status);
      if(unsuccessful) {

#if TEST_CONSISTENCY
        if(sys->s.iteration_limit_exceeded) {
          eligible_set_for_subregions(server,asys,&test);
            consistency_analysis_for_subregions(server,asys,&test);
          if(test != NULL) {
            ascfree(test);
          }
        }
#endif /* TEST_CONSISTENCY */

        sys->s.ready_to_solve = !unsuccessful;
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Non-convergence in nonlinear step.");
      }
    }else{
      sys->s.ready_to_solve = !sys->s.converged;
      /*
        The following was added 4/2
      */
      unsuccessful = update_unsuccessful(sys,&status);

#if TEST_CONSISTENCY
      if(sys->s.iteration_limit_exceeded) {
        consistency_analysis_for_subregions(server,asys,&test);
        if(test != NULL) {
          ascfree(test);
        }
      }
#endif /* TEST_CONSISTENCY */

      if(unsuccessful){
        sys->s.ready_to_solve = !unsuccessful;
        ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Non-convergence in nonlinear step.");
      }
      destroy_array(rvalues.cur_values);
      destroy_array(rvalues.pre_values);
    }
  }
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  gl_destroy(disvars);
  disvars = NULL;
  iteration_ends(sys);
  return 0;
}


static int slv9_solve(slv_system_t server, SlvClientToken asys){
  slv9_system_t sys;
  int err = 0;

  sys = SLV9(asys);
  if(server == NULL || sys==NULL)return 1;
  if(check_system(sys))return 2;

  while(sys->s.ready_to_solve)err = err | slv9_iterate(server,sys);

  return err;
}


static
mtx_matrix_t slv9_get_matrix(slv_system_t server, SlvClientToken sys){
  if(server == NULL || sys==NULL) return NULL;
  if(check_system(SLV9(sys))) return NULL;
  ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv9 does not get matrix.");
  return( NULL );
}

/*
 * Destroy the client tokens of the different solvers
 */
static
void destroy_solvers_tokens(slv_system_t  server){
  slv_set_client_token(server,token[LOGICAL_SOLVER]);
  slv_set_solver_index(server,solver_index[LOGICAL_SOLVER]);
  slv_destroy_client(server);
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  slv_destroy_client(server);
  slv_set_client_token(server,token[OPTIMIZATION_SOLVER]);
  slv_set_solver_index(server,solver_index[OPTIMIZATION_SOLVER]);
  slv_destroy_client(server);
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
}

static
int slv9_destroy(slv_system_t server, SlvClientToken asys){
  slv9_system_t sys;
  sys = SLV9(asys);
  if(check_system(sys)) return 1;
  destroy_subregion_information(asys);
  destroy_solvers_tokens(server);
  slv_destroy_parms(&(sys->p));
  sys->integrity = DESTROYED;
  if(sys->s.cost) ascfree(sys->s.cost);
  ascfree( (POINTER)asys );
  return 0;
}


int slv9_register(SlvFunctionsT *sft){
  if(sft==NULL)  {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"slv9_register called with NULL pointer");
    return 1;
  }

  sft->name = "CMSlv";
  sft->ccreate = slv9_create;
  sft->cdestroy = slv9_destroy;
  sft->celigible = slv9_eligible_solver;
  sft->getdefparam = slv9_get_default_parameters;
  sft->get_parameters = slv9_get_parameters;
  sft->setparam = slv9_set_parameters;
  sft->getstatus = slv9_get_status;
  sft->solve = slv9_solve;
  sft->presolve = slv9_presolve;
  sft->iterate = slv9_iterate;
  sft->resolve = slv9_resolve;
  sft->getlinsys = NULL;
  sft->get_sys_mtx = slv9_get_matrix;
  sft->dumpinternals = slv9_dump_internals;
  return 0;
}

#endif /* #else clause of DYNAMIC_CMSLV */
#endif /* #else clause of !STATIC_CMSLV && !DYNAMIC_CMSLV */

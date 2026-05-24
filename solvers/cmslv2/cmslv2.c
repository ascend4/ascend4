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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Conditional Modeling Solver (CMSlv) module.
*//*
	Conditional Modeling Solver
	by Vicente Rico-Ramirez, 04/1997
	Last in CVS: $Revision: 1.22 $ $Date: 2000/01/25 02:27:58 $ $Author: ballan $
*/

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/tm_time.h>
#include <ascend/general/mem.h>
#include <ascend/general/list.h>
#include <ascend/general/mathmacros.h>

#include <ascend/linear/mtx_reorder.h>

#include <ascend/system/calc.h>
#include <ascend/system/relman.h>
#include <ascend/system/logrelman.h>
#include <ascend/system/bndman.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/cond_config.h>
#include <ascend/system/decomp.h>
#include <ascend/solver/solver.h>
#include <ascend/solver/slvDOF.h>
#include <ascend/compiler/packages.h>

#include <ascend/solver/solver.h>

#include "cmslv2.h"

ASC_DLLSPEC SolverRegisterFn cmslv2_register;

//#define CMSLV_DEBUG
#ifdef CMSLV_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif

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
#define CMSLV2_SELECTOR_SEARCH_MAX_DVARS 10

/*
 * system definitions
 */
#define SLV9(s) ((slv9_system_t)(s))
#define SERVER (sys->slv)
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

static
void slv9_report_progress(slv9_system_t sys, const char *fmt, ...){
  char message[512];
  va_list args;

  if(sys == NULL || fmt == NULL) {
    return;
  }
  if(!PROGRESS_LOG && !PROGRESS_CALLBACKS) {
    return;
  }

  va_start(args,fmt);
  vsnprintf(message,sizeof(message),fmt,args);
  va_end(args);
  message[sizeof(message)-1] = '\0';

  if(PROGRESS_LOG) {
    ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"(CMSlv progress) %s",message);
  }
  if(PROGRESS_CALLBACKS) {
    (void)slv_report_progress("CMSlv2",message);
  }
}

static
int32 slv9_reanalyze_solver_lists_changed(slv9_system_t sys,
    int32 *active_changed){
  struct rel_relation **rels;
  struct logrel_relation **logrels;
  unsigned char *rel_active_save = NULL;
  unsigned char *logrel_active_save = NULL;
  int32 r, nrels, l, nlogrels;
  int32 changed = 0;

  if(active_changed != NULL) {
    *active_changed = 0;
  }
  if(sys == NULL || sys->slv == NULL) {
    return 1;
  }
  if(slv_has_classifier_whens(sys->slv)) {
    rels = slv_get_solvers_rel_list(sys->slv);
    logrels = slv_get_solvers_logrel_list(sys->slv);
    nrels = slv_get_num_solvers_rels(sys->slv);
    nlogrels = slv_get_num_solvers_logrels(sys->slv);
    if(nrels > 0) {
      rel_active_save = ASC_NEW_ARRAY(unsigned char,nrels);
      if(rel_active_save == NULL) {
        return 1;
      }
      for(r = 0; r < nrels; ++r) {
        rel_active_save[r] = rel_active(rels[r]) ? 1 : 0;
      }
    }
    if(nlogrels > 0) {
      logrel_active_save = ASC_NEW_ARRAY(unsigned char,nlogrels);
      if(logrel_active_save == NULL) {
        if(rel_active_save != NULL) ascfree(rel_active_save);
        return 1;
      }
      for(l = 0; l < nlogrels; ++l) {
        logrel_active_save[l] = logrel_active(logrels[l]) ? 1 : 0;
      }
    }
    if(slv_prepare_classifier_whens(sys->slv,WHEN_REGION_STEADY)) {
      if(rel_active_save != NULL) ascfree(rel_active_save);
      if(logrel_active_save != NULL) ascfree(logrel_active_save);
      return 1;
    }
    rels = slv_get_solvers_rel_list(sys->slv);
    logrels = slv_get_solvers_logrel_list(sys->slv);
    if(nrels != slv_get_num_solvers_rels(sys->slv)
        || nlogrels != slv_get_num_solvers_logrels(sys->slv)) {
      changed = 1;
    }
    for(r = 0; !changed && r < nrels; ++r) {
      if(rel_active_save[r] != (rel_active(rels[r]) ? 1 : 0)) {
        changed = 1;
      }
    }
    for(l = 0; !changed && l < nlogrels; ++l) {
      if(logrel_active_save[l] != (logrel_active(logrels[l]) ? 1 : 0)) {
        changed = 1;
      }
    }
    if(rel_active_save != NULL) ascfree(rel_active_save);
    if(logrel_active_save != NULL) ascfree(logrel_active_save);
    if(active_changed != NULL) {
      *active_changed = changed;
    }
    return 0;
  }
  reanalyze_solver_lists(sys->slv);
  return 0;
}

static
int32 slv9_reanalyze_solver_lists(slv9_system_t sys){
  return slv9_reanalyze_solver_lists_changed(sys,NULL);
}

#if USE_CONSISTENCY
/*
 * number of subregion visited during the solution of the conditional
 * model.
 */
static int32 g_subregions_visited;

#endif /* USE_CONSISTENCY */


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

#if 0 /* unused function */
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
#endif

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

#if 0 /* unused functions */
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
#endif

#if 0 /* unused functions */
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
#endif

#if 0 /* unused functions */
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
#endif

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
  SlvClientToken origtoken = slv_get_client_token(server);
  int32 origsolver = slv_get_selected_solver(server);

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
            slv_set_char_parameter(&(p.parms[len].info.c.value),value->c);
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

  /* return to original state */
  slv_set_solver_index(server,origsolver);
  slv_set_client_token(server,origtoken);

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
#if SHOW_BOUNDARY_ANALYSIS_DETAILS
  FILE *lif;
#endif

  sys = SLV9(asys);
  check_system(sys);
#if SHOW_BOUNDARY_ANALYSIS_DETAILS
  lif = LIF(sys);
#endif

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

  slv9_report_progress(sys,
    "event=boundary_return_done, iter=%d, count=%d, factor=%.17g, bisect_iter=%d",
    sys->s.iteration, numbndf, factor, iter
  );

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
      (void)relman_eval(rel,&status,1);
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
#ifdef ASC_CONOPT_API4
int COI_CALL slv9_conopt_readmatrix(
		double lower[], double curr[], double upper[]
		, int vsta[],  int type[], double rhs[]
		, int esta[],  int colsta[], int rowno[]
		, double value[], int nlflag[], int n_value, int m_value, int nz_value
		, void *usrmem
){
  int *n = &n_value;
  int *m = &m_value;
  int *nz = &nz_value;
#else
int COI_CALL slv9_conopt_readmatrix(
		double *lower, double *curr, double *upper
		, int *vsta,  int *type, double *rhs
		, int *esta,  int *colsta, int *rowno
		, double *value, int *nlflag, int *n, int *m, int *nz
		, double *usrmem
){
#endif
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

  sys = (slv9_system_t)usrmem;
  n_subregions = sys->subregions;
  coeff_matrix = sys->coeff_matrix;
  num_var = (*n) - n_subregions;
  num_eqns = num_var + 1;

  varlist = sys->mvlist;
  totvar = sys->mvtot;

  /* fetch the configured bound from solver parameters */
  limit = ASC_INFINITY;

  /* MSG("Got limit value of %g",limit); */

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
          /* MSG("Reducing lower bound limit for var %d to %e",count,lower[count]); */
      }

      if(uplow < limit){
          upper[count] = uplow;
      }else{
          upper[count] = 0.5*limit;
          /* MSG("Reducing upper bound limit for var %d to %e",count,upper[count]); */
      }

      curr[count] = 0.5 * nominal;
      vsta[count] = 1;
      count++;
    }
  }
  /* alphas for each subregion */
  for (c=count; c<(*n); c++) {
      lower[c] = 0.0;
      upper[c] = 1.0;
      curr[c] =  1.0;
      vsta[c] = 1;
  }

  /*MSG("ALL BOUNDS:");
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
    esta[c] = 1;
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
#ifdef ASC_CONOPT_API4
int COI_CALL slv9_conopt_fdeval(
		const double x[], double *g, double jac[]
		, int rowno_value, const int jcnm[], int mode_value, int ignerr
		, int *errcnt, int n_value, int nj_value, int thread
		, void *usrmem
){
  int *rowno = &rowno_value;
  int *mode = &mode_value;
  int *n = &n_value;
  int *nj = &nj_value;
  (void)ignerr;
  (void)thread;
#else
int COI_CALL slv9_conopt_fdeval(
		double *x, double *g, double *jac
		, int *rowno, int *jcnm, int *mode, int *ignerr
		, int *errcnt, int *newpt, int *n, int *nj
		, double *usrmem
){
#endif
  slv9_system_t sys;
  int32 num_vars, v;
  real64 obj, deriv;

#ifndef ASC_CONOPT_API4
  UNUSED_PARAMETER(jcnm);
  UNUSED_PARAMETER(newpt);
  UNUSED_PARAMETER(ignerr);
#endif
  UNUSED_PARAMETER(errcnt);
  UNUSED_PARAMETER(n);

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
#ifdef ASC_CONOPT_API4
      for (v=0; v<*nj; v++) {
        if(jcnm[v] >= 0 && jcnm[v] < *n) {
          jac[jcnm[v]] = 0.0;
        }
      }
#endif
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
#ifdef ASC_CONOPT_API4
int COI_CALL slv9_conopt_status(int modsta, int solsta, int iter
		, double objval, void *usrmem
){
#else
int COI_CALL slv9_conopt_status(int *modsta, int *solsta, int *iter
		, double *objval, double *usrmem
){
#endif
  slv9_system_t sys;

  sys = (slv9_system_t)usrmem;

#ifdef ASC_CONOPT_API4
  sys->con.modsta = modsta;
  sys->con.solsta = solsta;
  sys->con.iter = iter;
  sys->con.obj = objval;
#else
  sys->con.modsta = *modsta;
  sys->con.solsta = *solsta;
  sys->con.iter = *iter;
  sys->con.obj = *objval;
#endif

  return 0;
}

/**
	CONOPT error message reporting
*/
#ifdef ASC_CONOPT_API4
int COI_CALL slv9_conopt_errmsg( int ROWNO, int COLNO, int POSNO
		, const char* MSG, void* USRMEM
){
#else
int COI_CALL slv9_conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
){
#endif
	slv9_system_t sys;
	char *varname=NULL;
	struct var_variable **vp;
	int rowno;
	int colno;

	sys = (slv9_system_t)USRMEM;
	(void)POSNO;
#ifndef ASC_CONOPT_API4
	(void)LENMSG;
#endif

	rowno =
#ifdef ASC_CONOPT_API4
		ROWNO;
#else
		*ROWNO;
#endif
	colno =
#ifdef ASC_CONOPT_API4
		COLNO;
#else
		*COLNO;
#endif

	if(colno >= 0 && colno < sys->mvtot){
		vp=sys->mvlist;
		vp = vp + colno;
		assert(*vp!=NULL);
		varname= var_make_name(SERVER,*vp);
	}

	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
	if(rowno == -1){
	    FPRINTF(ASCERR,"Variable %d",colno);
	    if(varname != NULL) {
	      FPRINTF(ASCERR," (Maybe it's '%s')",varname);
	    }
	    FPRINTF(ASCERR,": ");
	}else if(colno == -1 ){
	    FPRINTF(ASCERR,"Relation %d: ",rowno);
	}else{
	    FPRINTF(ASCERR,"Variable %d",colno);
	    if(varname != NULL) {
	      FPRINTF(ASCERR," (Maybe it's '%s')",varname);
	    }
	    FPRINTF(ASCERR," appearing in relation %d: ",rowno);
	}
#ifdef ASC_CONOPT_API4
	FPRINTF(ASCERR,"%s", MSG);
#else
	FPRINTF(ASCERR,"%*s", *MSGLEN, MSG);
#endif
	if(varname != NULL) {
		ASC_FREE(varname);
	}
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
#ifdef ASC_CONOPT_API4
int COI_CALL slv9_conopt_solution(const double xval[], const double xmar[]
		, const int xbas[], const int xsta[], const double yval[]
		, const double ymar[], const int ybas[], const int ysta[]
		, int n_value, int m_value, void *usrmem
){
  int *n = &n_value;
  int *m = &m_value;
#else
int COI_CALL slv9_conopt_solution(double *xval, double *xmar, int *xbas, int *xsta,
		double *yval, double *ymar, int *ybas, int * ysta,
		int *n, int *m, double *usrmem
){
#endif
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
#ifdef ASC_CONOPT_API4
int COI_CALL slv9_conopt_option(
		int NCALL, double *rval, int *ival, int *logical
	    , char *name, void *usrmem
){
#else
int COI_CALL slv9_conopt_option(
		int *NCALL, double *rval, int *ival, int *logical
	    , double *usrmem, char *name, int lenname
){
#endif
  slv9_system_t sys;
  sys = (slv9_system_t)usrmem;

  UNUSED_PARAMETER(NCALL);
  UNUSED_PARAMETER(logical);
#ifdef ASC_CONOPT_API4
  UNUSED_PARAMETER(sys);
  UNUSED_PARAMETER(rval);
  UNUSED_PARAMETER(ival);
  name[0] = '\0';
  return 0;
#else
  UNUSED_PARAMETER(lenname);

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
#endif
}

#if 0 /* see slv9_bnd_iterate_conopt */
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
int32 slv9_bnd_iterate_conopt(slv9_system_t sys, int32 num_opt_vars,
		int32 num_opt_eqns, int32 num_vars, real64 *obj_val
){
  int solve_status;
  int stdout_flag = 0;
  int debugfv = 0;

  sys->con.n = num_opt_vars;
  sys->con.m = num_opt_eqns + 1;  /*including objective function */
  sys->con.objcon = num_opt_eqns; /* last row is the objective fn */
  sys->con.nz = (num_opt_eqns * sys->subregions) + 2 * num_vars;
  /* sys->con.nlnz = sys->con.nz - (num_opt_eqns - 1); */
  sys->con.nlnz = num_opt_vars - sys->subregions;
  sys->con.base = 0; /* C calling convention */
  sys->con.optdir = -1; /* minimisation */

  MSG("%d vars, %d rows",sys->con.n,sys->con.m);
  MSG("objective constraint: %d",sys->con.objcon);
  MSG("nonzeros: %d",sys->con.nz);
  MSG("nonlinear nonzeros: %d",sys->con.nlnz);

#ifdef ASC_CONOPT_API4
  if(sys->con.cntvect != NULL){
    COI_Free(&(sys->con.cntvect));
  }
  if(COI_Create(&(sys->con.cntvect))){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"CMSlv CONOPT boundary problem creation failed.");
    return 0;
  }
#else
  if(sys->con.cntvect == NULL){
	sys->con.cntvect = ASC_NEW_ARRAY(int,COIDEF_Size());
  }

  COIDEF_Ini(sys->con.cntvect);
#endif

  /*
	We pass pointer to sys as usrmem data.
	Cast back to slv9_system_t to access the information required
  */
#ifdef ASC_CONOPT_API4
  COIDEF_UsrMem(sys->con.cntvect,(void *)sys);

  COIDEF_NumVar(sys->con.cntvect, sys->con.n);
  COIDEF_NumCon(sys->con.cntvect, sys->con.m); /* include the obj fn */
  COIDEF_NumNZ(sys->con.cntvect, sys->con.nz);
  COIDEF_NumNlNz(sys->con.cntvect, sys->con.nlnz);
  COIDEF_OptDir(sys->con.cntvect, sys->con.optdir);
  COIDEF_ObjCon(sys->con.cntvect, sys->con.objcon); /* objective will be last row */
  COIDEF_ErrLim(sys->con.cntvect, DOMLIM);
  COIDEF_ItLim(sys->con.cntvect, OPT_ITER_LIMIT);
  COIDEF_StdOut(sys->con.cntvect, stdout_flag);
  COIDEF_DebugFV(sys->con.cntvect, debugfv);
#else
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
  COIDEF_StdOut(sys->con.cntvect, &stdout_flag);
  COIDEF_DebugFV(sys->con.cntvect, &debugfv);
#endif

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

  sys->con.modsta = 0;
  sys->con.solsta = 0;
  sys->con.iter = 0;
  sys->con.obj = 0.0;

  solve_status = COI_Solve(sys->con.cntvect);
  /* conopt_start(&(sys->con.kept), usrmem, &(sys->con.lwork),
	       sys->con.work, &(sys->con.maxusd), &(sys->con.curusd)); */

  sys->con.optimized =
    solve_status == 0
    && sys->con.solsta == 1
    && (
      sys->con.modsta == 1
      || sys->con.modsta == 2
      || sys->con.modsta == 15
      || sys->con.modsta == 16
      || sys->con.modsta == 17
    );
  if(!sys->con.optimized) {
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,
      "CMSlv CONOPT boundary solve failed: solve_status=%d, solsta=%d, modsta=%d.",
      solve_status,sys->con.solsta,sys->con.modsta
    );
    return 0;
  }
  if(obj_val != NULL) {
    *obj_val = sys->con.obj;
  }
  return 1;
}

#endif /* ASC_WITH_CONOPT  */

/*-------------------end of conopt callbacks----------------------------------*/

static
const char *slv9_optimizer_package(const char *name){
  if(name == NULL) {
    return NULL;
  }
  if(strcmp(name,"CONOPT") == 0) {
    return "conopt";
  }
  if(strcmp(name,"IPOPT") == 0) {
    return "ipopt";
  }
  return NULL;
}

static
int32 slv9_ensure_optimizer_loaded(const char *name){
  const char *package;

  if(name == NULL) {
    return 0;
  }
  if(strcmp(name,"CONOPT") == 0) {
#ifndef ASC_WITH_CONOPT
    return 0;
#endif
  }else if(strcmp(name,"IPOPT") == 0) {
#ifndef ASC_CMSLV_WITH_IPOPT
    return 0;
#endif
  }else{
    return 0;
  }

  if(solver_engine_named(name) != NULL) {
    return 1;
  }
  package = slv9_optimizer_package(name);
  if(package == NULL) {
    return 0;
  }
  if(package_load(package,NULL)) {
    return 0;
  }
  return solver_engine_named(name) != NULL;
}

static
int32 slv9_optimizer_available(const char *name){
  if(name == NULL) {
    return 0;
  }
  if(strcmp(name,"CONOPT") == 0) {
#ifdef ASC_WITH_CONOPT
    return slv9_ensure_optimizer_loaded("CONOPT");
#else
    return 0;
#endif
  }
  if(strcmp(name,"IPOPT") == 0) {
#ifdef ASC_CMSLV_WITH_IPOPT
    return slv9_ensure_optimizer_loaded("IPOPT");
#else
    return 0;
#endif
  }
  return 0;
}

static
void slv9_report_unavailable_optimizer(const char *name){
  if(name == NULL) {
    name = "";
  }
  ERROR_REPORTER_HERE(ASC_USER_ERROR,
    "CMSlv selected optimization solver '%s' is not available.",name
  );
  if(strcmp(name,"CONOPT") != 0 && slv9_optimizer_available("CONOPT")) {
    ERROR_REPORTER_HERE(ASC_USER_NOTE,
      "CONOPT is available; select optsolvers=CONOPT to use it with CMSlv2."
    );
  }
  if(strcmp(name,"IPOPT") != 0 && slv9_optimizer_available("IPOPT")) {
    ERROR_REPORTER_HERE(ASC_USER_NOTE,
      "IPOPT is available; select optsolvers=IPOPT to use it with CMSlv2."
    );
  }
}

static
int32 slv9_bnd_iterate(slv9_system_t sys, int32 num_opt_vars,
		int32 num_opt_eqns, int32 num_vars, real64 *obj_val
){
  if(strcmp(OPTSOLVER_OPTION,"CONOPT") == 0) {
#ifdef ASC_WITH_CONOPT
    return slv9_bnd_iterate_conopt(sys,num_opt_vars,num_opt_eqns,num_vars,obj_val);
#else
    slv9_report_unavailable_optimizer(OPTSOLVER_OPTION);
    return 0;
#endif
  }
  if(strcmp(OPTSOLVER_OPTION,"IPOPT") == 0) {
#ifdef ASC_CMSLV_WITH_IPOPT
    return slv9_bnd_iterate_ipopt(sys,num_opt_vars,num_opt_eqns,num_vars,obj_val);
#else
    slv9_report_unavailable_optimizer(OPTSOLVER_OPTION);
    return 0;
#endif
  }
  ERROR_REPORTER_HERE(ASC_USER_ERROR,
    "CMSlv selected optimization solver '%s' is not recognised.",OPTSOLVER_OPTION
  );
  return 0;
}


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
  int32 c, cr, len, row;
  real64 *weights;
  real64 summ;

  sys = SLV9(asys);
  check_system(sys);

  mtx_output_assign(sys->lin_mtx,nrel,nrel);
#if SHOW_LAGRANGE_DETAILS
  int32 rank;
  rank = mtx_symbolic_rank(sys->lin_mtx);
  FPRINTF(ASCERR, "cmslv2: get_multipliers: rank %d\n",rank);
#endif
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
#if SHOW_LAGRANGE_DETAILS
  FILE *lif;
#endif

  sys = SLV9(asys);
  check_system(sys);
#if SHOW_LAGRANGE_DETAILS
  lif = LIF(sys);
#endif

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
  int32 totvars, c, count;
  real64 up, low, pre_val;
  real64 value, dx, test_value,norm2;
  FILE *lif;

  sys = SLV9(asys);
  check_system(sys);
  lif = LIF(sys);

  vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
		       | VAR_SVAR | VAR_FIXED);
  vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
  vlist = slv_get_master_var_list(server);
#if DEBUG
  int32 num_vars;
  num_vars = slv_count_master_vars(server,&vfilter);
#endif
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

#if DEBUG
  int num_tot = num_vars+n_subregions;
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
  struct opt_matrix coeff_matrix = { NULL };
  struct opt_vector opt_var_values = { NULL };
  struct opt_vector invariant_vect_values = { NULL };
  struct opt_vector variant_vect_values = { NULL };
  struct opt_vector gradient = { NULL };
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

  slv9_report_progress(sys,
    "event=boundary_opt_start, iter=%d, solver=%s, n_subregions=%d, vars=%d, eqns=%d",
    sys->s.iteration, OPTSOLVER_OPTION, *n_subregions, num_opt_vars, num_opt_eqns
  );
  if(!slv9_bnd_iterate(sys,num_opt_vars,num_opt_eqns,num_vars,&obj_val)) {
    return_value = 0;
    goto restore;
  }
  slv9_report_progress(sys,
    "event=boundary_opt_done, iter=%d, solver=%s, obj=%.17g",
    sys->s.iteration, OPTSOLVER_OPTION, obj_val
  );

#if DEBUG
  FPRINTF(ASCERR," objective function = %f \n",obj_val);
#endif /* DEBUG */

  /*
   * Analyze and apply optimization step
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

restore:
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


/*------------------------------------------------------------------------------
  ITERATION BEGIN/END ROUTINES

	iteration_begins(sys)
	iteration_ends(sys)
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
  slv_status_nlp_t *main_nlp = slv_status_nlp_rw(main);
  const slv_status_nlp_t *slave_nlp = slv_status_nlp(slave);
  if(main_nlp == NULL || slave_nlp == NULL){
    return;
  }
  main->block.number_of = slave->block.number_of;
  main->block.residual = slave->block.residual;
  main->block.current_size = slave->block.current_size;
  main->block.current_block = slave->block.current_block;
  if(niter ==1 ) {
    main->block.iteration =  slave->block.iteration;
  }
  main->block.previous_total_size = slave->block.previous_total_size;
}

struct slv9_decomp_block_stats {
  int32 rows;
  int32 cols;
  int32 relrows;
  int32 logrows;
  int32 vars;
  int32 intvars;
  int32 dvars;
  int32 boundary_edges;
  int32 selector_edges;
};

struct slv9_decomp_summary {
  int32 pure_real;
  int32 pure_integer;
  int32 pure_logical;
  int32 selector_coupled;
  int32 boundary_mixed;
  int32 mixed;
  int32 empty;
  int32 max_rows;
  int32 max_cols;
};

struct slv9_cmslv2_plan_summary {
  int32 structural_blocks;
  int32 active_blocks;
  int32 qrslv_blocks;
  int32 lrslv_blocks;
  int32 enum_blocks;
  int32 boundary_blocks;
  int32 integer_blocks;
  int32 fallback_blocks;
  int32 skipped_blocks;
  int32 max_qrslv_rows;
  int32 max_qrslv_cols;
};

struct slv9_cmslv2_selector_summary {
  int32 envelopes;
  int32 attempted;
  int32 matched_cases;
  int32 active_subblocks;
  int32 qrslv_subblocks;
  int32 lrslv_subblocks;
  int32 unresolved_subblocks;
  int32 max_qrslv_rows;
  int32 max_qrslv_cols;
};

struct slv9_cmslv2_block_solved_report {
  int32 solved;
  int32 real_rels_checked;
  int32 real_rel_failures;
  int32 logrels_checked;
  int32 logrel_failures;
  int32 calc_errors;
  int32 active_subblocks;
  int32 qrslv_subblocks;
  int32 lrslv_subblocks;
  int32 unresolved_subblocks;
  real64 max_real_residual;
  const char *reason;
};

enum slv9_cmslv2_work_kind {
  SLV9_CMSLV2_WORK_END = 0,
  SLV9_CMSLV2_WORK_EMPTY,
  SLV9_CMSLV2_WORK_QRSLV,
  SLV9_CMSLV2_WORK_LRSLV,
  SLV9_CMSLV2_WORK_SELECTOR,
  SLV9_CMSLV2_WORK_BOUNDARY,
  SLV9_CMSLV2_WORK_INTEGER,
  SLV9_CMSLV2_WORK_MIXED
};

enum slv9_cmslv2_query_mode {
  SLV9_CMSLV2_QUERY_BLOCK = 0,
  SLV9_CMSLV2_QUERY_SEQUENCE = 1,
  SLV9_CMSLV2_QUERY_SKIP_EMPTY = 2
};

struct slv9_cmslv2_work_unit {
  enum slv9_cmslv2_work_kind kind;
  int32 start;
  int32 end;
  struct slv9_decomp_block_stats stats;
  struct slv9_cmslv2_selector_summary summary;
};

struct slv9_cmslv2_boundary_scope {
  unsigned char *rel_active_save;
  unsigned char *condrel_active_save;
  unsigned char *logrel_active_save;
  unsigned char *condlogrel_active_save;
  unsigned char *var_active_save;
  unsigned char *dvar_active_save;
  int32 nrels;
  int32 ncondrels;
  int32 nlogrels;
  int32 ncondlogrels;
  int32 nvars;
  int32 ndvars;
  int32 scoped_rels;
  int32 scoped_condrels;
  int32 scoped_logrels;
  int32 scoped_condlogrels;
  int32 scoped_vars;
  int32 scoped_dvars;
};

struct slv9_cmslv2_boundary_flags {
  uint32 *flags;
  int32 nbnds;
};

struct slv9_cmslv2_boundary_result {
  struct var_variable **real_vars;
  real64 *real_values;
  int32 real_count;
  struct dis_discrete **dis_vars;
  int32 *dis_values;
  int32 *dis_previous;
  int32 dis_count;
  struct slv9_cmslv2_boundary_flags bnd_flags;
  int32 accepted;
  int32 local_complete;
  int32 at_boundary;
  int32 optimize_ok;
  int32 returned;
  int32 n_subregions;
  int32 cur_subregion;
  unsigned long dvars;
  real64 factor;
};

static
void slv9_decomp_block_stats_init(struct slv9_decomp_block_stats *bs){
  memset(bs,0,sizeof(*bs));
}

static
void slv9_decomp_summary_init(struct slv9_decomp_summary *sum){
  memset(sum,0,sizeof(*sum));
}

static
void slv9_cmslv2_plan_summary_init(struct slv9_cmslv2_plan_summary *plan){
  memset(plan,0,sizeof(*plan));
}

static
void slv9_cmslv2_selector_summary_init(
    struct slv9_cmslv2_selector_summary *sum
){
  memset(sum,0,sizeof(*sum));
}

static
void slv9_cmslv2_block_solved_report_init(
    struct slv9_cmslv2_block_solved_report *report
){
  if(report != NULL) {
    memset(report,0,sizeof(*report));
    report->reason = "unknown";
  }
}

static
void slv9_cmslv2_work_unit_init(struct slv9_cmslv2_work_unit *work){
  if(work != NULL) {
    memset(work,0,sizeof(*work));
    work->kind = SLV9_CMSLV2_WORK_END;
    work->start = -1;
    work->end = -1;
  }
}

static
const char *slv9_cmslv2_work_kind_name(enum slv9_cmslv2_work_kind kind){
  switch(kind) {
  case SLV9_CMSLV2_WORK_END:
    return "end";
  case SLV9_CMSLV2_WORK_EMPTY:
    return "empty";
  case SLV9_CMSLV2_WORK_QRSLV:
    return "qrslv";
  case SLV9_CMSLV2_WORK_LRSLV:
    return "lrslv";
  case SLV9_CMSLV2_WORK_SELECTOR:
    return "selector";
  case SLV9_CMSLV2_WORK_BOUNDARY:
    return "boundary";
  case SLV9_CMSLV2_WORK_INTEGER:
    return "integer";
  case SLV9_CMSLV2_WORK_MIXED:
    return "mixed";
  }
  return "unknown";
}

static
void slv9_cmslv2_boundary_flags_init(
    struct slv9_cmslv2_boundary_flags *flags
){
  if(flags != NULL) {
    flags->flags = NULL;
    flags->nbnds = 0;
  }
}

static
void slv9_cmslv2_store_boundary_flags(slv_system_t server,
    struct slv9_cmslv2_boundary_flags *save
){
  int32 i;
  struct bnd_boundary **bnds;
  if(save == NULL) {
    return;
  }
  slv9_cmslv2_boundary_flags_init(save);
  if(server == NULL) {
    return;
  }
  save->nbnds = slv_get_num_solvers_bnds(server);
  bnds = slv_get_solvers_bnd_list(server);
  if(bnds == NULL || save->nbnds <= 0) {
    save->nbnds = 0;
    return;
  }
  save->flags = ASC_NEW_ARRAY(uint32,save->nbnds);
  if(save->flags == NULL) {
    save->nbnds = 0;
    return;
  }
  for(i = 0; i < save->nbnds; ++i) {
    save->flags[i] = bnd_flags(bnds[i]);
  }
}

static
void slv9_cmslv2_restore_boundary_flags(slv_system_t server,
    const struct slv9_cmslv2_boundary_flags *save
){
  int32 i, nbnds;
  struct bnd_boundary **bnds;
  if(server == NULL || save == NULL || save->flags == NULL
      || save->nbnds <= 0) {
    return;
  }
  nbnds = slv_get_num_solvers_bnds(server);
  bnds = slv_get_solvers_bnd_list(server);
  if(bnds == NULL || nbnds <= 0) {
    return;
  }
  if(save->nbnds < nbnds) {
    nbnds = save->nbnds;
  }
  for(i = 0; i < nbnds; ++i) {
    bnd_set_flags(bnds[i],save->flags[i]);
  }
}

static
void slv9_cmslv2_boundary_flags_destroy(
    struct slv9_cmslv2_boundary_flags *save
){
  if(save == NULL) {
    return;
  }
  if(save->flags != NULL) {
    ascfree(save->flags);
  }
  slv9_cmslv2_boundary_flags_init(save);
}

static
void slv9_cmslv2_boundary_result_init(
    struct slv9_cmslv2_boundary_result *result
){
  if(result != NULL) {
    result->real_vars = NULL;
    result->real_values = NULL;
    result->real_count = 0;
    result->dis_vars = NULL;
    result->dis_values = NULL;
    result->dis_previous = NULL;
    result->dis_count = 0;
    slv9_cmslv2_boundary_flags_init(&result->bnd_flags);
    result->accepted = 0;
    result->local_complete = 0;
    result->at_boundary = 0;
    result->optimize_ok = 0;
    result->returned = 0;
    result->n_subregions = 0;
    result->cur_subregion = -1;
    result->dvars = 0;
    result->factor = 1.0;
  }
}

static
void slv9_cmslv2_boundary_result_destroy(
    struct slv9_cmslv2_boundary_result *result
){
  if(result == NULL) {
    return;
  }
  if(result->real_vars != NULL) {
    ascfree(result->real_vars);
  }
  if(result->real_values != NULL) {
    ascfree(result->real_values);
  }
  if(result->dis_vars != NULL) {
    ascfree(result->dis_vars);
  }
  if(result->dis_values != NULL) {
    ascfree(result->dis_values);
  }
  if(result->dis_previous != NULL) {
    ascfree(result->dis_previous);
  }
  slv9_cmslv2_boundary_flags_destroy(&result->bnd_flags);
  slv9_cmslv2_boundary_result_init(result);
}

static
int32 slv9_decomp_index_in_range(int32 value, const mtx_range_t *range){
  return range != NULL && value >= range->low && value <= range->high;
}

static
int32 slv9_decomp_org_row_in_block(
    const slv_decomp_partition_t *decomp, int32 block, int32 orgrow
){
  int32 currow;
  if(decomp == NULL || block < 0 || block >= decomp->nblocks
      || orgrow < 0 || orgrow >= decomp->n_rows) {
    return 0;
  }
  currow = decomp->row_cur[orgrow];
  return slv9_decomp_index_in_range(currow,&(decomp->blocks[block].row));
}

static
int32 slv9_decomp_org_col_in_block(
    const slv_decomp_partition_t *decomp, int32 block, int32 orgcol
){
  int32 curcol;
  if(decomp == NULL || block < 0 || block >= decomp->nblocks
      || orgcol < 0 || orgcol >= decomp->n_cols) {
    return 0;
  }
  curcol = decomp->col_cur[orgcol];
  return slv9_decomp_index_in_range(curcol,&(decomp->blocks[block].col));
}

static
void slv9_decomp_assess_block(slv_system_t server,
    const slv_decomp_partition_t *decomp, int32 block,
    struct slv9_decomp_block_stats *bs
){
  int32 r, c, nz, local;
  mtx_region_t region;

  slv9_decomp_block_stats_init(bs);
  if(decomp == NULL || block < 0 || block >= decomp->nblocks) {
    return;
  }
  region = decomp->blocks[block];
  bs->rows = region.row.high >= region.row.low
    ? region.row.high - region.row.low + 1 : 0;
  bs->cols = region.col.high >= region.col.low
    ? region.col.high - region.col.low + 1 : 0;

  for(r = region.row.low; r <= region.row.high; ++r) {
    slv_decomp_row_kind_t kind =
      slv_decomp_row_kind(decomp,decomp->row_org[r],&local);
    switch(kind) {
    case slv_decomp_row_rel:
    case slv_decomp_row_condrel:
      bs->relrows++;
      break;
    case slv_decomp_row_logrel:
    case slv_decomp_row_condlogrel:
      bs->logrows++;
      break;
    default:
      break;
    }
  }

  for(c = region.col.low; c <= region.col.high; ++c) {
    slv_decomp_col_kind_t kind =
      slv_decomp_col_kind(decomp,decomp->col_org[c],&local);
    switch(kind) {
    case slv_decomp_col_var:{
      struct var_variable **vars = slv_get_solvers_var_list(server);
      uint32 flags = var_flags(vars[local]);
      bs->vars++;
      if(flags & (VAR_INTEGER | VAR_BINARY | VAR_SEMICONT)) {
        bs->intvars++;
      }
      break;
    }
    case slv_decomp_col_dvar:
      bs->dvars++;
      break;
    default:
      break;
    }
  }

  for(nz = 0; nz < decomp->nnz; ++nz) {
    int32 orgrow = decomp->nz_rows[nz];
    int32 orgcol = decomp->nz_cols[nz];
    int32 currow, curcol;
    slv_decomp_row_kind_t rowkind;
    slv_decomp_col_kind_t colkind;
    currow = orgrow >= 0 && orgrow < decomp->n_rows
      ? decomp->row_cur[orgrow] : -1;
    if(!slv9_decomp_index_in_range(currow,&region.row)) {
      continue;
    }
    curcol = orgcol >= 0 && orgcol < decomp->n_cols
      ? decomp->col_cur[orgcol] : -1;
    if(!slv9_decomp_index_in_range(curcol,&region.col)) {
      continue;
    }
    rowkind = slv_decomp_row_kind(decomp,orgrow,&local);
    colkind = slv_decomp_col_kind(decomp,orgcol,&local);
    if((rowkind == slv_decomp_row_logrel
        || rowkind == slv_decomp_row_condlogrel)
        && colkind == slv_decomp_col_var) {
      bs->boundary_edges++;
    }
    if((rowkind == slv_decomp_row_rel
        || rowkind == slv_decomp_row_condrel)
        && colkind == slv_decomp_col_dvar) {
      bs->selector_edges++;
    }
  }
}

static
int32 slv9_decomp_block_is_selector_envelope(
    const struct slv9_decomp_block_stats *bs
){
  return bs != NULL
    && bs->relrows > 0
    && bs->logrows == 0
    && bs->vars > 0
    && bs->dvars > 0
    && bs->selector_edges > 0
    && bs->boundary_edges == 0;
}

static
int32 slv9_decomp_block_is_boundary_envelope(
    const struct slv9_decomp_block_stats *bs
){
  return bs != NULL
    && (bs->boundary_edges > 0 || (bs->relrows > 0 && bs->logrows > 0));
}

static
int32 slv9_decomp_block_is_pure_real(const struct slv9_decomp_block_stats *bs){
  return bs != NULL
    && bs->relrows > 0
    && bs->logrows == 0
    && bs->vars > 0
    && bs->dvars == 0
    && bs->intvars == 0;
}

static
int32 slv9_decomp_block_is_pure_logical(
    const struct slv9_decomp_block_stats *bs
){
  return bs != NULL
    && bs->logrows > 0
    && bs->relrows == 0
    && bs->dvars > 0
    && bs->vars == 0;
}

static
void slv9_decomp_tally_block(const struct slv9_decomp_block_stats *bs,
    struct slv9_decomp_summary *sum
){
  if(bs->rows > sum->max_rows) sum->max_rows = bs->rows;
  if(bs->cols > sum->max_cols) sum->max_cols = bs->cols;
  if(bs->rows == 0 || bs->cols == 0) {
    sum->empty++;
  }else if(slv9_decomp_block_is_pure_real(bs)) {
    sum->pure_real++;
  }else if(bs->relrows > 0 && bs->logrows == 0 && bs->vars > 0
      && bs->dvars == 0 && bs->intvars > 0) {
    sum->pure_integer++;
  }else if(slv9_decomp_block_is_pure_logical(bs)) {
    sum->pure_logical++;
  }else if(slv9_decomp_block_is_selector_envelope(bs)) {
    sum->selector_coupled++;
  }else if(slv9_decomp_block_is_boundary_envelope(bs)) {
    sum->boundary_mixed++;
  }else{
    sum->mixed++;
  }
}

static
void slv9_decomp_summarize(slv_system_t server,
    const slv_decomp_partition_t *decomp, struct slv9_decomp_summary *sum
){
  int32 b;
  struct slv9_decomp_block_stats bs;
  slv9_decomp_summary_init(sum);
  if(decomp == NULL) {
    return;
  }
  for(b = 0; b < decomp->nblocks; ++b) {
    slv9_decomp_assess_block(server,decomp,b,&bs);
    slv9_decomp_tally_block(&bs,sum);
  }
}

static
void slv9_cmslv2_plan_from_decomp(slv_system_t server,
    const slv_decomp_partition_t *structural,
    const slv_decomp_partition_t *active,
    struct slv9_cmslv2_plan_summary *plan
){
  int32 b;
  struct slv9_decomp_block_stats bs;

  slv9_cmslv2_plan_summary_init(plan);
  if(structural != NULL) {
    plan->structural_blocks = structural->nblocks;
    for(b = 0; b < structural->nblocks; ++b) {
      slv9_decomp_assess_block(server,structural,b,&bs);
      if(bs.rows == 0 || bs.cols == 0) {
        plan->skipped_blocks++;
      }else if(slv9_decomp_block_is_selector_envelope(&bs)) {
        plan->enum_blocks++;
      }else if(slv9_decomp_block_is_boundary_envelope(&bs)) {
        plan->boundary_blocks++;
      }else if(bs.intvars > 0) {
        plan->integer_blocks++;
      }else if(!((bs.relrows > 0 && bs.logrows == 0 && bs.vars > 0
            && bs.dvars == 0)
          || (bs.logrows > 0 && bs.relrows == 0 && bs.dvars > 0
            && bs.vars == 0))) {
        plan->fallback_blocks++;
      }
    }
  }

  if(active != NULL) {
    plan->active_blocks = active->nblocks;
    for(b = 0; b < active->nblocks; ++b) {
      slv9_decomp_assess_block(server,active,b,&bs);
      if(bs.rows == 0 || bs.cols == 0) {
        plan->skipped_blocks++;
      }else if(slv9_decomp_block_is_pure_real(&bs)) {
        plan->qrslv_blocks++;
        if(bs.rows > plan->max_qrslv_rows) plan->max_qrslv_rows = bs.rows;
        if(bs.cols > plan->max_qrslv_cols) plan->max_qrslv_cols = bs.cols;
      }else if(slv9_decomp_block_is_pure_logical(&bs)) {
        plan->lrslv_blocks++;
      }
    }
  }
}

static int32 slv9_decomp_active_block_in_structural_region(
    const slv_decomp_partition_t *active,
    int32 active_block,
    const slv_decomp_partition_t *structural,
    const mtx_region_t *sregion
);

static void slv9_cmslv2_tally_active_subblocks(slv_system_t server,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const slv_decomp_partition_t *active,
    struct slv9_cmslv2_selector_summary *sum
);

static void slv9_set_qrslv_external_mode(slv_system_t server,
    int32 external_blocks
);
static void slv9_set_lrslv_external_mode(slv_system_t server,
    int32 external_blocks
);
static int32 slv9_cmslv2_solve_lrslv_structural_run(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    int32 structural_start, int32 structural_end,
    const slv_decomp_partition_t *active, const char *phase,
    const char *event_name, int32 *installed_blocks
);
static void slv9_cmslv2_store_real_values(slv_system_t server,
    real64 **values, int32 *nvalues
);
static void slv9_cmslv2_restore_real_values(slv_system_t server,
    const real64 *values, int32 nvalues
);
static int32 slv9_cmslv2_check_block_currently_solved(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const char *phase, struct slv9_cmslv2_block_solved_report *report
);
static int32 slv9_cmslv2_validate_boundary_envelope(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const char *phase, real64 *max_residual, const char **reason
);
static int32 slv9_cmslv2_get_structural_partition(slv9_system_t sys,
    const char *phase, const slv_decomp_partition_t **structural
);
static void slv9_cmslv2_clear_structural_cache(slv9_system_t sys);
static int32 slv9_cmslv2_get_scheduler_structural(slv9_system_t sys,
    const char *phase, int32 fresh, slv_decomp_partition_t *storage,
    const slv_decomp_partition_t **structural
);

static
int32 slv9_cmslv2_block_qrslv_installable(
    const slv_decomp_partition_t *decomp, int32 block
){
  int32 r, c, local;
  mtx_region_t region;

  if(decomp == NULL || block < 0 || block >= decomp->nblocks) {
    return 0;
  }
  region = decomp->blocks[block];
  if((region.row.high - region.row.low)
      != (region.col.high - region.col.low)) {
    return 0;
  }
  for(r = region.row.low; r <= region.row.high; ++r) {
    if(slv_decomp_row_kind(decomp,decomp->row_org[r],&local)
        != slv_decomp_row_rel) {
      return 0;
    }
  }
  for(c = region.col.low; c <= region.col.high; ++c) {
    if(slv_decomp_col_kind(decomp,decomp->col_org[c],&local)
        != slv_decomp_col_var) {
      return 0;
    }
  }
  return 1;
}

static
void slv9_cmslv2_restore_qrslv_scope(slv9_system_t sys){
  int32 i;
  struct rel_relation **rels;
  struct var_variable **vars;

  if(sys == NULL || sys->slv == NULL || !sys->cmslv2_scope_active) {
    return;
  }
  rels = slv_get_solvers_rel_list(sys->slv);
  vars = slv_get_solvers_var_list(sys->slv);
  if(rels != NULL && sys->cmslv2_rel_active_save != NULL) {
    for(i = 0; i < sys->cmslv2_scope_nrels; ++i) {
      rel_set_active(rels[i],sys->cmslv2_rel_active_save[i]);
    }
  }
  if(vars != NULL && sys->cmslv2_var_active_save != NULL) {
    for(i = 0; i < sys->cmslv2_scope_nvars; ++i) {
      var_set_active(vars[i],sys->cmslv2_var_active_save[i]);
    }
  }
  if(sys->cmslv2_rel_active_save != NULL) {
    ascfree(sys->cmslv2_rel_active_save);
  }
  if(sys->cmslv2_var_active_save != NULL) {
    ascfree(sys->cmslv2_var_active_save);
  }
  sys->cmslv2_rel_active_save = NULL;
  sys->cmslv2_var_active_save = NULL;
  sys->cmslv2_scope_active = 0;
  sys->cmslv2_scope_nrels = 0;
  sys->cmslv2_scope_nvars = 0;
}

static
int32 slv9_cmslv2_active_block_in_structural_block(
    const slv_decomp_partition_t *active, int32 active_block,
    const slv_decomp_partition_t *structural, int32 structural_block
){
  if(structural == NULL || structural_block < 0
      || structural_block >= structural->nblocks) {
    return 0;
  }
  return slv9_decomp_active_block_in_structural_region(
    active,active_block,structural,&(structural->blocks[structural_block])
  );
}

static
int32 slv9_cmslv2_structural_block_qrslv_ready(slv_system_t server,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const slv_decomp_partition_t *active,
    struct slv9_cmslv2_selector_summary *sum
){
  struct slv9_decomp_block_stats bs;
  if(sum != NULL) {
    slv9_cmslv2_selector_summary_init(sum);
  }
  if(server == NULL || structural == NULL || active == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return 0;
  }
  slv9_decomp_assess_block(server,structural,structural_block,&bs);
  if(!slv9_decomp_block_is_pure_real(&bs)) {
    return 0;
  }
  if(sum != NULL) {
    slv9_cmslv2_tally_active_subblocks(
      server,structural,structural_block,active,sum
    );
    return sum->qrslv_subblocks > 0 && sum->unresolved_subblocks == 0;
  }else{
    struct slv9_cmslv2_selector_summary local;
    slv9_cmslv2_selector_summary_init(&local);
    slv9_cmslv2_tally_active_subblocks(
      server,structural,structural_block,active,&local
    );
    return local.qrslv_subblocks > 0 && local.unresolved_subblocks == 0;
  }
}

static
int32 slv9_cmslv2_structural_block_lrslv_ready(slv_system_t server,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const slv_decomp_partition_t *active,
    struct slv9_cmslv2_selector_summary *sum
){
  struct slv9_decomp_block_stats bs;
  if(sum != NULL) {
    slv9_cmslv2_selector_summary_init(sum);
  }
  if(server == NULL || structural == NULL || active == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return 0;
  }
  slv9_decomp_assess_block(server,structural,structural_block,&bs);
  if(!slv9_decomp_block_is_pure_logical(&bs)) {
    return 0;
  }
  if(sum != NULL) {
    slv9_cmslv2_tally_active_subblocks(
      server,structural,structural_block,active,sum
    );
    return sum->lrslv_subblocks > 0 && sum->qrslv_subblocks == 0
      && sum->unresolved_subblocks == 0;
  }else{
    struct slv9_cmslv2_selector_summary local;
    slv9_cmslv2_selector_summary_init(&local);
    slv9_cmslv2_tally_active_subblocks(
      server,structural,structural_block,active,&local
    );
    return local.lrslv_subblocks > 0 && local.qrslv_subblocks == 0
      && local.unresolved_subblocks == 0;
  }
}

static
enum slv9_cmslv2_work_kind slv9_cmslv2_classify_structural_block(
    slv_system_t server, const slv_decomp_partition_t *structural,
    int32 structural_block, const slv_decomp_partition_t *active,
    struct slv9_decomp_block_stats *stats,
    struct slv9_cmslv2_selector_summary *summary
){
  struct slv9_decomp_block_stats local_stats;
  struct slv9_cmslv2_selector_summary local_summary;

  if(stats == NULL) {
    stats = &local_stats;
  }
  if(summary == NULL) {
    summary = &local_summary;
  }
  slv9_decomp_block_stats_init(stats);
  slv9_cmslv2_selector_summary_init(summary);
  if(server == NULL || structural == NULL || active == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return SLV9_CMSLV2_WORK_END;
  }

  slv9_decomp_assess_block(server,structural,structural_block,stats);
  if(stats->rows == 0 || stats->cols == 0) {
    return SLV9_CMSLV2_WORK_EMPTY;
  }
  if(slv9_cmslv2_structural_block_qrslv_ready(
      server,structural,structural_block,active,summary
  )) {
    return SLV9_CMSLV2_WORK_QRSLV;
  }
  if(slv9_cmslv2_structural_block_lrslv_ready(
      server,structural,structural_block,active,summary
  )) {
    return SLV9_CMSLV2_WORK_LRSLV;
  }
  slv9_cmslv2_tally_active_subblocks(
    server,structural,structural_block,active,summary
  );
  if(slv9_decomp_block_is_selector_envelope(stats)) {
    return SLV9_CMSLV2_WORK_SELECTOR;
  }
  if(slv9_decomp_block_is_boundary_envelope(stats)) {
    return SLV9_CMSLV2_WORK_BOUNDARY;
  }
  if(stats->intvars > 0 && stats->dvars == 0 && stats->logrows == 0) {
    return SLV9_CMSLV2_WORK_INTEGER;
  }
  return SLV9_CMSLV2_WORK_MIXED;
}

static
int32 slv9_cmslv2_scheduler_query(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    const slv_decomp_partition_t *active,
    int32 mode, struct slv9_cmslv2_work_unit *work
){
  enum slv9_cmslv2_work_kind kind;
  int32 cursor;

  if(work != NULL) {
    slv9_cmslv2_work_unit_init(work);
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL || active == NULL
      || work == NULL || structural->nblocks <= 0) {
    return 0;
  }
  cursor = sys->cmslv2_next_structural_block;
  if(cursor < 0 || cursor >= structural->nblocks) {
    return 0;
  }

  for(; cursor < structural->nblocks; ++cursor) {
    kind = slv9_cmslv2_classify_structural_block(
      sys->slv,structural,cursor,active,&work->stats,&work->summary
    );
    if(kind == SLV9_CMSLV2_WORK_EMPTY
        && (mode & SLV9_CMSLV2_QUERY_SKIP_EMPTY)) {
      continue;
    }
    work->kind = kind;
    work->start = cursor;
    work->end = cursor;
    break;
  }
  if(work->start < 0) {
    return 0;
  }

  if((mode & SLV9_CMSLV2_QUERY_SEQUENCE)
      && (work->kind == SLV9_CMSLV2_WORK_QRSLV
        || work->kind == SLV9_CMSLV2_WORK_LRSLV)) {
    struct slv9_decomp_block_stats next_stats;
    struct slv9_cmslv2_selector_summary next_summary;
    for(cursor = work->start + 1; cursor < structural->nblocks; ++cursor) {
      kind = slv9_cmslv2_classify_structural_block(
        sys->slv,structural,cursor,active,&next_stats,&next_summary
      );
      if(kind != work->kind) {
        break;
      }
      work->end = cursor;
      work->summary.active_subblocks += next_summary.active_subblocks;
      work->summary.qrslv_subblocks += next_summary.qrslv_subblocks;
      work->summary.lrslv_subblocks += next_summary.lrslv_subblocks;
      work->summary.unresolved_subblocks += next_summary.unresolved_subblocks;
      if(next_summary.max_qrslv_rows > work->summary.max_qrslv_rows) {
        work->summary.max_qrslv_rows = next_summary.max_qrslv_rows;
      }
      if(next_summary.max_qrslv_cols > work->summary.max_qrslv_cols) {
        work->summary.max_qrslv_cols = next_summary.max_qrslv_cols;
      }
    }
  }
  return 1;
}

static
int32 slv9_cmslv2_consume_due_lrslv_blocks(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    const slv_decomp_partition_t *active, const char *phase,
    int32 *run_start_out, int32 *run_end_out
){
  struct slv9_cmslv2_work_unit work;
  int32 run_start, run_end;
  int32 solved;

  if(run_start_out != NULL) *run_start_out = -1;
  if(run_end_out != NULL) *run_end_out = -1;
  if(sys == NULL || sys->slv == NULL || structural == NULL || active == NULL
      || structural->nblocks <= 0 || active->nblocks <= 0) {
    return 0;
  }

  if(!slv9_cmslv2_scheduler_query(
      sys,structural,active,
      SLV9_CMSLV2_QUERY_SEQUENCE | SLV9_CMSLV2_QUERY_SKIP_EMPTY,&work
  )) {
    return 0;
  }
  if(work.kind != SLV9_CMSLV2_WORK_LRSLV) {
    sys->cmslv2_next_structural_block = work.start;
    return 0;
  }
  run_start = work.start;
  run_end = work.end;

  solved = slv9_cmslv2_solve_lrslv_structural_run(
    sys,structural,run_start,run_end,active,phase,
    "cmslv2_lrslv_handoff",NULL
  );
  slv9_report_progress(sys,
    "event=cmslv2_scheduler phase=%s structural_start=%d structural_end=%d action=%s",
    phase != NULL ? phase : "unknown", run_start, run_end,
    solved ? "lrslv_advance" : "lrslv_defer"
  );
  if(solved) {
    sys->cmslv2_next_structural_block = run_end + 1;
    sys->cmslv2_pending_after_qrslv =
      sys->cmslv2_next_structural_block < structural->nblocks;
    slv9_reanalyze_solver_lists(sys);
    update_relations_residuals(sys->slv);
    if(run_start_out != NULL) *run_start_out = run_start;
    if(run_end_out != NULL) *run_end_out = run_end;
    return 1;
  }
  return 0;
}

static
int32 slv9_cmslv2_install_due_qrslv_blocks(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    const slv_decomp_partition_t *active, const char *phase,
    int32 *installed_rows, int32 *installed_cols,
    int32 *struct_start, int32 *struct_end
){
  slv_system_t server;
  struct rel_relation **rels, **newrels;
  struct var_variable **vars, **newvars;
  unsigned char *relused, *varused;
  mtx_region_t *blocks;
  int32 nrels, nvars;
  int32 selected_rows, selected_cols;
  int32 b, r, c, local, nblocks;
  int32 rowpos, colpos;
  int32 run_start, run_end;
  struct slv9_cmslv2_work_unit work;

  if(installed_rows != NULL) *installed_rows = 0;
  if(installed_cols != NULL) *installed_cols = 0;
  if(struct_start != NULL) *struct_start = -1;
  if(struct_end != NULL) *struct_end = -1;
  if(sys == NULL || structural == NULL || active == NULL) {
    return 0;
  }
  server = sys->slv;
  if(server == NULL || structural->nblocks <= 0 || active->nblocks <= 0) {
    return 0;
  }

  slv9_cmslv2_restore_qrslv_scope(sys);

  if(!slv9_cmslv2_scheduler_query(
      sys,structural,active,
      SLV9_CMSLV2_QUERY_SEQUENCE | SLV9_CMSLV2_QUERY_SKIP_EMPTY,&work
  )) {
    return 0;
  }
  if(work.kind != SLV9_CMSLV2_WORK_QRSLV) {
    slv9_report_progress(sys,
      "event=cmslv2_scheduler phase=%s structural_block=%d work=%s action=defer_subsolver qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d",
      phase != NULL ? phase : "unknown", work.start,
      slv9_cmslv2_work_kind_name(work.kind),
      work.summary.qrslv_subblocks, work.summary.lrslv_subblocks,
      work.summary.unresolved_subblocks
    );
    sys->cmslv2_next_structural_block = work.start;
    return 0;
  }
  run_start = work.start;
  run_end = work.end;

  nrels = slv_get_num_solvers_rels(server);
  nvars = slv_get_num_solvers_vars(server);
  rels = slv_get_solvers_rel_list(server);
  vars = slv_get_solvers_var_list(server);
  if(rels == NULL || vars == NULL) {
    return 0;
  }

  newrels = ASC_NEW_ARRAY_CLEAR(struct rel_relation *,nrels + 1);
  newvars = ASC_NEW_ARRAY_CLEAR(struct var_variable *,nvars + 1);
  relused = ASC_NEW_ARRAY_CLEAR(unsigned char,nrels);
  varused = ASC_NEW_ARRAY_CLEAR(unsigned char,nvars);
  blocks = ASC_NEW_ARRAY_CLEAR(mtx_region_t,active->nblocks);
  if(newrels == NULL || newvars == NULL || relused == NULL
      || varused == NULL || blocks == NULL) {
    if(newrels != NULL) ascfree(newrels);
    if(newvars != NULL) ascfree(newvars);
    if(relused != NULL) ascfree(relused);
    if(varused != NULL) ascfree(varused);
    if(blocks != NULL) ascfree(blocks);
    return 0;
  }

  nblocks = 0;
  rowpos = 0;
  colpos = 0;
  for(b = 0; b < active->nblocks; ++b) {
    struct slv9_decomp_block_stats bs;
    mtx_region_t region = active->blocks[b];
    int32 rowlow, collow;
    int32 in_run = 0;
    int32 sblock;

    for(sblock = run_start; sblock <= run_end; ++sblock) {
      if(slv9_cmslv2_active_block_in_structural_block(
          active,b,structural,sblock
      )) {
        in_run = 1;
        break;
      }
    }
    if(!in_run) {
      continue;
    }
    slv9_decomp_assess_block(server,active,b,&bs);
    if(!slv9_decomp_block_is_pure_real(&bs)) {
      continue;
    }
    if(!slv9_cmslv2_block_qrslv_installable(active,b)) {
      goto fail;
    }

    rowlow = rowpos;
    collow = colpos;
    for(r = region.row.low; r <= region.row.high; ++r) {
      (void)slv_decomp_row_kind(active,active->row_org[r],&local);
      if(local < 0 || local >= nrels || relused[local]) {
        goto fail;
      }
      newrels[rowpos++] = rels[local];
      relused[local] = 1;
    }
    for(c = region.col.low; c <= region.col.high; ++c) {
      (void)slv_decomp_col_kind(active,active->col_org[c],&local);
      if(local < 0 || local >= nvars || varused[local]) {
        goto fail;
      }
      newvars[colpos++] = vars[local];
      varused[local] = 1;
    }

    blocks[nblocks].row.low = rowlow;
    blocks[nblocks].row.high = rowpos - 1;
    blocks[nblocks].col.low = collow;
    blocks[nblocks].col.high = colpos - 1;
    nblocks++;
  }

  selected_rows = rowpos;
  selected_cols = colpos;
  if(nblocks <= 0 || selected_rows <= 0 || selected_cols <= 0) {
    goto fail;
  }

  for(r = 0; r < nrels; ++r) {
    if(!relused[r]) {
      newrels[rowpos++] = rels[r];
    }
  }
  for(c = 0; c < nvars; ++c) {
    if(!varused[c]) {
      newvars[colpos++] = vars[c];
    }
  }
  for(r = 0; r < nrels; ++r) {
    rels[r] = newrels[r];
    rel_set_sindex(rels[r],r);
  }
  for(c = 0; c < nvars; ++c) {
    vars[c] = newvars[c];
    var_set_sindex(vars[c],c);
  }
  sys->cmslv2_rel_active_save = ASC_NEW_ARRAY(unsigned char,nrels);
  sys->cmslv2_var_active_save = ASC_NEW_ARRAY(unsigned char,nvars);
  if(sys->cmslv2_rel_active_save == NULL
      || sys->cmslv2_var_active_save == NULL) {
    goto fail_after_reorder;
  }
  for(r = 0; r < nrels; ++r) {
    sys->cmslv2_rel_active_save[r] = rel_active(rels[r]) ? 1 : 0;
    rel_set_active(rels[r],r < selected_rows ? TRUE : FALSE);
  }
  for(c = 0; c < nvars; ++c) {
    sys->cmslv2_var_active_save[c] = var_active(vars[c]) ? 1 : 0;
    var_set_active(vars[c],c < selected_cols ? TRUE : FALSE);
  }
  sys->cmslv2_scope_active = 1;
  sys->cmslv2_scope_nrels = nrels;
  sys->cmslv2_scope_nvars = nvars;
  slv_set_solvers_blocks(server,nblocks,blocks);
  blocks = NULL;
  if(installed_rows != NULL) *installed_rows = selected_rows;
  if(installed_cols != NULL) *installed_cols = selected_cols;
  if(struct_start != NULL) *struct_start = run_start;
  if(struct_end != NULL) *struct_end = run_end;
  sys->cmslv2_next_structural_block = run_end + 1;
  sys->cmslv2_last_structural_blocks = structural->nblocks;
  sys->cmslv2_pending_after_qrslv =
    sys->cmslv2_next_structural_block < structural->nblocks;

  ascfree(newrels);
  ascfree(newvars);
  ascfree(relused);
  ascfree(varused);
  return nblocks;

fail_after_reorder:
  if(sys->cmslv2_rel_active_save != NULL) {
    ascfree(sys->cmslv2_rel_active_save);
    sys->cmslv2_rel_active_save = NULL;
  }
  if(sys->cmslv2_var_active_save != NULL) {
    ascfree(sys->cmslv2_var_active_save);
    sys->cmslv2_var_active_save = NULL;
  }
  sys->cmslv2_scope_active = 0;
  if(blocks != NULL) ascfree(blocks);
  ascfree(newrels);
  ascfree(newvars);
  ascfree(relused);
  ascfree(varused);
  return 0;

fail:
  ascfree(newrels);
  ascfree(newvars);
  ascfree(relused);
  ascfree(varused);
  if(blocks != NULL) ascfree(blocks);
  return 0;
}

static
void slv9_cmslv2_save_solver_order(slv_system_t server,
    struct rel_relation ***rel_order, int32 *nrels,
    struct var_variable ***var_order, int32 *nvars
){
  struct rel_relation **rels;
  struct var_variable **vars;

  if(rel_order != NULL) *rel_order = NULL;
  if(var_order != NULL) *var_order = NULL;
  if(nrels != NULL) *nrels = 0;
  if(nvars != NULL) *nvars = 0;
  if(server == NULL || rel_order == NULL || var_order == NULL
      || nrels == NULL || nvars == NULL) {
    return;
  }

  *nrels = slv_get_num_solvers_rels(server);
  *nvars = slv_get_num_solvers_vars(server);
  rels = slv_get_solvers_rel_list(server);
  vars = slv_get_solvers_var_list(server);
  if(rels != NULL && *nrels > 0) {
    *rel_order = ASC_NEW_ARRAY(struct rel_relation *,*nrels);
    if(*rel_order != NULL) {
      memcpy(*rel_order,rels,(size_t)(*nrels) * sizeof(struct rel_relation *));
    }
  }
  if(vars != NULL && *nvars > 0) {
    *var_order = ASC_NEW_ARRAY(struct var_variable *,*nvars);
    if(*var_order != NULL) {
      memcpy(*var_order,vars,(size_t)(*nvars) * sizeof(struct var_variable *));
    }
  }
}

static
void slv9_cmslv2_restore_solver_order(slv_system_t server,
    struct rel_relation **rel_order, int32 nrels,
    struct var_variable **var_order, int32 nvars
){
  int32 i;
  struct rel_relation **rels;
  struct var_variable **vars;

  if(server == NULL) {
    return;
  }
  rels = slv_get_solvers_rel_list(server);
  vars = slv_get_solvers_var_list(server);
  if(rels != NULL && rel_order != NULL) {
    for(i = 0; i < nrels; ++i) {
      rels[i] = rel_order[i];
      rel_set_sindex(rels[i],i);
    }
  }
  if(vars != NULL && var_order != NULL) {
    for(i = 0; i < nvars; ++i) {
      vars[i] = var_order[i];
      var_set_sindex(vars[i],i);
    }
  }
}

static
int32 slv9_cmslv2_solve_due_qrslv_run(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    const slv_decomp_partition_t *active, const char *phase,
    const char *event_name, int32 *installed_blocks
){
  slv_system_t server;
  SlvClientToken orig_token;
  int32 orig_solver;
  struct rel_relation **rel_order = NULL;
  struct var_variable **var_order = NULL;
  real64 *orig_real_values = NULL;
  int32 orig_real_count = 0;
  int32 nrels = 0, nvars = 0;
  int32 rows = 0, cols = 0, blocks;
  int32 structural_start = -1, structural_end = -1;
  int32 orig_cursor;
  int32 converged = 0, boundary_crossed = 0, accepted = 0;
  const char *fallback_reason = "none";
  slv_status_t status;

  if(installed_blocks != NULL) {
    *installed_blocks = 0;
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL || active == NULL) {
    return 0;
  }
  server = sys->slv;
  orig_token = slv_get_client_token(server);
  orig_solver = slv_get_selected_solver(server);
  orig_cursor = sys->cmslv2_next_structural_block;
  slv9_cmslv2_save_solver_order(
    server,&rel_order,&nrels,&var_order,&nvars
  );
  slv9_cmslv2_store_real_values(
    server,&orig_real_values,&orig_real_count
  );

  blocks = slv9_cmslv2_install_due_qrslv_blocks(
    sys,structural,active,phase,&rows,&cols,&structural_start,&structural_end
  );
  if(installed_blocks != NULL) {
    *installed_blocks = blocks;
  }
  if(blocks <= 0) {
    fallback_reason = "no_due_qrslv_run";
    slv9_report_progress(sys,
      "event=%s phase=%s structural_start=%d structural_end=%d installed_blocks=0 action=defer reason=%s",
      event_name != NULL ? event_name : "cmslv2_qrslv_local",
      phase != NULL ? phase : "unknown", structural_start, structural_end,
      fallback_reason
    );
    goto finish;
  }

  slv9_set_qrslv_external_mode(server,1);
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  slv_presolve(server);
  slv_get_status(server,&status);
  if(status.ready_to_solve || status.converged) {
    slv_solve(server);
    slv_get_status(server,&status);
  }
  converged = status.ok && status.converged;
  update_boundaries(server,(SlvClientToken)sys);
  boundary_crossed = some_boundaries_crossed(server,(SlvClientToken)sys);
  accepted = converged;
  if(!converged) {
    fallback_reason = "qrslv_not_converged";
  }
  slv9_report_progress(sys,
    "event=%s phase=%s structural_start=%d structural_end=%d installed_blocks=%d installed_rows=%d installed_cols=%d converged=%d ok=%d boundary_crossed=%d action=%s reason=%s",
    event_name != NULL ? event_name : "cmslv2_qrslv_local",
    phase != NULL ? phase : "unknown", structural_start, structural_end,
    blocks, rows, cols, converged ? 1 : 0, status.ok ? 1 : 0,
    boundary_crossed ? 1 : 0, accepted ? "advance" : "defer",
    fallback_reason
  );

finish:
  if(!accepted) {
    if(orig_real_values != NULL) {
      slv9_cmslv2_restore_real_values(
        server,orig_real_values,orig_real_count
      );
    }
    sys->cmslv2_next_structural_block = orig_cursor;
    sys->cmslv2_pending_after_qrslv =
      orig_cursor < (structural != NULL ? structural->nblocks : 0);
  }
  slv9_cmslv2_restore_qrslv_scope(sys);
  slv9_cmslv2_restore_solver_order(server,rel_order,nrels,var_order,nvars);
  if(rel_order != NULL) ascfree(rel_order);
  if(var_order != NULL) ascfree(var_order);
  if(orig_real_values != NULL) ascfree(orig_real_values);
  slv9_set_qrslv_external_mode(server,0);
  slv_set_solver_index(server,orig_solver);
  slv_set_client_token(server,orig_token);
  update_relations_residuals(server);
  return accepted;
}

static
void slv9_cmslv2_store_real_values(slv_system_t server, real64 **values,
    int32 *nvalues
){
  int32 i, nvars;
  struct var_variable **master;

  if(values != NULL) *values = NULL;
  if(nvalues != NULL) *nvalues = 0;
  if(server == NULL || values == NULL || nvalues == NULL) {
    return;
  }
  nvars = slv_get_num_master_vars(server);
  master = slv_get_master_var_list(server);
  if(master == NULL || nvars <= 0) {
    return;
  }
  *values = ASC_NEW_ARRAY(real64,nvars);
  if(*values == NULL) {
    return;
  }
  *nvalues = nvars;
  for(i = 0; i < nvars; ++i) {
    (*values)[i] = var_value(master[i]);
  }
}

static
void slv9_cmslv2_store_discrete_values(slv_system_t server, int32 **values,
    int32 **previous, int32 *nvalues
){
  int32 i, ndvars;
  struct dis_discrete **dvars;

  if(values != NULL) *values = NULL;
  if(previous != NULL) *previous = NULL;
  if(nvalues != NULL) *nvalues = 0;
  if(server == NULL || values == NULL || previous == NULL
      || nvalues == NULL) {
    return;
  }
  ndvars = slv_get_num_solvers_dvars(server);
  dvars = slv_get_solvers_dvar_list(server);
  if(dvars == NULL || ndvars <= 0) {
    return;
  }
  *values = ASC_NEW_ARRAY(int32,ndvars);
  *previous = ASC_NEW_ARRAY(int32,ndvars);
  if(*values == NULL || *previous == NULL) {
    if(*values != NULL) ascfree(*values);
    if(*previous != NULL) ascfree(*previous);
    *values = NULL;
    *previous = NULL;
    return;
  }
  *nvalues = ndvars;
  for(i = 0; i < ndvars; ++i) {
    (*values)[i] = dis_value(dvars[i]);
    (*previous)[i] = dis_previous_value(dvars[i]);
  }
}

static
void slv9_cmslv2_restore_discrete_values(slv_system_t server,
    const int32 *values, const int32 *previous, int32 nvalues
){
  int32 i, ndvars;
  struct dis_discrete **dvars;

  if(server == NULL || values == NULL || previous == NULL || nvalues <= 0) {
    return;
  }
  ndvars = slv_get_num_solvers_dvars(server);
  dvars = slv_get_solvers_dvar_list(server);
  if(dvars == NULL || ndvars <= 0) {
    return;
  }
  if(nvalues < ndvars) {
    ndvars = nvalues;
  }
  for(i = 0; i < ndvars; ++i) {
    if(dis_kind(dvars[i]) == e_dis_boolean_t) {
      dis_set_boolean_value(dvars[i],values[i]);
    }else{
      dis_set_value(dvars[i],values[i]);
    }
    dis_set_previous_value(dvars[i],previous[i]);
  }
}

static
void slv9_cmslv2_restore_real_values(slv_system_t server,
    const real64 *values, int32 nvalues
){
  int32 i, nvars;
  struct var_variable **master;

  if(server == NULL || values == NULL || nvalues <= 0) {
    return;
  }
  nvars = slv_get_num_master_vars(server);
  master = slv_get_master_var_list(server);
  if(master == NULL || nvars <= 0) {
    return;
  }
  if(nvalues < nvars) {
    nvars = nvalues;
  }
  for(i = 0; i < nvars; ++i) {
    var_set_value(master[i],values[i]);
  }
}

static
int32 slv9_cmslv2_boundary_result_capture(slv_system_t server,
    const slv_decomp_partition_t *structural, int32 structural_block,
    struct slv9_cmslv2_boundary_result *result,
    int32 at_boundary, int32 opt_ok, int32 returned,
    int32 n_subregions, int32 cur_subregion, unsigned long dvars,
    real64 factor
){
  mtx_region_t region;
  struct var_variable **vars;
  struct dis_discrete **dvars_list;
  int32 c, kind, local, real_count, dis_count, rpos, dpos;
  int32 nvars, ndvars;

  if(server == NULL || structural == NULL || result == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return 0;
  }
  slv9_cmslv2_boundary_result_destroy(result);
  result->at_boundary = at_boundary;
  result->optimize_ok = opt_ok;
  result->returned = returned;
  result->n_subregions = n_subregions;
  result->cur_subregion = cur_subregion;
  result->dvars = dvars;
  result->factor = factor;
  if(!at_boundary || !opt_ok) {
    return 0;
  }

  vars = slv_get_solvers_var_list(server);
  dvars_list = slv_get_solvers_dvar_list(server);
  nvars = slv_get_num_solvers_vars(server);
  ndvars = slv_get_num_solvers_dvars(server);
  region = structural->blocks[structural_block];
  real_count = 0;
  dis_count = 0;
  for(c = region.col.low; c <= region.col.high; ++c) {
    kind = slv_decomp_col_kind(structural,structural->col_org[c],&local);
    if(kind == slv_decomp_col_var && local >= 0 && local < nvars) {
      real_count++;
    }else if(kind == slv_decomp_col_dvar && local >= 0 && local < ndvars) {
      dis_count++;
    }
  }
  if(real_count > 0) {
    result->real_vars = ASC_NEW_ARRAY(struct var_variable *,real_count);
    result->real_values = ASC_NEW_ARRAY(real64,real_count);
  }
  if(dis_count > 0) {
    result->dis_vars = ASC_NEW_ARRAY(struct dis_discrete *,dis_count);
    result->dis_values = ASC_NEW_ARRAY(int32,dis_count);
    result->dis_previous = ASC_NEW_ARRAY(int32,dis_count);
  }
  slv9_cmslv2_store_boundary_flags(server,&result->bnd_flags);
  if((real_count > 0 && (result->real_vars == NULL
        || result->real_values == NULL))
      || (dis_count > 0 && (result->dis_vars == NULL
        || result->dis_values == NULL || result->dis_previous == NULL))
      || result->bnd_flags.flags == NULL) {
    slv9_cmslv2_boundary_result_destroy(result);
    return 0;
  }

  rpos = 0;
  dpos = 0;
  for(c = region.col.low; c <= region.col.high; ++c) {
    kind = slv_decomp_col_kind(structural,structural->col_org[c],&local);
    if(kind == slv_decomp_col_var && vars != NULL
        && local >= 0 && local < nvars) {
      result->real_vars[rpos] = vars[local];
      result->real_values[rpos] = var_value(vars[local]);
      rpos++;
    }else if(kind == slv_decomp_col_dvar && dvars_list != NULL
        && local >= 0 && local < ndvars) {
      result->dis_vars[dpos] = dvars_list[local];
      result->dis_values[dpos] = dis_value(dvars_list[local]);
      result->dis_previous[dpos] = dis_previous_value(dvars_list[local]);
      dpos++;
    }
  }
  if(rpos != real_count || dpos != dis_count) {
    slv9_cmslv2_boundary_result_destroy(result);
    return 0;
  }
  result->real_count = real_count;
  result->dis_count = dis_count;
  result->accepted = 1;
  return 1;
}

static
void slv9_cmslv2_boundary_result_apply(slv9_system_t sys,
    const char *phase, int32 structural_block,
    const struct slv9_cmslv2_boundary_result *result
){
  int32 i;
  if(sys == NULL || sys->slv == NULL || result == NULL
      || !result->accepted) {
    return;
  }
  for(i = 0; i < result->real_count; ++i) {
    if(result->real_vars[i] != NULL) {
      var_set_value(result->real_vars[i],result->real_values[i]);
    }
  }
  for(i = 0; i < result->dis_count; ++i) {
    if(result->dis_vars[i] != NULL) {
      if(dis_kind(result->dis_vars[i]) == e_dis_boolean_t) {
        dis_set_boolean_value(result->dis_vars[i],result->dis_values[i]);
      }else{
        dis_set_value(result->dis_vars[i],result->dis_values[i]);
      }
      dis_set_previous_value(result->dis_vars[i],result->dis_previous[i]);
    }
  }
  /*
   * Preserve active/inactive row outcomes by replaying the accepted discrete
   * values through the existing WHEN/logical analysis, rather than copying
   * scoped active bits directly back into the full system.
   */
  slv9_reanalyze_solver_lists(sys);
  slv9_cmslv2_clear_structural_cache(sys);
  update_boundaries(sys->slv,(SlvClientToken)sys);
  slv9_cmslv2_restore_boundary_flags(sys->slv,&result->bnd_flags);
  update_relations_residuals(sys->slv);
  slv9_report_progress(sys,
    "event=cmslv2_boundary_local_commit phase=%s block=%d action=accepted at_boundary=%d optimize_ok=%d returned=%d n_subregions=%d cur_subregion=%d dvars=%lu factor=%.17g",
    phase != NULL ? phase : "unknown", structural_block,
    result->at_boundary, result->optimize_ok, result->returned,
    result->n_subregions, result->cur_subregion, result->dvars,
    result->factor
  );
}

static
int32 slv9_cmslv2_check_block_currently_solved(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const char *phase, struct slv9_cmslv2_block_solved_report *report
){
  slv_system_t server;
  slv_decomp_partition_t active;
  struct slv9_cmslv2_selector_summary local;
  struct rel_relation **rels, **condrels;
  struct logrel_relation **logrels, **condlogrels;
  mtx_region_t region;
  int32 r, local_index, status, active_status;
  struct slv9_cmslv2_block_solved_report local_report;

  slv9_cmslv2_block_solved_report_init(&local_report);
  local_report.reason = "invalid_input";
  if(sys == NULL || sys->slv == NULL || structural == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    if(report != NULL) {
      *report = local_report;
    }
    return 0;
  }
  server = sys->slv;
  rels = slv_get_solvers_rel_list(server);
  condrels = slv_get_solvers_condrel_list(server);
  logrels = slv_get_solvers_logrel_list(server);
  condlogrels = slv_get_solvers_condlogrel_list(server);
  region = structural->blocks[structural_block];

  update_boundaries(server,(SlvClientToken)sys);

#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
#endif
  for(r = region.row.low; r <= region.row.high; ++r) {
    slv_decomp_row_kind_t kind =
      slv_decomp_row_kind(structural,structural->row_org[r],&local_index);
    struct rel_relation *rel = NULL;
    struct logrel_relation *lrel = NULL;
    real64 res;
    switch(kind) {
    case slv_decomp_row_rel:
      if(rels != NULL && local_index >= 0
          && local_index < slv_get_num_solvers_rels(server)) {
        rel = rels[local_index];
      }
      break;
    case slv_decomp_row_condrel:
      if(condrels != NULL && local_index >= 0
          && local_index < slv_get_num_solvers_condrels(server)) {
        rel = condrels[local_index];
      }
      break;
    case slv_decomp_row_logrel:
      if(logrels != NULL && local_index >= 0
          && local_index < slv_get_num_solvers_logrels(server)) {
        lrel = logrels[local_index];
      }
      break;
    case slv_decomp_row_condlogrel:
      if(condlogrels != NULL && local_index >= 0
          && local_index < slv_get_num_solvers_condlogrels(server)) {
        lrel = condlogrels[local_index];
      }
      break;
    default:
      break;
    }
    if(rel != NULL) {
      if(!rel_active(rel)) {
        continue;
      }
      local_report.real_rels_checked++;
      res = relman_eval(rel,&status,1);
      if(!status) {
        local_report.calc_errors++;
        local_report.reason = "residual_eval_failed";
        goto fail;
      }
      res = fabs(res);
      if(res > local_report.max_real_residual) {
        local_report.max_real_residual = res;
      }
      if(res > 1e-6) {
        local_report.real_rel_failures++;
      }
    }else if(lrel != NULL) {
      if(!logrel_active(lrel)) {
        continue;
      }
      local_report.logrels_checked++;
      if(!logrel_satisfied(lrel)) {
        local_report.logrel_failures++;
      }
    }
  }
#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif

  if(local_report.real_rel_failures > 0) {
    local_report.reason = "residual_not_satisfied";
    goto fail_no_signal;
  }
  if(local_report.logrel_failures > 0) {
    local_report.reason = "logrel_not_satisfied";
    goto fail_no_signal;
  }

  slv_decomp_init(&active);
  active_status = slv_decomp_partition_active(server,&active);
  if(active_status) {
    slv_decomp_destroy(&active);
    local_report.reason = "active_partition_failed";
    goto fail_no_signal;
  }
  slv9_cmslv2_selector_summary_init(&local);
  slv9_cmslv2_tally_active_subblocks(
    server,structural,structural_block,&active,&local
  );
  slv_decomp_destroy(&active);
  local_report.active_subblocks = local.active_subblocks;
  local_report.qrslv_subblocks = local.qrslv_subblocks;
  local_report.lrslv_subblocks = local.lrslv_subblocks;
  local_report.unresolved_subblocks = local.unresolved_subblocks;
  if(local.unresolved_subblocks > 0) {
    local_report.reason = "unresolved_active_subblock";
    goto fail_no_signal;
  }

  local_report.solved = 1;
  local_report.reason = "solved";
  if(report != NULL) {
    *report = local_report;
  }
  slv9_report_progress(sys,
    "event=cmslv2_block_solved phase=%s block=%d solved=1 real_rels=%d real_failures=%d logrels=%d logrel_failures=%d calc_errors=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d max_real_residual=%.17g reason=%s",
    phase != NULL ? phase : "unknown", structural_block,
    local_report.real_rels_checked, local_report.real_rel_failures,
    local_report.logrels_checked, local_report.logrel_failures,
    local_report.calc_errors, local_report.active_subblocks,
    local_report.qrslv_subblocks, local_report.lrslv_subblocks,
    local_report.unresolved_subblocks, local_report.max_real_residual,
    local_report.reason
  );
  return 1;

fail:
#ifdef ASC_SIGNAL_TRAPS
  Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
#endif
fail_no_signal:
  local_report.solved = 0;
  if(report != NULL) {
    *report = local_report;
  }
  slv9_report_progress(sys,
    "event=cmslv2_block_solved phase=%s block=%d solved=0 real_rels=%d real_failures=%d logrels=%d logrel_failures=%d calc_errors=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d max_real_residual=%.17g reason=%s",
    phase != NULL ? phase : "unknown", structural_block,
    local_report.real_rels_checked, local_report.real_rel_failures,
    local_report.logrels_checked, local_report.logrel_failures,
    local_report.calc_errors, local_report.active_subblocks,
    local_report.qrslv_subblocks, local_report.lrslv_subblocks,
    local_report.unresolved_subblocks, local_report.max_real_residual,
    local_report.reason
  );
  return 0;
}

static
int32 slv9_cmslv2_validate_boundary_envelope(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const char *phase, real64 *max_residual, const char **reason
){
  struct slv9_cmslv2_block_solved_report report;
  int32 solved;

  solved = slv9_cmslv2_check_block_currently_solved(
    sys,structural,structural_block,phase,&report
  );
  if(max_residual != NULL) {
    *max_residual = report.max_real_residual;
  }
  if(reason != NULL) {
    *reason = report.reason;
  }
  slv9_report_progress(sys,
    "event=cmslv2_boundary_local_validate phase=%s block=%d action=%s max_residual=%.17g unresolved_subblocks=%d reason=%s",
    phase != NULL ? phase : "unknown", structural_block,
    solved ? "local_complete" : "defer", report.max_real_residual,
    report.unresolved_subblocks, report.reason
  );
  return solved;
}

static
void slv9_cmslv2_transition_after_solved_block(slv9_system_t sys,
    const char *phase, const char *solver, int32 from_block, int32 next_block
){
  slv_decomp_partition_t structural, active;
  struct slv9_decomp_block_stats bs;
  int32 structural_status, active_status;

  if(sys == NULL || sys->slv == NULL) {
    return;
  }

  sys->cmslv2_next_structural_block = next_block;
  sys->cmslv2_pending_after_qrslv = 0;

  slv9_reanalyze_solver_lists(sys);
  update_boundaries(sys->slv,(SlvClientToken)sys);
  update_relations_residuals(sys->slv);

  if(!PROGRESS_LOG) {
    sys->cmslv2_pending_after_qrslv = next_block >= 0 ? 1 : 0;
    slv9_report_progress(sys,
      "event=cmslv2_transition phase=%s solver=%s from_structural=%d next_structural=%d action=reanalyzed diagnostic=0",
      phase != NULL ? phase : "unknown",
      solver != NULL ? solver : "unknown",
      from_block, sys->cmslv2_next_structural_block
    );
    return;
  }

  slv_decomp_init(&structural);
  slv_decomp_init(&active);
  structural_status = slv_decomp_partition(sys->slv,&structural);
  active_status = slv_decomp_partition_active(sys->slv,&active);
  if(structural_status || active_status) {
    slv9_report_progress(sys,
      "event=cmslv2_transition phase=%s solver=%s from_structural=%d next_structural=%d action=reanalysis_failed structural_status=%d active_status=%d",
      phase != NULL ? phase : "unknown",
      solver != NULL ? solver : "unknown",
      from_block, sys->cmslv2_next_structural_block,
      structural_status, active_status
    );
    slv_decomp_destroy(&active);
    slv_decomp_destroy(&structural);
    return;
  }

  sys->cmslv2_pending_after_qrslv =
    sys->cmslv2_next_structural_block < structural.nblocks;
  if(sys->cmslv2_next_structural_block >= 0
      && sys->cmslv2_next_structural_block < structural.nblocks) {
    slv9_decomp_assess_block(
      sys->slv,&structural,sys->cmslv2_next_structural_block,&bs
    );
    slv9_report_progress(sys,
      "event=cmslv2_transition phase=%s solver=%s from_structural=%d next_structural=%d action=reanalyzed structural_blocks=%d active_blocks=%d next_rows=%d next_cols=%d next_relrows=%d next_logrows=%d next_dvars=%d next_boundary_edges=%d next_selector_edges=%d",
      phase != NULL ? phase : "unknown",
      solver != NULL ? solver : "unknown",
      from_block, sys->cmslv2_next_structural_block,
      structural.nblocks, active.nblocks,
      bs.rows, bs.cols, bs.relrows, bs.logrows, bs.dvars,
      bs.boundary_edges, bs.selector_edges
    );
  }else{
    slv9_report_progress(sys,
      "event=cmslv2_transition phase=%s solver=%s from_structural=%d next_structural=%d action=complete structural_blocks=%d active_blocks=%d",
      phase != NULL ? phase : "unknown",
      solver != NULL ? solver : "unknown",
      from_block, sys->cmslv2_next_structural_block,
      structural.nblocks, active.nblocks
    );
  }

  slv_decomp_destroy(&active);
  slv_decomp_destroy(&structural);
}

static
int32 slv9_cmslv2_install_qrslv_blocks_for_structural_block(
    slv9_system_t sys, const slv_decomp_partition_t *structural,
    int32 structural_block, const slv_decomp_partition_t *active,
    int32 *installed_rows, int32 *installed_cols
){
  slv_system_t server;
  struct rel_relation **rels, **newrels;
  struct var_variable **vars, **newvars;
  unsigned char *relused, *varused;
  mtx_region_t *blocks;
  int32 nrels, nvars;
  int32 selected_rows, selected_cols;
  int32 b, r, c, local, nblocks;
  int32 rowpos, colpos;

  if(installed_rows != NULL) *installed_rows = 0;
  if(installed_cols != NULL) *installed_cols = 0;
  if(sys == NULL || structural == NULL || active == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return 0;
  }
  server = sys->slv;
  if(server == NULL || active->nblocks <= 0) {
    return 0;
  }

  slv9_cmslv2_restore_qrslv_scope(sys);

  nrels = slv_get_num_solvers_rels(server);
  nvars = slv_get_num_solvers_vars(server);
  rels = slv_get_solvers_rel_list(server);
  vars = slv_get_solvers_var_list(server);
  if(rels == NULL || vars == NULL) {
    return 0;
  }

  newrels = ASC_NEW_ARRAY_CLEAR(struct rel_relation *,nrels + 1);
  newvars = ASC_NEW_ARRAY_CLEAR(struct var_variable *,nvars + 1);
  relused = ASC_NEW_ARRAY_CLEAR(unsigned char,nrels);
  varused = ASC_NEW_ARRAY_CLEAR(unsigned char,nvars);
  blocks = ASC_NEW_ARRAY_CLEAR(mtx_region_t,active->nblocks);
  if(newrels == NULL || newvars == NULL || relused == NULL
      || varused == NULL || blocks == NULL) {
    if(newrels != NULL) ascfree(newrels);
    if(newvars != NULL) ascfree(newvars);
    if(relused != NULL) ascfree(relused);
    if(varused != NULL) ascfree(varused);
    if(blocks != NULL) ascfree(blocks);
    return 0;
  }

  nblocks = 0;
  rowpos = 0;
  colpos = 0;
  for(b = 0; b < active->nblocks; ++b) {
    struct slv9_decomp_block_stats bs;
    mtx_region_t region = active->blocks[b];
    int32 rowlow, collow;

    if(!slv9_cmslv2_active_block_in_structural_block(
        active,b,structural,structural_block
    )) {
      continue;
    }
    slv9_decomp_assess_block(server,active,b,&bs);
    if(!slv9_decomp_block_is_pure_real(&bs)) {
      continue;
    }
    if(!slv9_cmslv2_block_qrslv_installable(active,b)) {
      goto fail;
    }

    rowlow = rowpos;
    collow = colpos;
    for(r = region.row.low; r <= region.row.high; ++r) {
      (void)slv_decomp_row_kind(active,active->row_org[r],&local);
      if(local < 0 || local >= nrels || relused[local]) {
        goto fail;
      }
      newrels[rowpos++] = rels[local];
      relused[local] = 1;
    }
    for(c = region.col.low; c <= region.col.high; ++c) {
      (void)slv_decomp_col_kind(active,active->col_org[c],&local);
      if(local < 0 || local >= nvars || varused[local]) {
        goto fail;
      }
      newvars[colpos++] = vars[local];
      varused[local] = 1;
    }

    blocks[nblocks].row.low = rowlow;
    blocks[nblocks].row.high = rowpos - 1;
    blocks[nblocks].col.low = collow;
    blocks[nblocks].col.high = colpos - 1;
    nblocks++;
  }

  selected_rows = rowpos;
  selected_cols = colpos;
  if(nblocks <= 0 || selected_rows <= 0 || selected_cols <= 0) {
    goto fail;
  }

  for(r = 0; r < nrels; ++r) {
    if(!relused[r]) {
      newrels[rowpos++] = rels[r];
    }
  }
  for(c = 0; c < nvars; ++c) {
    if(!varused[c]) {
      newvars[colpos++] = vars[c];
    }
  }
  for(r = 0; r < nrels; ++r) {
    rels[r] = newrels[r];
    rel_set_sindex(rels[r],r);
  }
  for(c = 0; c < nvars; ++c) {
    vars[c] = newvars[c];
    var_set_sindex(vars[c],c);
  }

  sys->cmslv2_rel_active_save = ASC_NEW_ARRAY(unsigned char,nrels);
  sys->cmslv2_var_active_save = ASC_NEW_ARRAY(unsigned char,nvars);
  if(sys->cmslv2_rel_active_save == NULL
      || sys->cmslv2_var_active_save == NULL) {
    goto fail_after_reorder;
  }
  for(r = 0; r < nrels; ++r) {
    sys->cmslv2_rel_active_save[r] = rel_active(rels[r]) ? 1 : 0;
    rel_set_active(rels[r],r < selected_rows ? TRUE : FALSE);
  }
  for(c = 0; c < nvars; ++c) {
    sys->cmslv2_var_active_save[c] = var_active(vars[c]) ? 1 : 0;
    var_set_active(vars[c],c < selected_cols ? TRUE : FALSE);
  }
  sys->cmslv2_scope_active = 1;
  sys->cmslv2_scope_nrels = nrels;
  sys->cmslv2_scope_nvars = nvars;
  slv_set_solvers_blocks(server,nblocks,blocks);
  blocks = NULL;
  if(installed_rows != NULL) *installed_rows = selected_rows;
  if(installed_cols != NULL) *installed_cols = selected_cols;

  ascfree(newrels);
  ascfree(newvars);
  ascfree(relused);
  ascfree(varused);
  return nblocks;

fail_after_reorder:
  if(sys->cmslv2_rel_active_save != NULL) {
    ascfree(sys->cmslv2_rel_active_save);
    sys->cmslv2_rel_active_save = NULL;
  }
  if(sys->cmslv2_var_active_save != NULL) {
    ascfree(sys->cmslv2_var_active_save);
    sys->cmslv2_var_active_save = NULL;
  }
  sys->cmslv2_scope_active = 0;
  if(blocks != NULL) ascfree(blocks);
  ascfree(newrels);
  ascfree(newvars);
  ascfree(relused);
  ascfree(varused);
  return 0;

fail:
  ascfree(newrels);
  ascfree(newvars);
  ascfree(relused);
  ascfree(varused);
  if(blocks != NULL) ascfree(blocks);
  return 0;
}

static
int32 slv9_cmslv2_solve_selector_qrslv_subblocks(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const slv_decomp_partition_t *active, const char *phase,
    int32 *installed_blocks
){
  slv_system_t server;
  SlvClientToken orig_token;
  int32 orig_solver;
  struct rel_relation **rel_order = NULL;
  struct var_variable **var_order = NULL;
  int32 nrels = 0, nvars = 0;
  int32 rows = 0, cols = 0, blocks;
  int32 converged = 0;
  slv_status_t status;

  if(installed_blocks != NULL) {
    *installed_blocks = 0;
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL || active == NULL) {
    return 0;
  }
  server = sys->slv;
  orig_token = slv_get_client_token(server);
  orig_solver = slv_get_selected_solver(server);
  slv9_cmslv2_save_solver_order(
    server,&rel_order,&nrels,&var_order,&nvars
  );

  blocks = slv9_cmslv2_install_qrslv_blocks_for_structural_block(
    sys,structural,structural_block,active,&rows,&cols
  );
  if(installed_blocks != NULL) {
    *installed_blocks = blocks;
  }
  if(blocks <= 0) {
    slv9_report_progress(sys,
      "event=cmslv2_selector_qrslv phase=%s block=%d installed_blocks=0 action=defer",
      phase != NULL ? phase : "unknown", structural_block
    );
    goto finish;
  }

  slv9_set_qrslv_external_mode(server,1);
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  slv_presolve(server);
  slv_get_status(server,&status);
  if(status.ready_to_solve || status.converged) {
    slv_solve(server);
    slv_get_status(server,&status);
  }
  converged = status.ok && status.converged;
  slv9_report_progress(sys,
    "event=cmslv2_selector_qrslv phase=%s block=%d installed_blocks=%d installed_rows=%d installed_cols=%d converged=%d ok=%d",
    phase != NULL ? phase : "unknown", structural_block,
    blocks, rows, cols, converged ? 1 : 0, status.ok ? 1 : 0
  );

finish:
  slv9_cmslv2_restore_qrslv_scope(sys);
  slv9_cmslv2_restore_solver_order(server,rel_order,nrels,var_order,nvars);
  if(rel_order != NULL) ascfree(rel_order);
  if(var_order != NULL) ascfree(var_order);
  slv9_set_qrslv_external_mode(server,0);
  slv_set_solver_index(server,orig_solver);
  slv_set_client_token(server,orig_token);
  update_relations_residuals(server);
  return converged;
}

static
void slv9_cmslv2_save_logical_order(slv_system_t server,
    struct logrel_relation ***logrel_order, int32 *nlogrels,
    struct dis_discrete ***dvar_order, int32 *ndvars
){
  struct logrel_relation **logrels;
  struct dis_discrete **dvars;

  if(logrel_order != NULL) *logrel_order = NULL;
  if(dvar_order != NULL) *dvar_order = NULL;
  if(nlogrels != NULL) *nlogrels = 0;
  if(ndvars != NULL) *ndvars = 0;
  if(server == NULL || logrel_order == NULL || dvar_order == NULL
      || nlogrels == NULL || ndvars == NULL) {
    return;
  }

  *nlogrels = slv_get_num_solvers_logrels(server);
  *ndvars = slv_get_num_solvers_dvars(server);
  logrels = slv_get_solvers_logrel_list(server);
  dvars = slv_get_solvers_dvar_list(server);
  if(logrels != NULL && *nlogrels > 0) {
    *logrel_order = ASC_NEW_ARRAY(struct logrel_relation *,*nlogrels);
    if(*logrel_order != NULL) {
      memcpy(*logrel_order,logrels,
        (size_t)(*nlogrels) * sizeof(struct logrel_relation *)
      );
    }
  }
  if(dvars != NULL && *ndvars > 0) {
    *dvar_order = ASC_NEW_ARRAY(struct dis_discrete *,*ndvars);
    if(*dvar_order != NULL) {
      memcpy(*dvar_order,dvars,
        (size_t)(*ndvars) * sizeof(struct dis_discrete *)
      );
    }
  }
}

static
void slv9_cmslv2_restore_logical_order(slv_system_t server,
    struct logrel_relation **logrel_order, int32 nlogrels,
    struct dis_discrete **dvar_order, int32 ndvars
){
  int32 i;
  struct logrel_relation **logrels;
  struct dis_discrete **dvars;

  if(server == NULL) {
    return;
  }
  logrels = slv_get_solvers_logrel_list(server);
  dvars = slv_get_solvers_dvar_list(server);
  if(logrels != NULL && logrel_order != NULL) {
    for(i = 0; i < nlogrels; ++i) {
      logrels[i] = logrel_order[i];
      logrel_set_sindex(logrels[i],i);
    }
  }
  if(dvars != NULL && dvar_order != NULL) {
    for(i = 0; i < ndvars; ++i) {
      dvars[i] = dvar_order[i];
      dis_set_sindex(dvars[i],i);
    }
  }
}

static
int32 slv9_cmslv2_log_block_lrslv_installable(
    const slv_decomp_partition_t *decomp, int32 block
){
  int32 r, c, local;
  mtx_region_t region;

  if(decomp == NULL || block < 0 || block >= decomp->nblocks) {
    return 0;
  }
  region = decomp->blocks[block];
  if(region.row.high != region.row.low || region.col.high != region.col.low) {
    return 0;
  }
  for(r = region.row.low; r <= region.row.high; ++r) {
    slv_decomp_row_kind_t kind =
      slv_decomp_row_kind(decomp,decomp->row_org[r],&local);
    if(kind != slv_decomp_row_logrel) {
      return 0;
    }
  }
  for(c = region.col.low; c <= region.col.high; ++c) {
    if(slv_decomp_col_kind(decomp,decomp->col_org[c],&local)
        != slv_decomp_col_dvar) {
      return 0;
    }
  }
  return 1;
}

static
int32 slv9_cmslv2_install_lrslv_blocks_for_structural_run(
    slv9_system_t sys, const slv_decomp_partition_t *structural,
    int32 structural_start, int32 structural_end,
    const slv_decomp_partition_t *active,
    int32 *installed_rows, int32 *installed_cols
){
  slv_system_t server;
  struct logrel_relation **logrels, **newlogrels;
  struct dis_discrete **dvars, **newdvars;
  unsigned char *logrelused, *dvarused;
  unsigned char *logrel_active_save, *dvar_active_save;
  mtx_region_t *blocks;
  int32 nlogrels, ndvars;
  int32 selected_rows, selected_cols;
  int32 b, r, c, local, nblocks;
  int32 rowpos, colpos;

  if(installed_rows != NULL) *installed_rows = 0;
  if(installed_cols != NULL) *installed_cols = 0;
  if(sys == NULL || structural == NULL || active == NULL
      || structural_start < 0 || structural_start >= structural->nblocks
      || structural_end < structural_start
      || structural_end >= structural->nblocks) {
    return 0;
  }
  server = sys->slv;
  if(server == NULL || active->nblocks <= 0) {
    return 0;
  }

  nlogrels = slv_get_num_solvers_logrels(server);
  ndvars = slv_get_num_solvers_dvars(server);
  logrels = slv_get_solvers_logrel_list(server);
  dvars = slv_get_solvers_dvar_list(server);
  if(logrels == NULL || dvars == NULL) {
    return 0;
  }

  newlogrels = ASC_NEW_ARRAY_CLEAR(struct logrel_relation *,nlogrels + 1);
  newdvars = ASC_NEW_ARRAY_CLEAR(struct dis_discrete *,ndvars + 1);
  logrelused = ASC_NEW_ARRAY_CLEAR(unsigned char,nlogrels);
  dvarused = ASC_NEW_ARRAY_CLEAR(unsigned char,ndvars);
  logrel_active_save = ASC_NEW_ARRAY(unsigned char,nlogrels);
  dvar_active_save = ASC_NEW_ARRAY(unsigned char,ndvars);
  blocks = ASC_NEW_ARRAY_CLEAR(mtx_region_t,active->nblocks);
  if(newlogrels == NULL || newdvars == NULL || logrelused == NULL
      || dvarused == NULL || logrel_active_save == NULL
      || dvar_active_save == NULL || blocks == NULL) {
    if(newlogrels != NULL) ascfree(newlogrels);
    if(newdvars != NULL) ascfree(newdvars);
    if(logrelused != NULL) ascfree(logrelused);
    if(dvarused != NULL) ascfree(dvarused);
    if(logrel_active_save != NULL) ascfree(logrel_active_save);
    if(dvar_active_save != NULL) ascfree(dvar_active_save);
    if(blocks != NULL) ascfree(blocks);
    return 0;
  }
  for(r = 0; r < nlogrels; ++r) {
    logrel_active_save[r] = logrel_active(logrels[r]) ? 1 : 0;
  }
  for(c = 0; c < ndvars; ++c) {
    dvar_active_save[c] = dis_active(dvars[c]) ? 1 : 0;
  }

  nblocks = 0;
  rowpos = 0;
  colpos = 0;
  for(b = 0; b < active->nblocks; ++b) {
    struct slv9_decomp_block_stats bs;
    mtx_region_t region = active->blocks[b];
    int32 rowlow, collow;

    int32 in_run = 0;
    int32 sblock;

    for(sblock = structural_start; sblock <= structural_end; ++sblock) {
      if(slv9_cmslv2_active_block_in_structural_block(
          active,b,structural,sblock
      )) {
        in_run = 1;
        break;
      }
    }
    if(!in_run) {
      continue;
    }
    slv9_decomp_assess_block(server,active,b,&bs);
    if(!slv9_decomp_block_is_pure_logical(&bs)) {
      continue;
    }
    if(!slv9_cmslv2_log_block_lrslv_installable(active,b)) {
      goto fail;
    }

    rowlow = rowpos;
    collow = colpos;
    for(r = region.row.low; r <= region.row.high; ++r) {
      (void)slv_decomp_row_kind(active,active->row_org[r],&local);
      if(local < 0 || local >= nlogrels || logrelused[local]) {
        goto fail;
      }
      newlogrels[rowpos++] = logrels[local];
      logrelused[local] = 1;
    }
    for(c = region.col.low; c <= region.col.high; ++c) {
      (void)slv_decomp_col_kind(active,active->col_org[c],&local);
      if(local < 0 || local >= ndvars || dvarused[local]) {
        goto fail;
      }
      newdvars[colpos++] = dvars[local];
      dvarused[local] = 1;
    }

    blocks[nblocks].row.low = rowlow;
    blocks[nblocks].row.high = rowpos - 1;
    blocks[nblocks].col.low = collow;
    blocks[nblocks].col.high = colpos - 1;
    nblocks++;
  }

  selected_rows = rowpos;
  selected_cols = colpos;
  if(nblocks <= 0 || selected_rows <= 0 || selected_cols <= 0) {
    goto fail;
  }

  for(r = 0; r < nlogrels; ++r) {
    if(!logrelused[r]) {
      newlogrels[rowpos++] = logrels[r];
    }
  }
  for(c = 0; c < ndvars; ++c) {
    if(!dvarused[c]) {
      newdvars[colpos++] = dvars[c];
    }
  }
  for(r = 0; r < nlogrels; ++r) {
    logrels[r] = newlogrels[r];
    logrel_set_sindex(logrels[r],r);
    logrel_set_active(logrels[r],r < selected_rows ? TRUE : FALSE);
  }
  for(c = 0; c < ndvars; ++c) {
    dvars[c] = newdvars[c];
    dis_set_sindex(dvars[c],c);
    dis_set_active(dvars[c],c < selected_cols ? TRUE : FALSE);
  }
  slv_set_solvers_log_blocks(server,nblocks,blocks);
  blocks = NULL;
  if(installed_rows != NULL) *installed_rows = selected_rows;
  if(installed_cols != NULL) *installed_cols = selected_cols;

  ascfree(newlogrels);
  ascfree(newdvars);
  ascfree(logrelused);
  ascfree(dvarused);
  ascfree(logrel_active_save);
  ascfree(dvar_active_save);
  return nblocks;

fail:
  for(r = 0; r < nlogrels; ++r) {
    logrel_set_active(logrels[r],logrel_active_save[r] ? TRUE : FALSE);
  }
  for(c = 0; c < ndvars; ++c) {
    dis_set_active(dvars[c],dvar_active_save[c] ? TRUE : FALSE);
  }
  ascfree(newlogrels);
  ascfree(newdvars);
  ascfree(logrelused);
  ascfree(dvarused);
  ascfree(logrel_active_save);
  ascfree(dvar_active_save);
  if(blocks != NULL) ascfree(blocks);
  return 0;
}

static
int32 slv9_cmslv2_solve_lrslv_structural_run(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    int32 structural_start, int32 structural_end,
    const slv_decomp_partition_t *active, const char *phase,
    const char *event_name,
    int32 *installed_blocks
){
  slv_system_t server;
  SlvClientToken orig_token;
  int32 orig_solver;
  struct logrel_relation **logrel_order = NULL;
  struct dis_discrete **dvar_order = NULL;
  unsigned char *logrel_active_save = NULL;
  unsigned char *dvar_active_save = NULL;
  int32 nlogrels = 0, ndvars = 0;
  int32 rows = 0, cols = 0, blocks;
  int32 converged = 0;
  slv_status_t status;
  int32 i;

  if(installed_blocks != NULL) {
    *installed_blocks = 0;
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL || active == NULL) {
    return 0;
  }
  server = sys->slv;
  orig_token = slv_get_client_token(server);
  orig_solver = slv_get_selected_solver(server);
  slv9_cmslv2_save_logical_order(
    server,&logrel_order,&nlogrels,&dvar_order,&ndvars
  );
  if(logrel_order != NULL && nlogrels > 0) {
    logrel_active_save = ASC_NEW_ARRAY(unsigned char,nlogrels);
    if(logrel_active_save != NULL) {
      for(i = 0; i < nlogrels; ++i) {
        logrel_active_save[i] = logrel_active(logrel_order[i]) ? 1 : 0;
      }
    }
  }
  if(dvar_order != NULL && ndvars > 0) {
    dvar_active_save = ASC_NEW_ARRAY(unsigned char,ndvars);
    if(dvar_active_save != NULL) {
      for(i = 0; i < ndvars; ++i) {
        dvar_active_save[i] = dis_active(dvar_order[i]) ? 1 : 0;
      }
    }
  }

  blocks = slv9_cmslv2_install_lrslv_blocks_for_structural_run(
    sys,structural,structural_start,structural_end,active,&rows,&cols
  );
  if(installed_blocks != NULL) {
    *installed_blocks = blocks;
  }
  if(blocks <= 0) {
    slv9_report_progress(sys,
      "event=%s phase=%s structural_start=%d structural_end=%d installed_blocks=0 action=defer",
      event_name != NULL ? event_name : "cmslv2_lrslv",
      phase != NULL ? phase : "unknown", structural_start, structural_end
    );
    goto finish;
  }

  slv9_set_lrslv_external_mode(server,1);
  slv_set_client_token(server,token[LOGICAL_SOLVER]);
  slv_set_solver_index(server,solver_index[LOGICAL_SOLVER]);
  slv_presolve(server);
  slv_get_status(server,&status);
  if(status.ready_to_solve || status.converged) {
    slv_solve(server);
    slv_get_status(server,&status);
  }
  converged = status.ok && status.converged;
  slv9_report_progress(sys,
    "event=%s phase=%s structural_start=%d structural_end=%d installed_blocks=%d installed_rows=%d installed_cols=%d converged=%d ok=%d",
    event_name != NULL ? event_name : "cmslv2_lrslv",
    phase != NULL ? phase : "unknown", structural_start, structural_end,
    blocks, rows, cols, converged ? 1 : 0, status.ok ? 1 : 0
  );

finish:
  slv9_cmslv2_restore_logical_order(
    server,logrel_order,nlogrels,dvar_order,ndvars
  );
  if(logrel_order != NULL && logrel_active_save != NULL) {
    for(i = 0; i < nlogrels; ++i) {
      logrel_set_active(logrel_order[i],
        logrel_active_save[i] ? TRUE : FALSE
      );
    }
  }
  if(dvar_order != NULL && dvar_active_save != NULL) {
    for(i = 0; i < ndvars; ++i) {
      dis_set_active(dvar_order[i],dvar_active_save[i] ? TRUE : FALSE);
    }
  }
  if(logrel_order != NULL) ascfree(logrel_order);
  if(dvar_order != NULL) ascfree(dvar_order);
  if(logrel_active_save != NULL) ascfree(logrel_active_save);
  if(dvar_active_save != NULL) ascfree(dvar_active_save);
  slv9_set_lrslv_external_mode(server,0);
  slv_set_solver_index(server,orig_solver);
  slv_set_client_token(server,orig_token);
  update_relations_residuals(server);
  return converged;
}

static
int32 slv9_cmslv2_solve_selector_lrslv_subblocks(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const slv_decomp_partition_t *active, const char *phase,
    int32 *installed_blocks
){
  return slv9_cmslv2_solve_lrslv_structural_run(
    sys,structural,structural_block,structural_block,active,phase,
    "cmslv2_selector_lrslv",installed_blocks
  );
}

static
int32 slv9_gl_contains_ptr(struct gl_list_t *list, void *ptr){
  unsigned long i, len;
  if(list == NULL || ptr == NULL) {
    return 0;
  }
  len = gl_length(list);
  for(i = 1; i <= len; ++i) {
    if(gl_fetch(list,i) == ptr) {
      return 1;
    }
  }
  return 0;
}

static
void slv9_gl_append_unique_ptr(struct gl_list_t *list, void *ptr){
  if(list != NULL && ptr != NULL && !slv9_gl_contains_ptr(list,ptr)) {
    gl_append_ptr(list,ptr);
  }
}

static
void slv9_cmslv2_collect_selector_dvars(slv_system_t server,
    const slv_decomp_partition_t *decomp, int32 block,
    struct gl_list_t *disvars
){
  int32 c, local;
  struct dis_discrete **dvars;
  mtx_region_t region;

  if(server == NULL || decomp == NULL || block < 0
      || block >= decomp->nblocks || disvars == NULL) {
    return;
  }

  dvars = slv_get_solvers_dvar_list(server);
  region = decomp->blocks[block];
  for(c = region.col.low; c <= region.col.high; ++c) {
    if(slv_decomp_col_kind(decomp,decomp->col_org[c],&local)
        == slv_decomp_col_dvar) {
      slv9_gl_append_unique_ptr(disvars,dvars[local]);
    }
  }
}

static
int32 slv9_cmslv2_selector_dvars_are_boolean(struct gl_list_t *disvars){
  unsigned long i, len;

  if(disvars == NULL) {
    return 0;
  }
  len = gl_length(disvars);
  for(i = 1; i <= len; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(disvars,i);
    if(dvar == NULL || dis_kind(dvar) != e_dis_boolean_t) {
      return 0;
    }
  }
  return 1;
}

static
unsigned long slv9_cmslv2_selector_current_mask(struct gl_list_t *disvars){
  unsigned long i, len, mask;

  mask = 0;
  if(disvars == NULL) {
    return mask;
  }
  len = gl_length(disvars);
  for(i = 1; i <= len; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(disvars,i);
    if(dvar != NULL && dis_value(dvar)) {
      mask |= (1UL << (i - 1));
    }
  }
  return mask;
}

static
void slv9_cmslv2_selector_set_mask(struct gl_list_t *disvars,
    unsigned long mask
){
  unsigned long i, len;

  if(disvars == NULL) {
    return;
  }
  len = gl_length(disvars);
  for(i = 1; i <= len; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(disvars,i);
    if(dvar != NULL) {
      int32 value = (mask & (1UL << (i - 1))) ? TRUE : FALSE;
      dis_set_boolean_value(dvar,value);
    }
  }
}

static
void slv9_cmslv2_selector_restore_values(struct gl_list_t *disvars,
    const int32 *values, const int32 *previous
){
  unsigned long i, len;

  if(disvars == NULL || values == NULL || previous == NULL) {
    return;
  }
  len = gl_length(disvars);
  for(i = 1; i <= len; ++i) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(disvars,i);
    if(dvar != NULL) {
      dis_set_boolean_value(dvar,values[i - 1]);
      dis_set_previous_value(dvar,previous[i - 1]);
    }
  }
}

static
int32 slv9_cmslv2_selector_eval_candidate(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    slv_decomp_partition_t *active,
    struct slv9_cmslv2_selector_summary *local, struct gl_list_t *disvars,
    const char *phase, int32 *matched_cases, int32 *active_status
){
  int32 *cases = NULL;
  int32 ncases = 0;
  int32 status;
  unsigned long assumed_mask, resolved_mask;
  int32 subblocks_ok;

  if(matched_cases != NULL) {
    *matched_cases = 0;
  }
  if(active_status != NULL) {
    *active_status = 0;
  }
  if(local != NULL) {
    slv9_cmslv2_selector_summary_init(local);
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL
      || active == NULL || disvars == NULL) {
    return 0;
  }

  cases = cases_matching(disvars,&ncases);
  if(cases != NULL) {
    ascfree(cases);
  }
  if(matched_cases != NULL) {
    *matched_cases = ncases;
  }
  if(ncases <= 0) {
    return 0;
  }
  assumed_mask = slv9_cmslv2_selector_current_mask(disvars);

  slv9_reanalyze_solver_lists(sys);
  update_relations_residuals(sys->slv);
  slv_decomp_destroy(active);
  slv_decomp_init(active);
  status = slv_decomp_partition_active(sys->slv,active);
  if(active_status != NULL) {
    *active_status = status;
  }
  if(status) {
    return 0;
  }

  if(local != NULL) {
    slv9_cmslv2_tally_active_subblocks(
      sys->slv,structural,structural_block,active,local
    );
    subblocks_ok = local->active_subblocks > 0
      && local->unresolved_subblocks == 0
      && (local->lrslv_subblocks <= 0
        || slv9_cmslv2_solve_selector_lrslv_subblocks(
          sys,structural,structural_block,active,phase,NULL
        ))
      && (local->qrslv_subblocks <= 0
        || slv9_cmslv2_solve_selector_qrslv_subblocks(
          sys,structural,structural_block,active,phase,NULL
        ));
    if(!subblocks_ok) {
      return 0;
    }
    update_boundaries(sys->slv,(SlvClientToken)sys);
    solve_logical_relations(sys->slv);
    update_relations_residuals(sys->slv);
    resolved_mask = slv9_cmslv2_selector_current_mask(disvars);
    if(resolved_mask != assumed_mask) {
      slv9_report_progress(sys,
        "event=cmslv2_selector_consistency phase=%s block=%d action=reject assumed_mask=%lu resolved_mask=%lu",
        phase != NULL ? phase : "unknown", structural_block,
        assumed_mask, resolved_mask
      );
      return 0;
    }
    slv9_report_progress(sys,
      "event=cmslv2_selector_consistency phase=%s block=%d action=accept assumed_mask=%lu resolved_mask=%lu",
      phase != NULL ? phase : "unknown", structural_block,
      assumed_mask, resolved_mask
    );
    return 1;
  }
  return 0;
}

static
int32 slv9_cmslv2_selector_branch_search(slv9_system_t sys,
    const slv_decomp_partition_t *structural, int32 structural_block,
    slv_decomp_partition_t *active, struct gl_list_t *disvars,
    struct slv9_cmslv2_selector_summary *accepted, int32 *accepted_cases,
    unsigned long *accepted_mask, const char *phase, int32 *tried,
    int32 *status_code
){
  unsigned long len, combinations, current_mask, mask, attempt;
  int32 *orig_values = NULL, *orig_previous = NULL;
  real64 *orig_real_values = NULL;
  int32 *orig_dis_values = NULL, *orig_dis_previous = NULL;
  int32 orig_real_count = 0;
  int32 orig_dis_count = 0;
  int32 ok = 0, ncases = 0, active_status = 0;

  if(accepted != NULL) {
    slv9_cmslv2_selector_summary_init(accepted);
  }
  if(accepted_cases != NULL) {
    *accepted_cases = 0;
  }
  if(accepted_mask != NULL) {
    *accepted_mask = 0;
  }
  if(tried != NULL) {
    *tried = 0;
  }
  if(status_code != NULL) {
    *status_code = 0;
  }
  if(sys == NULL || structural == NULL || active == NULL || disvars == NULL) {
    if(status_code != NULL) *status_code = -1;
    return 0;
  }

  len = gl_length(disvars);
  if(len == 0) {
    if(status_code != NULL) *status_code = -2;
    return 0;
  }
  if(!slv9_cmslv2_selector_dvars_are_boolean(disvars)) {
    if(status_code != NULL) *status_code = -3;
    return 0;
  }
  if(len > CMSLV2_SELECTOR_SEARCH_MAX_DVARS) {
    if(status_code != NULL) *status_code = -4;
    return 0;
  }

  orig_values = ASC_NEW_ARRAY(int32,len);
  orig_previous = ASC_NEW_ARRAY(int32,len);
  if(orig_values == NULL || orig_previous == NULL) {
    if(status_code != NULL) *status_code = -5;
    if(orig_values != NULL) ascfree(orig_values);
    if(orig_previous != NULL) ascfree(orig_previous);
    return 0;
  }
  slv9_cmslv2_store_real_values(sys->slv,&orig_real_values,&orig_real_count);
  if(orig_real_values == NULL) {
    if(status_code != NULL) *status_code = -7;
    ascfree(orig_values);
    ascfree(orig_previous);
    return 0;
  }
  slv9_cmslv2_store_discrete_values(
    sys->slv,&orig_dis_values,&orig_dis_previous,&orig_dis_count
  );
  if(orig_dis_values == NULL || orig_dis_previous == NULL) {
    if(status_code != NULL) *status_code = -8;
    ascfree(orig_values);
    ascfree(orig_previous);
    ascfree(orig_real_values);
    if(orig_dis_values != NULL) ascfree(orig_dis_values);
    if(orig_dis_previous != NULL) ascfree(orig_dis_previous);
    return 0;
  }
  for(attempt = 1; attempt <= len; ++attempt) {
    struct dis_discrete *dvar =
      (struct dis_discrete *)gl_fetch(disvars,attempt);
    orig_values[attempt - 1] = dvar != NULL ? dis_value(dvar) : FALSE;
    orig_previous[attempt - 1] =
      dvar != NULL ? dis_previous_value(dvar) : orig_values[attempt - 1];
  }

  combinations = 1UL << len;
  current_mask = slv9_cmslv2_selector_current_mask(disvars);

  for(attempt = 0; attempt <= combinations; ++attempt) {
    struct slv9_cmslv2_selector_summary local;

    if(attempt == 0) {
      mask = current_mask;
    }else{
      mask = attempt - 1;
      if(mask == current_mask) {
        continue;
      }
    }
    if(mask >= combinations) {
      continue;
    }
    if(tried != NULL) {
      (*tried)++;
    }
    slv9_cmslv2_selector_set_mask(disvars,mask);
    slv9_cmslv2_selector_summary_init(&local);
    ok = slv9_cmslv2_selector_eval_candidate(
      sys,structural,structural_block,active,&local,disvars,
      phase,&ncases,&active_status
    );
    if(ok) {
      unsigned long i;
      for(i = 1; i <= len; ++i) {
        struct dis_discrete *dvar =
          (struct dis_discrete *)gl_fetch(disvars,i);
        if(dvar != NULL) {
          dis_set_previous_value(dvar,orig_values[i - 1]);
        }
      }
      if(accepted != NULL) {
        *accepted = local;
      }
      if(accepted_cases != NULL) {
        *accepted_cases = ncases;
      }
      if(accepted_mask != NULL) {
        *accepted_mask = mask;
      }
      if(status_code != NULL) {
        *status_code = 0;
      }
      ascfree(orig_values);
      ascfree(orig_previous);
      ascfree(orig_real_values);
      ascfree(orig_dis_values);
      ascfree(orig_dis_previous);
      return 1;
    }
    slv9_cmslv2_restore_real_values(
      sys->slv,orig_real_values,orig_real_count
    );
    slv9_cmslv2_restore_discrete_values(
      sys->slv,orig_dis_values,orig_dis_previous,orig_dis_count
    );
  }

  slv9_cmslv2_selector_restore_values(
    disvars,orig_values,orig_previous
  );
  slv9_reanalyze_solver_lists(sys);
  update_relations_residuals(sys->slv);
  if(status_code != NULL) {
    *status_code = active_status != 0 ? active_status : -6;
  }
  ascfree(orig_values);
  ascfree(orig_previous);
  ascfree(orig_real_values);
  ascfree(orig_dis_values);
  ascfree(orig_dis_previous);
  return 0;
}

static
int32 slv9_decomp_active_block_in_structural_region(
    const slv_decomp_partition_t *active,
    int32 active_block,
    const slv_decomp_partition_t *structural,
    const mtx_region_t *sregion
){
  int32 r, c, orgrow, orgcol, scur;
  mtx_region_t aregion;

  if(active == NULL || structural == NULL || sregion == NULL
      || active_block < 0 || active_block >= active->nblocks) {
    return 0;
  }

  aregion = active->blocks[active_block];
  for(r = aregion.row.low; r <= aregion.row.high; ++r) {
    orgrow = active->row_org[r];
    scur = orgrow >= 0 && orgrow < structural->n_rows
      ? structural->row_cur[orgrow] : -1;
    if(!slv9_decomp_index_in_range(scur,&sregion->row)) {
      return 0;
    }
  }
  for(c = aregion.col.low; c <= aregion.col.high; ++c) {
    orgcol = active->col_org[c];
    scur = orgcol >= 0 && orgcol < structural->n_cols
      ? structural->col_cur[orgcol] : -1;
    if(!slv9_decomp_index_in_range(scur,&sregion->col)) {
      return 0;
    }
  }
  return 1;
}

static
void slv9_cmslv2_tally_active_subblocks(slv_system_t server,
    const slv_decomp_partition_t *structural, int32 structural_block,
    const slv_decomp_partition_t *active,
    struct slv9_cmslv2_selector_summary *sum
){
  int32 b;
  mtx_region_t sregion;
  struct slv9_decomp_block_stats bs;

  if(server == NULL || structural == NULL || active == NULL || sum == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return;
  }

  sregion = structural->blocks[structural_block];
  for(b = 0; b < active->nblocks; ++b) {
    if(!slv9_decomp_active_block_in_structural_region(
        active,b,structural,&sregion
    )) {
      continue;
    }
    sum->active_subblocks++;
    slv9_decomp_assess_block(server,active,b,&bs);
    if(slv9_decomp_block_is_pure_real(&bs)) {
      sum->qrslv_subblocks++;
      if(bs.rows > sum->max_qrslv_rows) sum->max_qrslv_rows = bs.rows;
      if(bs.cols > sum->max_qrslv_cols) sum->max_qrslv_cols = bs.cols;
    }else if(slv9_decomp_block_is_pure_logical(&bs)) {
      sum->lrslv_subblocks++;
    }else{
      sum->unresolved_subblocks++;
    }
  }
}

static
void slv9_cmslv2_run_selector_envelopes(slv9_system_t sys, const char *phase){
  slv_decomp_partition_t structural, active;
  struct slv9_cmslv2_selector_summary total, local;
  struct slv9_decomp_block_stats bs;
  int32 status, b;

  if(sys == NULL || sys->slv == NULL || !CMSLV2_BLOCKSOLVE) {
    return;
  }
  /*
   * This all-envelope sweep is diagnostic only. The actual CMSlv2 selector
   * consumer below works from the structural cursor and performs the branch
   * search needed to advance the solve. Keeping this behind progress_log avoids
   * repeatedly rebuilding active partitions during normal solves.
   */
  if(!PROGRESS_LOG) {
    return;
  }

  slv9_cmslv2_selector_summary_init(&total);
  slv_decomp_init(&structural);
  slv_decomp_init(&active);

  status = slv_decomp_partition(sys->slv,&structural);
  if(status) {
    slv9_report_progress(sys,
      "event=cmslv2_enum phase=%s status=failed structural_status=%d",
      phase != NULL ? phase : "unknown", status
    );
    slv_decomp_destroy(&structural);
    return;
  }

  for(b = 0; b < structural.nblocks; ++b) {
    struct gl_list_t *disvars;
    int32 *cases = NULL;
    int32 ncases = 0;

    slv9_decomp_assess_block(sys->slv,&structural,b,&bs);
    if(!slv9_decomp_block_is_selector_envelope(&bs)) {
      continue;
    }
    total.envelopes++;
    disvars = gl_create((unsigned long)(bs.dvars > 0 ? bs.dvars : 1));
    if(disvars == NULL) {
      total.unresolved_subblocks++;
      continue;
    }
    slv9_cmslv2_collect_selector_dvars(sys->slv,&structural,b,disvars);
    cases = cases_matching(disvars,&ncases);
    if(cases != NULL && ncases > 0) {
      total.attempted++;
      total.matched_cases += ncases;
      /*
       * The existing conditional analysis owns the full row/column active
       * state. For this first flat selector sub-solver, use the matched cases
       * as proof of selector resolution, then rebuild the complete active view
       * before measuring the subblocks that QRSlv/LRSlv can consume.
       */
      slv9_reanalyze_solver_lists(sys);
      update_relations_residuals(sys->slv);

      slv_decomp_destroy(&active);
      slv_decomp_init(&active);
      status = slv_decomp_partition_active(sys->slv,&active);
      if(!status) {
        slv9_cmslv2_selector_summary_init(&local);
        slv9_cmslv2_tally_active_subblocks(
          sys->slv,&structural,b,&active,&local
        );
        total.active_subblocks += local.active_subblocks;
        total.qrslv_subblocks += local.qrslv_subblocks;
        total.lrslv_subblocks += local.lrslv_subblocks;
        total.unresolved_subblocks += local.unresolved_subblocks;
        if(local.max_qrslv_rows > total.max_qrslv_rows) {
          total.max_qrslv_rows = local.max_qrslv_rows;
        }
        if(local.max_qrslv_cols > total.max_qrslv_cols) {
          total.max_qrslv_cols = local.max_qrslv_cols;
        }
        slv9_report_progress(sys,
          "event=cmslv2_enum phase=%s block=%d dvars=%lu matched_cases=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d reduced=%d",
          phase != NULL ? phase : "unknown", b, gl_length(disvars), ncases,
          local.active_subblocks, local.qrslv_subblocks,
          local.lrslv_subblocks, local.unresolved_subblocks,
          local.unresolved_subblocks == 0 ? 1 : 0
        );
      }else{
        total.unresolved_subblocks++;
        slv9_report_progress(sys,
          "event=cmslv2_enum phase=%s block=%d dvars=%lu matched_cases=%d status=failed active_status=%d",
          phase != NULL ? phase : "unknown", b, gl_length(disvars), ncases,
          status
        );
      }
    }else{
      total.unresolved_subblocks++;
      slv9_report_progress(sys,
        "event=cmslv2_enum phase=%s block=%d dvars=%lu matched_cases=0 reduced=0",
        phase != NULL ? phase : "unknown", b, gl_length(disvars)
      );
    }
    if(cases != NULL) {
      ascfree(cases);
    }
    gl_destroy(disvars);
  }

  slv9_report_progress(sys,
    "event=cmslv2_enum_summary phase=%s envelopes=%d attempted=%d matched_cases=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d max_qrslv_rows=%d max_qrslv_cols=%d",
    phase != NULL ? phase : "unknown", total.envelopes, total.attempted,
    total.matched_cases, total.active_subblocks, total.qrslv_subblocks,
    total.lrslv_subblocks, total.unresolved_subblocks,
    total.max_qrslv_rows, total.max_qrslv_cols
  );

  slv_decomp_destroy(&active);
  slv_decomp_destroy(&structural);
}

static
int32 slv9_cmslv2_consume_selector_cursor(slv9_system_t sys, const char *phase){
  const slv_decomp_partition_t *structural = NULL;
  slv_decomp_partition_t structural_storage;
  slv_decomp_partition_t active;
  struct slv9_cmslv2_selector_summary local;
  struct slv9_decomp_block_stats bs;
  struct gl_list_t *disvars = NULL;
  unsigned long accepted_mask = 0, current_mask = 0;
  int32 ncases = 0, tried = 0, search_status = 0;
  int32 status, b, consumed = 0;
  int32 fresh_structural = 1;

  if(sys == NULL || sys->slv == NULL || !CMSLV2_BLOCKSOLVE) {
    return 0;
  }
  b = sys->cmslv2_next_structural_block;
  if(b < 0) {
    b = 0;
  }

  slv_decomp_init(&structural_storage);
  slv_decomp_init(&active);
  status = slv9_cmslv2_get_scheduler_structural(
    sys,phase,fresh_structural,&structural_storage,&structural
  );
  if(status) {
    slv9_report_progress(sys,
      "event=cmslv2_selector_consume phase=%s status=failed structural_status=%d",
      phase != NULL ? phase : "unknown", status
    );
    slv_decomp_destroy(&active);
    slv_decomp_destroy(&structural_storage);
    return 0;
  }
  while(b < structural->nblocks) {
    slv9_decomp_assess_block(sys->slv,structural,b,&bs);
    if(bs.rows == 0 || bs.cols == 0
        || slv9_decomp_block_is_pure_logical(&bs)) {
      slv9_report_progress(sys,
        "event=cmslv2_selector_consume phase=%s block=%d action=skip",
        phase != NULL ? phase : "unknown", b
      );
      b++;
      continue;
    }
    break;
  }
  sys->cmslv2_next_structural_block = b;
  if(b >= structural->nblocks) {
    slv_decomp_destroy(&active);
    slv_decomp_destroy(&structural_storage);
    return 0;
  }

  if(!slv9_decomp_block_is_selector_envelope(&bs)) {
    slv9_report_progress(sys,
      "event=cmslv2_selector_consume phase=%s block=%d action=not_selector",
      phase != NULL ? phase : "unknown", b
    );
    slv_decomp_destroy(&active);
    slv_decomp_destroy(&structural_storage);
    return 0;
  }

  disvars = gl_create((unsigned long)(bs.dvars > 0 ? bs.dvars : 1));
  if(disvars == NULL) {
    slv_decomp_destroy(&active);
    slv_decomp_destroy(&structural_storage);
    return 0;
  }
  slv9_cmslv2_collect_selector_dvars(sys->slv,structural,b,disvars);
  current_mask = slv9_cmslv2_selector_current_mask(disvars);
  consumed = slv9_cmslv2_selector_branch_search(
    sys,structural,b,&active,disvars,&local,&ncases,&accepted_mask,
    phase,&tried,&search_status
  );
  slv9_report_progress(sys,
    "event=cmslv2_selector_search phase=%s block=%d mode=branch_search dvars=%lu tried=%d accepted=%d changed=%d accepted_mask=%lu status=%d",
    phase != NULL ? phase : "unknown", b, gl_length(disvars), tried,
    consumed ? 1 : 0, consumed && accepted_mask != current_mask ? 1 : 0,
    accepted_mask, search_status
  );
  if(consumed) {
    struct slv9_cmslv2_block_solved_report solved_report;
    int32 solved = slv9_cmslv2_check_block_currently_solved(
      sys,structural,b,phase,&solved_report
    );
    if(solved) {
      slv9_cmslv2_transition_after_solved_block(
        sys,phase,"selector",b,b + 1
      );
      slv9_report_progress(sys,
        "event=cmslv2_selector_consume phase=%s block=%d dvars=%lu matched_cases=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d action=advance next_structural=%d",
        phase != NULL ? phase : "unknown", b, gl_length(disvars), ncases,
        local.active_subblocks, local.qrslv_subblocks,
        local.lrslv_subblocks, local.unresolved_subblocks,
        sys->cmslv2_next_structural_block
      );
    }else{
      consumed = 0;
      slv9_report_progress(sys,
        "event=cmslv2_selector_consume phase=%s block=%d dvars=%lu matched_cases=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d action=defer status=%d next_structural=%d reason=%s",
        phase != NULL ? phase : "unknown", b, gl_length(disvars), ncases,
        local.active_subblocks, local.qrslv_subblocks,
        local.lrslv_subblocks, local.unresolved_subblocks, search_status,
        sys->cmslv2_next_structural_block, solved_report.reason
      );
    }
  }else{
    slv9_report_progress(sys,
      "event=cmslv2_selector_consume phase=%s block=%d dvars=%lu matched_cases=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d action=defer status=%d next_structural=%d",
      phase != NULL ? phase : "unknown", b, gl_length(disvars), ncases,
      local.active_subblocks, local.qrslv_subblocks,
      local.lrslv_subblocks, local.unresolved_subblocks, search_status,
      sys->cmslv2_next_structural_block
    );
  }

  gl_destroy(disvars);
  slv_decomp_destroy(&active);
  slv_decomp_destroy(&structural_storage);
  return consumed;
}

static
void slv9_cmslv2_boundary_scope_init(
    struct slv9_cmslv2_boundary_scope *scope
){
  if(scope != NULL) {
    memset(scope,0,sizeof(*scope));
  }
}

static
int32 slv9_cmslv2_boundary_scope_alloc(
    struct slv9_cmslv2_boundary_scope *scope
){
  if(scope == NULL) {
    return 0;
  }
  if(scope->nrels > 0) {
    scope->rel_active_save = ASC_NEW_ARRAY(unsigned char,scope->nrels);
    if(scope->rel_active_save == NULL) return 0;
  }
  if(scope->ncondrels > 0) {
    scope->condrel_active_save = ASC_NEW_ARRAY(unsigned char,scope->ncondrels);
    if(scope->condrel_active_save == NULL) return 0;
  }
  if(scope->nlogrels > 0) {
    scope->logrel_active_save = ASC_NEW_ARRAY(unsigned char,scope->nlogrels);
    if(scope->logrel_active_save == NULL) return 0;
  }
  if(scope->ncondlogrels > 0) {
    scope->condlogrel_active_save =
      ASC_NEW_ARRAY(unsigned char,scope->ncondlogrels);
    if(scope->condlogrel_active_save == NULL) return 0;
  }
  if(scope->nvars > 0) {
    scope->var_active_save = ASC_NEW_ARRAY(unsigned char,scope->nvars);
    if(scope->var_active_save == NULL) return 0;
  }
  if(scope->ndvars > 0) {
    scope->dvar_active_save = ASC_NEW_ARRAY(unsigned char,scope->ndvars);
    if(scope->dvar_active_save == NULL) return 0;
  }
  return 1;
}

static
void slv9_cmslv2_boundary_scope_restore(slv_system_t server,
    struct slv9_cmslv2_boundary_scope *scope
){
  int32 i;
  struct rel_relation **rels, **condrels;
  struct logrel_relation **logrels, **condlogrels;
  struct var_variable **vars;
  struct dis_discrete **dvars;

  if(server == NULL || scope == NULL) {
    return;
  }
  rels = slv_get_solvers_rel_list(server);
  condrels = slv_get_solvers_condrel_list(server);
  logrels = slv_get_solvers_logrel_list(server);
  condlogrels = slv_get_solvers_condlogrel_list(server);
  vars = slv_get_solvers_var_list(server);
  dvars = slv_get_solvers_dvar_list(server);
  if(rels != NULL && scope->rel_active_save != NULL) {
    for(i = 0; i < scope->nrels; ++i) {
      rel_set_active(rels[i],scope->rel_active_save[i] ? TRUE : FALSE);
    }
  }
  if(condrels != NULL && scope->condrel_active_save != NULL) {
    for(i = 0; i < scope->ncondrels; ++i) {
      rel_set_active(condrels[i],
        scope->condrel_active_save[i] ? TRUE : FALSE
      );
    }
  }
  if(logrels != NULL && scope->logrel_active_save != NULL) {
    for(i = 0; i < scope->nlogrels; ++i) {
      logrel_set_active(logrels[i],
        scope->logrel_active_save[i] ? TRUE : FALSE
      );
    }
  }
  if(condlogrels != NULL && scope->condlogrel_active_save != NULL) {
    for(i = 0; i < scope->ncondlogrels; ++i) {
      logrel_set_active(condlogrels[i],
        scope->condlogrel_active_save[i] ? TRUE : FALSE
      );
    }
  }
  if(vars != NULL && scope->var_active_save != NULL) {
    for(i = 0; i < scope->nvars; ++i) {
      var_set_active(vars[i],scope->var_active_save[i] ? TRUE : FALSE);
    }
  }
  if(dvars != NULL && scope->dvar_active_save != NULL) {
    for(i = 0; i < scope->ndvars; ++i) {
      dis_set_active(dvars[i],scope->dvar_active_save[i] ? TRUE : FALSE);
    }
  }
}

static
void slv9_cmslv2_boundary_scope_destroy(
    struct slv9_cmslv2_boundary_scope *scope
){
  if(scope == NULL) {
    return;
  }
  if(scope->rel_active_save != NULL) ascfree(scope->rel_active_save);
  if(scope->condrel_active_save != NULL) ascfree(scope->condrel_active_save);
  if(scope->logrel_active_save != NULL) ascfree(scope->logrel_active_save);
  if(scope->condlogrel_active_save != NULL) {
    ascfree(scope->condlogrel_active_save);
  }
  if(scope->var_active_save != NULL) ascfree(scope->var_active_save);
  if(scope->dvar_active_save != NULL) ascfree(scope->dvar_active_save);
  slv9_cmslv2_boundary_scope_init(scope);
}

static
int32 slv9_cmslv2_install_boundary_scope(slv_system_t server,
    const slv_decomp_partition_t *structural, int32 structural_block,
    struct slv9_cmslv2_boundary_scope *scope
){
  int32 i, org;
  struct rel_relation **rels, **condrels;
  struct logrel_relation **logrels, **condlogrels;
  struct var_variable **vars;
  struct dis_discrete **dvars;

  if(server == NULL || structural == NULL || scope == NULL
      || structural_block < 0 || structural_block >= structural->nblocks) {
    return 0;
  }
  slv9_cmslv2_boundary_scope_init(scope);
  scope->nrels = slv_get_num_solvers_rels(server);
  scope->ncondrels = slv_get_num_solvers_condrels(server);
  scope->nlogrels = slv_get_num_solvers_logrels(server);
  scope->ncondlogrels = slv_get_num_solvers_condlogrels(server);
  scope->nvars = slv_get_num_solvers_vars(server);
  scope->ndvars = slv_get_num_solvers_dvars(server);
  if(!slv9_cmslv2_boundary_scope_alloc(scope)) {
    slv9_cmslv2_boundary_scope_destroy(scope);
    return 0;
  }

  rels = slv_get_solvers_rel_list(server);
  condrels = slv_get_solvers_condrel_list(server);
  logrels = slv_get_solvers_logrel_list(server);
  condlogrels = slv_get_solvers_condlogrel_list(server);
  vars = slv_get_solvers_var_list(server);
  dvars = slv_get_solvers_dvar_list(server);

  if(rels != NULL) {
    for(i = 0; i < scope->nrels; ++i) {
      int32 keep;
      scope->rel_active_save[i] = rel_active(rels[i]) ? 1 : 0;
      keep = slv9_decomp_org_row_in_block(structural,structural_block,i);
      rel_set_active(rels[i],
        keep && scope->rel_active_save[i] ? TRUE : FALSE
      );
      if(keep && scope->rel_active_save[i]) scope->scoped_rels++;
    }
  }
  if(condrels != NULL) {
    for(i = 0; i < scope->ncondrels; ++i) {
      int32 keep;
      org = structural->n_rels + i;
      scope->condrel_active_save[i] = rel_active(condrels[i]) ? 1 : 0;
      keep = slv9_decomp_org_row_in_block(
        structural,structural_block,org
      );
      rel_set_active(condrels[i],
        keep && scope->condrel_active_save[i] ? TRUE : FALSE
      );
      if(keep && scope->condrel_active_save[i]) scope->scoped_condrels++;
    }
  }
  if(logrels != NULL) {
    for(i = 0; i < scope->nlogrels; ++i) {
      int32 keep;
      org = structural->n_rels + structural->n_condrels + i;
      scope->logrel_active_save[i] = logrel_active(logrels[i]) ? 1 : 0;
      keep = slv9_decomp_org_row_in_block(
        structural,structural_block,org
      );
      logrel_set_active(logrels[i],
        keep && scope->logrel_active_save[i] ? TRUE : FALSE
      );
      if(keep && scope->logrel_active_save[i]) scope->scoped_logrels++;
    }
  }
  if(condlogrels != NULL) {
    for(i = 0; i < scope->ncondlogrels; ++i) {
      int32 keep;
      org = structural->n_rels + structural->n_condrels
        + structural->n_logrels + i;
      scope->condlogrel_active_save[i] =
        logrel_active(condlogrels[i]) ? 1 : 0;
      keep = slv9_decomp_org_row_in_block(
        structural,structural_block,org
      );
      logrel_set_active(condlogrels[i],
        keep && scope->condlogrel_active_save[i] ? TRUE : FALSE
      );
      if(keep && scope->condlogrel_active_save[i]) {
        scope->scoped_condlogrels++;
      }
    }
  }
  if(vars != NULL) {
    for(i = 0; i < scope->nvars; ++i) {
      int32 keep;
      scope->var_active_save[i] = var_active(vars[i]) ? 1 : 0;
      keep = slv9_decomp_org_col_in_block(structural,structural_block,i);
      var_set_active(vars[i],
        keep && scope->var_active_save[i] ? TRUE : FALSE
      );
      if(keep && scope->var_active_save[i]) scope->scoped_vars++;
    }
  }
  if(dvars != NULL) {
    for(i = 0; i < scope->ndvars; ++i) {
      int32 keep;
      org = structural->n_vars + i;
      scope->dvar_active_save[i] = dis_active(dvars[i]) ? 1 : 0;
      keep = slv9_decomp_org_col_in_block(structural,structural_block,org);
      dis_set_active(dvars[i],
        keep && scope->dvar_active_save[i] ? TRUE : FALSE
      );
      if(keep && scope->dvar_active_save[i]) scope->scoped_dvars++;
    }
  }
  return 1;
}

static
int32 slv9_cmslv2_boundary_local_traverse(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    const char *phase, int32 structural_block,
    struct slv9_cmslv2_boundary_result *result
){
  slv_system_t server;
  SlvClientToken orig_token;
  int32 orig_solver;
  real64 *orig_real_values = NULL;
  int32 *orig_dis_values = NULL, *orig_dis_previous = NULL;
  int32 orig_real_count = 0, orig_dis_count = 0;
  struct slv9_cmslv2_boundary_flags orig_bnd_flags;
  struct matching_cases *subregions = NULL;
  struct real_values rvalues;
  struct gl_list_t *disvars = NULL;
  var_filter_t vfilter;
  real64 factor = 1.0;
  int32 n_subregions = 0, cur_subregion = -1;
  int32 at_boundary = 0, opt_ok = 0, returned = 0;
  unsigned long dvar_count = 0;

  if(sys == NULL || sys->slv == NULL || structural == NULL) {
    return 0;
  }
  if(result != NULL) {
    slv9_cmslv2_boundary_result_destroy(result);
  }
  server = sys->slv;
  orig_token = slv_get_client_token(server);
  orig_solver = slv_get_selected_solver(server);
  rvalues.pre_values = NULL;
  rvalues.cur_values = NULL;
  slv9_cmslv2_boundary_flags_init(&orig_bnd_flags);

  slv9_cmslv2_store_real_values(server,&orig_real_values,&orig_real_count);
  slv9_cmslv2_store_discrete_values(
    server,&orig_dis_values,&orig_dis_previous,&orig_dis_count
  );
  slv9_cmslv2_store_boundary_flags(server,&orig_bnd_flags);
  if(orig_real_values == NULL || orig_dis_values == NULL
      || orig_dis_previous == NULL || orig_bnd_flags.flags == NULL) {
    slv9_report_progress(sys,
      "event=cmslv2_boundary_local_traverse phase=%s block=%d action=snapshot_failed",
      phase != NULL ? phase : "unknown", structural_block
    );
    goto finish;
  }

  disvars = gl_create(1L);
  if(disvars == NULL) {
    slv9_report_progress(sys,
      "event=cmslv2_boundary_local_traverse phase=%s block=%d action=alloc_failed",
      phase != NULL ? phase : "unknown", structural_block
    );
    goto finish;
  }

  update_boundaries(server,(SlvClientToken)sys);
  at_boundary = at_a_boundary(
    server,(SlvClientToken)sys,&n_subregions,&subregions,&cur_subregion,disvars
  );
  if(at_boundary) {
    store_real_pre_values(server,&rvalues);
    opt_ok = optimize_at_boundary(
      server,(SlvClientToken)sys,&n_subregions,subregions,&cur_subregion,
      disvars,&rvalues
    );
    subregions = NULL;
    if(opt_ok) {
      store_real_cur_values(server,&rvalues);
      update_boundaries(server,(SlvClientToken)sys);
      if(some_boundaries_crossed(server,(SlvClientToken)sys)) {
        vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
          | VAR_SVAR | VAR_FIXED);
        vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
        factor = return_to_first_boundary(
          server,(SlvClientToken)sys,&rvalues,&vfilter
        );
        update_real_var_values(server,&rvalues,&vfilter,factor);
        rvalues.pre_values = NULL;
        rvalues.cur_values = NULL;
        update_boundaries(server,(SlvClientToken)sys);
        update_relations_residuals(server);
        returned = 1;
      }else if(rvalues.cur_values != NULL && rvalues.pre_values != NULL) {
        destroy_array(rvalues.cur_values);
        destroy_array(rvalues.pre_values);
        rvalues.cur_values = NULL;
        rvalues.pre_values = NULL;
      }
    }else if(rvalues.pre_values != NULL) {
      destroy_array(rvalues.pre_values);
      rvalues.pre_values = NULL;
    }
  }

  slv9_report_progress(sys,
    "event=cmslv2_boundary_local_traverse phase=%s block=%d action=ran at_boundary=%d optimize_ok=%d returned=%d n_subregions=%d cur_subregion=%d dvars=%lu factor=%.17g",
    phase != NULL ? phase : "unknown", structural_block,
    at_boundary, opt_ok, returned, n_subregions, cur_subregion,
    disvars != NULL ? gl_length(disvars) : 0, factor
  );
  dvar_count = disvars != NULL ? gl_length(disvars) : 0;
  if(result != NULL) {
    if(!slv9_cmslv2_boundary_result_capture(
        server,structural,structural_block,result,
        at_boundary,opt_ok,returned,n_subregions,
        cur_subregion,dvar_count,factor
    )) {
      slv9_report_progress(sys,
        "event=cmslv2_boundary_local_commit phase=%s block=%d action=no_accept at_boundary=%d optimize_ok=%d",
        phase != NULL ? phase : "unknown", structural_block,
        at_boundary, opt_ok
      );
    }
  }

finish:
  if(rvalues.cur_values != NULL) {
    destroy_array(rvalues.cur_values);
  }
  if(rvalues.pre_values != NULL) {
    destroy_array(rvalues.pre_values);
  }
  if(subregions != NULL && n_subregions > 0) {
    int32 i;
    for(i = 0; i < n_subregions; ++i) {
      if(subregions[i].case_list != NULL) {
        destroy_array(subregions[i].case_list);
      }
    }
    destroy_array(subregions);
  }
  if(orig_real_values != NULL) {
    slv9_cmslv2_restore_real_values(server,orig_real_values,orig_real_count);
  }
  if(orig_dis_values != NULL && orig_dis_previous != NULL) {
    slv9_cmslv2_restore_discrete_values(
      server,orig_dis_values,orig_dis_previous,orig_dis_count
    );
  }
  slv9_cmslv2_restore_boundary_flags(server,&orig_bnd_flags);
  update_relations_residuals(server);
  slv_set_solver_index(server,orig_solver);
  slv_set_client_token(server,orig_token);
  if(disvars != NULL) {
    gl_destroy(disvars);
  }
  if(orig_real_values != NULL) ascfree(orig_real_values);
  if(orig_dis_values != NULL) ascfree(orig_dis_values);
  if(orig_dis_previous != NULL) ascfree(orig_dis_previous);
  slv9_cmslv2_boundary_flags_destroy(&orig_bnd_flags);
  return at_boundary && opt_ok;
}

static
int32 slv9_cmslv2_probe_due_boundary_block(slv9_system_t sys,
    const slv_decomp_partition_t *structural,
    const slv_decomp_partition_t *active, const char *phase
){
  int32 cursor, active_status;
  slv_decomp_partition_t scoped_active;
  struct slv9_decomp_block_stats bs;
  struct slv9_cmslv2_selector_summary local;
  struct slv9_cmslv2_work_unit work;
  struct slv9_cmslv2_boundary_scope scope;
  struct slv9_cmslv2_boundary_result result;
  real64 *orig_real_values = NULL;
  int32 *orig_dis_values = NULL, *orig_dis_previous = NULL;
  int32 orig_real_count = 0, orig_dis_count = 0;
  struct slv9_cmslv2_boundary_flags orig_bnd_flags;
  int32 accepted;
  int32 local_complete;

  if(sys == NULL || sys->slv == NULL || structural == NULL || active == NULL
      || structural->nblocks <= 0) {
    return 0;
  }
  if(!slv9_cmslv2_scheduler_query(
      sys,structural,active,
      SLV9_CMSLV2_QUERY_BLOCK | SLV9_CMSLV2_QUERY_SKIP_EMPTY,&work
  )) {
    return 0;
  }
  if(work.kind != SLV9_CMSLV2_WORK_BOUNDARY) {
    sys->cmslv2_next_structural_block = work.start;
    return 0;
  }
  cursor = work.start;
  bs = work.stats;

  slv9_cmslv2_selector_summary_init(&local);
  local = work.summary;
  slv9_cmslv2_boundary_result_init(&result);
  slv9_cmslv2_boundary_flags_init(&orig_bnd_flags);
  slv9_cmslv2_boundary_scope_init(&scope);
  slv_decomp_init(&scoped_active);
  if(!slv9_cmslv2_install_boundary_scope(
      sys->slv,structural,cursor,&scope
  )) {
    slv9_report_progress(sys,
      "event=cmslv2_boundary_local phase=%s block=%d action=scope_failed relrows=%d logrows=%d vars=%d dvars=%d boundary_edges=%d selector_edges=%d",
      phase != NULL ? phase : "unknown", cursor,
      bs.relrows, bs.logrows, bs.vars, bs.dvars,
      bs.boundary_edges, bs.selector_edges
    );
    slv9_report_progress(sys,
      "event=cmslv2_scheduler phase=%s structural_block=%d action=boundary_local_defer next_structural=%d reason=scope_failed",
      phase != NULL ? phase : "unknown", cursor,
      sys->cmslv2_next_structural_block
    );
    return 0;
  }
  active_status = -1;
  if(PROGRESS_LOG) {
    active_status = slv_decomp_partition_active(sys->slv,&scoped_active);
  }
  slv9_report_progress(sys,
    "event=cmslv2_boundary_local phase=%s block=%d action=scoped relrows=%d logrows=%d vars=%d dvars=%d boundary_edges=%d selector_edges=%d scoped_rels=%d scoped_condrels=%d scoped_logrels=%d scoped_condlogrels=%d scoped_vars=%d scoped_dvars=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d scoped_active_status=%d scoped_active_blocks=%d",
    phase != NULL ? phase : "unknown", cursor,
    bs.relrows, bs.logrows, bs.vars, bs.dvars,
    bs.boundary_edges, bs.selector_edges,
    scope.scoped_rels, scope.scoped_condrels,
    scope.scoped_logrels, scope.scoped_condlogrels,
    scope.scoped_vars, scope.scoped_dvars,
    local.active_subblocks, local.qrslv_subblocks,
    local.lrslv_subblocks, local.unresolved_subblocks,
    active_status,
    active_status == 0 ? scoped_active.nblocks : 0
  );
  (void)slv9_cmslv2_boundary_local_traverse(
    sys,structural,phase,cursor,&result
  );
  slv_decomp_destroy(&scoped_active);
  slv9_cmslv2_boundary_scope_restore(sys->slv,&scope);
  slv9_cmslv2_boundary_scope_destroy(&scope);
  if(result.accepted) {
    real64 max_residual = 0.0;
    const char *validate_reason = "not_validated";
    slv9_cmslv2_store_real_values(
      sys->slv,&orig_real_values,&orig_real_count
    );
    slv9_cmslv2_store_discrete_values(
      sys->slv,&orig_dis_values,&orig_dis_previous,&orig_dis_count
    );
    slv9_cmslv2_store_boundary_flags(sys->slv,&orig_bnd_flags);
    slv9_cmslv2_boundary_result_apply(sys,phase,cursor,&result);
    result.local_complete = slv9_cmslv2_validate_boundary_envelope(
      sys,structural,cursor,phase,&max_residual,&validate_reason
    );
    if(result.local_complete) {
      slv9_cmslv2_transition_after_solved_block(
        sys,phase,"boundary",cursor,cursor + 1
      );
    }else{
      if(orig_real_values != NULL) {
        slv9_cmslv2_restore_real_values(
          sys->slv,orig_real_values,orig_real_count
        );
      }
      if(orig_dis_values != NULL && orig_dis_previous != NULL) {
        slv9_cmslv2_restore_discrete_values(
          sys->slv,orig_dis_values,orig_dis_previous,orig_dis_count
        );
      }
      if(orig_bnd_flags.flags != NULL) {
        slv9_cmslv2_restore_boundary_flags(sys->slv,&orig_bnd_flags);
      }
      slv9_reanalyze_solver_lists(sys);
      update_boundaries(sys->slv,(SlvClientToken)sys);
      update_relations_residuals(sys->slv);
      slv9_report_progress(sys,
        "event=cmslv2_boundary_local_commit phase=%s block=%d action=reverted reason=not_local_complete",
        phase != NULL ? phase : "unknown", cursor
      );
    }
  }
  if(result.accepted && result.local_complete) {
    slv9_report_progress(sys,
      "event=cmslv2_scheduler phase=%s structural_block=%d action=boundary_local_advance next_structural=%d reason=accepted local_complete=%d",
      phase != NULL ? phase : "unknown", cursor,
      sys->cmslv2_next_structural_block, result.local_complete ? 1 : 0
    );
  }else{
    const char *reason = "capture_failed";
    if(result.accepted) {
      reason = "not_local_complete";
    }else if(!result.at_boundary) {
      reason = "not_at_boundary";
    }else if(!result.optimize_ok) {
      reason = "optimize_failed";
    }
    slv9_report_progress(sys,
      "event=cmslv2_scheduler phase=%s structural_block=%d action=boundary_local_defer next_structural=%d reason=%s",
      phase != NULL ? phase : "unknown", cursor,
      sys->cmslv2_next_structural_block, reason
    );
  }
  accepted = result.accepted;
  local_complete = result.local_complete;
  if(orig_real_values != NULL) ascfree(orig_real_values);
  if(orig_dis_values != NULL) ascfree(orig_dis_values);
  if(orig_dis_previous != NULL) ascfree(orig_dis_previous);
  slv9_cmslv2_boundary_flags_destroy(&orig_bnd_flags);
  slv9_cmslv2_boundary_result_destroy(&result);
  return accepted && local_complete ? 3 : 1;
}

static
void slv9_cmslv2_report_boundary_handoff(slv9_system_t sys,
    const char *phase, const char *action,
    int32 n_subregions, int32 cur_subregion, struct gl_list_t *disvars
){
  slv_decomp_partition_t structural, active;
  struct slv9_cmslv2_selector_summary total, local;
  struct slv9_decomp_block_stats bs;
  int32 structural_status, active_status, b;

  if(sys == NULL || sys->slv == NULL || !CMSLV2_BLOCKSOLVE) {
    return;
  }

  if(!PROGRESS_LOG) {
    slv9_report_progress(sys,
      "event=cmslv2_boundary_summary phase=%s action=%s mode=full_cmslv_boundary diagnostic=0 n_subregions=%d cur_subregion=%d dvars=%lu",
      phase != NULL ? phase : "unknown",
      action != NULL ? action : "unknown",
      n_subregions, cur_subregion, disvars != NULL ? gl_length(disvars) : 0
    );
    return;
  }

  slv9_cmslv2_selector_summary_init(&total);
  slv_decomp_init(&structural);
  slv_decomp_init(&active);

  structural_status = slv_decomp_partition(sys->slv,&structural);
  active_status = slv_decomp_partition_active(sys->slv,&active);
  if(structural_status || active_status) {
    slv9_report_progress(sys,
      "event=cmslv2_boundary phase=%s action=%s status=failed structural_status=%d active_status=%d",
      phase != NULL ? phase : "unknown",
      action != NULL ? action : "unknown",
      structural_status, active_status
    );
    slv_decomp_destroy(&active);
    slv_decomp_destroy(&structural);
    return;
  }

  for(b = 0; b < structural.nblocks; ++b) {
    slv9_decomp_assess_block(sys->slv,&structural,b,&bs);
    if(!slv9_decomp_block_is_boundary_envelope(&bs)) {
      continue;
    }
    total.envelopes++;
    slv9_cmslv2_selector_summary_init(&local);
    slv9_cmslv2_tally_active_subblocks(sys->slv,&structural,b,&active,&local);
    total.active_subblocks += local.active_subblocks;
    total.qrslv_subblocks += local.qrslv_subblocks;
    total.lrslv_subblocks += local.lrslv_subblocks;
    total.unresolved_subblocks += local.unresolved_subblocks;
    if(local.max_qrslv_rows > total.max_qrslv_rows) {
      total.max_qrslv_rows = local.max_qrslv_rows;
    }
    if(local.max_qrslv_cols > total.max_qrslv_cols) {
      total.max_qrslv_cols = local.max_qrslv_cols;
    }
    slv9_report_progress(sys,
      "event=cmslv2_boundary phase=%s action=%s block=%d mode=full_cmslv_boundary relrows=%d logrows=%d vars=%d dvars=%d boundary_edges=%d active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d",
      phase != NULL ? phase : "unknown",
      action != NULL ? action : "unknown",
      b, bs.relrows, bs.logrows, bs.vars, bs.dvars, bs.boundary_edges,
      local.active_subblocks, local.qrslv_subblocks,
      local.lrslv_subblocks, local.unresolved_subblocks
    );
  }

  slv9_report_progress(sys,
    "event=cmslv2_boundary_summary phase=%s action=%s mode=full_cmslv_boundary envelopes=%d n_subregions=%d cur_subregion=%d dvars=%lu active_subblocks=%d qrslv_subblocks=%d lrslv_subblocks=%d unresolved_subblocks=%d max_qrslv_rows=%d max_qrslv_cols=%d",
    phase != NULL ? phase : "unknown",
    action != NULL ? action : "unknown",
    total.envelopes, n_subregions, cur_subregion,
    disvars != NULL ? gl_length(disvars) : 0,
    total.active_subblocks, total.qrslv_subblocks,
    total.lrslv_subblocks, total.unresolved_subblocks,
    total.max_qrslv_rows, total.max_qrslv_cols
  );

  slv_decomp_destroy(&active);
  slv_decomp_destroy(&structural);
}

static
void slv9_report_decomp_assessment(slv9_system_t sys, const char *event){
  slv_decomp_partition_t structural, active;
  struct slv9_decomp_summary as;
  int structural_status, active_status;

  if(sys == NULL || sys->slv == NULL) {
    return;
  }
  if(!PROGRESS_LOG && !PROGRESS_CALLBACKS) {
    return;
  }
  slv_decomp_init(&structural);
  slv_decomp_init(&active);
  structural_status = slv_decomp_partition(sys->slv,&structural);
  active_status = slv_decomp_partition_active(sys->slv,&active);
  if(structural_status || active_status) {
    slv9_report_progress(sys,
      "event=decomp_assess phase=%s status=failed structural_status=%d active_status=%d",
      event != NULL ? event : "unknown", structural_status, active_status
    );
    slv_decomp_destroy(&structural);
    slv_decomp_destroy(&active);
    return;
  }

  slv9_decomp_summarize(sys->slv,&active,&as);
  slv9_report_progress(sys,
    "event=decomp_assess phase=%s structural_blocks=%d active_blocks=%d structural_nnz=%d active_nnz=%d pure_real=%d pure_integer=%d pure_logical=%d selector=%d boundary_mixed=%d mixed=%d empty=%d max_active_rows=%d max_active_cols=%d",
    event != NULL ? event : "unknown",
    structural.nblocks, active.nblocks, structural.nnz, active.nnz,
    as.pure_real, as.pure_integer, as.pure_logical, as.selector_coupled,
    as.boundary_mixed, as.mixed, as.empty, as.max_rows, as.max_cols
  );
  slv_decomp_destroy(&structural);
  slv_decomp_destroy(&active);
}

static
int32 slv9_decomp_summary_partitionable(const struct slv9_decomp_summary *summary){
  return summary != NULL
    && summary->pure_real > 0
    && summary->pure_integer == 0
    && summary->selector_coupled == 0
    && summary->boundary_mixed == 0
    && summary->mixed == 0;
}

static
void slv9_set_qrslv_external_mode(slv_system_t server, int32 external_blocks){
  char *param;
  union param_value u;

  param = "partition";
  u.b = 0;
  set_param_in_solver(server,NONLINEAR_SOLVER,bool_parm,param,&u);
  param = "external_blocks";
  u.b = external_blocks ? 1 : 0;
  set_param_in_solver(server,NONLINEAR_SOLVER,bool_parm,param,&u);
}

static
void slv9_set_lrslv_external_mode(slv_system_t server, int32 external_blocks){
  char *param;
  union param_value u;

  param = "external_blocks";
  u.b = external_blocks ? 1 : 0;
  set_param_in_solver(server,LOGICAL_SOLVER,bool_parm,param,&u);
}

enum slv9_cmslv2_qr_policy {
  SLV9_CMSLV2_QR_SUPPRESS = 0,
  SLV9_CMSLV2_QR_INSTALL_FOR_CALLER = 1
};

static
void slv9_cmslv2_clear_structural_cache(slv9_system_t sys){
  if(sys == NULL) {
    return;
  }
  slv_decomp_destroy(&(sys->cmslv2_structural));
  slv_decomp_init(&(sys->cmslv2_structural));
  sys->cmslv2_structural_valid = 0;
}

static
int32 slv9_cmslv2_get_structural_partition(slv9_system_t sys,
    const char *phase, const slv_decomp_partition_t **structural
){
  int32 structural_status;

  if(structural != NULL) {
    *structural = NULL;
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL) {
    return 1;
  }
  if(!sys->cmslv2_structural_valid) {
    slv_decomp_destroy(&(sys->cmslv2_structural));
    slv_decomp_init(&(sys->cmslv2_structural));
    structural_status =
      slv_decomp_partition_connected(sys->slv,&(sys->cmslv2_structural));
    if(structural_status) {
      slv9_set_qrslv_external_mode(sys->slv,0);
      slv9_report_progress(sys,
        "event=decomp_partition phase=%s partition=structural status=failed structural_status=%d",
        phase != NULL ? phase : "unknown", structural_status
      );
      return 1;
    }
    sys->cmslv2_structural_valid = 1;
  }
  *structural = &(sys->cmslv2_structural);
  return 0;
}

static
int32 slv9_cmslv2_get_scheduler_structural(slv9_system_t sys,
    const char *phase, int32 fresh, slv_decomp_partition_t *storage,
    const slv_decomp_partition_t **structural
){
  int32 structural_status;

  if(structural != NULL) {
    *structural = NULL;
  }
  if(sys == NULL || sys->slv == NULL || structural == NULL) {
    return 1;
  }
  if(!fresh) {
    return slv9_cmslv2_get_structural_partition(sys,phase,structural);
  }
  if(storage == NULL) {
    return 1;
  }
  slv_decomp_destroy(storage);
  slv_decomp_init(storage);
  structural_status = slv_decomp_partition(sys->slv,storage);
  if(structural_status) {
    slv9_set_qrslv_external_mode(sys->slv,0);
    slv9_report_progress(sys,
      "event=decomp_partition phase=%s partition=structural status=failed structural_status=%d",
      phase != NULL ? phase : "unknown", structural_status
    );
    return 1;
  }
  *structural = storage;
  return 0;
}

static
int32 slv9_cmslv2_rebuild_active_partition(slv9_system_t sys,
    const char *phase, slv_decomp_partition_t *active
){
  int32 active_status;

  if(sys == NULL || sys->slv == NULL || active == NULL) {
    return 1;
  }

  slv_decomp_destroy(active);
  slv_decomp_init(active);

  active_status = slv_decomp_partition_active(sys->slv,active);
  if(active_status) {
    slv9_set_qrslv_external_mode(sys->slv,0);
    slv9_report_progress(sys,
      "event=decomp_partition phase=%s partition=active status=failed active_status=%d",
      phase != NULL ? phase : "unknown", active_status
    );
    return 1;
  }
  return 0;
}

static
int32 slv9_cmslv2_consume_lrslv_runs(slv9_system_t sys,
    const char *phase, const slv_decomp_partition_t *structural,
    slv_decomp_partition_t *active, int32 *lrslv_runs
){
  int32 lr_start = -1, lr_end = -1;

  if(sys == NULL || structural == NULL || active == NULL) {
    return 1;
  }
  while(slv9_cmslv2_consume_due_lrslv_blocks(
      sys,structural,active,phase,&lr_start,&lr_end
  )) {
    if(lrslv_runs != NULL) {
      (*lrslv_runs)++;
    }
    if(slv9_cmslv2_rebuild_active_partition(sys,phase,active)) {
      return 1;
    }
    lr_start = -1;
    lr_end = -1;
  }
  return 0;
}

static
int32 slv9_cmslv2_consume_local_qrslv_runs(slv9_system_t sys,
    const char *phase, const slv_decomp_partition_t *structural,
    slv_decomp_partition_t *active, int32 *qrslv_runs, int32 *lrslv_runs
){
  while(slv9_cmslv2_solve_due_qrslv_run(
      sys,structural,active,phase,"cmslv2_qrslv_local",NULL
  )) {
    if(qrslv_runs != NULL) {
      (*qrslv_runs)++;
    }
    if(slv9_cmslv2_consume_lrslv_runs(
        sys,phase,structural,active,lrslv_runs
    )) {
      return 1;
    }
  }
  return 0;
}

static
int32 slv9_apply_decomp_partition_policy_ex(slv9_system_t sys,
    const char *phase, enum slv9_cmslv2_qr_policy qr_policy
){
  const slv_decomp_partition_t *structural = NULL;
  slv_decomp_partition_t structural_storage;
  slv_decomp_partition_t active;
  struct slv9_decomp_summary ss, as;
  struct slv9_cmslv2_plan_summary plan;
  int32 active_recommended, structural_safe, recommend_partition;
  int32 installed_blocks = 0, installed_rows = 0, installed_cols = 0;
  int32 structural_start = -1, structural_end = -1;
  int32 lrslv_runs = 0;
  int32 local_qrslv_runs = 0;
  int32 boundary_status = 0;
  int32 want_progress = 0;
  int32 fresh_structural = 1;

  if(sys == NULL || sys->slv == NULL || !sys->solvers_ready) {
    return 0;
  }
  want_progress = PROGRESS_LOG || PROGRESS_CALLBACKS;
  if(!want_progress
      && phase != NULL
      && (strcmp(phase,"nl_presolve") == 0
        || strcmp(phase,"nl_represolve") == 0)) {
    fresh_structural = 0;
  }else{
    fresh_structural = 1;
  }
  slv_decomp_init(&structural_storage);
  slv_decomp_init(&active);
  if(slv9_cmslv2_get_scheduler_structural(
        sys,phase,fresh_structural,&structural_storage,&structural
      )
      || slv9_cmslv2_rebuild_active_partition(sys,phase,&active)) {
    slv_decomp_destroy(&structural_storage);
    slv_decomp_destroy(&active);
    return 0;
  }

  if(CMSLV2_BLOCKSOLVE) {
    if(slv9_cmslv2_consume_lrslv_runs(
        sys,phase,structural,&active,&lrslv_runs
    )) {
      slv_decomp_destroy(&structural_storage);
      slv_decomp_destroy(&active);
      return 0;
    }
    boundary_status =
      slv9_cmslv2_probe_due_boundary_block(sys,structural,&active,phase);
    if(boundary_status >= 2) {
      if(slv9_cmslv2_get_scheduler_structural(
            sys,phase,fresh_structural,&structural_storage,&structural
          )
          || slv9_cmslv2_rebuild_active_partition(sys,phase,&active)) {
        slv_decomp_destroy(&structural_storage);
        slv_decomp_destroy(&active);
        return 0;
      }
      if(slv9_cmslv2_consume_lrslv_runs(
          sys,phase,structural,&active,&lrslv_runs
      )) {
        slv_decomp_destroy(&structural_storage);
        slv_decomp_destroy(&active);
        return 0;
      }
      if(qr_policy == SLV9_CMSLV2_QR_SUPPRESS) {
        if(slv9_cmslv2_consume_local_qrslv_runs(
            sys,phase,structural,&active,&local_qrslv_runs,&lrslv_runs
        )) {
          slv_decomp_destroy(&structural_storage);
          slv_decomp_destroy(&active);
          return 0;
        }
      }
    }
  }

  memset(&ss,0,sizeof(ss));
  memset(&as,0,sizeof(as));
  slv9_cmslv2_plan_summary_init(&plan);
  active_recommended = 1;
  structural_safe = 1;
  if(want_progress) {
    slv9_decomp_summarize(sys->slv,structural,&ss);
    slv9_decomp_summarize(sys->slv,&active,&as);
    slv9_cmslv2_plan_from_decomp(sys->slv,structural,&active,&plan);
    active_recommended = slv9_decomp_summary_partitionable(&as);
    structural_safe = slv9_decomp_summary_partitionable(&ss);
  }
  recommend_partition = active_recommended && structural_safe;

  if(CMSLV2_BLOCKSOLVE
      && qr_policy == SLV9_CMSLV2_QR_INSTALL_FOR_CALLER
      && (want_progress ? active_recommended : 1)) {
    installed_blocks = slv9_cmslv2_install_due_qrslv_blocks(
      sys,structural,&active,phase,&installed_rows,&installed_cols,
      &structural_start,&structural_end
    );
  }
  slv9_set_qrslv_external_mode(sys->slv,installed_blocks > 0);

  slv9_report_progress(sys,
    "event=decomp_partition phase=%s partition=0 external_blocks=%d cmslv2=%d recommended=%d active_recommended=%d structural_safe=%d structural_blocks=%d active_blocks=%d structural_nnz=%d active_nnz=%d pure_real=%d pure_integer=%d pure_logical=%d selector=%d boundary_mixed=%d mixed=%d",
    phase != NULL ? phase : "unknown",
    installed_blocks > 0 ? 1 : 0, CMSLV2_BLOCKSOLVE ? 1 : 0,
    recommend_partition, active_recommended, structural_safe,
    structural->nblocks, active.nblocks, structural->nnz, active.nnz,
    as.pure_real, as.pure_integer, as.pure_logical,
    as.selector_coupled, as.boundary_mixed, as.mixed
  );
  if(CMSLV2_BLOCKSOLVE) {
    slv9_report_progress(sys,
      "event=cmslv2_qrslv_handoff phase=%s mode=%s structural_start=%d structural_end=%d next_structural=%d installed_blocks=%d installed_rows=%d installed_cols=%d lrslv_runs=%d local_qrslv_runs=%d boundary_local_complete=%d",
      phase != NULL ? phase : "unknown",
      installed_blocks > 0 ? "external_blocks"
        : (qr_policy == SLV9_CMSLV2_QR_INSTALL_FOR_CALLER
            ? "fallback" : "suppressed"),
      structural_start, structural_end, sys->cmslv2_next_structural_block,
      installed_blocks, installed_rows, installed_cols, lrslv_runs,
      local_qrslv_runs, boundary_status == 3 ? 1 : 0
    );
  }
  slv9_report_progress(sys,
    "event=cmslv2_plan phase=%s structural_blocks=%d active_blocks=%d qrslv_blocks=%d lrslv_blocks=%d enum_blocks=%d boundary_blocks=%d integer_blocks=%d fallback_blocks=%d skipped_blocks=%d max_qrslv_rows=%d max_qrslv_cols=%d driver=%s",
    phase != NULL ? phase : "unknown",
    plan.structural_blocks, plan.active_blocks,
    plan.qrslv_blocks, plan.lrslv_blocks, plan.enum_blocks,
    plan.boundary_blocks, plan.integer_blocks, plan.fallback_blocks,
    plan.skipped_blocks, plan.max_qrslv_rows, plan.max_qrslv_cols,
    CMSLV2_BLOCKSOLVE ? "cmslv2" : "legacy"
  );
  if(CMSLV2_BLOCKSOLVE
      && qr_policy == SLV9_CMSLV2_QR_SUPPRESS
      && boundary_status == 3
      && structural->nblocks > 0
      && sys->cmslv2_next_structural_block >= structural->nblocks) {
    slv_decomp_destroy(&structural_storage);
    slv_decomp_destroy(&active);
    return -1;
  }
  slv_decomp_destroy(&structural_storage);
  slv_decomp_destroy(&active);
  return installed_blocks;
}

static
int32 slv9_apply_decomp_partition_policy(slv9_system_t sys, const char *phase){
  return slv9_apply_decomp_partition_policy_ex(
    sys,phase,SLV9_CMSLV2_QR_INSTALL_FOR_CALLER
  );
}

/*------------------------------------------------------------------------------
  PARAMETER ASSIGNMENT
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
	    "CONOPT",
	    "IPOPT"
	  };
	  static char *qrslv_convopt_names[] = {
	    "ABSOLUTE",
	    "RELNOM_SCALE"
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

  slv_define_parm(parameters, bool_parm,
	       "progress_callbacks", "enable progress callbacks",
               "enable progress callbacks",
	       U_p_bool(val,1),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(PROGRESS_CALLBACKS_PTR,parameters);

  slv_define_parm(parameters, bool_parm,
	       "progress_log", "log progress to console",
               "log progress to console",
	       U_p_bool(val,0),U_p_bool(lo,0),U_p_bool(hi,1), 2);
  SLV_BPARM_MACRO(PROGRESS_LOG_PTR,parameters);

	  slv_define_parm(parameters, bool_parm,
		       "cmslv2", "experimental block-local solve",
	               "experimental block-local solve",
		       U_p_bool(val,1),U_p_bool(lo,0),U_p_bool(hi,1), 2);
	  SLV_BPARM_MACRO(CMSLV2_BLOCKSOLVE_PTR,parameters);

	  slv_define_parm(parameters, char_parm,
		       "convopt", "QRSlv convergence test", "QRSlv convergence test",
		       U_p_string(val,"ABSOLUTE"),
		       U_p_strings(lo,qrslv_convopt_names),
		       U_p_int(hi,sizeof(qrslv_convopt_names)/sizeof(char *)),1);
	  SLV_CPARM_MACRO(QRSLV_CONVOPT_PTR,parameters);

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

/*------------------------------------------------------------------------------
  EXTERNAL ROUTINES
*/

/**
	Create the tokens for the nonlinear solver and the logical solver.
	The token of the conditional solver will be assigned until the
	end slv9_create, which calls this function. Regarding the optimizer,
	we use the selected NLP solver in two different ways. We use a
	CMSlv-specific boundary adapter for optimization at a boundary, but
	we can also use a standard solver token if the model itself is an
	optimization problem. The vars and rels for the optimization problem
	at the boundary do not correspond to the vars of the slv, and
	therefore we have to create the data and calculate the gradients and
	residuals on the fly.

	This function will return 0 if successful. If some of the solvers
	required by the nonlinear, logical or optimization steps are not
	available, the function will return 1, and the system will not be
	created.
*/
static
int32 get_solvers_tokens(slv9_system_t sys, slv_system_t server){
		int32 newsolver;
			int32 num_log_reg, num_nl_reg, num_opt_reg = -1, num_cond_reg;
		char *param;
		union param_value u;

	const SlvFunctionsT *S;
	S = solver_engine_named(LOGSOLVER_OPTION);
	if(!S){
		FPRINTF(ASCERR,"Solver %s not available\n",LOGSOLVER_OPTION);
		return 1;
	}
	num_log_reg = S->number;

	S = solver_engine_named(NONLISOLVER_OPTION);
	if(!S){
		FPRINTF(ASCERR,"Solver %s not available\n",NONLISOLVER_OPTION);
		return 1;
	}
	num_nl_reg = S->number;

	if(sys == NULL || !sys->qrslv_fallback) {
		if(!slv9_ensure_optimizer_loaded(OPTSOLVER_OPTION)) {
			slv9_report_unavailable_optimizer(OPTSOLVER_OPTION);
			return 1;
		}
		S = solver_engine_named(OPTSOLVER_OPTION);
		if(!S){
			slv9_report_unavailable_optimizer(OPTSOLVER_OPTION);
			return 1;
		}
		MSG("Optimization solver found with name '%s'",S->name);
		MSG("Optimization solver found with number '%d'",S->number);
		num_opt_reg = S->number;
	}

	/* this is us! */
	S = solver_engine_named("CMSlv2");
	if(!S){
		FPRINTF(ASCERR,"Solver CMSlv2 was not registered\n");
		return 1;
	}
	num_cond_reg = S->number;

	/*
		Create solver tokens
	*/

		MSG("SETTING UP SUB-SOLVERS");

		MSG("SETTING UP CMSLV");
		solver_index[CONDITIONAL_SOLVER] = num_cond_reg;

		if(sys != NULL && sys->qrslv_fallback) {
			token[LOGICAL_SOLVER] = NULL;
			token[OPTIMIZATION_SOLVER] = NULL;
			solver_index[LOGICAL_SOLVER] = -1;
			solver_index[OPTIMIZATION_SOLVER] = -1;

			MSG("SETTING UP QRSLV FALLBACK");
			newsolver = slv_switch_solver(server,num_nl_reg);
			if (newsolver == -1) {
				FPRINTF(ASCERR,"Solver qrslv was not registered\n");
				return 1;
			}
			token[NONLINEAR_SOLVER] = slv_get_client_token(server);
			solver_index[NONLINEAR_SOLVER] = slv_get_selected_solver(server);
			u.i = ITER_LIMIT;
			set_param_in_solver(server,NONLINEAR_SOLVER,int_parm,"iterationlimit",&u);
			u.i = TIME_LIMIT;
			set_param_in_solver(server,NONLINEAR_SOLVER,int_parm,"timelimit",&u);
			u.c = QRSLV_CONVOPT;
			set_param_in_solver(server,NONLINEAR_SOLVER,char_parm,"convopt",&u);
			slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
			slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
			return 0;
		}

		MSG("SETTING UP LRSLV");
	newsolver = slv_switch_solver(server,num_log_reg);
	if (newsolver == -1) {
		FPRINTF(ASCERR,"Solver lrslv was not registered\n");
		return 1;
	}
	token[LOGICAL_SOLVER] = slv_get_client_token(server);
	solver_index[LOGICAL_SOLVER] = slv_get_selected_solver(server);
	param = "external_blocks";
	u.b = 0;
	set_param_in_solver(server,LOGICAL_SOLVER,bool_parm,param,&u);

	MSG("SETTING UP QRSLV");
	newsolver = slv_switch_solver(server,num_nl_reg);
	if (newsolver == -1) {
		FPRINTF(ASCERR,"Solver qrslv was not registered\n");
		return 1;
	}
	token[NONLINEAR_SOLVER] = slv_get_client_token(server);
	solver_index[NONLINEAR_SOLVER] = slv_get_selected_solver(server);

	MSG("SETTING UP OPTIMIZER %s (%d)",OPTSOLVER_OPTION,num_opt_reg);
	newsolver = slv_switch_solver(server,num_opt_reg);
	if (newsolver == -1) {
		FPRINTF(ASCERR,"Solver %s was not registered\n",OPTSOLVER_OPTION);
		return 1;
	}
	token[OPTIMIZATION_SOLVER] = slv_get_client_token(server);
	solver_index[OPTIMIZATION_SOLVER] = slv_get_selected_solver(server);

	/*
		Disabling the partition mode flag in the nonlinear solver.
		PARTITION is a boolean parameter of the nonlinear solver
		QRSlv. This parameter tells the solver whether it should block
		partition or not.
		CMSlv2 also keeps this disabled: when it can hand off real-only
		blocks, it installs the block list itself and enables QRSlv's
		external_blocks option immediately before QRSlv presolve.
	*/
	MSG("setting QRSlv.partition");
	param = "partition";
	u.b = 0;
	set_param_in_solver(server,NONLINEAR_SOLVER,bool_parm,param,&u);
	MSG("setting QRSlv.external_blocks");
	param = "external_blocks";
	u.b = 0;
	set_param_in_solver(server,NONLINEAR_SOLVER,bool_parm,param,&u);


	/*
		Setting the value of the number of iterations in the optimizer.
		For us, the optimizer is a blackbox. The only way of asking the
		values of the variables at each iteration is to stop the optimizer
		at each iteration by assigning the number of iterations equal to 1.
		We have seen though  that, because CONOPT work in four different
		phases, we need to assign a number of iterations a little bit bigger,
		so that CONOPT is able to determine optimality if we are
		already at the solution.
	*/
	if(strcmp(OPTSOLVER_OPTION,"CONOPT") == 0) {
		MSG("setting CONOPT.iterationlimit");
		param = "iterationlimit";
		u.i = 20;
		set_param_in_solver(server,OPTIMIZATION_SOLVER,int_parm,param,&u);
	}

	/*
		Maximum number of subsequent iterations in nonlinear solver.
		We will give a big value to this parameter, since we really do
		not care about the limits in the number of iterations in the
		nonlinear solver; what we care about here is in the number
		of iterations controlled by CMSlv.
	*/
	MSG("setting QRSlv.iterationlimit");
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
	  slv_decomp_init(&(sys->cmslv2_structural));
	  sys->cmslv2_structural_valid = 0;
	  if(slv_has_classifier_whens(server)
	      && slv_prepare_classifier_whens(server,WHEN_REGION_STEADY)) {
	    ascfree(sys);
	    ERROR_REPORTER_HERE(ASC_USER_ERROR,
	      "CMSlv2 could not prepare CASE IF/APPLIES IF classifier regions");
	    *statusindex = -2;
	    return NULL;
	  }
		  sys->need_consistency_analysis = slv_need_consistency(server);
		  sys->qrslv_fallback =
	    !sys->need_consistency_analysis
	    && slv_get_num_solvers_logrels(server) == 0
	    && slv_get_num_solvers_bnds(server) == 0
	    && slv_get_num_solvers_dvars(server) == 0;
	  sys->nliter = 0;
  sys->p.output.more_important = stdout;
  sys->p.output.less_important = stdout;
  sys->p.whose = (*statusindex);
  sys->s.kind = SLV_STATUS_NLP;
  sys->s.ok = TRUE;
  sys->s.calc_ok = TRUE;
  sys->s.u.nlp.costsize = 0;
  sys->s.u.nlp.cost = NULL; /*redundant, but sanity preserving */
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
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv2 called with no variables.");
    *statusindex = -2;
    return NULL;
  }
  if(sys->rlist == NULL && sys->obj == NULL) {
    ascfree(sys);
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv2 called with no relations or objective.\n");
    *statusindex = -1;
    return NULL;
  }
	  if(!sys->qrslv_fallback && sys->dvlist == NULL) {
	    ascfree(sys);
	    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv2 called with no discrete variables.\n");
	    *statusindex = -2;
	    return NULL;
	  }
	  if(!sys->qrslv_fallback && sys->lrlist == NULL) {
	    ascfree(sys);
	    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv2 called with no logrelations.\n");
	    *statusindex = -1;
	    return NULL;
	  }
	  if(!sys->qrslv_fallback && sys->blist == NULL) {
	    ascfree(sys);
	    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"CMSlv2 called with no boundaries.\n");
	    *statusindex = -2;
	    return NULL;
	  }
  slv_check_var_initialization(server);
  slv_check_dvar_initialization(server);
  slv_bnd_initialization(server);

  *statusindex = 0;
  token[CONDITIONAL_SOLVER] = (SlvClientToken)sys;
  return((SlvClientToken)sys);
}



static
int slv9_eligible_solver(slv_system_t server){
  const char *msg;
  int32 nlogrels, nbnds, ndvars;
  if(!slv_get_num_solvers_rels(server)){
		msg = "No relations were found";
  }else{
    nlogrels = slv_get_num_solvers_logrels(server);
    nbnds = slv_get_num_solvers_bnds(server);
    ndvars = slv_get_num_solvers_dvars(server);
    if(nlogrels == 0 && nbnds == 0 && ndvars == 0) {
      return TRUE;
    }
    if(!nlogrels) {
		msg = "Model must contain at least one logical relation";
    }else if(!nbnds) {
		msg = "Model must contain at least one boundary";
    }else if(!ndvars) {
		msg = "Model must contain at least one discrete variable";
    }else{
	    return TRUE;
    }
  }

  ERROR_REPORTER_HERE(ASC_USER_ERROR
	,"CMSlv2 not elegible for this model: %s",msg
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
void slv9_copy_subsolver_status(slv9_system_t sys, const slv_status_t *status){
  if(sys != NULL && status != NULL) {
    struct slv_block_cost *old_cost = slv_status_cost_rw(&(sys->s));
    const struct slv_block_cost *status_cost = slv_status_cost(status);
    int32 status_costsize = slv_status_costsize(status);
    struct slv_block_cost *new_cost = NULL;

    if(status_cost != NULL && status_costsize > 0) {
      new_cost = create_array(status_costsize,struct slv_block_cost);
      mem_copy_cast(status_cost,new_cost,
        status_costsize*sizeof(struct slv_block_cost));
    }
    if(old_cost != NULL && old_cost != status_cost) {
      destroy_array(old_cost);
    }
    mem_copy_cast(status,&(sys->s),sizeof(slv_status_t));
    if(slv_status_nlp_rw(&(sys->s)) != NULL) {
      sys->s.u.nlp.cost = new_cost;
      sys->s.u.nlp.costsize = status_costsize;
    }else if(new_cost != NULL) {
      destroy_array(new_cost);
    }
  }
}

static
int slv9_qrslv_fallback_presolve(slv_system_t server, slv9_system_t sys){
  slv_status_t status;
  int res;

  if(server == NULL || sys == NULL || token[NONLINEAR_SOLVER] == NULL) {
    return 1;
  }
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  res = slv_presolve(server);
  slv_get_status(server,&status);
  slv9_copy_subsolver_status(sys,&status);
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  slv9_report_progress(sys,
    "event=cmslv_qrslv_fallback action=presolve status=%d ready=%d converged=%d ok=%d blocks=%d",
    res, status.ready_to_solve, status.converged, status.ok,
    status.block.number_of
  );
  return res;
}

static
int slv9_qrslv_fallback_resolve(slv_system_t server, slv9_system_t sys){
  slv_status_t status;
  int res;

  if(server == NULL || sys == NULL || token[NONLINEAR_SOLVER] == NULL) {
    return 1;
  }
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  res = slv_resolve(server);
  slv_get_status(server,&status);
  slv9_copy_subsolver_status(sys,&status);
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  slv9_report_progress(sys,
    "event=cmslv_qrslv_fallback action=resolve status=%d ready=%d converged=%d ok=%d blocks=%d",
    res, status.ready_to_solve, status.converged, status.ok,
    status.block.number_of
  );
  return res;
}

static
int slv9_qrslv_fallback_iterate(slv_system_t server, slv9_system_t sys){
  slv_status_t status;
  int res;

  if(server == NULL || sys == NULL || token[NONLINEAR_SOLVER] == NULL) {
    return 1;
  }
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  res = slv_iterate(server);
  slv_get_status(server,&status);
  slv9_copy_subsolver_status(sys,&status);
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  slv9_report_progress(sys,
    "event=cmslv_qrslv_fallback action=iterate status=%d ready=%d converged=%d ok=%d current_block=%d",
    res, status.ready_to_solve, status.converged, status.ok,
    status.block.current_block
  );
  return res;
}

static
int slv9_qrslv_fallback_solve(slv_system_t server, slv9_system_t sys){
  slv_status_t status;
  int res;

  if(server == NULL || sys == NULL || token[NONLINEAR_SOLVER] == NULL) {
    return 1;
  }
  slv_set_client_token(server,token[NONLINEAR_SOLVER]);
  slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
  res = slv_solve(server);
  slv_get_status(server,&status);
  slv9_copy_subsolver_status(sys,&status);
  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  slv9_report_progress(sys,
    "event=cmslv_qrslv_fallback action=solve status=%d ready=%d converged=%d ok=%d blocks=%d",
    res, status.ready_to_solve, status.converged, status.ok,
    status.block.number_of
  );
  return res;
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

  if(cost == NULL || costsize <= 0) {
    return;
  }
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

static
int32 slv9_ensure_cost_capacity(slv_status_t *status, int32 mincostsize){
  slv_status_nlp_t *nlp = slv_status_nlp_rw(status);
  struct slv_block_cost *new_cost;
  int32 old_costsize;

  if(nlp == NULL) {
    return 0;
  }
  if(mincostsize <= 0) {
    mincostsize = 1;
  }
  if(nlp->cost != NULL && nlp->costsize >= mincostsize) {
    return 1;
  }
  old_costsize = nlp->costsize;
  new_cost = create_zero_array(mincostsize,struct slv_block_cost);
  if(new_cost == NULL) {
    return 0;
  }
  if(nlp->cost != NULL && old_costsize > 0) {
    mem_copy_cast(nlp->cost,new_cost,
      MIN(old_costsize,mincostsize)*sizeof(struct slv_block_cost)
    );
    destroy_array(nlp->cost);
  }
  nlp->cost = new_cost;
  nlp->costsize = mincostsize;
  return 1;
}

static
void slv9_reset_cost_array(slv_status_t *status, int32 costsize){
  slv_status_nlp_t *nlp = slv_status_nlp_rw(status);
  if(nlp == NULL) {
    return;
  }
  if(nlp->cost != NULL) {
    destroy_array(nlp->cost);
    nlp->cost = NULL;
  }
  nlp->costsize = costsize;
  nlp->cost = create_zero_array(nlp->costsize,struct slv_block_cost);
  reset_cost(nlp->cost,nlp->costsize);
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
		int32 current_block, int32 previous_block, int32 costsize
){
  const struct slv_block_cost *status_cost = slv_status_cost(status);
  int32 status_costsize = slv_status_costsize(status);
  if(cost == NULL || costsize <= 0) {
    return;
  }
  if(current_block >= 0 && current_block < costsize) {
    cost[current_block].size = status->block.current_size;
    cost[current_block].iterations	= status->block.iteration;
    cost[current_block].funcs = status->block.funcs;
    cost[current_block].jacs = status->block.jacs;
    cost[current_block].functime = status->block.functime;
    cost[current_block].jactime = status->block.jactime;
    cost[current_block].time = status->block.cpu_elapsed;
    cost[current_block].resid = status->block.residual;
    if(previous_block != -1 && previous_block != current_block
        && previous_block >= 0 && previous_block < costsize
        && status_cost != NULL && previous_block < status_costsize) {
      cost[previous_block].size	= status_cost[previous_block].size;
      cost[previous_block].iterations=status_cost[previous_block].iterations;
      cost[previous_block].funcs = status_cost[previous_block].funcs;
      cost[previous_block].jacs	= status_cost[previous_block].jacs;
      cost[previous_block].functime = status_cost[previous_block].functime;
      cost[previous_block].jactime = status_cost[previous_block].jactime;
      cost[previous_block].time	= status_cost[previous_block].time;
      cost[previous_block].resid = status_cost[previous_block].resid;
    }
  }
}

static
int32 is_an_optimization_problem(slv_system_t server,
		SlvClientToken asys
){
  slv9_system_t sys;
  slv_status_t status;
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

  MSG("...");

  sys = SLV9(asys);
  iteration_begins(sys);
  check_system(sys);
	if(slv_has_classifier_whens(server)) {
	  if(slv_prepare_classifier_whens(server,WHEN_REGION_STEADY)) {
	    ERROR_REPORTER_HERE(ASC_USER_ERROR,
	      "CMSlv2 could not prepare CASE IF/APPLIES IF classifier regions");
	    return 1;
	  }
	  sys->lrlist = slv_get_solvers_logrel_list(server);
	  sys->blist = slv_get_solvers_bnd_list(server);
	  slv_bnd_initialization(server);
	}
  if(sys->vlist == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Variable list was never set.");
    return 1;
  }
  if(sys->rlist == NULL && sys->obj == NULL ) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Relation list and objective never set.");
    return 2;
  }
	  if(!sys->solvers_ready && get_solvers_tokens(sys,server)) {
	    ERROR_REPORTER_HERE(ASC_USER_ERROR,
	      "Solver(s) required by CMSlv were not available for selected options."
	    );
	    return 3;
	  }
	  sys->solvers_ready = 1;
	  if(sys->qrslv_fallback) {
	    return slv9_qrslv_fallback_presolve(server,sys);
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
  slv9_cmslv2_restore_qrslv_scope(sys);
  slv9_cmslv2_clear_structural_cache(sys);
  sys->cmslv2_next_structural_block = 0;
  sys->cmslv2_last_structural_blocks = 0;
  sys->cmslv2_pending_after_qrslv = 0;
  /* Reset status */
  sys->s.iteration = 0;
  sys->nliter = 0;
  sys->s.cpu_elapsed = 0.0;
  sys->s.converged = sys->s.diverged = sys->s.inconsistent = FALSE;
  sys->s.over_defined = sys->s.under_defined = sys->s.struct_singular = FALSE;
  sys->s.block.previous_total_size = 0;
  sys->s.block.current_block = -1;
  sys->s.block.current_size = 0;
  sys->s.calc_ok = TRUE;
  sys->s.block.iteration = 0;

  update_status(sys);
  slv9_report_progress(sys,
    "event=presolve, optimizing=%d, vars=%d, rels=%d",
    g_optimizing, sys->vtot, sys->rtot
  );
  slv9_report_decomp_assessment(sys,"presolve");
  iteration_ends(sys);

  return 0;
}

static
int slv9_resolve(slv_system_t server, SlvClientToken asys){
  struct var_variable **vp;
  struct rel_relation **rp;
  slv9_system_t sys;

	  sys = SLV9(asys);
		  check_system(sys);
		  if(sys->qrslv_fallback) {
		    return slv9_qrslv_fallback_resolve(server,sys);
		  }

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
  sys->s.over_defined = sys->s.under_defined = sys->s.struct_singular = FALSE;
  sys->s.block.previous_total_size = 0;
  slv9_cmslv2_restore_qrslv_scope(sys);
  slv9_cmslv2_clear_structural_cache(sys);
  sys->cmslv2_next_structural_block = 0;
  sys->cmslv2_last_structural_blocks = 0;
  sys->cmslv2_pending_after_qrslv = 0;

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
  int32 cmslv2_external_blocks;
  boolean unsuccessful;
  int32 system_was_reanalyzed;
  int32 classifier_active_changed;
#if TEST_CONSISTENCY
  int32 *test= NULL;
#endif /* TEST_CONSISTENCY */

	  sys = SLV9(asys);

	  if(server == NULL || sys==NULL) return 1;
	  if(check_system(SLV9(sys))) return 2;
	  if(sys->qrslv_fallback) {
	    return slv9_qrslv_fallback_iterate(server,sys);
	  }
	  if(!sys->s.ready_to_solve ) {
	    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not ready to solve.");
	    return 3;
  }

  unsuccessful = FALSE;
  cmslv2_external_blocks = 0;
  classifier_active_changed = 0;
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
    slv9_report_progress(sys,
      "event=boundary_at_zero, iter=%d, n_subregions=%d, cur_subregion=%d",
      sys->s.iteration, n_subregions, cur_subregion
    );
    if(CMSLV2_BLOCKSOLVE) {
      int32 cmslv2_boundary_status;
      cmslv2_boundary_status = slv9_apply_decomp_partition_policy_ex(
        sys,"boundary_at_zero",SLV9_CMSLV2_QR_SUPPRESS
      );
      if(cmslv2_boundary_status < 0) {
        int32 i;
        destroy_array(rvalues.pre_values);
        if(subregions != NULL && n_subregions > 0) {
          for(i = 0; i < n_subregions; ++i) {
            if(subregions[i].case_list != NULL) {
              destroy_array(subregions[i].case_list);
            }
          }
          destroy_array(subregions);
        }
        gl_destroy(disvars);
        disvars = NULL;
        update_boundaries(server,asys);
        update_relations_residuals(server);
        sys->s.converged = TRUE;
        sys->s.ready_to_solve = FALSE;
        sys->s.ok = TRUE;
        slv9_report_progress(sys,
          "event=cmslv2_scheduler phase=boundary_at_zero action=local_complete next_structural=%d",
          sys->cmslv2_next_structural_block
        );
        iteration_ends(sys);
        return 0;
      }
    }
    slv9_cmslv2_report_boundary_handoff(
      sys,"boundary_at_zero","optimize_start",
      n_subregions,cur_subregion,disvars
    );
    if(optimize_at_boundary(server,asys,&(n_subregions),
                            subregions,&(cur_subregion),disvars,&(rvalues))){
      slv9_cmslv2_report_boundary_handoff(
        sys,"boundary_at_zero","optimize_done",
        n_subregions,cur_subregion,disvars
      );
      store_real_cur_values(server,&(rvalues));
      update_boundaries(server,asys);
      slv9_cmslv2_clear_structural_cache(sys);
      if(some_boundaries_crossed(server,asys)) {
        vfilter.matchbits = (VAR_ACTIVE_AT_BND | VAR_INCIDENT
			     | VAR_SVAR | VAR_FIXED);
        vfilter.matchvalue = (VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR);
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Boundary(ies) crossed. Returning to boundary first crossed...\n");
        slv9_report_progress(sys,
          "event=boundary_return_start, iter=%d",
          sys->s.iteration
        );
        slv9_cmslv2_report_boundary_handoff(
          sys,"boundary_at_zero","return_start",
          n_subregions,cur_subregion,disvars
        );
        factor = return_to_first_boundary(server,asys,&rvalues,&vfilter);
        update_real_var_values(server,&rvalues,&vfilter,factor);
        update_boundaries(server,asys);
        update_relations_residuals(server);
        slv9_cmslv2_report_boundary_handoff(
          sys,"boundary_at_zero","return_done",
          n_subregions,cur_subregion,disvars
        );
      }else{
        destroy_array(rvalues.cur_values);
        destroy_array(rvalues.pre_values);
      }
      update_status(sys);
    }else{
      destroy_array(rvalues.pre_values);
      sys->s.converged  = TRUE;
      sys->s.ready_to_solve = FALSE;
      slv9_cmslv2_clear_structural_cache(sys);
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"No progress can be achieved: solution at current boundary.");
      slv9_report_progress(sys,
        "event=stop_at_boundary, iter=%d",
        sys->s.iteration
      );
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
      slv9_cmslv2_restore_qrslv_scope(sys);
      sys->cmslv2_next_structural_block = 0;
      sys->cmslv2_pending_after_qrslv = 0;
      slv9_reanalyze_solver_lists(sys);
      slv9_cmslv2_clear_structural_cache(sys);
      update_relations_residuals(server);
      system_was_reanalyzed = 1;
      slv9_report_progress(sys,
        "event=reconfigure, iter=%d",
        sys->s.iteration
      );
      slv9_report_decomp_assessment(sys,"reconfigure");
    }
    slv9_cmslv2_run_selector_envelopes(sys,"post_logic");
    if(slv9_cmslv2_consume_selector_cursor(sys,"post_logic")) {
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
        slv9_report_progress(sys,
          "event=optimizer_start, iter=%d, solver=%s",
          sys->s.iteration, OPTSOLVER_OPTION
        );
        slv_presolve(server);
        slv_get_status(server,&status);
        update_real_status(&(sys->s),&status,0);
        slv9_reset_cost_array(&(sys->s),1 + status.block.number_of);
      }else{
        slv_get_status(server,&status);
        update_struct_info(sys,&status);
        if(status.converged) {
          slv_presolve(server);
          update_real_status(&(sys->s),&status,0);
          slv9_reset_cost_array(&(sys->s),1 + status.block.number_of);
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
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Iterating with nonlinear solver...\n");
        slv9_report_progress(sys,
          "event=nl_start, iter=%d, solver=%s",
          sys->s.iteration, NONLISOLVER_OPTION
        );
        cmslv2_external_blocks =
          slv9_apply_decomp_partition_policy(sys,"nl_presolve");
        slv_presolve(server);
        slv_get_status(server,&status);
        slv9_report_progress(sys,
          "event=nl_presolved phase=nl_presolve partition=0 external_blocks=%d blocks=%d current_block=%d current_size=%d",
          cmslv2_external_blocks > 0 ? 1 : 0,
          status.block.number_of, status.block.current_block,
          status.block.current_size
        );
        update_struct_info(sys,&status);
        update_real_status(&(sys->s),&status,sys->nliter);
        slv9_reset_cost_array(&(sys->s),1 + status.block.number_of);
#if TEST_CONSISTENCY
        ID_and_storage_subregion_information(server,asys);
        MSG("New region, iteration = %d\n",sys->s.block.iteration);
#endif /* TEST_CONSISTENCY  */
      }
      slv_get_status(server,&status);
      update_struct_info(sys,&status);
      if(status.converged) {
        cmslv2_external_blocks =
          slv9_apply_decomp_partition_policy(sys,"nl_represolve");
        slv_presolve(server);
        update_real_status(&(sys->s),&status,0);
        slv9_reset_cost_array(&(sys->s),1 + status.block.number_of);
      }
    }
    /*
      Iteration steps common to optimizer and nonlinear solver
    */
    previous_block = sys->s.block.current_block;
    if(cmslv2_external_blocks > 0 && !g_optimizing) {
      slv9_report_progress(sys,
        "event=cmslv2_qrslv_solve iter=%d blocks=%d next_structural=%d pending_after=%d",
        sys->s.iteration, cmslv2_external_blocks,
        sys->cmslv2_next_structural_block,
        sys->cmslv2_pending_after_qrslv
      );
      slv_solve(server);
      slv9_cmslv2_restore_qrslv_scope(sys);
    }else{
      slv_iterate(server);
    }
    store_real_cur_values(server,&(rvalues));
    update_boundaries(server,asys);
    if(slv_has_classifier_whens(server)) {
      if(slv9_reanalyze_solver_lists_changed(
          sys,&classifier_active_changed
      )) {
        sys->s.calc_ok = FALSE;
        sys->s.ready_to_solve = FALSE;
        destroy_array(rvalues.cur_values);
        destroy_array(rvalues.pre_values);
        slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
        slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
        gl_destroy(disvars);
        disvars = NULL;
        iteration_ends(sys);
        return 5;
      }
      if(classifier_active_changed) {
        slv9_cmslv2_clear_structural_cache(sys);
        update_boundaries(server,asys);
        update_relations_residuals(server);
      }
    }
    slv_get_status(server,&status);
    sys->s.converged  = status.converged;
    /*
      The following statement was added 4/2
    */
    update_struct_info(sys,&status);
    update_real_status(&(sys->s),&status,0);
    if(classifier_active_changed) {
      sys->s.converged = FALSE;
      sys->s.ready_to_solve = TRUE;
    }
    if(cmslv2_external_blocks > 0 && sys->cmslv2_pending_after_qrslv) {
      sys->s.converged = FALSE;
      sys->s.ready_to_solve = TRUE;
    }
    slv9_ensure_cost_capacity(
      &(sys->s),
      1 + MAX(sys->s.block.current_block,previous_block)
    );
    update_cost(sys->s.u.nlp.cost,&status,
                sys->s.block.current_block,previous_block,
                sys->s.u.nlp.costsize);
    if(!sys->s.converged || some_boundaries_crossed(server,asys) ) {
      sys->s.converged = FALSE;
      sys->s.ready_to_solve = !sys->s.converged;
      if(some_boundaries_crossed(server,asys)) {
        vfilter.matchbits = (VAR_ACTIVE | VAR_INCIDENT
			     | VAR_SVAR | VAR_FIXED);
        vfilter.matchvalue = (VAR_ACTIVE | VAR_INCIDENT | VAR_SVAR);
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Boundary(ies) crossed. Returning to boundary first crossed...\n");
        slv9_report_progress(sys,
          "event=boundary_crossed, iter=%d",
          sys->s.iteration
        );
        slv9_report_progress(sys,
          "event=boundary_return_start, iter=%d",
          sys->s.iteration
        );
        slv9_cmslv2_report_boundary_handoff(
          sys,"boundary_crossed","return_start",
          -1,-1,NULL
        );
        factor = return_to_first_boundary(server,asys,&rvalues,&vfilter);
        update_real_var_values(server,&rvalues,&vfilter,factor);
        update_boundaries(server,asys);
        slv9_reanalyze_solver_lists(sys);
        update_relations_residuals(server);
        slv9_cmslv2_clear_structural_cache(sys);
        sys->cmslv2_next_structural_block = 0;
        sys->cmslv2_pending_after_qrslv = 0;
        slv9_cmslv2_report_boundary_handoff(
          sys,"boundary_crossed","return_done",
          -1,-1,NULL
        );
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
	  if(sys->qrslv_fallback) {
	    return slv9_qrslv_fallback_solve(server,sys);
	  }

	  while(sys->s.ready_to_solve)err = err | slv9_iterate(server,sys);

  return err;
}


static
mtx_matrix_t slv9_get_matrix(slv_system_t server, SlvClientToken sys){
  if(server == NULL || sys==NULL) return NULL;
  if(check_system(SLV9(sys))) return NULL;
  ERROR_REPORTER_HERE(ASC_PROG_ERR,"CMSlv2 solver does not support 'get_matrix'.");
  return NULL;
}

/*
 * Destroy the client tokens of the different solvers
 */
static
void destroy_solvers_tokens(slv_system_t server, slv9_system_t sys){
	  if(sys == NULL || !sys->solvers_ready) {
	    return;
	  }
	  if(token[LOGICAL_SOLVER] != NULL && solver_index[LOGICAL_SOLVER] >= 0) {
	    slv_set_client_token(server,token[LOGICAL_SOLVER]);
	    slv_set_solver_index(server,solver_index[LOGICAL_SOLVER]);
	    slv_destroy_client(server);
	  }
	  if(token[NONLINEAR_SOLVER] != NULL && solver_index[NONLINEAR_SOLVER] >= 0) {
	    slv_set_client_token(server,token[NONLINEAR_SOLVER]);
	    slv_set_solver_index(server,solver_index[NONLINEAR_SOLVER]);
	    slv_destroy_client(server);
	  }
	  if(token[OPTIMIZATION_SOLVER] != NULL
	      && solver_index[OPTIMIZATION_SOLVER] >= 0) {
	    slv_set_client_token(server,token[OPTIMIZATION_SOLVER]);
	    slv_set_solver_index(server,solver_index[OPTIMIZATION_SOLVER]);
	    slv_destroy_client(server);
	  }
	  slv_set_client_token(server,token[CONDITIONAL_SOLVER]);
	  slv_set_solver_index(server,solver_index[CONDITIONAL_SOLVER]);
  sys->solvers_ready = 0;
}

static
int slv9_destroy(slv_system_t server, SlvClientToken asys){
  slv9_system_t sys;
  sys = SLV9(asys);
  if(check_system(sys)) return 1;
  slv9_cmslv2_restore_qrslv_scope(sys);
  slv9_cmslv2_clear_structural_cache(sys);
  destroy_subregion_information(asys);
  destroy_solvers_tokens(server,sys);
  slv_destroy_parms(&(sys->p));
  sys->integrity = DESTROYED;
  if(sys->s.u.nlp.cost) ascfree(sys->s.u.nlp.cost);
#ifdef ASC_WITH_CONOPT
  if(sys->con.cntvect != NULL) {
# ifdef ASC_CONOPT_API4
    COI_Free(&(sys->con.cntvect));
# else
    ASC_FREE(sys->con.cntvect);
    sys->con.cntvect = NULL;
# endif
  }
#endif
  ascfree( (POINTER)asys );
  return 0;
}


static const SlvFunctionsT slv9_internals = {
	SOLVER_CMSLV2
	,"CMSlv2"
	,slv9_create
  	,slv9_destroy
	,slv9_eligible_solver
	,slv9_get_default_parameters
	,slv9_get_parameters
	,slv9_set_parameters
	,slv9_get_status
	,slv9_solve
	,slv9_presolve
	,slv9_iterate
	,slv9_resolve
	,NULL
	,slv9_get_matrix
	,slv9_dump_internals
};

int cmslv2_register(void){
	MSG("Registering CMSlv2");
	if(!solver_engine_named("LRSlv")){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"LRSlv must be registered before CMSlv2");
		return 1;
	}
	if(!solver_engine_named("QRSlv")){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"QRSlv must be registered before CMSlv2");
		return 1;
	}
	return solver_register(&slv9_internals);
}

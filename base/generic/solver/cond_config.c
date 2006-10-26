/*
 *  Conditional Modeling Configuration
 *  by Vicente Rico-Ramirez
 *  Created: 04/97
 *  Version: $Revision: 1.11 $
 *  Version control file: $RCSfile: cond_config.c,v $
 *  Date last modified: $Date: 2000/01/25 02:26:51 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 *
 */
#include <stdarg.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/ascMalloc.h>
#include <compiler/compiler.h>
#include <compiler/instance_enum.h>
#include <compiler/check.h>
#include <general/list.h>
#include <general/dstring.h>
#include <general/tm_time.h>
#include "mtx.h"
#include "slv_types.h"
#include "var.h"
#include "discrete.h"
#include "conditional.h"
#include "linsolqr.h"
#define _SLV_SERVER_C_SEEN_
#include <compiler/extcall.h>
#include "rel.h"
#include "logrel.h"
#include "bnd.h"
#include "slv_server.h"
#include "system.h"
#include "analyze.h"
#include "cond_config.h"
#include "linsol.h"
#include "slv_common.h"
#include "slv_client.h"

#define IPTR(i) ((struct Instance *) (i))
#define USEDCODE 0
#define DEBUG FALSE
#define CASE_NUMBER FALSE
#define DEBUG_PRE_ANALYSIS FALSE

/*
 *              Configuration of Conditional Models
 */


/*
 * structure dynamically allocated/reallocated to store the number of
 * matched cases in a list of when statements
 */

struct ds_case_list {
   int32 length,capacity;
   int32 *case_number;
};

/*
 * forward declarations
 */
void analyze_when(struct w_when *);
static void simplified_analyze_when(struct w_when *);
static void cases_matching_in_when_list(struct gl_list_t *,
					struct ds_case_list *,
					int32 *);
/*
 * global variable for finding the number of cases in a when and
 * enumerating the cases
 */
static int32 g_case_number = 0;

/**
	Set the ACTIVE bit to value for all the relations
	included in the case
*/
static void set_rels_status_in_case(struct when_case *cur_case
		, uint32 value
){
  struct gl_list_t *rels;
  struct rel_relation *rel;
  int32 r,rlen;

  /* CONSOLE_DEBUG("..."); */

  rels = when_case_rels_list(cur_case);
  rlen = gl_length(rels);
  for(r=1;r<=rlen;r++) {
    rel = (struct rel_relation *)(gl_fetch(rels,r));
	if(!value){
		/* CONSOLE_DEBUG("INACTIVE REL AT %p",rel); */
	}
    rel_set_active(rel,value);
  }
}


/*
 * Set the ACTIVE bit to value for all the logrelations
 * included in the case
 */
static void set_logrels_status_in_case(struct when_case *cur_case,
				       uint32 value)
{
  struct gl_list_t *logrels;
  struct logrel_relation *lrel;
  int32 lr,lrlen;

  logrels = when_case_logrels_list(cur_case);
  if (logrels==NULL) return;
  lrlen = gl_length(logrels);
  for(lr=1;lr<=lrlen;lr++) {
    lrel = (struct logrel_relation *)(gl_fetch(logrels,lr));
    logrel_set_active(lrel,value);
  }
}

/*
 * Set the ACTIVE bit to value for all the relations and logrelations
 * included in a when (implicitly or explcitly).
 */
void set_rels_status_in_when(struct w_when *when, uint32 value)
{
  struct gl_list_t *cases;
  struct gl_list_t *whens;
  struct w_when *nested_when;
  struct when_case *cur_case;
  int32 c,clen,w,wlen;

  cases = when_cases_list(when);
  clen = gl_length(cases);
  for (c=1;c<=clen;c++){
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    when_case_set_active(cur_case,value);
    set_rels_status_in_case(cur_case,value);
    set_logrels_status_in_case(cur_case,value);
    whens = when_case_whens_list(cur_case);
    wlen = gl_length(whens);  /* nested whens */
    for (w=1;w<=wlen;w++){
      nested_when = (struct w_when *)(gl_fetch(whens,w));
      set_rels_status_in_when(nested_when,value); /* recursion */
    }
  }
}


/*
 * After a case is found to apply for the current values of the
 * conditional variables, the rel_relations and logrel_relations
 * in such a case are set ACTIVE.
 * If a WHEN is found in a CASE, the analysis of the WHEN is done
 * recursively.
 */

static void apply_case(struct when_case *cur_case)
{
  struct gl_list_t *rels;
  struct gl_list_t *logrels;
  struct gl_list_t *whens;
  struct rel_relation *rel;
  struct logrel_relation *lrel;
  struct w_when *when;
  int32 r,rlen,w,wlen,lr,lrlen;

  rels = when_case_rels_list(cur_case);
  if (rels != NULL) {
    rlen = gl_length(rels);
    for(r=1;r<=rlen;r++) {
      rel = (struct rel_relation *)(gl_fetch(rels,r));
      rel_set_active(rel,TRUE);
    }
  }
  logrels = when_case_logrels_list(cur_case);
  if (logrels != NULL) {
    lrlen = gl_length(logrels);
    for(lr=1;lr<=lrlen;lr++) {
      lrel = (struct logrel_relation *)(gl_fetch(logrels,lr));
      logrel_set_active(lrel,TRUE);
    }
  }
  whens = when_case_whens_list(cur_case);
  if (whens != NULL) {
    wlen = gl_length(whens);
    for(w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      analyze_when(when);
    }
  }
}

/*
 * Compare current values of the conditional variables with
 * the set of values in a CASE, and try to find is such
 * values are the same. If they are, then the previous function
 * is called.
 */


static int32 analyze_case(struct when_case *cur_case,
			  struct gl_list_t *dvars)
{

  struct dis_discrete *dvar;
  int32 d,dlen;
  int32 values[MAX_VAR_IN_LIST];
  int32 *value;
  int32 *case_values,dindex;

  value = &(values[0]);
  case_values = when_case_values_list(cur_case);
  for(dindex =0; dindex<MAX_VAR_IN_LIST; dindex++) {
    *value = *case_values;
    value++;
    case_values++;
  }
  dlen = gl_length(dvars);
  for (d=1;d<=dlen;d++) {
    dvar = (struct dis_discrete *)(gl_fetch(dvars,d));
    if ( (values[d-1]!= -2) && (values[d-1]!= dis_value(dvar)) ) {
      return 0;
    }
  }
  apply_case(cur_case);
  when_case_set_active(cur_case,TRUE);  /* Case active */
  return 1;
}


/*
 * This function will determine which case of a WHEN statement
 * applies for the current values of the conditional variables.
 * The relations in that case are set ACTIVE
 */

void analyze_when(struct w_when *when)
{
  struct gl_list_t *dvars;
  struct gl_list_t *cases;
  int32 values[MAX_VAR_IN_LIST];
  struct when_case *cur_case;
  int32 c,clen;
  int32 case_match;
  int32 *value;
  int32 *case_values;

  dvars = when_dvars_list(when);
  cases = when_cases_list(when);
  clen = gl_length(cases);
  case_match =0;
  for (c=1;c<=clen;c++){
    if(case_match==1){
      break;
    }
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    value = &(values[0]);
    case_values = when_case_values_list(cur_case);
    *value = *case_values;
    if (values[0]!=-1) {
      case_match = analyze_case(cur_case,dvars);
    } else {
      if (case_match==0) {
	apply_case(cur_case);
        when_case_set_active(cur_case,TRUE);  /* Otherwise case active */
        case_match = 1;
      }
    }
  }
  if (case_match == 0) {
    FPRINTF(ASCERR,"No case matched in when\n");
  }
}


/*
 * Simplified functions.
 * The following functions also perform the analysis of a WHEN structure,
 * but it only set ACTIVE/INACTIVE those CASEs included in the WHENs, they
 * do not mess up with relations or logrels. It's a shorthand of the analysis
 * while finding the different subregions that have to be analyzed when the
 * iteration solution scheme of a conditional model hits a boundary. We
 * can use the previous functions, but we would be doing extra work.
 *
 */


/*
 * Set as Inactive all of cases found in a w_when.
 */
static void disactive_cases_in_when(struct w_when *when)
{
  struct gl_list_t *cases;
  struct gl_list_t *case_whens;
  struct when_case *cur_case;
  struct w_when *cur_when;
  int32 c,clen,w,wlen;

  cases = when_cases_list(when);
  if (cases == NULL) return;
  clen = gl_length(cases);
  for (c=1;c<=clen;c++){
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    when_case_set_active(cur_case,FALSE); /* Case not active */
    case_whens = when_case_whens_list(cur_case);
    if (case_whens != NULL) {
      wlen = gl_length(case_whens);
      for (w=1;w<=wlen;w++) {
        cur_when = (struct w_when *)(gl_fetch(case_whens,w));
	disactive_cases_in_when(cur_when);
      }
    }
  }
}

/* After a case is found to apply for the current values of the
 * conditional variables, it checks for nested WHENs inside the CASE.
 * If a WHEN is found in a CASE, the analysis of the WHEN is done
 * recursively.
 */

static void simplified_apply_case(struct when_case *cur_case)
{

  struct gl_list_t *whens;
  struct w_when *when;
  int32 w,wlen;

  whens = when_case_whens_list(cur_case);
  if (whens != NULL) {
    wlen = gl_length(whens);
    for(w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      simplified_analyze_when(when);
    }
  }
}


/*
 * Compare current values of the conditional variables with
 * the set of values in a CASE, and try to find is such
 * values are the same. If they are, then the previous function
 * is called.
 */
static int32 simplified_analyze_case(struct when_case *cur_case,
				     struct gl_list_t *dvars)
{

  struct dis_discrete *dvar;
  int32 d,dlen;
  int32 values[MAX_VAR_IN_LIST];
  int32 *value;
  int32 *case_values,dindex;

  value = &(values[0]);
  case_values = when_case_values_list(cur_case);
  for(dindex =0; dindex<MAX_VAR_IN_LIST; dindex++) {
    *value = *case_values;
    value++;
    case_values++;
  }
  dlen = gl_length(dvars);
  for (d=1;d<=dlen;d++) {
    dvar = (struct dis_discrete *)(gl_fetch(dvars,d));
    if ( (values[d-1]!= -2) && (values[d-1]!= dis_value(dvar)) ) {
      return 0;
    }
  }
  simplified_apply_case(cur_case);
  when_case_set_active(cur_case,TRUE);  /* Case active */
  return 1;
}


/*
 * This function will determine which case of a WHEN statement
 * applies for the current values of the conditional variables.
 * That case will be set ACTIVE
 */

static void simplified_analyze_when(struct w_when *when)
{
  struct gl_list_t *dvars;
  struct gl_list_t *cases;
  int32 values[MAX_VAR_IN_LIST];
  struct when_case *cur_case;
  int32 c,clen;
  int32 case_match;
  int32 *value;
  int32 *case_values;

  dvars = when_dvars_list(when);
  cases = when_cases_list(when);
  clen = gl_length(cases);
  case_match =0;
  for (c=1;c<=clen;c++){
    if(case_match==1){
      break;
    }
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    value = &(values[0]);
    case_values = when_case_values_list(cur_case);
    *value = *case_values;
    if (values[0]!=-1) {
      case_match = simplified_analyze_case(cur_case,dvars);
    } else {
      if (case_match==0) {
	simplified_apply_case(cur_case);
        when_case_set_active(cur_case,TRUE);  /* Otherwise case active */
        case_match = 1;
      }
    }
  }
  if (case_match == 0) {
    FPRINTF(ASCERR,"No case matched in when\n");
  }
}



/*
 * Set the ACTIVE bit to TRUE for all the rels included in the
 * rel list.
 */
void set_active_rels_in_list(struct rel_relation **rlist)
{
  int32 c;
  struct rel_relation *rel;

  for (c = 0; rlist[c]!= NULL; c++) {
    rel = rlist[c];
    rel_set_active(rel,TRUE);
  }
}

/*
 * Sets the INVARIANT bit to TRUE for all the rels in the rel list which
 * are ACTIVE and INCLUDED AND EUQALITY. Sets it to FALSE otherwise
 */
void set_active_rels_as_invariant(struct rel_relation **rlist)
{
  int32 c;
  struct rel_relation *rel;

  for (c = 0; rlist[c]!= NULL; c++) {
    rel = rlist[c];
    if (rel_active(rel) && rel_included(rel) && rel_equality(rel)) {
      rel_set_invariant(rel,TRUE);
    } else {
      rel_set_invariant(rel,FALSE);
    }
  }
}


/*
 * Sets the ACTIVE bit to TRUE for all the logrels included in the
 * logrel list.
 */
void set_active_logrels_in_list(struct logrel_relation **lrlist)
{
  int32 c;
  struct logrel_relation *lrel;

  for (c = 0; lrlist[c]!= NULL; c++) {
    lrel = lrlist[c];
    logrel_set_active(lrel,TRUE);
  }
}


/*
 * Sets the ACTIVE bit to FALSE for all the vars included in the
 * var list
 */
void set_inactive_vars_in_list(struct var_variable **vlist)
{
  int32 c;
  struct var_variable *var;

  for (c = 0; vlist[c]!= NULL; c++) {
    var = vlist[c];
    var_set_active(var,FALSE);
  }
}

/*
 * Sets the ACTIVE_AT_BND bit to FALSE for all the vars included in the
 * var list
 */
static void set_inactive_vars_at_bnd_in_list(struct var_variable **vlist)
{
  int32 c;
  struct var_variable *var;

  for (c = 0; vlist[c]!= NULL; c++) {
    var = vlist[c];
    var_set_active_at_bnd(var,FALSE);
  }
}


/*
 * Set the INVARIAT bit to value for all the rels included in the
 * rel list.
 */
void set_invariant_in_rels_list(struct rel_relation **rlist,uint32 value)
{
  int32 c;
  struct rel_relation *rel;

  for (c = 0; rlist[c]!= NULL; c++) {
    rel = rlist[c];
    rel_set_invariant(rel,value);
  }
}

/*
 * Set the IN_CUR_SUBREGION bit to value for all the rels included in the
 * rel list.
 */
void set_in_cur_subregion_in_rels_list(struct rel_relation **rlist,
				       uint32 value)
{
  int32 c;
  struct rel_relation *rel;

  for (c = 0; rlist[c]!= NULL; c++) {
    rel = rlist[c];
    rel_set_in_cur_subregion(rel,value);
  }
}


/*
 * Set the ACTIVE bit to FALSE for all the discrete vars included in the
 * discrete var list
 */
void set_inactive_disvars_in_list(struct dis_discrete **dvlist)
{
  int32 c;
  struct dis_discrete *dvar;

  for (c = 0; dvlist[c]!= NULL; c++) {
    dvar = dvlist[c];
    dis_set_active(dvar,FALSE);
  }
}


/*
 * Set the ACTIVE bit to TRUE for all the variables included in
 * ACTIVE relations.
 */
void set_active_vars_in_active_rels(struct rel_relation **solverrl)
{
  struct var_variable *var;
  struct rel_relation *rel;
  struct var_variable **incidence;
  int32 v,c,vlen;

  for (v = 0; solverrl[v]!=NULL; v++) {
    rel = solverrl[v];
    if (rel_active(rel)) {
      vlen = rel_n_incidences(rel);
      incidence = rel_incidence_list_to_modify(rel);
      for(c=0; c<vlen;c++) {
        var = incidence[c];
        var_set_active(var,TRUE);
      }
    }
  }
}

/*
 * Set the ACTIVE_AT_BND bit to TRUE for all the variables included in
 * ACTIVE relations.
 */
static
void set_active_vars_at_bnd_in_active_rels(struct rel_relation **solverrl)
{
  struct var_variable *var;
  struct rel_relation *rel;
  struct var_variable **incidence;
  int32 v,c,vlen;

  for (v = 0; solverrl[v]!=NULL; v++) {
    rel = solverrl[v];
    if (rel_active(rel)) {
      vlen = rel_n_incidences(rel);
      incidence = rel_incidence_list_to_modify(rel);
      for(c=0; c<vlen;c++) {
        var = incidence[c];
        var_set_active_at_bnd(var,TRUE);
      }
    }
  }
}


/*
 * Set the ACTIVE bit to TRUE for all the discrete variables included in
 * ACTIVE logrelations.
 */
void set_active_disvars_in_active_logrels(struct logrel_relation **solverll)
{
  struct dis_discrete *dvar;
  struct logrel_relation *lrel;
  struct dis_discrete **incidence;
  int32 v,c,vlen;

  for (v = 0; solverll[v]!=NULL; v++) {
    lrel = solverll[v];
    if (logrel_active(lrel)) {
      vlen = logrel_n_incidences(lrel);
      incidence = logrel_incidence_list_to_modify(lrel);
      for(c=0; c<vlen;c++) {
        dvar = incidence[c];
        dis_set_active(dvar,TRUE);
      }
    }
  }
}


/*
 * Set the ACTIVE_AT_BDN bit to TRUE for all the variables incident in all
 * the relations of all the subregions neighboring a boundary(ies)
 */
void set_active_vars_at_bnd(slv_system_t sys, struct gl_list_t *disvars)
{
  struct rel_relation **solverrl;
  struct var_variable **solvervl;
  struct dis_discrete *dis;
  struct gl_list_t *whens;
  struct w_when *when;
  int32 d,dlen,w,wlen;

  solverrl = slv_get_solvers_rel_list(sys);
  solvervl = slv_get_solvers_var_list(sys);

  if (disvars == NULL) {
    return;
  }
  dlen = gl_length(disvars);

  /*
   * set active rels in the whens including the disvars,
   * then set active vars in active rels
   */
  for (d=1;d<=dlen;d++) {
    dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    whens = dis_whens_list(dis);
    if (whens == NULL) {
      continue;
    }
    wlen = gl_length(whens);
    for (w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      set_rels_status_in_when(when,TRUE);
    }
  }
  set_inactive_vars_at_bnd_in_list(solvervl);
  set_active_vars_at_bnd_in_active_rels(solverrl);
}



/*
 * Set the INVARIANT flag to TRUE for all the relations invariant with
 * respect to the current boundary(ies)
 */
void identify_invariant_rels_at_bnd(slv_system_t sys,
				    struct gl_list_t *disvars)
{
  struct rel_relation **solverrl;
  struct dis_discrete *dis;
  struct gl_list_t *whens;
  struct w_when *when;
  int32 d,dlen,w,wlen;

  solverrl = slv_get_solvers_rel_list(sys);

  if (disvars == NULL) {
    return;
  }
  dlen = gl_length(disvars);

  for (d=1;d<=dlen;d++) {
    dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    whens = dis_whens_list(dis);
    if (whens == NULL) {
      continue;
    }
    wlen = gl_length(whens);
    for (w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      set_rels_status_in_when(when,FALSE);
    }
  }
  set_invariant_in_rels_list(solverrl,FALSE);
  set_active_rels_as_invariant(solverrl);
}

/*
 * find if the current case is included in the array of cases
 */
static int32 case_in_array_of_cases(int32 num_case, int32 *cases, int32 ncases)
{
  int32 n;

  for (n=0; n<ncases; n++) {
    if (num_case == cases[n]) {
      return 1;
    }
  }
  return 0;
}

/*
 * find if a some case of the array of cases belongs to some of the
 * nested whens of a when statement
 */
static int32 case_in_nested_whens(int32 *cases, int32 ncases,
				  struct when_case *cur_case)
{
  struct gl_list_t *case_list;
  struct gl_list_t *whens;
  struct w_when *nested_when;
  struct when_case *nested_case;
  int32  c,clen,w,wlen;
  int32 num_case;


  whens = when_case_whens_list(cur_case);
  wlen = gl_length(whens);  /* nested whens */
  for (w=1;w<=wlen;w++){
    nested_when = (struct w_when *)(gl_fetch(whens,w));
    case_list = when_cases_list(nested_when);
    clen = gl_length(case_list);
    for (c=1; c<=clen; c++) {
      nested_case = (struct when_case *)(gl_fetch(case_list,c));
      num_case = when_case_case_number(cur_case);
      if (num_case == -1) {
        if (case_in_nested_whens(cases,ncases,nested_case)) { /* recursion */
          return 1;
	}
      } else {
        if(case_in_array_of_cases(num_case,cases,ncases)) {
          return 1;
	}
      }
    }
  }

  return 0;
}

/*
 * If some case of the when belong to the list of cases, the rels
 * in such a case are set as active
 */
static void set_active_rels_in_cases(int32 *cases, int32 ncases,
				     struct w_when *when)
{
  struct gl_list_t *case_list;
  struct gl_list_t *whens;
  struct w_when *nested_when;
  struct when_case *cur_case;
  int32 c,clen,w,wlen;
  int32 num_case;

  case_list = when_cases_list(when);
  clen = gl_length(case_list);
  for (c=1;c<=clen;c++){
    cur_case = (struct when_case *)(gl_fetch(case_list,c));
    num_case = when_case_case_number(cur_case);
    if (num_case == -1) { /* nested whens in case */
      if (case_in_nested_whens(cases,ncases,cur_case)) {
        when_case_set_active(cur_case,TRUE);
        set_rels_status_in_case(cur_case,TRUE);
        set_logrels_status_in_case(cur_case,TRUE);
        whens = when_case_whens_list(cur_case);
        wlen = gl_length(whens);  /* nested whens */
        for (w=1;w<=wlen;w++){
          nested_when = (struct w_when *)(gl_fetch(whens,w));
          set_active_rels_in_cases(cases,ncases,nested_when); /* recursion */
        }
      }
    } else {
      if(case_in_array_of_cases(num_case,cases,ncases)) {
        when_case_set_active(cur_case,TRUE);
        set_rels_status_in_case(cur_case,TRUE);
        set_logrels_status_in_case(cur_case,TRUE);
      }
    }
  }
}

/*
 * get the list of whens for each discrete variable in disvars,
 * set all the relation in those whens as inactive, and then
 * set as active the relations corresponding to the cases passed
 * as argument in cases.
 */
void set_active_rels_in_subregion(slv_system_t sys, int32 *cases,
				  int32 ncases, struct gl_list_t *disvars)
{
  struct rel_relation **solverrl;
  struct dis_discrete *dis;
  struct gl_list_t *whens;
  struct w_when *when;
  int32 d,dlen,w,wlen;

  solverrl = slv_get_solvers_rel_list(sys);

  if (disvars == NULL) {
    return;
  }
  dlen = gl_length(disvars);

  for (d=1;d<=dlen;d++) {
    dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    whens = dis_whens_list(dis);
    if (whens == NULL) {
      continue;
    }
    wlen = gl_length(whens);
    for (w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      set_rels_status_in_when(when,FALSE); /* initialize the active flag */
    }

    for (w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      set_active_rels_in_cases(cases,ncases,when);
    }
  }
}

/*
 * For each relation active, included, equality and not invariant, set the
 * flag in_cur_subregion as TRUE
 */
static void set_variant_rels_in_subregion(struct rel_relation **rlist)
{
  int32 c;
  struct rel_relation *rel;

  for (c = 0; rlist[c]!= NULL; c++) {
    rel = rlist[c];
    if (rel_active(rel) && (!rel_invariant(rel))
        && rel_included(rel) && rel_equality(rel)) {
      rel_set_in_cur_subregion(rel,TRUE);
    }
  }
}

/*
 * For each relation active and not invariant, set the flag
 * in_cur_subregion as TRUE. First, the same flag is initialized
 * to FALSE for all of the relations
 */
void identify_variant_rels_in_subregion(slv_system_t sys)
{
  struct rel_relation **solverrl;
  solverrl = slv_get_solvers_rel_list(sys);

  /* initialize in_cur_subregion flag */
  set_in_cur_subregion_in_rels_list(solverrl,FALSE);

  set_variant_rels_in_subregion(solverrl);
}


/*
 * Set the ACTIVE bit flag as TRUE, for all of the variables
 * incident in the currently active variables. Used for analysis
 * in a subregion.
 */
void set_active_vars_in_subregion(slv_system_t sys)
{
  struct rel_relation **solverrl;
  struct var_variable **solvervl;

  solverrl = slv_get_solvers_rel_list(sys);
  solvervl = slv_get_solvers_var_list(sys);

  set_inactive_vars_in_list(solvervl);
  set_active_vars_in_active_rels(solverrl);
}


/*
 * ------------------------------------------------
 * STRUCTURALCOMPARISON OF CONDITIONAL ALTERNATIVES
 * ------------------------------------------------
 *
 * The following subroutines compare the structure (incidence
 * pattern) of the different alternatives in a conditional
 * model. So, alternatives that have the same incidence
 * pattern can be identified and the combinatorial cosistency
 * analysis (proper selection of the degrees of freedom in a
 * conditional model) can be simplified. This current
 * implementation does not handle nested WHENs. So, if a WHEN have
 * a nested WHEN, the outter most WHEN is considered as a WHEN
 * which will change the incidence pattern. This can be modified
 * at any time, of course.
 */

/*
 * Order the list of incidences of a relation according to the
 * master index.
 */
static int32 *order_incidences_of_relation(struct var_variable **var_list,
				           int32 vlen)
{
  struct var_variable *var;
  int32 *indexbuf;
  int32 *ordered_index;
  int32 v,vind,vcount = 0;
  int32 aux,vin,vindex = 0;

  indexbuf = ASC_NEW_ARRAY(int32,vlen);
  ordered_index = ASC_NEW_ARRAY(int32,vlen);

  for (v=0; v<vlen; v++) {
    var = var_list[v];
    indexbuf[v] = var_mindex(var);
  }

  aux = 0;
  while (aux < vlen) {
    for (v=0; v<vlen; v++) {
      vcount = indexbuf[aux];
      if (vcount >=0) {
        break;
      }
    }
    for (vin=0; vin<vlen; vin++) {
      vind = indexbuf[vin];
      if (vind >=0) {
        if (vcount >= vind ) {
          vcount = vind;
          vindex = vin;
        }
      }
    }
    indexbuf[vindex] = -1;
    ordered_index[aux] = vcount;
    aux++;
  }

  ascfree(indexbuf);
  return ordered_index;
}


/*
 * Compare structure of two CASEs of a WHEN
 */
static int32 compare_alternative_cases(struct when_case *cur_case1,
				      struct when_case *cur_case2)
{
  struct gl_list_t *rel_list1, *rel_list2;
  struct rel_relation *rel1, *rel2;
  struct var_variable **inc1, **inc2;
  int32 numcase1, numcase2;
  int32 nrel1, nrel2;
  int32 nvar1, nvar2;
  int32 v,r, ind1, ind2;
  int32 nivr1, nivr2;
  int32 *ninc1, *ninc2;
  int32 *ord_ind1, *ord_ind2;

  numcase1 = when_case_case_number(cur_case1);
  numcase2 = when_case_case_number(cur_case2);
#if DEBUG_PRE_ANALYSIS
  FPRINTF(ASCERR,"Making comparison of CASEs:\n");
  FPRINTF(ASCERR,"case A = %d  case B = %d \n",numcase1,numcase2);
#endif /* DEBUG_PRE_ANALYSIS */

  nrel1 = when_case_num_rels(cur_case1);
  nrel2 = when_case_num_rels(cur_case2);

  if (nrel1 != nrel2) {
#if DEBUG_PRE_ANALYSIS
    FPRINTF(ASCERR,"CASEs have different number of relations\n");
    FPRINTF(ASCERR,"case A = %d   case B = %d\n",nrel1,nrel2);
#endif /* DEBUG_PRE_ANALYSIS */
    return 0;
  }

  nvar1 = when_case_num_inc_var(cur_case1);
  nvar2 = when_case_num_inc_var(cur_case2);

  if (nvar1 != nvar2) {
#if DEBUG_PRE_ANALYSIS
    FPRINTF(ASCERR,"CASEs have different number of incidences\n");
    FPRINTF(ASCERR,"case A = %d   case B = %d\n",nvar1,nvar2);
#endif /* DEBUG_PRE_ANALYSIS */
    return 0;
  }

  ninc1 = when_case_ind_inc(cur_case1);
  ninc2 = when_case_ind_inc(cur_case2);

  for (v=0; v<nvar1; v++) {
    ind1 = ninc1[v];
    ind2 = ninc2[v];

    if (ind1 != ind2) {
#if DEBUG_PRE_ANALYSIS
      FPRINTF(ASCERR,"Incidences are different in CASEs\n");
      FPRINTF(ASCERR,"index in case A =%d  index in case B =%d\n",ind1,ind2);
#endif /* DEBUG_PRE_ANALYSIS */
      return 0;
    }
  }

  rel_list1 = when_case_rels_list(cur_case1);
  rel_list2 = when_case_rels_list(cur_case2);

  for (r=1; r<=nrel1; r++) {
    rel1 = (struct rel_relation *)(gl_fetch(rel_list1,r));
    rel2 = (struct rel_relation *)(gl_fetch(rel_list2,r));
    nivr1 = rel_n_incidences(rel1);
    nivr2 = rel_n_incidences(rel2);
    if (nivr1 != nivr2) {
#if DEBUG_PRE_ANALYSIS
      FPRINTF(ASCERR,"relations of different CASEs have different ");
      FPRINTF(ASCERR,"number of incidences\n");
      FPRINTF(ASCERR,"No. in rel of A = %d   No. in rel of B = %d\n",
	      nivr1,nivr2);
#endif /* DEBUG_PRE_ANALYSIS */
      return 0;
    }
    inc1 = rel_incidence_list_to_modify(rel1);
    inc2 = rel_incidence_list_to_modify(rel1);
    ord_ind1 = order_incidences_of_relation(inc1,nivr1);
    ord_ind2 = order_incidences_of_relation(inc2,nivr2);
    for (v=0; v<nivr1; v++) {
      ind1 = ord_ind1[v];
      ind2 = ord_ind2[v];
      if (ind1 != ind2) {
#if DEBUG_PRE_ANALYSIS
        FPRINTF(ASCERR,"relations of different CASEs have different \n");
         FPRINTF(ASCERR,"Incidences\n");
        FPRINTF(ASCERR,"index in rel of A =%d  index in rel pf B =%d\n",
		ind1,ind2);
#endif /* DEBUG_PRE_ANALYSIS */
        ascfree(ord_ind1);
        ascfree(ord_ind2);
        return 0;
      }
    }
    ascfree(ord_ind1);
    ascfree(ord_ind2);
  }

#if DEBUG_PRE_ANALYSIS
        FPRINTF(ASCERR,"CASEs have the same structure \n");
#endif /* DEBUG_PRE_ANALYSIS */
  return 1;
}

/*
 * Compares the structure (number of equations, variables, incidence
 * pattern, etc) of the different CASEs of a WHEN statement
 */
static int32 compare_alternative_structures_in_when(struct w_when *when)
{
  struct when_case *cur_case1;
  struct when_case *cur_case2;
  struct gl_list_t *cases;
  int32 c,clen;

  cases = when_cases_list(when);
  if (cases == NULL) {
    FPRINTF(ASCERR,"WARNING: No list of cases in When\n");
    return 1;
  }

  clen = gl_length (cases);

  if (clen >1) {
    cur_case1 = (struct when_case *)(gl_fetch(cases,1));
    for (c=2; c<=clen; c++) {
      cur_case2 = (struct when_case *)(gl_fetch(cases,c));
      if (!compare_alternative_cases(cur_case1,cur_case2)) {
#if DEBUG_PRE_ANALYSIS
        FPRINTF(ASCERR,"CASEs have different structure\n");
#endif /* DEBUG_PRE_ANALYSIS */
        return 0;
      }
    }
    return 1;
  } else {
    if (clen == 1 ) {
      return 0;
    } else {
      return 1;
    }
  }
}

/*
 * Define values for the flag incident_in_case of the variables in the
 * master list
 */
static void set_incident_in_case_status(struct var_variable **vlist,
					uint32 value)
{
  int32 c;
  struct var_variable *var;

  for (c = 0; vlist[c]!= NULL; c++) {
    var = vlist[c];
    var_set_incident_in_case(var,value);
  }
}

/*
 * Get the list of master indices of the variables incident in a
 * CASE
 */
static void get_incidences_in_case(struct when_case *cur_case,
			    struct var_variable **mastervl)
{
  struct gl_list_t *rels;
  struct rel_relation *rel;
  struct var_variable **var_list;
  struct var_variable *var;
  int32 *inc;
  int32 rlen, vlen, r,v;
  int32 ninc;

  rels = when_case_rels_list(cur_case);
  set_incident_in_case_status(mastervl,FALSE);
  if (rels != NULL) {
    rlen = gl_length(rels);
    for (r=1;r<=rlen;r++) {
      rel = (struct rel_relation *)(gl_fetch(rels,r));
      vlen = rel_n_incidences(rel);
      var_list = rel_incidence_list_to_modify(rel);
      for (v=0; v<vlen; v++) {
       var = var_list[v];
       var_set_incident_in_case(var,TRUE);
      }
    }
  }
  ninc = 0;
  for (v=0; mastervl[v]!= NULL; v++) {
    var = mastervl[v];
    if (var_incident_in_case(var)) {
      ninc++;
    }
  }

  if (ninc>0) {
    inc = ASC_NEW_ARRAY(int32,ninc);
    ninc = 0;
    for (v=0; mastervl[v]!= NULL; v++) {
      var = mastervl[v];
      if (var_incident_in_case(var)) {
        inc[ninc] = var_mindex(var);
        ninc++;
      }
    }
     when_case_set_num_inc_var(cur_case,ninc);
     when_case_set_ind_inc(cur_case,inc);
#if DEBUG_PRE_ANALYSIS
     FPRINTF(ASCERR,"Number of incidences = %d \n",ninc);
#endif /* DEBUG_PRE_ANALYSIS */
  } else {
     when_case_set_num_inc_var(cur_case,0);
  }
}


/*
 * Order a list of relations according to the index of the incidences
 */
static void order_relations_by_incidences(struct gl_list_t *scratch)
{
  struct rel_relation *rel;
  struct rel_relation **array_rel;
  struct var_variable **var_list;
  int32 r,rlen,rin,rind = 0;
  int32 num_inc = 0, v, vv, vindex = 0, vind;
  int32 glob_count, n1, n2;
  int32 *num_rel, *ordered_num_rel;
  int32 **var_ind;

  rlen = gl_length(scratch);
  num_rel = ASC_NEW_ARRAY(int32,rlen);
  ordered_num_rel = ASC_NEW_ARRAY(int32,rlen);
  var_ind = (int32 **)ascmalloc(rlen*sizeof(int32 *));
  array_rel = (struct rel_relation **)ascmalloc
                                      (rlen*sizeof(struct rel_relation *));

  for (r=1; r<=rlen; r++) {
    num_rel[r-1] = r;
    ordered_num_rel[r-1] = r;
    rel = (struct rel_relation *)(gl_fetch(scratch,r));
    array_rel[r-1] = rel;
    var_list = rel_incidence_list_to_modify(rel);
    num_inc = rel_n_incidences(rel);
    var_ind[r-1] = order_incidences_of_relation(var_list,num_inc);
  }

  v=0;
  glob_count = 0;
  while (glob_count <rlen) {
    for (r=0; r <rlen; r++) {
      vindex = var_ind[r][v];
      if (num_rel[r] > 0) {
        vindex = var_ind[r][v];
        rind =r;
        break;
      }
    }
    for (rin=0; rin<rlen; rin++) {
      vind = var_ind[rin][v];
      if (num_rel[rin] > 0) {
        if (rin != rind) {
          if (vindex > vind ) {
            vindex = vind;
            rind = rin;
          } else {
            if (vindex == vind) {
              for(vv=0; vv<num_inc; vv++) {
                n1 = var_ind[rind][v];
                n2 = var_ind[rin][v];
                if (n1 != n2) {
                  if (n1 > n2) {
                    rind = rin;
		  }
                  break;
		}
	      }
	    }
	  }
	}
      }
    }
    num_rel[rind] = -1;
    ordered_num_rel[glob_count] = rind;
    glob_count = glob_count + 1;
  }

  gl_reset(scratch);

  for (r=0; r<rlen; r++) {
    for (rin=0; rin<rlen; rin++) {
      if (r == ordered_num_rel[rin]) {
        rel = array_rel[rin];
        gl_append_ptr(scratch,rel);
        break;
      }
    }
  }

  ascfree(num_rel);
  ascfree(ordered_num_rel);
  for (r=0; r<rlen; r++) {
    ascfree(var_ind[r]);
  }
  ascfree(var_ind);
  ascfree(array_rel);
}

/*
 * Order the list of relations of a CASE according to the number of
 * incidences and index of the incidences
 */
static void order_relations_in_case(struct when_case *cur_case)
{
  struct gl_list_t *rels;
  struct gl_list_t *new_rels;
  struct gl_list_t *tmp;
  struct gl_list_t *scratch;
  struct rel_relation *rel;
  int32 *num_inc;
  int32 *ordered_num_inc;
  int32 r, rin, rlen, rcount = 0, rind;
  int32 indexr = 0, aux, glob_count;

  rels = when_case_rels_list(cur_case);
  if (rels != NULL) {
    rlen = gl_length(rels);
    if (rlen > 1) {
      num_inc = ASC_NEW_ARRAY(int32,rlen);
      ordered_num_inc = ASC_NEW_ARRAY(int32,rlen);
      new_rels = gl_create(rlen);
      tmp =  gl_create(rlen);
      for (r=1;r<=rlen;r++) {    /* get number of incidences */
        rel = (struct rel_relation *)(gl_fetch(rels,r));
        num_inc[r-1] = rel_n_incidences(rel);
      }

      /* order number of incidences */
      aux = 0;
      while (aux < rlen) {
        for (r=0;r<rlen;r++) {
          rcount = num_inc[r];
          if (rcount >= 0) {
            break;
	  }
        }
        for (rin=0; rin<rlen; rin++) {
          rind = num_inc[rin];
          if (rind >=0) {
            if (rcount >= rind ) {
              rcount = rind;
              indexr = rin;
            }
          }
        }
        num_inc[indexr] = -1;
        ordered_num_inc[aux] = rcount;
        aux++;
      }

      /* order relations in same order as number of incidences */
      for (r=0;r<rlen;r++) {
        num_inc[r] = ordered_num_inc[r];
      }
      for (r=0; r<rlen; r++) {
        rcount = ordered_num_inc[r];
        for (rin=0; rin<rlen;rin++) {
          if (num_inc[rin] >= 0) {
            rel = (struct rel_relation *)(gl_fetch(rels,rin+1));
            rind = rel_n_incidences(rel);
            if (rcount == rind) {
              gl_append_ptr(new_rels,rel);
              num_inc[rin] = -1;
              break;
            }
	  }
        }
      }

      /*
       * order relations with same order of incidences according to
       * the master indices of their incidences
       */
       glob_count = 0;
       aux = 0;
       while(glob_count<rlen) {
         for (r=aux; r<rlen; r++) {
           rcount = ordered_num_inc[r];
           if ((r+1) < rlen) {
             rind = ordered_num_inc[r+1];
             if (rcount == rind) {
               glob_count = glob_count+1;
               continue;
	     } else {
               break;
	     }
	   } else {
             break;
	   }
         }
         if (aux != glob_count) {
           scratch = gl_create(glob_count - aux + 1);
	   for (r=aux; r<=glob_count; r++) {
             rel = (struct rel_relation *)(gl_fetch(rels,r+1));
             gl_append_ptr(scratch,rel);
	   }
           order_relations_by_incidences(scratch);
           for (r=1; r<=glob_count - aux + 1; r++) {
            rel = (struct rel_relation *)(gl_fetch(scratch,r));
            gl_append_ptr(tmp,rel);
	   }
           gl_destroy(scratch);
	 } else {
            rel = (struct rel_relation *)(gl_fetch(rels,glob_count+1));
            gl_append_ptr(tmp,rel);
	 }
         aux = glob_count+1;
         glob_count = aux;
       }
      /*
       * set ordered list of relations
       */
      when_case_set_rels_list(cur_case,tmp);

      ascfree(num_inc);
      ascfree(ordered_num_inc);
      gl_destroy(rels);
      gl_destroy(new_rels);
    }
  }
}


/*
 * Analyzes (get number of relations and variables, get global list of
 * incident variables, order relation and variables by master index)
 * a CASE of a WHEN statement
 */
static int32 analyze_structure_of_case(struct when_case *cur_case,
				       struct var_variable **mastervl)
{
  struct gl_list_t *rels;
  struct gl_list_t *whens;
  int32 rlen;
  int32 wlen;

  whens = when_case_whens_list(cur_case);
#if DEBUG_PRE_ANALYSIS
  FPRINTF(ASCERR,"case # = %d  \n",when_case_case_number(cur_case));
#endif /* DEBUG_PRE_ANALYSIS */
/*
 * No nested WHENs right now
 * Here we'll need to recursively call
 * the analysis of the nested when and
 * the internal structure of the WHEN
 */

  if (whens != NULL) {
    wlen = gl_length(whens);
    if (wlen > 0) {
#if DEBUG_PRE_ANALYSIS
      FPRINTF(ASCERR,"CASE contains nested WHENs\n");
#endif /* DEBUG_PRE_ANALYSIS */
      return 0;
    }
  }

  rels = when_case_rels_list(cur_case);
  if (rels != NULL) {
    rlen = gl_length(rels);
#if DEBUG_PRE_ANALYSIS
    FPRINTF(ASCERR,"Number of relations = %d \n",rlen);
#endif /* DEBUG_PRE_ANALYSIS */
    when_case_set_num_rels(cur_case,rlen);
    order_relations_in_case(cur_case);
    get_incidences_in_case(cur_case,mastervl);
  } else {
    when_case_set_num_rels(cur_case,0);
    when_case_set_num_inc_var(cur_case,0);
  }
  return 1;
}


/*
 * analyzes the structure of the different alternatives (CASES)
 * in a WHEN statement so  that we can find out later if it is
 * necessary to perform a combinatorial search for a consistent
 * variable partitioning
 */

static int32 analyze_alternative_structures_in_when(struct w_when *when,
					     struct var_variable **mastervl)
{
  struct when_case *cur_case;
  struct gl_list_t *cases;
  int32 c,clen;

  cases = when_cases_list(when);
  if (cases == NULL) {
    FPRINTF(ASCERR,"WARNING: No list of cases in When\n");
    return 1;
  }

  clen = gl_length (cases);

  for (c=1; c<=clen; c++) {
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    if (!analyze_structure_of_case(cur_case,mastervl)) {
      when_set_changes_structure(when,TRUE);
#if DEBUG_PRE_ANALYSIS
      FPRINTF(ASCERR,"WHEN CHANGES structure\n");
      FPRINTF(ASCERR,"\n");
#endif /* DEBUG_PRE_ANALYSIS */
      return 0;
    }
  }

  if (!compare_alternative_structures_in_when(when)) {
    when_set_changes_structure(when,TRUE);
#if DEBUG_PRE_ANALYSIS
      FPRINTF(ASCERR,"WHEN CHANGES structure\n");
      FPRINTF(ASCERR,"\n");
#endif /* DEBUG_PRE_ANALYSIS */
    return 0;
  }
  when_set_changes_structure(when,FALSE);
#if DEBUG_PRE_ANALYSIS
      FPRINTF(ASCERR,"WHEN DOES NOT CHANGE structure\n");
      FPRINTF(ASCERR,"\n");
#endif /* DEBUG_PRE_ANALYSIS */
  return 1;
}


/*
 *-------------------------------------------
 * ENUMERATION OF CASES
 *------------------------------------------
 */

/*
 * Finds the number of cases in a when. This number will include nested
 * cases (in nested when statements). It also assigns a identifier number
 * to each case in the when statement. If this number is equal to -1
 * for some case, it implies that the case contains nested whens and it
 * is necessary to perform a recursive analysis to find the numbers of the
 * cases embedded in the current case.
 * In general, the number assigned to each of the cases is only for
 * identification purposes, so, 1,2,3, whatever, does not matter, we only
 * want to distinguish among them. The identification number depends
 * on the order of the whens in the master when list, which is not
 * expected to change unless we destroy the system.
 *
 * This function uses the global variable g_case_number. Any caller
 * function has to reinitialize that global variable if it is required.
 */
void enumerate_cases_in_when(struct w_when *when)
{
  struct when_case *cur_case;
  struct w_when *casewhen;
  struct gl_list_t *cases;
  struct gl_list_t *whens;
  int32 w,wlen,c,clen;
  int32 scratch;
  int32 cur_num_cases; /* number of cases in current when */

  scratch = g_case_number;

  cases = when_cases_list(when);
  if (cases == NULL) {
    FPRINTF(ASCERR,"WARNING: No list of cases in When\n");
    return;
  }

  clen = gl_length (cases);
  for (c=1; c<=clen; c++) {
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    whens = when_case_whens_list(cur_case);
    if ( (whens == NULL) || (gl_length(whens) == 0) ) {
      g_case_number++;
      when_case_set_case_number(cur_case,g_case_number);
#if CASE_NUMBER
      FPRINTF(ASCERR,"Case number = %d \n",
                           when_case_case_number(cur_case));
#endif /* CASE NUMBER*/
    } else {
      wlen = gl_length(whens);
      for (w=1;w<=wlen;w++) {
        when_case_set_case_number(cur_case,-1); /* nested cases */
        casewhen = (struct w_when *)(gl_fetch(whens,w));
        enumerate_cases_in_when (casewhen);
      }
    }
  }
  cur_num_cases = g_case_number - scratch;
  when_set_num_cases(when,cur_num_cases);
}


#define alloc_case_array(ncases,type)   \
   ((ncases) > 0 ? (type *)ascmalloc((ncases)*sizeof(type)) : NULL)
#define copy_case_num(from,too,nnums)  \
   asc_memcpy((from),(too),(nnums)*sizeof(int32))

/*
 * Appends a case_number onto the list
 */
static void append_case_number( struct ds_case_list *cl, int32 case_number)
{
   if( cl->length == cl->capacity ) {
      int32 newcap;
      int32 *newlist;
      newcap = cl->capacity + 40;
      newlist = alloc_case_array(newcap,int);
      copy_case_num((char *)cl->case_number,(char *)newlist,cl->length);
      if( cl->case_number != NULL )
	 ascfree(cl->case_number);
      cl->case_number = newlist;
      cl->capacity = newcap;
   }
   cl->case_number[cl->length++] = case_number;
}


#if USEDCODE
/*
 * Removes the case_number at given index from the list.
 *
 * Currently not in use
 */
static void remove_case_number( struct ds_case_list *cl, int32 ndx)
{
  copy_case_num((char *)(cl->case_number+ndx+1),
  	     (char *)(cl->case_number+ndx), --(cl->length) - ndx);
}
#endif /* USEDCODE */



/*
 * Get the case number of the matching cases in a when statement. If
 * such number is -1, that means that the case contains nested whens
 * (and therefore nested cases) and the search is done recursively
 */
static void cases_matching_in_when(struct w_when *when,
				   struct ds_case_list *cl,
				   int32 *ncases)
{
  struct gl_list_t *cases;
  struct gl_list_t *whens_in_case;
  struct when_case *cur_case;
  int32 c,clen,case_number;

  cases = when_cases_list(when);
  if (cases == NULL) {
    return;
  }
  clen = gl_length(cases);
  for (c=1;c<=clen;c++) {
    cur_case = (struct when_case *)(gl_fetch(cases,c));
    if (when_case_active(cur_case)){
      case_number = when_case_case_number(cur_case);
      if (case_number == -1) {
        whens_in_case = when_case_whens_list(cur_case);
        cases_matching_in_when_list(whens_in_case,cl,ncases);
      } else {
        append_case_number(cl,case_number);
	(*ncases)++;
      }
    }
  }
}


/*
 * Disentagle a list of whens and analyze each one of them, looking
 * for the number of matching cases.
 */
static void cases_matching_in_when_list(struct gl_list_t *whens,
                                        struct ds_case_list *cl,
					int32 *ncases)
{
  struct w_when *when;
  int32 w,wlen;

  if (whens == NULL) {
    return;
  }
  wlen = gl_length(whens);
  for (w=1;w<=wlen;w++) {
    when = (struct w_when *)(gl_fetch(whens,w));
    if (!when_visited(when)) {
      cases_matching_in_when(when,cl,ncases);
    }
    when_set_visited(when,TRUE);
  }
}

/*
 * Given a list of discrete variables, finds which cases apply
 * in the whens depending on those variables.
 * The caller funtion should modify the values of these discrete
 * variables so that we can make combinatorial search. Also,
 * the list should contain discrete variables IN_WHEN.
 * This functions assumes that the cases in the whens have been
 * previously enumerated. Need to check performance regarding the
 * visiting of cases when some of the whens are nested.
 */
int32 *cases_matching(struct gl_list_t *disvars, int32 *ncases)
{
  struct ds_case_list cl;
  struct dis_discrete *dis;
  struct w_when *when;
  struct gl_list_t *whens;
  int32 d,dlen,w,wlen;

  if (disvars == NULL) {
    return NULL;
  }
  dlen = gl_length(disvars);


  (*ncases) =  0;
  cl.length = cl.capacity = 0;
  cl.case_number = NULL;
  append_case_number(&cl,0);

  /*
   * First make sure that all of the Whens has the flag VISITED off,
   * then disactive the cases, and then analyze the whens.
   */
  for (d=1;d<=dlen;d++) {
    dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    whens = dis_whens_list(dis);
    if (whens == NULL) {
      continue;
    }
    wlen = gl_length(whens);
    for (w=1;w<=wlen;w++) {
      when = (struct w_when *)(gl_fetch(whens,w));
      when_set_visited(when,FALSE);
      disactive_cases_in_when(when);
      simplified_analyze_when(when);
    }
  }

  /*
   * Then get the list of matched cases in each when
   */

  for (d=1;d<=dlen;d++) {
    dis = (struct dis_discrete *)(gl_fetch(disvars,d));
    whens = dis_whens_list(dis);
    if (whens == NULL) {
      continue;
    }
    wlen = gl_length(whens);
    if (wlen > 0) {
      cases_matching_in_when_list(whens,&cl,ncases);
    }
  }

  return cl.case_number;
}


/*
 * configure_conditional_problem
 * analyze the when statements included in our problem so that, we
 * determine which rels, vars, disvars, and logrels are currently
 * active. It is called by analyze.c at the time of the system
 * building. For reconfiguration of the system call
 * reanalyze_solver_lists
 */
void configure_conditional_problem(int32 numwhens,
                                   struct w_when **whenlist,
                                   struct rel_relation **solverrl,
                                   struct logrel_relation **solverll,
				   struct var_variable **mastervl)
{
  int32 w,result;
  struct w_when *when;

  /* Enumerate cases in when's */
  g_case_number = 0;
  for (w = 0; w < numwhens; w++) {
    when = whenlist[w];
    if (!when_inwhen(when)) {
      enumerate_cases_in_when(when);
    }
  }

  /* get structure of the different alternatives */
  for (w = 0; w < numwhens; w++) {
    when = whenlist[w];
    result = analyze_alternative_structures_in_when(when,mastervl);
  }

  /*
   *All rel_relations and logrel_relations explicitly or implicitly
   * (models) inside a w_when are deactivated
   */
  for (w = 0; w < numwhens; w++) {
    when = whenlist[w];
    if (!when_inwhen(when)) {
      set_rels_status_in_when(when,FALSE);
    }
  }

  /* All of the rels which are ACTIVE, are also INVARIANT */
  set_active_rels_as_invariant(solverrl);

  /*
   * Analyze whens and find active relations and logrelations
   * in each of them
   */

  for (w = 0; w < numwhens; w++) {
    when = whenlist[w];
    if (!when_inwhen(when)) {
      analyze_when(when);
    }
  }

  /* All variables in active relations are
   * set as active */
  set_active_vars_in_active_rels(solverrl);
  set_active_disvars_in_active_logrels(solverll);
}

/*
 *       Reconfiguration/Rebuilding of Conditional Models
 */

/*
 * After a change of the value of some conditional variable present
 * in a When, the solver lists of variables, relations and logrelations
 * are reanalyzed to set the bit ACTIVE for the vars, rels and
 * logrels corresponding to a possible new configuration.
 */
void reanalyze_solver_lists(slv_system_t sys)
{
  struct rel_relation **solverrl;
  struct rel_relation **solverol;
  struct logrel_relation **solverll;
  struct var_variable **solvervl;
  struct dis_discrete **solverdl;
  struct dis_discrete **dislist;
  struct w_when **whenlist;
  struct w_when *when;
  struct dis_discrete *dvar;
  struct gl_list_t *symbol_list;
  int32 c;

  solverrl = slv_get_solvers_rel_list(sys);
  solverol = slv_get_solvers_obj_list(sys);
  solverll = slv_get_solvers_logrel_list(sys);
  solvervl = slv_get_solvers_var_list(sys);
  solverdl = slv_get_solvers_dvar_list(sys);
  whenlist = slv_get_solvers_when_list(sys);
  dislist =  slv_get_master_dvar_list(sys);
  symbol_list = slv_get_symbol_list(sys);

  set_inactive_vars_in_list(solvervl);
  set_inactive_disvars_in_list(solverdl);
  set_active_rels_in_list(solverrl);
  set_active_logrels_in_list(solverll);

  for (c=0; dislist[c]!=NULL; c++) {
    dvar = dislist[c];
    dis_set_value_from_inst(dvar,symbol_list);
  }

  for (c=0; whenlist[c]!=NULL ; c++) {
    when = whenlist[c];
    if (!when_inwhen(when)) {
      set_rels_status_in_when(when,FALSE);
    }
  }

  /* All of the rels which are ACTIVE, are also INVARIANT */
  set_active_rels_as_invariant(solverrl);

  for (c=0; whenlist[c]!=NULL; c++) {
    when = whenlist[c];
    if (!when_inwhen(when)) { /* nested whens are analyzed recursively */
      analyze_when(when);
    }
  }
  set_active_vars_in_active_rels(solverrl);
  set_active_vars_in_active_rels(solverol);
  set_active_disvars_in_active_logrels(solverll);
}


/*
 * Returns 1 if the system is reanalyzed. O if nothing happens.
 * System will be reanalyzed if a boolean variable included in some
 * when of slv_system has been modified.
 * There is a bit of insanity in using the IPTR here, since we are
 * supossed to work only with the structures in the solver side.
 * The reason is that this function is receiving the information from
 * the user interface, which is working in the compiler side.
 */

int32 system_reanalyze(slv_system_t sys, SlvBackendToken inst)
{
  if (inst==NULL) {
    reanalyze_solver_lists(sys);
    return 1;
  }
  if (varinst_found_in_whenlist(sys,IPTR(inst))) {
    reanalyze_solver_lists(sys);
    return 1;
  } else {
    return 0;
  }
}


/*
 * The next four functions are not currently in use, but they may
 * in the future. See the header
 */

/*
 * Build the rel solver list from the master list in the case of our
 * problem contains when's. This function is not currently in use. It
 * could be use to build a solver rel list of ACTIVE relations, by
 * using a master rel list with all of the relations in it. These
 * ACTIVE relations could be INCLUDED or UNINCLUDED. It returns the
 * number of relation in the list.
 */
int32 build_rel_solver_from_master(struct rel_relation **masterrl,
			         struct rel_relation **solverrl)
{
  struct var_variable *var;
  struct rel_relation *rel;
  struct var_variable **incidence;
  int32 rel_active;
  int32 v,c,vlen;

  rel_active = 0;

  /* create the solver list of active relations */

  for (v = 0; masterrl[v]!=NULL; v++) {
    rel = masterrl[v];
    if (rel_active(rel)) {
      rel_set_sindex(rel,rel_active);
      solverrl[rel_active] = rel;
      vlen = rel_n_incidences(rel);
      incidence = rel_incidence_list_to_modify(rel);
      for(c=0; c<vlen;c++) {
        var = incidence[c];
        var_set_active(var,TRUE);
      }
      rel_active++;
    }
  }
  solverrl[rel_active] = NULL; /* terminator */
  return rel_active;
}


/*
 * Build the logrel solver list from the master list in the case of our
 * problem contains when's. This function is not currently in use. It
 * could be use to build a solver logrel list of ACTIVE logrelations, by
 * using a master logrel list with all of the logrelations in it. These
 * ACTIVE logrelations could be INCLUDED or UNINCLUDED. It returns the
 * number of logrelation in the list.
 */
int32 build_logrel_solver_from_master(struct logrel_relation **masterll,
			            struct logrel_relation **solverll)
{
  struct dis_discrete *dvar;
  struct logrel_relation *lrel;
  struct dis_discrete **incidence;
  int32 lrel_active;
  int32 v,c,vlen;

  lrel_active = 0;

  /* create the solver list of active logrelations */

  for (v = 0; masterll[v]!=NULL; v++) {
    lrel = masterll[v];
    if (logrel_active(lrel)) {
      logrel_set_sindex(lrel,lrel_active);
      solverll[lrel_active] = lrel;
      vlen = logrel_n_incidences(lrel);
      incidence = logrel_incidence_list_to_modify(lrel);
      for(c=0; c<vlen;c++) {
        dvar = incidence[c];
        dis_set_active(dvar,TRUE);
      }
      lrel_active++;
    }
  }
  solverll[lrel_active] = NULL; /* terminator */
  return lrel_active;
}


/*
 * Build the var solver list from the master list in the case of our
 * problem contains when's.  This function is not currently in use. It
 * could be use to build a solver var list of ACTIVE variables (Vars
 * incident in ACTIVE relations). It returns the number of variables
 * in the list.
 */
int32 build_var_solver_from_master(struct var_variable **mastervl,
			         struct var_variable **solvervl)
{
  struct var_variable *var;
  int32 var_active;
  int32 v;

  var_active = 0;

  /* create the solver list of active variables */

  for (v = 0; mastervl[v]!=NULL; v++) {
    var = mastervl[v];
    if (var_active(var)) {
      var_set_sindex(var,var_active);
      solvervl[var_active] = var;
      var_active++;
    }
  }
  solvervl[var_active] = NULL; /* terminator */
  return var_active;
}


/*
 * Build the discrete var solver list from the master list in the case
 * of our problem contains when's.  This function is not currently in use.
 * It could be use to build a solver discrete var list of ACTIVE discrete
 * variables (Discrete Vars incident in ACTIVE logrelations). It returns
 * the number of discrete variables in the list.
 */
int32 build_disvar_solver_from_master(struct dis_discrete **masterdl,
			            struct dis_discrete **solverdl)
{
  struct dis_discrete *dvar;
  int32 dvar_active;
  int32 v;

  dvar_active = 0;

  /* create the solver list of active discrete variables */

  for (v = 0; masterdl[v]!=NULL; v++) {
    dvar = masterdl[v];
    if (dis_active(dvar)) {
      dis_set_sindex(dvar,dvar_active);
      solverdl[dvar_active] = dvar;
      dvar_active++;
    }
  }
  solverdl[dvar_active] = NULL; /* terminator */
  return dvar_active;
}


#if USEDCODE

/**
 ** Not currently in use, but maybe someday :) . It creates the solver
 ** lists based in the master lists. The solver lists build based on
 ** this function contains only ACTIVE vars, rels and logrels.
 ** We are using the function system_reanalyze instead
 **/

void rebuild_solvers_from_masters(slv_system_t sys)
{
  struct rel_relation **masterrl;
  struct rel_relation **solverrl;
  struct logrel_relation **masterll;
  struct logrel_relation **solverll;
  struct var_variable **mastervl;
  struct var_variable **solvervl;
  struct dis_discrete **masterdl;
  struct dis_discrete **solverdl;
  struct w_when **whenlist;
  struct w_when *when;
  struct dis_discrete *dvar;
  struct gl_list_t *symbol_list;
  int32 c,len;
  int32 nrel_active, nlrel_active;
  int32 nvar_active, ndvar_active;

  masterrl = slv_get_master_rel_list(sys);
  masterll = slv_get_master_logrel_list(sys);
  mastervl = slv_get_master_var_list(sys);
  masterdl = slv_get_master_dvar_list(sys);
  whenlist = slv_get_solvers_when_list(sys);
  symbol_list = slv_get_symbol_list(sys);

  if( (solvervl = slv_get_solvers_var_list(sys))!=NULL ) {
      ascfree(solvervl);
  }

  if( (solverrl = slv_get_solvers_rel_list(sys))!=NULL ) {
      ascfree(solverrl);
  }

  set_inactive_vars_in_list(mastervl);
  set-active_rels_in_list(masterrl);
  set_inactive_disvars_in_list(masterdl);
  set_active_logrels_in_list(masterll);

  for (c=0; masterdl[c]!=NULL; c++) {
    dvar = masterdl[c];
    dis_set_value_from_inst(dvar,symbol_list);
  }

  for (c=0; whenlist[c]!=NULL ; c++) {
    when = whenlist[c];
    if (!when_inwhen(when)) {
      set_rels_status_in_when(when,FALSE);
    }
  }

  for (c=0; whenlist[c]!=NULL; c++) {
    when = whenlist[c];
    if (!when_inwhen(when)) {
      analyze_when(when);
    }
  }

  nrel_active = build_rel_solver_from_master(masterrl,solverrl);
  nvar_active = build_var_solver_from_master(mastervl,solvervl);

  slv_set_solvers_rel_list(sys,solverrl,nrel_active);
  slv_set_solvers_var_list(sys,solvervl,nvar_active);

  nlrel_active = build_logrel_solver_from_master(masterll,solverll);
  ndvar_active = build_disvar_solver_from_master(masterdl,solverdl);

  slv_set_solvers_logrel_list(sys,solverll,nlrel_active);
  slv_set_solvers_dvar_list(sys,solverdl,ndvar_active);
}

#endif  /* USEDCODE */





/* 
 *  Conditional Modeling Configuration File
 *  by Vicente Rico-Ramirez
 *  Created: 04/97
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: cond_config.h,v $
 *  Date last modified: $Date: 1998/03/30 22:06:51 $
 *  Last modified by: $Author: rv2a $
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
 */

/** @file
 *  Conditional Modeling Configuration File.
 *  <pre>
 *  Requires:     #include "utilities/ascConfig.h"
 *                #include "system.h"
 *                #include "compiler/instance_enum.h"
 *  </pre>
 */

#ifndef ASC_COND_CONFIG_H
#define ASC_COND_CONFIG_H

extern void set_rels_status_in_when(struct w_when *when, uint32 value);
/**<
 * Set the ACTIVE bit to FALSE/TRUE for all the relations and logrelations
 * included in the list of when
 */

extern void analyze_when(struct w_when *when);
/**<
 * Analyzes the when list to see which case is active in each of the
 * whens. For all the relations in the matched cases, set the ACTIVE
 * flag to TRUE
 */

extern void set_active_rels_in_list(struct rel_relation **rlist);
/**<
 * Set the ACTIVE bit to TRUE for all the rels included in the
 * rel list.
 */

extern void set_active_rels_as_invariant(struct rel_relation **rlist);
/**<
 * Sets the INVARIANT bit to TRUE for all the rels in the rel list which
 * are ACTIVE. Sets it to FALSE for all the INACTIVE rels.
 */

extern void
set_active_logrels_in_list(struct logrel_relation **lrlist);
/**<
 * Sets the ACTIVE bit to TRUE for all the logrels included in the
 * logrel list.
 */

extern void set_inactive_vars_in_list(struct var_variable **vlist);
/**<
 * Sets the ACTIVE bit to FALSE for all the vars included in the
 * var list
 */

extern void set_invariant_in_rels_list(struct rel_relation **rlist,
                                       uint32 value);
/**<
 * Set the INVARIAT bit to value for all the rels included in the
 * rel list.
 */

extern void
set_in_cur_subregion_in_rels_list(struct rel_relation **rlist,
                                  uint32 value);
/**<
 * Set the IN_CUR_SUBREGION bit to value for all the rels included in the
 * rel list.
 */

extern void
set_inactive_disvars_in_list(struct dis_discrete **dvlist);
/**<
 * Set the ACTIVE bit to FALSE for all the discrete vars included in the
 * discrete var list
 */

extern void
set_active_vars_in_active_rels(struct rel_relation **solverrl);
/**<
 * Set the ACTIVE bit to TRUE for all the variables included in
 * ACTIVE relations.
 */

extern void
set_active_disvars_in_active_logrels(struct logrel_relation **solverlr);
/**<
 * Set the ACTIVE bit to TRUE for all the discrete variables included in
 * ACTIVE logrelations.
 */

extern void set_active_vars_at_bnd(slv_system_t sys,
                                   struct gl_list_t *disvars);
/**<
 * Set the ACTIVE bit to TRUE for all the variables incident in all
 * the relations of all the subregions neighboring a boundary(ies)
 */

extern void identify_invariant_rels_at_bnd(slv_system_t sys,
                                                    struct gl_list_t *disvars);
/**<
 * Set the INVARIANT flag to TRUE for all the relations invariant with
 * respect to the current boundary(ies)
 */

extern void set_active_rels_in_subregion(slv_system_t sys,
                                         int32 *cases,
                                         int32 ncases,
                                         struct gl_list_t *disvars);
/**<
 * Get the list of whens for each discrete variable in disvars,
 * set all the relation in those whens as inactive, and then
 * set as active the relations corresponding to the cases passed
 * as argument in cases.
 */

extern void identify_variant_rels_in_subregion(slv_system_t sys);
/**<
 * For each relation active and not invariant, set the flag
 * in_cur_subregion as TRUE. First, the same flag is initialized
 * to FALSE for all of the relations
 */

extern void set_active_vars_in_subregion(slv_system_t sys);
/**<
 * Set the ACTIVE bit flag as TRUE, for all of the variables
 * incident in the currently active relations. Used for analysis
 * at a boundary.
 */

extern void enumerate_cases_in_when(struct w_when *when);
/**<
 * Finds the number of cases in a when. This number will include nested
 * cases (in nested when statements). It also assigns a identifier number
 * to each case in the when statement. If this number is equal to -1
 * for some case, it implies that the case contains nested whens and it
 * is necessary to perform a recursive analysis to find the number of the
 * cases embedded in the current case.
 * In general, the number assigned to each of the cases is only for
 * identification purposes, so, 1,2,3, whatever, does not matter, we only
 * want to distinguish among them. The identification number depends
 * on the order of the whens in the master when list, which is not
 * expected to change unless we destroy the system.<br><br>
 *
 * This function uses the global variable g_case_number. Any caller
 * function has to reinitialize that global variable if it is required.
 */

extern int *cases_matching(struct gl_list_t *disvars, int *ncases);
/**<
 * Given a list of discrete variables, it finds which cases apply
 * in the whens depending on those variables.
 * The caller funtion should modify the values of these discrete
 * variables so that we can make combinatorial search. Also,
 * the list should contain discrete variables INWHEN.
 * This functions assumes that the cases in the whens have been
 * previously enumerated. Need to check performance regarding the
 * visiting of cases when some of the whens are nested.
 * It returns an array containing the (ID number of) cases which
 * applies for the current values of the discrete varaibles.
 */

extern void configure_conditional_problem(int numwhens,
                                          struct w_when **whenlist,
                                          struct rel_relation **solverrl,
                                          struct logrel_relation **solverll,
                                          struct var_variable **mastervl);
/**<
 * <!--  configure_conditional_problem                                 -->
 * Analyze the when statements included in our problem so that, we
 * determine which rels, vars, disvars, and logrels are currently
 * active. It is called by analyze.c at the time of the system
 * building. For reconfiguration of the system call
 * reanalyze_solver_lists
 */

extern void reanalyze_solver_lists(slv_system_t sys);
/**<
 * For conditional modeling. This functions analyzes the when's
 * of the solver when list  and set the current value of the
 * flag ACTIVE for variables and relations in the solvers lists.
 */

ASC_DLLSPEC int32 system_reanalyze(slv_system_t sys, SlvBackendToken inst);
/**<
 * For conditional modeling. If a whenvarlist has been changed
 * or a method has been run, this function calls
 * reanlyze_solver_lists.
 */

extern int build_rel_solver_from_master(struct rel_relation **masterrl,
                                        struct rel_relation **solverrl);
/**<
 * Build the rel solver list from the master list in the case of our
 * problem contains when's. This function is not currently in use. It
 * could be use to build a solver rel list of ACTIVE relations, by
 * using a master rel list with all of the relations in it. These
 * ACTIVE relations could be INCLUDED or UNINCLUDED. It returns the
 * number of relation in the list.
 */

extern int32 build_logrel_solver_from_master(struct logrel_relation **masterll,
                                             struct logrel_relation **solverll);
/**<
 * Build the logrel solver list from the master list in the case of our
 * problem contains when's. This function is not currently in use. It
 * could be use to build a solver logrel list of ACTIVE logrelations, by
 * using a master logrel list with all of the logrelations in it. These
 * ACTIVE logrelations could be INCLUDED or UNINCLUDED. It returns the
 * number of logrelation in the list.
 */

extern int32 build_var_solver_from_master(struct var_variable **mastervl,
                                          struct var_variable **solvervl);
/**<
 * Build the var solver list from the master list in the case of our
 * problem contains when's.  This function is not currently in use. It
 * could be use to build a solver var list of ACTIVE variables (Vars
 * incident in ACTIVE relations). It returns the number of variables
 * in the list.
 */

extern int32 build_disvar_solver_from_master(struct dis_discrete **masterdl,
                                             struct dis_discrete **solverdl);
/**<
 * Build the discrete var solver list from the master list in the case
 * of our problem contains when's.  This function is not currently in use.
 * It could be use to build a solver discrete var list of ACTIVE discrete
 * variables (Discrete Vars incident in ACTIVE logrelations). It returns
 * the number of discrete variables in the list.
 */

/*
 * extern void rebuild_solvers_from_masters(slv_system_t);
 *
 * For conditional modeling. This function rebuilds the solver
 * list using the master lists and the analysis of the when
 * structures present in the problem. It is not currently in
 * use. It would be used if I want only ACTIVE relations
 * (INCLUDED or NOT) and ACTIVE vars in the solver lists
 */

#endif /*  ASC_COND_CONFIG_H */


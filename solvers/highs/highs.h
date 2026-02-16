/*
 *  MPS: Ascend MPS file generator
 *  by Craig Schmidt
 *  Created: 2/11/95
 *  Version: $Revision: 1.13 $
 *  Version control file: $RCSfile: highs.h,v $
 *  Date last modified: $Date: 1997/07/18 12:16:23 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1995 Craig Schmidt
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  HiGHS solver registration module.
 *  <pre>
 *  Contents:     HiGHS module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                04/91 - fine tuned modified marquadt computation,
 *                        provided minor iterations for step generation
 *                        within each major iteration of jacobian
 *                        updates
 *                06/93 - eliminated pointer sublists being generated
 *                        at the beginning of each block
 *                04/94 - extended scope to equality constrained
 *                        optimization.
 *
 *  Description:  This file is created by make_slv_header, so don't
 *                modify it yourself.  All functions defined in this
 *                header have identical protocols to the corresponding
 *                functions in slv.h except that slv_system_t ==>
 *                highs_system_t and highs_eligible_solver() only takes one
 *                parameter: the system.  Note also that the select
 *                solver functions don't exist.
 *  </pre>
 *  @todo HiGHS (solver/highs.c) remains legacy code and needs refactoring.
 *        It currently compiles and is covered by solver tests.
 *  @todo Restructure solver/slv6 & mps so can remove declarations in
 *        solver/highs.h out of header.  Currently needed by mps.[ch].
 */

#ifndef ASC_HIGHS_H
#define ASC_HIGHS_H

#include <ascend/solver/solver.h>
#include <ascend/system/slv_client.h>

#include <ascend/system/lp_data.h>

/* Parameter token IDs used by solvers/highs/highs.c. */

typedef struct highs_system_structure *highs_system_t;

int highs_register(void);
/**<
 *  Registration function for the ASCEND HiGHS solver.
 *  This is the function that tells the system about the HiGHS solver.
 *  Our index is not necessarily going to be 6. That everything here is
 *  named highs* is just a historical event.
 *
 *  @param sff SlvFunctionsT to receive the solver registration info.
 *  @return Returns non-zero on error (e.g. f == NULL), zero if all is ok.
 */

/* WOULD LIKE TO REMOVE EVERYTHING BELOW THIS POINT */
/* THIS DETAIL SHOULD BE IN SOURCE FILE, BUT IS NEEDED BY mps.[ch] */
/*
# if 0
*/
#define highs_solver_name "HiGHS" /**< Solver's name. don't mess with the caps!*/
#define highs_solver_number 60   /**< Solver's number */

extern boolean highs_free_inc_var_filter(struct var_variable *var);
/**<
 ***  I've been calling this particular var filter a lot ,
 ***  so I decided to make it a subroutine.  Returns true if
 ***  var is not fixed and incident in something.
 **/

#if 0
extern void highs_set_var_list();
extern struct var_variable **highs_get_var_list();
extern void highs_set_bnd_list();
extern void highs_set_rel_list();
extern struct rel_relation **highs_get_rel_list();
extern void highs_set_extrel_list();
extern struct ExtRelCache **highs_get_extrel_list();
extern int highs_count_vars();
extern int highs_count_bnds();
extern int highs_count_rels();
extern void highs_set_obj_relation();
extern struct rel_relation *highs_get_obj_relation();
extern boolean highs_eligible_solver();
extern void highs_get_parameters();
extern void highs_set_parameters();
extern void highs_get_status();
//extern linsol_system_t highs_get_linsol_sys();
extern void highs_dump_internals();
extern void highs_presolve();
extern boolean highs_change_basis();
extern void highs_resolve();
extern void highs_iterate();
extern void highs_solve();
#endif

enum{
	/** ASCEND OPTIONS */
	ASCEND_PARAM_SAFEEVAL = 0
	/**< boolean-valued */
	, HIGHS_PARAM_NONLIN
	, HIGHS_PARAM_RELAXED
	, HIGHS_PARAM_PROGRESS_CALLBACKS
	/* integer-valued */
	, HIGHS_PARAM_THREADS
	, HIGHS_PARAM_RANDOM_SEED
	/* real-valued */
	, HIGHS_PARAM_TIME_LIMIT
	, HIGHS_PARAM_MIP_REL_GAP
	, HIGHS_PARAM_MIP_ABS_GAP
	, HIGHS_PARAM_PINF
	, HIGHS_PARAM_MINF
	/* string-valued */
	, HIGHS_PARAM_PRESOLVE
	, HIGHS_PARAM_SOLVER
	, HIGHS_PARAM_PARALLEL
	, HIGHS_PARAMS
};

/**< define another token to go with
   rel_TOK_less, rel_TOK_equal, and rel_TOK_greater,
   defined in rel.h */
#define rel_TOK_nonincident 00

#endif  /* ASC_HIGHS_H */

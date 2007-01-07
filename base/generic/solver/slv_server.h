/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2006 Carnegie Mellon University

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
	Server functions for the SLV solver.

	Requires:
	#include <solver/var.h>
	#include <solver/rel.h>
	#include <solver/discrete.h>
	#include <solver/conditional.h>
	#include <solver/logrel.h>
	#include <solver/bnd.h>
	#include <solver/linsol.h>
	#include <solver/linsolqr.h>
	#include <solver/slv_common.h>
	#include <solver/slv_types.h>
	#include <solver/slv_client.h>

	@NOTE
		We are going through a solver API definition restructuring.
		The appearance of a NOTE section in the header means the code in question
		has, or is about to have, a change in its meaning or is code that
		is new and replaces some or all the functionality of an old
		function definition. Basically, expect to have to read NOTE sections
		carefully and maybe patch calls dependent on them.
*//*
	by Karl Michael Westerberg
	Created: 2/6/90
 */

#ifndef ASC_SLV_SERVER_H
#define ASC_SLV_SERVER_H

#include <utilities/ascConfig.h>

extern slv_system_t slv_create(void);
/**<
	Creates a system of the currently selected type in sys.
*/

extern int slv_destroy(slv_system_t sys);
/**<
	Destroys all currently created systems in sys.

	@sys a system
	@return 0 on success, or else the number of solvers with trouble destroying if there were problems.
*/

extern SlvBackendToken slv_instance(slv_system_t sys);
/**<
	Returns the root instance of the slv system.
	@see slv_set_instance().
*/

extern void slv_set_instance(slv_system_t sys, SlvBackendToken root);
/**<
	Sets the root instance of the slv system.

	All naming within the context of the slv_system_t is
	done relative to this instance pointer.

	NOTE: THESE TWO FUNCTIONS SHOULD TAKE A VOID * AND
	THEN THE USER SHOULD CAST IT BACK TO WHATEVER IS
	NEEDED GIVEN THE KNOWLEDGE OF THE BACK END IN QUESTION.
*/

extern void slv_set_num_models(slv_system_t sys, int32 nmod);
/**<
	Sets the number of models associated with a system.

	@param sys a system
	@param nmod number of models associated with the system
*/

extern void slv_set_need_consistency(slv_system_t sys, int32 need_consistency);
/**<
	Sets the int need_consistency associated with the system.
*/

extern void slv_set_master_var_list(slv_system_t sys,
                                    struct var_variable **vlist,
                                    int size);
/**<  Set the master variable list to vlist of length size. */

extern void slv_set_master_par_list(slv_system_t sys,
                                    struct var_variable **vlist,
                                    int size);
/**<  Set the master parameter list to vlist of length size. */

extern void slv_set_master_unattached_list(slv_system_t sys,
                                           struct var_variable **vlist,
                                           int size);
/**<  Set the master unattached variable list to vlist of length size. */

extern void slv_set_master_dvar_list(slv_system_t sys,
                                     struct dis_discrete **dvlist,
                                     int size);
/**<  Set the master discrete variable list to dvlist of length size. */

extern void slv_set_master_disunatt_list(slv_system_t sys,
                                         struct dis_discrete **dvlist,
                                         int size);
/**<  Set the master unattached discrete variable list to dvlist of length size. */

extern void slv_set_master_rel_list(slv_system_t sys,
                                    struct rel_relation **rlist,
                                    int size);
/**<  Set the master relation list to rlist of length size. */

extern void slv_set_master_condrel_list(slv_system_t sys,
                                        struct rel_relation **rlist,
                                        int size);
/**<  Set the master conditional relation list to rlist of length size. */

extern void slv_set_master_obj_list(slv_system_t sys,
                                    struct rel_relation **rlist,
                                    int size);
/**<  Set the master objective relation list to rlist of length size. */

extern void slv_set_master_logrel_list(slv_system_t sys,
                                       struct logrel_relation **lrlist,
                                       int size);
/**<  Set the master logical relation list to lrlist of length size. */

extern void slv_set_master_condlogrel_list(slv_system_t sys,
                                           struct logrel_relation **lrlist,
                                           int size);
/**<  Set the master conditional logical relation list to lrlist of length size. */

extern void slv_set_master_when_list(slv_system_t sys,
                                     struct w_when **wlist,
                                     int size);
/**<  Set the master when list to wlist of length size. */

extern void slv_set_master_bnd_list(slv_system_t sys,
                                    struct bnd_boundary **wlist,
                                    int size);
/**<
	Set the master boundaries list to blist of length size.

	@NOTE
		There are now 2 lists: the master  list pulled of the instance
		tree, and the solvers  list is to be handed to the solvers.
		Eventually the solvers_list will only include those elements the specific
		solver needs to know about.
		For the moment,the content of the two lists is the same, but the ordering
		is not. The master list is in the order collected. The solvers list
		is reordered in some useful fashion defined elsewhere.
*/

extern void slv_set_symbol_list(slv_system_t sys, struct gl_list_t *sv);
/**<
	Set gllist of SymbolValues struct to sys.
	They are used to assign an integer value to a symbol value
*/

extern void slv_set_var_buf(slv_system_t sys, struct var_variable *vbuf);
/**<
	Set the array variables of for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_par_buf(slv_system_t sys, struct var_variable *pbuf);
/**<
	Set the array of parameters for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_unattached_buf(slv_system_t sys, struct var_variable *ubuf);
/**<
	Set the array of unattached variables for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_dvar_buf(slv_system_t sys, struct dis_discrete *dbuf, int len);
/**<
	Set the array of discrete variables for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_disunatt_buf(slv_system_t sys, struct dis_discrete *udbuf);
/**<
	Set the array of unattached discrete variables for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_rel_buf(slv_system_t sys, struct rel_relation *rbuf);
/**<
	Set the array of relations for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_condrel_buf(slv_system_t sys, struct rel_relation *cbuf);
/**<
	Set the array of conditional relations for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_obj_buf(slv_system_t sys, struct rel_relation *obuf);
/**<
	Set the array of objective relations for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_logrel_buf(slv_system_t sys, struct logrel_relation *lbuf);
/**<
	Set the array of logical relations for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_condlogrel_buf(slv_system_t sys, struct logrel_relation *clbuf);
/**<
	Set the array of conditional logical relations for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_when_buf(slv_system_t sys, struct w_when *wbuf, int len);
/**<
	Set the array of whens for the system.
	See slv_set_bnd_buf() for more information.
*/

extern void slv_set_bnd_buf(slv_system_t sys, struct bnd_boundary *bbuf, int len);
/**<
	Set the array of boundaries for the system.

	slv_set_*_buf(sys,array);
	You should give these functions a pointer to an array of
	the var or rel structure. These arrays should contain the
	memory pointed to by any and all pointers in the var/rel/when/bdn
	lists associated with the slv_system_t. If you have none
	of a particular type, its buf may be NULL.

	There is no way to RETRIEVE these -- they are destroyed with
	the slv_system_t. You should have no individually allocated
	vars/rels in the system: these are the only ones we will free.

	Calling any of these twice on the system is an extreme error.
*/

extern void slv_set_incidence(slv_system_t sys,
                              struct var_variable **array,
                              long size);
/**<
	Set the list of variables incident in a relation.

	You should give this function a pointer to an array of
	struct var_variable *. This array should contain the
	memory pointed to by any and all pointers in rel incidence
	lists associated with the slv_system_t.
	There is no way to RETRIEVE these -- they are destroyed with
	the slv_system_t. You should have no individually allocated
	rel incidence in the system: this is the only one we will free.
	Size is the length of the array, which we want for accounting.

	Calling this function twice on the system is an extreme error.
*/

extern void slv_set_var_incidence(slv_system_t sys,
                                  struct rel_relation **array,
                                  long size);
/**<
	Set the list of logical relations in which a discrete variable is
	incident. Calling this function twice on the system is an extreme error.
	See slv_set_var_incidence() for more information.
*/

extern void slv_set_logincidence(slv_system_t sys,
                                 struct dis_discrete **array,
                                 long size);
/**<
	Set the list of relations in which a variable is incident.
	Calling this function twice on the system is an extreme error.
	See slv_set_var_incidence() for more information.
*/

extern void slv_set_extrel_list(slv_system_t sys,
                                struct ExtRelCache **rlist,
                                int size);
/**<
	The solver will merely reference the list, it will not make
	its own private copy of the list, nor will it destroy the list
	when the system is destroyed. This is a list of pointers to
	struct ExtRelCache's. These external rel caches provide convenient
	and quick access to information needed by external relations. All
	external relations with the same node stamp will point to the same
	ExtRelCache. The extrel list contains only unique ExtRelCaches though
	the list is not sorted. Size is the number of such ExtRelCaches.
	Calling slv_set_extrel_list twice on the same system is fatal.
*/

extern struct ExtRelCache **slv_get_extrel_list(slv_system_t sys);
/**<
	Returns the most recently set extrel list (or NULL if it is empty).
	This is for the convenience of those who were not around when it
	was initially set.
*/

extern int slv_get_num_extrels(slv_system_t sys);
/**<  Returns the size of the most recently set extrel list. */

ASC_DLLSPEC int32 slv_obj_select_list(slv_system_t sys, int32 **rip);
/**<
	Allocates rip and fills it with solver objective list
	positions of included objectives.
	The rip list is terminated with a -1.
	Returns number of objectives found.
	The calling function must free rip.
*/

ASC_DLLSPEC int32 slv_get_obj_num(slv_system_t sys);
/**<
	Returns the solver list index of the current objective.
	If the objective is NULL then -1 is returned.
*/

ASC_DLLSPEC int32 slv_near_bounds(slv_system_t sys, real64 epsilon, int32 **rip);
/**<
	Allocates rip and fills it with:
	-#  the number of vars close to lower bounds
	-#  the number of vars close to upper bounds
	-#  solver variable list positions of close to UB.
	-#  solver variable list positions of close to LB.
	-#  Returns number of variables close to bounds.

	A variable is close to its bound if
	abs(value - bound)/nominal < epsilon

	@par The calling function must free rip.
*/

ASC_DLLSPEC int32 slv_far_from_nominals(slv_system_t sys, real64 bignum, int32 **rip);
/**<
	Allocates rip and fills it with
	solver variable list positions of variables far
	from nominal values.

	Test used: abs(value - nominal)/nominal > bignum
	<br><br>
	The calling function must free rip.
*/

#endif  /* ASC_SLV_SERVER_H */


/*
 *  SLV: Ascend Nonlinear Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.22 $
 *  Version control file: $RCSfile: slv_server.h,v $
 *  Date last modified: $Date: 1998/03/30 22:07:09 $
 *  Last modified by: $Author: rv2a $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *  Copyright (C) 1996 Benjamin Andrew Allan
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
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */
#ifndef slv_server_h__already_included
#define slv_server_h__already_included

/* requires #include "base.h" */
/* requires #include "var.h" */
/* requires #include "rel.h" */
/* requires #include "discrete.h" */
/* requires #include "conditional.h" */
/* requires #include "logrel.h" */
/* requires #include "bnd.h" */
/* requires #include "linsol.h" */
/* requires #include "linsolqr.h" */
/* requires #include "slv_common.h" */
/* requires #include "slv_types.h" */
/* requires #include "slv_client.h" */

/***  ! ! We are going through a solver API definition restructuring.
 ! !  The appearance of ! ! in the header means the code in question
 ! !  has, or is about to have, a change in its meaning or is code that
 ! !  is new and replaces some or all the functionality of an old
 ! !  function definition. Basically, expect to have to read ! ! sections
 ! !  carefully and maybne patch calls dependent on them.
 **/

extern slv_system_t slv_create(void);
/**
 ***  sys = slv_create();
 ***  slv_system_t sys;
 ***
 ***  Creates a system of the currently selected type in sys.
 **/

extern int slv_destroy(slv_system_t);
/**
 ***  slv_destroy(sys)
 ***  slv_system_t sys;
 ***
 ***  Destroys all currently created systems in sys.
 ***  returns n if something untoward happens in destroy process.
 ***  returns 0 normally.
 ***  n is the total number of solvers with trouble destroying.
 **/

extern SlvBackendToken slv_instance(slv_system_t);
extern void slv_set_instance(slv_system_t, SlvBackendToken);
/**
 ***  returns or sets the root instance of the slv system.
 ***  All naming within the context of the slv_system_t is
 ***  done relative to this instance pointer.
 ***
 ***  NOTE: THESE TWO FUNCTIONS SHOULD TAKE A VOID * AND
 ***  THEN THE USER SHOULD CAST IT BACK TO WHATEVER IS
 ***  NEEDED GIVEN THE KNOWLEDGE OF THE BACK END IN QUESTION.
 **/

extern void slv_set_num_models(slv_system_t,int32);
/**
 ***  slv_set_num_models(sys,nmod);
 ***  Sets the number of models associated with the system to nmod.
 **/

extern void slv_set_need_consistency(slv_system_t, int32);
/*   
 *  slv_set_need_consistency(sys,need_consistency);
 *  slv_need_consistency(sys);
 *  Sets the int need_consitency associated with the system.
 */

extern void slv_set_master_var_list(slv_system_t, struct var_variable **,int);
extern void slv_set_master_par_list(slv_system_t, struct var_variable **,int);
extern void slv_set_master_unattached_list(slv_system_t,
                                           struct var_variable **,int);
extern void slv_set_master_dvar_list(slv_system_t, 
                                     struct dis_discrete **,int);
extern void slv_set_master_disunatt_list(slv_system_t,
					 struct dis_discrete **,int);
extern void slv_set_master_rel_list(slv_system_t, struct rel_relation **,int);
extern void slv_set_master_condrel_list(slv_system_t,
					struct rel_relation **,int);
extern void slv_set_master_obj_list(slv_system_t, struct rel_relation **,int);
extern void slv_set_master_logrel_list(slv_system_t, 
                                       struct logrel_relation **,
				       int);
extern void slv_set_master_condlogrel_list(slv_system_t, 
                                           struct logrel_relation **,
				           int);
extern void slv_set_master_when_list(slv_system_t, struct w_when **,int);
extern void slv_set_master_bnd_list(slv_system_t, struct bnd_boundary **,int);
/**
 ! !  slv_set_master_*_list(sys,list,size)
 ***  slv_system_t sys;
 ***  struct var_variable **vlist;
 ***  struct rel_relation **rlist;
 ***  struct dis_discrete **dvlist;
 ***  struct logrel_relation **lrlist;
 ***  struct w_when **wlist;
 ***  struct bnd_boundary **wlist;
 ***  int size;
 ***
 ***  Set the master list of the given type.
 ***
 ! !
 ! !  There are now 2 lists: the master  list pulled of the instance
 ! !  tree, and the solvers  list is to be handed to the solvers.
 ! !  Eventually the solvers_list will only include those elements the specific
 ! !  solver needs to know about.
 ! !  For the moment,the content of the two lists is the same, but the ordering
 ! !  is not. The master list is in the order collected. The solvers list
 ! !  is reordered in some useful fashion defined elsewhere.
 **/

extern void slv_set_symbol_list(slv_system_t, struct gl_list_t *);
/*
 * set gllist of SymbolValues struct to sys. they are used to assign an
 * integer value to a symbol value
 */

extern void slv_set_var_buf(slv_system_t, struct var_variable *);
extern void slv_set_par_buf(slv_system_t, struct var_variable *);
extern void slv_set_unattached_buf(slv_system_t, struct var_variable *);
extern void slv_set_dvar_buf(slv_system_t, struct dis_discrete *,int);
extern void slv_set_disunatt_buf(slv_system_t, struct dis_discrete *);
extern void slv_set_rel_buf(slv_system_t, struct rel_relation *);
extern void slv_set_condrel_buf(slv_system_t, struct rel_relation *);
extern void slv_set_obj_buf(slv_system_t, struct rel_relation *);
extern void slv_set_logrel_buf(slv_system_t, struct logrel_relation *);
extern void slv_set_condlogrel_buf(slv_system_t, struct logrel_relation *);
extern void slv_set_when_buf(slv_system_t, struct w_when *,int);
extern void slv_set_bnd_buf(slv_system_t, struct bnd_boundary *,int);
/** 
 ***  slv_set_*_buf(sys,array);
 ***  You should give these functions a pointer to an array of
 ***  the var or rel structure. These arrays should contain the
 ***  memory pointed to by any and all pointers in the var/rel/when/bdn
 ***  lists associated with the slv_system_t. If you have none
 ***  of a particular type, its buf may be NULL.
 ***  There is no way to RETRIEVE these -- they are destroyed with
 ***  the slv_system_t. You should have no individually allocated
 ***  vars/rels in the system: these are the only ones we will free.
 ***
 ***  Calling any of these twice on the system is an extreme error.
 **/

extern void slv_set_incidence(slv_system_t, struct var_variable **,long);
extern void slv_set_var_incidence(slv_system_t, struct rel_relation **,long);
extern void slv_set_logincidence(slv_system_t, struct dis_discrete **,long);
/** 
 ***  slv_set_incidence(sys,array,size);
 ***  slv_set_var_incidence(sys,array,size);
 ***  slv_set_logincidence(sys,array,size);
 ***  
 ***  slv_set_incidence:
 ***  You should give this function a pointer to an array of
 ***  struct var_variable *. This array should contain the
 ***  memory pointed to by any and all pointers in rel incidence
 ***  lists associated with the slv_system_t.
 ***  There is no way to RETRIEVE these -- they are destroyed with
 ***  the slv_system_t. You should have no individually allocated
 ***  rel incidence in the system: this is the only one we will free.
 ***  Size is the length of the array, which we want for accounting.
 ***  slv_set_logincidence:
 ***  slv_set_logincidence is the equivalent of slv_set_incidence but
 ***  using logrels instead of rels and discrete vars instead of real
 ***  vars.
 ***  slv_set_var_incidence:
 ***  corresponds to the list of relations in which a variable is incident
 ***  as opposed to slv_set_incidence which is the list of variables incident
 ***  in a relation.
 ***
 ***  Calling any of this twice on the system is an extreme error.
 **/

extern void slv_set_extrel_list(slv_system_t,struct ExtRelCache **,int);
extern struct ExtRelCache **slv_get_extrel_list(slv_system_t);
extern int slv_get_num_extrels(slv_system_t);
/**
 ***  slv_set_extrel_list(sys,rlist,size)
 ***  erlist = slv_get_extrel_list(sys)
 ***
 ***  slv_system_t sys;
 ***  struct ExtRelCache **erlist;
 ***  int size;
 ***
 ***  slv_set_extrel_list sets the external rellsit of the solver to the given
 ***  one.  The solver will merely reference the list, it will not
 ***  make its own private copy of the list, nor will it destroy the
 ***  list when the system is destroyed. This is a list of pointers to
 ***  struct ExtRelCache's. These external rel caches provide convenient
 ***  and quick access to information needed by external relations. All
 ***  external relations with the same node stamp will point to the same
 ***  ExtRelCache. The extrel list contains only unique ExtRelCaches though
 ***  the list is not sorted. Size is the number of such ExtRelCaches.
 ***  Calling slv_set_extrel_list twice on the same system is fatal.
 ***
 ***  slv_get_extrel_list will return the most recently set extrel list (or
 ***  NULL if it is empty) for the convenience of those who were not around
 ***  when it was initially set.
 **/

extern int32 slv_obj_select_list(slv_system_t, int32 **);
/**
 ***  count = slv_obj_select_list(sys, rip)
 ***  int32 count;
 ***  slv_system_t sys;
 ***  int32 **rip;
 ***
 ***  slv_obj_select_list allocates rip and fills it with solver
 ***  objective list positions of included objectives.
 ***  The rip list is terminated with a -1.
 ***  Returns number of objectives found.
 ***  The calling function must free rip.
 **/

extern int32 slv_get_obj_num(slv_system_t);
/**
 ***  num = slv_get_obj_num(sys)
 ***  int32 num;
 ***  slv_system_t sys;
 ***
 ***  slv_get_obj_num returns the solver list index
 ***  of the current objective.  If the objective is
 ***  NULL then -1 is returned.
 **/

extern int32 slv_near_bounds(slv_system_t, real64, int32 **);
/**
 ***  count = slv_near_bounds(sys, epsilon, rip)
 ***  int32 count;
 ***  real64 epsilon;
 ***  slv_system_t sys;
 ***  int32 **rip;
 ***
 ***  slv_check bounds allocates rip and fills it with
 ***  1) the number of vars close to lower bounds
 ***  2) the number of vars close to upper bounds
 ***  3) solver variable list positions of close to UB.
 ***  4) solver variable list positions of close to LB.
 ***  5) Returns number of variables close to bounds.
 ***
 ***  A variable is close to its bound if
 ***  abs(value - bound)/nominal < epsilon
 ***
 ***  The calling function must free rip.
 **/

extern int32 slv_far_from_nominals(slv_system_t, real64, int32 **);
/**
 ***  count = slv_far_from_nominals(sys, bignum, rip)
 ***  int32 count;
 ***  real64 bignum;
 ***  slv_system_t sys;
 ***  int32 **rip;
 ***
 ***  slv_check bounds allocates rip and fills it with
 ***  solver variable list positions of variables far
 ***  from nominal values.
 ***
 ***  Test used: abs(value - nominal)/nominal > bignum
 ***
 ***  The calling function must free rip.


 **/


#endif

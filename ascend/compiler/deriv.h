/*	ASCEND modelling environment
	Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
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
*//** @file
	Routines for managing pointers between derivative, state and independent
	variable instances.
*//*
	by Ksenija Bestuzheva (GSOC 2012)
	Created: 27/06/2012
*/

#include "instance_enum.h"
#include "name.h"

extern
void SetDerInfo(struct Instance *deriv, struct Instance *state, struct Instance *indep);
/**<
  *  Creates pointers between the instances. All the instances should be of
  *  type REAL_ATOM_INST.
  */

ASC_DLLSPEC
struct Instance *FindDerByArgs(struct Instance *state, struct Instance *indep);
/**<
 *  Finds a derivative given the state and independent variables. All the instances
 *  should be of type REAL_ATOM_INST.
 */

extern
void ModifyIderivPointers(struct Instance *deriv,
                          struct gl_list_t *indlist,
                          CONST struct Instance *old,
                          CONST struct Instance *inew);
/**<
 *  Change the independent variable in a derivative from old to new.
 */

extern
void ModifyStatePointers(struct Instance *state,
                         struct gl_list_t *derlist,
                         CONST struct Instance *old,
                         CONST struct Instance *inew);
/**<
 *  Change the derivative in the list contained by the state variable from old to new.
 */

extern
void ModifyIndepPointers(struct Instance *deriv,
                          struct gl_list_t *indlist,
                          CONST struct Instance *old,
                          CONST struct Instance *inew);
/**<
 *  Change the derivative in the list contained by the independent variable from old to new.
 */

extern
void ModifySderivPointers(struct Instance *deriv,
                          struct gl_list_t *stlist,
                          CONST struct Instance *old,
                          CONST struct Instance *inew);
/**<
 *  Change the state variable in a derivative from old to new.
 */

ASC_DLLSPEC
void WriteDerInfo(FILE *f, struct Instance *inst);
/**<
 *  Output the lists of derivative, independent and state variable
 *  instances connected with inst.
 */

ASC_DLLSPEC
int IsDeriv(struct Instance *inst);
/**<
 *  If the instance in a derivative, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
int IsState(struct Instance *inst);
/**<
 *  If the instance in a state variable, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
int IsIndep(struct Instance *inst);
/**<
 *  If the instance in an independent variable, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
struct gl_list_t *StateVars(struct Instance *inst);
/**<
 *  Return a list of state variables for the given derivative. If inst is not
 *  a derivative, return NULL. Destruction of the returned list is the
 *  responsibility of the caller, but be careful to call gl_destroy(), not
 *  gl_free_and_destroy().
 */

ASC_DLLSPEC
struct gl_list_t *Sderivs(struct Instance *inst);
/**<
 *  Return a list of derivatives for the given instance. If inst does not
 *  have derivatives, return NULL. Destruction of the returned list is the
 *  responsibility of the caller, but be careful to call gl_destroy(), not
 *  gl_free_and_destroy().
 */

ASC_DLLSPEC
struct gl_list_t *Iderivs(struct Instance *inst);
/**<
 *  Return a list of derivatives with respect to the given instance. If inst does not
 *  have derivatives with respect to it, return NULL. Destruction of the returned list is the
 *  responsibility of the caller, but be careful to call gl_destroy(), not
 *  gl_free_and_destroy().
 */

ASC_DLLSPEC
struct gl_list_t *IndepVars(struct Instance *inst);
/**<
 *  Return a list of independent variables for the given derivative. If inst
 *  is not a derivative, return NULL. Destruction of the returned list is the
 *  responsibility of the caller, but be careful to call gl_destroy(), not
 *  gl_free_and_destroy().
 */


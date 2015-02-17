/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//** @defgroup system_jacobian System Jacobian
	General-purpose jacobian routines that should hopefully be shareable
	but at this stage are only going to be used by IDA.
*/
#ifndef ASC_SYS_JACOBIAN_H
#define ASC_SYS_JACOBIAN_H

#include "var.h"
#include "rel.h"

#include <ascend/linear/mtx.h>

/**	@addtogroup system_jacobian
	@{
*/

ASC_DLLSPEC const rel_filter_t system_rfilter_algeb;
ASC_DLLSPEC const rel_filter_t system_rfilter_diff;
ASC_DLLSPEC const rel_filter_t system_rfilter_all;

ASC_DLLSPEC const var_filter_t system_vfilter_algeb;
ASC_DLLSPEC const var_filter_t system_vfilter_diff;
ASC_DLLSPEC const var_filter_t system_vfilter_deriv;
ASC_DLLSPEC const var_filter_t system_vfilter_nonderiv;


/**
	This mischevious data structure is a quick dodge to avoid having to
	set some some big complicated slv_system_t structure for the purpose of
	reporting back on a matrix slice. It seems that the better way to do 
	all this would be using a mtx_matrix_t with rol/col permutations mapping
	back to the solvers_var/rel_list.

	@DEPRECATED from day one, therefore. Only used by IDA at this stage.
*/
struct SystemJacobianStruct{
	mtx_matrix_t M;
	struct rel_relation **rels;
	struct var_variable **vars;
	int n_rels, n_vars;
};

/**
	Create a new matrix and put into it the the derivatives for the rels
	and vars matching the filter. Return the list of rels and vars.

	This routine uses relman_diff2 to calculate derivatives and
	mtx_set_value to insert them into the matrix. 

	It's not in any way optimised for fast evaluation / mtx updates etc.
*/
ASC_DLLSPEC int system_jacobian(slv_system_t sys
	, const rel_filter_t *rfilter, const var_filter_t *vfilter, const int safe
	, struct SystemJacobianStruct *sysjac
);

#endif

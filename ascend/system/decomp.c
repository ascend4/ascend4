/*	ASCEND modelling environment
	Copyright (C) 2026

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.
*/

#include "decomp.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/general/mathmacros.h>
#include <ascend/linear/mtx_basic.h>
#include <ascend/linear/mtx_perms.h>
#include <ascend/linear/mtx_reorder.h>

#include "bnd.h"
#include "conditional.h"
#include "discrete.h"
#include "logrel.h"
#include "rel.h"
#include "slv_client.h"
#include "var.h"

static void decomp_clear_owned(slv_decomp_partition_t *decomp){
	if(decomp == NULL){
		return;
	}
	if(decomp->blocks != NULL){
		ascfree(decomp->blocks);
	}
	if(decomp->row_org != NULL){
		ascfree(decomp->row_org);
	}
	if(decomp->col_org != NULL){
		ascfree(decomp->col_org);
	}
	if(decomp->row_cur != NULL){
		ascfree(decomp->row_cur);
	}
	if(decomp->col_cur != NULL){
		ascfree(decomp->col_cur);
	}
	if(decomp->nz_rows != NULL){
		ascfree(decomp->nz_rows);
	}
	if(decomp->nz_cols != NULL){
		ascfree(decomp->nz_cols);
	}
	slv_decomp_init(decomp);
}

void slv_decomp_init(slv_decomp_partition_t *decomp){
	if(decomp == NULL){
		return;
	}
	decomp->nblocks = 0;
	decomp->blocks = NULL;
	decomp->rank = 0;
	decomp->n_rows = 0;
	decomp->n_cols = 0;
	decomp->n_rels = 0;
	decomp->n_condrels = 0;
	decomp->n_logrels = 0;
	decomp->n_condlogrels = 0;
	decomp->n_vars = 0;
	decomp->n_dvars = 0;
	decomp->nnz = 0;
	decomp->nz_rows = NULL;
	decomp->nz_cols = NULL;
	decomp->row_org = NULL;
	decomp->col_org = NULL;
	decomp->row_cur = NULL;
	decomp->col_cur = NULL;
}

void slv_decomp_destroy(slv_decomp_partition_t *decomp){
	decomp_clear_owned(decomp);
}

static int decomp_var_col(slv_system_t sys, const struct var_variable *var){
	struct var_variable **vars;
	int32 nvars, i, sindex;
	if(sys == NULL || var == NULL){
		return -1;
	}
	vars = slv_get_solvers_var_list(sys);
	nvars = slv_get_num_solvers_vars(sys);
	sindex = var_sindex(var);
	if(sindex >= 0 && sindex < nvars && vars[sindex] == var){
		return sindex;
	}
	for(i = 0; i < nvars; ++i){
		if(vars[i] == var){
			return i;
		}
	}
	return -1;
}

static int decomp_dvar_col(slv_system_t sys, const struct dis_discrete *dvar){
	struct dis_discrete **dvars;
	int32 ndvars, i, sindex;
	if(sys == NULL || dvar == NULL){
		return -1;
	}
	dvars = slv_get_solvers_dvar_list(sys);
	ndvars = slv_get_num_solvers_dvars(sys);
	sindex = dis_sindex(dvar);
	if(sindex >= 0 && sindex < ndvars && dvars[sindex] == dvar){
		return slv_get_num_solvers_vars(sys) + sindex;
	}
	for(i = 0; i < ndvars; ++i){
		if(dvars[i] == dvar){
			return slv_get_num_solvers_vars(sys) + i;
		}
	}
	return -1;
}

static struct dis_discrete *decomp_resolve_dvar_selector(
		slv_system_t sys, const void *selector
){
	struct dis_discrete **dvars;
	int32 ndvars, i;
	if(sys == NULL || selector == NULL){
		return NULL;
	}
	dvars = slv_get_solvers_dvar_list(sys);
	ndvars = slv_get_num_solvers_dvars(sys);
	for(i = 0; i < ndvars; ++i){
		if(dvars[i] == selector || dis_instance(dvars[i]) == selector){
			return dvars[i];
		}
	}
	return NULL;
}

static int decomp_var_usable(const struct var_variable *var){
	return var != NULL
		&& var_incident((struct var_variable *)var)
		&& (var_flags((struct var_variable *)var) & VAR_SVAR)
		&& !var_fixed((struct var_variable *)var)
		&& var_active((struct var_variable *)var);
}

static int decomp_dvar_usable(const struct dis_discrete *dvar){
	uint32 flags;
	if(dvar == NULL){
		return 0;
	}
	flags = dis_flags(dvar);
	return dvar != NULL
		&& (!(flags & DIS_FIXED) || !(flags & DIS_BVAR))
		&& (!(flags & DIS_CONST) || (flags & DIS_INWHEN))
		&& ((flags & DIS_INCIDENT)
			|| (flags & DIS_INWHEN)
			|| (flags & DIS_ACTIVE));
}

static int decomp_dvar_selector_usable(const struct dis_discrete *dvar){
	uint32 flags;
	if(dvar == NULL){
		return 0;
	}
	flags = dis_flags(dvar);
	return (!(flags & DIS_FIXED) || !(flags & DIS_BVAR));
}

static void decomp_record_org(slv_decomp_partition_t *decomp, int32 row, int32 col){
	int32 *new_rows, *new_cols;
	int32 next;
	if(decomp == NULL || row < 0 || col < 0){
		return;
	}
	next = decomp->nnz + 1;
	new_rows = ASC_REALLOC(decomp->nz_rows,sizeof(int32) * next);
	if(new_rows == NULL){
		return;
	}
	decomp->nz_rows = new_rows;
	new_cols = ASC_REALLOC(decomp->nz_cols,sizeof(int32) * next);
	if(new_cols == NULL){
		return;
	}
	decomp->nz_cols = new_cols;
	decomp->nz_rows[decomp->nnz] = row;
	decomp->nz_cols[decomp->nnz] = col;
	decomp->nnz = next;
}

static void decomp_add_org(mtx_matrix_t mtx, slv_decomp_partition_t *decomp,
		int32 row, int32 col
){
	mtx_coord_t nz;
	if(row < 0 || col < 0){
		return;
	}
	nz.row = row;
	nz.col = col;
	mtx_fill_org_value(mtx,&nz,1.0);
	decomp_record_org(decomp,row,col);
}

static void decomp_add_rel_incidences(
		slv_system_t sys, mtx_matrix_t mtx, struct rel_relation *rel,
		slv_decomp_partition_t *decomp, int32 row
){
	const struct var_variable **vars;
	int32 i, len, col;
	if(rel == NULL){
		return;
	}
	vars = rel_incidence_list(rel);
	len = rel_n_incidences(rel);
	for(i = 0; i < len; ++i){
		if(decomp_var_usable(vars[i])){
			col = decomp_var_col(sys,vars[i]);
			decomp_add_org(mtx,decomp,row,col);
		}
	}
}

static void decomp_add_logrel_dvars(
		slv_system_t sys, mtx_matrix_t mtx, struct logrel_relation *logrel,
		slv_decomp_partition_t *decomp, int32 row
){
	const struct dis_discrete **dvars;
	int32 i, len, col;
	if(logrel == NULL){
		return;
	}
	dvars = logrel_incidence_list(logrel);
	len = logrel_n_incidences(logrel);
	for(i = 0; i < len; ++i){
		if(decomp_dvar_usable(dvars[i])){
			col = decomp_dvar_col(sys,dvars[i]);
			decomp_add_org(mtx,decomp,row,col);
		}
	}
}

static int decomp_gl_contains_ptr(struct gl_list_t *list, void *ptr){
	unsigned long i, len;
	if(list == NULL || ptr == NULL){
		return 0;
	}
	len = gl_length(list);
	for(i = 1; i <= len; ++i){
		if(gl_fetch(list,i) == ptr){
			return 1;
		}
	}
	return 0;
}

static int decomp_case_contains_rel(struct gl_list_t *list, struct rel_relation *rel){
	unsigned long i, len;
	if(list == NULL || rel == NULL){
		return 0;
	}
	len = gl_length(list);
	for(i = 1; i <= len; ++i){
		struct rel_relation *item = gl_fetch(list,i);
		if(item == rel || rel_instance(item) == rel_instance(rel)){
			return 1;
		}
	}
	return 0;
}

static int decomp_case_contains_logrel(
		struct gl_list_t *list, struct logrel_relation *logrel
){
	unsigned long i, len;
	if(list == NULL || logrel == NULL){
		return 0;
	}
	len = gl_length(list);
	for(i = 1; i <= len; ++i){
		struct logrel_relation *item = gl_fetch(list,i);
		if(item == logrel || logrel_instance(item) == logrel_instance(logrel)){
			return 1;
		}
	}
	return 0;
}

static void decomp_add_boundary_for_logrel(
		slv_system_t sys, mtx_matrix_t mtx, struct logrel_relation *logrel,
		slv_decomp_partition_t *decomp, int32 row
){
	struct bnd_boundary **bnds;
	int32 i, nbnds;
	if(logrel == NULL){
		return;
	}
	bnds = slv_get_solvers_bnd_list(sys);
	nbnds = slv_get_num_solvers_bnds(sys);
	for(i = 0; i < nbnds; ++i){
		struct bnd_boundary *bnd = bnds[i];
		if(!decomp_gl_contains_ptr(bnd_logrels(bnd),logrel)){
			continue;
		}
		if(bnd_kind(bnd) == e_bnd_rel){
			decomp_add_rel_incidences(sys,mtx,bnd_real_cond(bnd),decomp,row);
		}else if(bnd_kind(bnd) == e_bnd_logrel){
			decomp_add_logrel_dvars(sys,mtx,bnd_log_cond(bnd),decomp,row);
		}
	}
}

static void decomp_add_when_selectors(
		slv_system_t sys, mtx_matrix_t mtx, struct gl_list_t *selectors,
		slv_decomp_partition_t *decomp, int32 row
){
	unsigned long i, len;
	if(selectors == NULL){
		return;
	}
	len = gl_length(selectors);
	for(i = 1; i <= len; ++i){
		struct dis_discrete *dvar = decomp_resolve_dvar_selector(
			sys,gl_fetch(selectors,i)
		);
		if(decomp_dvar_selector_usable(dvar)){
			decomp_add_org(mtx,decomp,row,decomp_dvar_col(sys,dvar));
		}
	}
}

static void decomp_add_all_when_selectors(
		slv_system_t sys, mtx_matrix_t mtx, slv_decomp_partition_t *decomp,
		int32 row
){
	struct w_when **whens;
	int32 i, nwhens;
	whens = slv_get_solvers_when_list(sys);
	nwhens = slv_get_num_solvers_whens(sys);
	for(i = 0; i < nwhens; ++i){
		if(whens[i] != NULL){
			decomp_add_when_selectors(sys,mtx,when_dvars_list(whens[i]),decomp,row);
		}
	}
}

static int decomp_add_when_edges_for_object(
		slv_system_t sys, mtx_matrix_t mtx, void *object, int is_logrel,
		slv_decomp_partition_t *decomp, int32 row
){
	struct w_when **whens;
	int32 i, nwhens;
	int added = 0;
	whens = slv_get_solvers_when_list(sys);
	nwhens = slv_get_num_solvers_whens(sys);
	for(i = 0; i < nwhens; ++i){
		struct w_when *when = whens[i];
		struct gl_list_t *cases;
		unsigned long c, ncases;
		if(when == NULL){
			continue;
		}
		cases = when_cases_list(when);
		if(cases == NULL){
			continue;
		}
		ncases = gl_length(cases);
		for(c = 1; c <= ncases; ++c){
			struct when_case *wc = gl_fetch(cases,c);
			struct gl_list_t *items = is_logrel
				? when_case_logrels_list(wc)
				: when_case_rels_list(wc);
			if((is_logrel && decomp_case_contains_logrel(items,object))
				|| (!is_logrel && decomp_case_contains_rel(items,object))
			){
				decomp_add_when_selectors(sys,mtx,when_dvars_list(when),decomp,row);
				added = 1;
				break;
			}
		}
	}
	return added;
}

typedef enum decomp_partition_mode {
	DECOMP_CONSERVATIVE,
	DECOMP_ACTIVE
} decomp_partition_mode_t;

static int decomp_rel_row_current(const struct rel_relation *rel,
		decomp_partition_mode_t mode
){
	if(rel == NULL || !rel_included((struct rel_relation *)rel)
			|| !rel_equality((struct rel_relation *)rel)){
		return 0;
	}
	if(mode == DECOMP_ACTIVE){
		return rel_active((struct rel_relation *)rel);
	}
	return rel != NULL && rel_included((struct rel_relation *)rel)
		&& (rel_active((struct rel_relation *)rel)
			|| rel_in_when((struct rel_relation *)rel))
		&& rel_equality((struct rel_relation *)rel);
}

static int decomp_logrel_row_current(const struct logrel_relation *logrel,
		decomp_partition_mode_t mode
){
	if(logrel == NULL || !logrel_included((struct logrel_relation *)logrel)){
		return 0;
	}
	if(mode == DECOMP_ACTIVE){
		return logrel_active((struct logrel_relation *)logrel);
	}
	return logrel != NULL && logrel_included((struct logrel_relation *)logrel)
		&& (logrel_active((struct logrel_relation *)logrel)
			|| logrel_in_when((struct logrel_relation *)logrel));
}

static int decomp_copy_blocks_and_perms(
		mtx_matrix_t mtx, slv_decomp_partition_t *decomp
){
	int32 i;
	decomp->nblocks = mtx_number_of_blocks(mtx);
	if(decomp->nblocks > 0){
		decomp->blocks = ASC_NEW_ARRAY(mtx_region_t,decomp->nblocks);
		if(decomp->blocks == NULL){
			return 2;
		}
		for(i = 0; i < decomp->nblocks; ++i){
			mtx_block(mtx,i,&(decomp->blocks[i]));
		}
	}
	if(decomp->n_rows > 0){
		decomp->row_org = ASC_NEW_ARRAY(int32,decomp->n_rows);
		if(decomp->row_org == NULL){
			return 2;
		}
		decomp->row_cur = ASC_NEW_ARRAY(int32,decomp->n_rows);
		if(decomp->row_cur == NULL){
			return 2;
		}
		for(i = 0; i < decomp->n_rows; ++i){
			decomp->row_org[i] = mtx_row_to_org(mtx,i);
			decomp->row_cur[i] = mtx_org_to_row(mtx,i);
		}
	}
	if(decomp->n_cols > 0){
		decomp->col_org = ASC_NEW_ARRAY(int32,decomp->n_cols);
		if(decomp->col_org == NULL){
			return 2;
		}
		decomp->col_cur = ASC_NEW_ARRAY(int32,decomp->n_cols);
		if(decomp->col_cur == NULL){
			return 2;
		}
		for(i = 0; i < decomp->n_cols; ++i){
			decomp->col_org[i] = mtx_col_to_org(mtx,i);
			decomp->col_cur[i] = mtx_org_to_col(mtx,i);
		}
	}
	return 0;
}

static int decomp_partition_mode(slv_system_t sys, slv_decomp_partition_t *decomp,
		decomp_partition_mode_t mode
){
	mtx_matrix_t mtx;
	int32 row, i, order;
	int ret;
	struct rel_relation **rels, **condrels;
	struct logrel_relation **logrels, **condlogrels;

	if(sys == NULL || decomp == NULL){
		return 1;
	}
	decomp_clear_owned(decomp);
	decomp->n_rels = slv_get_num_solvers_rels(sys);
	decomp->n_condrels = slv_get_num_solvers_condrels(sys);
	decomp->n_logrels = slv_get_num_solvers_logrels(sys);
	decomp->n_condlogrels = slv_get_num_solvers_condlogrels(sys);
	decomp->n_vars = slv_get_num_solvers_vars(sys);
	decomp->n_dvars = slv_get_num_solvers_dvars(sys);
	decomp->n_rows = decomp->n_rels + decomp->n_condrels
		+ decomp->n_logrels + decomp->n_condlogrels;
	decomp->n_cols = decomp->n_vars + decomp->n_dvars;
	if(decomp->n_rows == 0 || decomp->n_cols == 0){
		return 1;
	}

	order = MAX(decomp->n_rows,decomp->n_cols);
	mtx = mtx_create();
	if(mtx == NULL){
		return 2;
	}
	mtx_set_order(mtx,order);

	rels = slv_get_solvers_rel_list(sys);
	condrels = slv_get_solvers_condrel_list(sys);
	logrels = slv_get_solvers_logrel_list(sys);
	condlogrels = slv_get_solvers_condlogrel_list(sys);

	row = 0;
	for(i = 0; i < decomp->n_rels; ++i, ++row){
		if(mode == DECOMP_CONSERVATIVE){
			if(!decomp_add_when_edges_for_object(sys,mtx,rels[i],0,decomp,row)
					&& rel_in_when(rels[i])){
				decomp_add_all_when_selectors(sys,mtx,decomp,row);
			}
		}
		if(decomp_rel_row_current(rels[i],mode)){
			decomp_add_rel_incidences(sys,mtx,rels[i],decomp,row);
		}
	}
	for(i = 0; i < decomp->n_condrels; ++i, ++row){
		if(mode == DECOMP_CONSERVATIVE){
			if(!decomp_add_when_edges_for_object(sys,mtx,condrels[i],0,decomp,row)
					&& rel_in_when(condrels[i])){
				decomp_add_all_when_selectors(sys,mtx,decomp,row);
			}
		}
		if(decomp_rel_row_current(condrels[i],mode)){
			decomp_add_rel_incidences(sys,mtx,condrels[i],decomp,row);
		}
	}
	for(i = 0; i < decomp->n_logrels; ++i, ++row){
		if(mode == DECOMP_CONSERVATIVE){
			if(!decomp_add_when_edges_for_object(sys,mtx,logrels[i],1,decomp,row)
					&& logrel_in_when(logrels[i])){
				decomp_add_all_when_selectors(sys,mtx,decomp,row);
			}
		}
		if(decomp_logrel_row_current(logrels[i],mode)){
			decomp_add_logrel_dvars(sys,mtx,logrels[i],decomp,row);
			decomp_add_boundary_for_logrel(sys,mtx,logrels[i],decomp,row);
		}
	}
	for(i = 0; i < decomp->n_condlogrels; ++i, ++row){
		if(mode == DECOMP_CONSERVATIVE){
			if(!decomp_add_when_edges_for_object(sys,mtx,condlogrels[i],1,decomp,row)
					&& logrel_in_when(condlogrels[i])){
				decomp_add_all_when_selectors(sys,mtx,decomp,row);
			}
		}
		if(decomp_logrel_row_current(condlogrels[i],mode)){
			decomp_add_logrel_dvars(sys,mtx,condlogrels[i],decomp,row);
			decomp_add_boundary_for_logrel(sys,mtx,condlogrels[i],decomp,row);
		}
	}

	mtx_output_assign(mtx,decomp->n_rows,decomp->n_cols);
	decomp->rank = mtx_symbolic_rank(mtx);
	if(decomp->rank == 0){
		if(mode == DECOMP_ACTIVE){
			ret = decomp_copy_blocks_and_perms(mtx,decomp);
			mtx_destroy(mtx);
			if(ret != 0){
				decomp_clear_owned(decomp);
			}
			return ret;
		}
		mtx_destroy(mtx);
		return 1;
	}
	mtx_partition(mtx);
	ret = decomp_copy_blocks_and_perms(mtx,decomp);
	mtx_destroy(mtx);
	if(ret != 0){
		decomp_clear_owned(decomp);
	}
	return ret;
}

int slv_decomp_partition(slv_system_t sys, slv_decomp_partition_t *decomp){
	return decomp_partition_mode(sys,decomp,DECOMP_CONSERVATIVE);
}

int slv_decomp_partition_active(slv_system_t sys, slv_decomp_partition_t *decomp){
	return decomp_partition_mode(sys,decomp,DECOMP_ACTIVE);
}

slv_decomp_row_kind_t slv_decomp_row_kind(
		const slv_decomp_partition_t *decomp, int32 orgrow, int32 *local
){
	if(decomp == NULL || orgrow < 0 || orgrow >= decomp->n_rows){
		return slv_decomp_row_invalid;
	}
	if(orgrow < decomp->n_rels){
		if(local != NULL) *local = orgrow;
		return slv_decomp_row_rel;
	}
	orgrow -= decomp->n_rels;
	if(orgrow < decomp->n_condrels){
		if(local != NULL) *local = orgrow;
		return slv_decomp_row_condrel;
	}
	orgrow -= decomp->n_condrels;
	if(orgrow < decomp->n_logrels){
		if(local != NULL) *local = orgrow;
		return slv_decomp_row_logrel;
	}
	orgrow -= decomp->n_logrels;
	if(orgrow < decomp->n_condlogrels){
		if(local != NULL) *local = orgrow;
		return slv_decomp_row_condlogrel;
	}
	return slv_decomp_row_invalid;
}

slv_decomp_col_kind_t slv_decomp_col_kind(
		const slv_decomp_partition_t *decomp, int32 orgcol, int32 *local
){
	if(decomp == NULL || orgcol < 0 || orgcol >= decomp->n_cols){
		return slv_decomp_col_invalid;
	}
	if(orgcol < decomp->n_vars){
		if(local != NULL) *local = orgcol;
		return slv_decomp_col_var;
	}
	orgcol -= decomp->n_vars;
	if(orgcol < decomp->n_dvars){
		if(local != NULL) *local = orgcol;
		return slv_decomp_col_dvar;
	}
	return slv_decomp_col_invalid;
}

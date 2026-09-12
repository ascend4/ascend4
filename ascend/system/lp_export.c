/* Shared LP export and solution mapping, extracted from the HiGHS adapter.
 * Copyright (C) 2026 John Pye
 * Distributed under the GNU GPL, version 2 or (at your option) any later version.
 */
#include "lp_utils.h"
#include "relman.h"
#include <ascend/general/ascMalloc.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/utilities/error.h>
#include <math.h>
#include <limits.h>
#include <string.h>

boolean lp_var_needs_relaxation(struct var_variable *var){
	return lp_free_inc_var_filter(var)
		&& (solver_int(var_instance(var)) || solver_semi(var_instance(var)))
		&& !var_relaxed(var);
}

/* 0: constant in the selected variables, 1: affine, 2: unsupported/nonlinear.
 * This deliberately does not infer linearity from derivatives at one point.
 */
static int affine_degree(const struct relation *r, const struct relation_term *t,
	struct rel_relation *rel, const var_filter_t *filter){
	int a, b;
	unsigned long i;
	double exponent;
	const struct var_variable **vars = rel_incidence_list((struct rel_relation *)rel);
	if(!t)return 0;
	switch(RelationTermType(t)){
	case e_zero: case e_int: case e_real: return 0;
	case e_var:
		i = TermVarNumber(t);
		if(i < 1 || i > (unsigned long)rel_n_incidences(rel))return 2;
		return var_apply_filter(vars[i-1],filter) ? 1 : 0;
	case e_uminus: return affine_degree(r,TermUniLeft(t),rel,filter);
	case e_func:
		return affine_degree(r,TermFuncLeft(t),rel,filter) == 0 ? 0 : 2;
	case e_plus: case e_minus: case e_times: case e_divide:
	case e_power: case e_ipower:
		a = affine_degree(r,TermBinLeft(t),rel,filter);
		b = affine_degree(r,TermBinRight(t),rel,filter);
		if(a == 2 || b == 2)return 2;
		if(a == 0 && b == 0)return 0;
		switch(RelationTermType(t)){
		case e_plus: case e_minus: return a > b ? a : b;
		case e_times: return a + b;
		case e_divide: return b == 0 ? a : 2;
		default:
			if(b || RelationEvaluateTermSafe(r,TermBinRight(t),&exponent) != safe_ok)return 2;
			return exponent == 1.0 ? a : (exponent == 0.0 ? 0 : 2);
		}
	default: return 2;
	}
}

int lp_relation_is_affine(struct rel_relation *rel, const var_filter_t *filter){
	const struct relation *r;
	if(!rel || !filter || rel->type != e_rel_token)return 0;
	r = GetInstanceRelationOnly(rel_instance(rel));
	return r && affine_degree(r,Infix_LhsSide(r),rel,filter) < 2
		&& affine_degree(r,Infix_RhsSide(r),rel,filter) < 2;
}

int lp_prepare(slv_system_t sys, mps_data_t *m, slv_status_t *status,
	int scale_variables, int scale_relations){
	struct var_variable **v = slv_get_solvers_var_list(sys);
	struct rel_relation **r = slv_get_solvers_rel_list(sys);
	struct rel_relation *obj = slv_get_obj_relation(sys);
	int32 i;
	lp_nuke_pointers(m);
	memset(m,0,sizeof(*m));
	if(!v || !r || !obj)return 1;
	for(i=0; v[i]; ++i){
		if(var_sindex(v[i]) != i){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"LP export requires original solver-list indices.");
			return 1;
		}
		if(lp_free_inc_var_filter(v[i]))++m->vinc;
		++m->vused;
	}
	for(i=0; r[i]; ++i){
		if(rel_sindex(r[i]) != i)return 1;
		if(lp_inc_rel_filter(r[i]))++m->rinc;
		++m->rused;
	}
	if(!m->vused || !m->rused)return 1;
	m->crow = m->rused;
	m->cap = m->vused > m->rused+1 ? m->vused : m->rused+1;
	m->Ac_mtx = lp_calc_matrix(m->cap,m->rused,m->vused,r,obj,m->crow,status,&m->rank,&m->bcol);
	if(!m->Ac_mtx)goto fail;
	m->lbrow = lp_calc_bounds(v,m->vused,FALSE);
	m->ubrow = lp_calc_bounds(v,m->vused,TRUE);
	m->relopcol = lp_calc_reloplist(r,m->rused);
	m->typerow = lp_calc_svtlist(v,m->vused,&m->solver_var_used,&m->solver_relaxed_used,
		&m->solver_int_used,&m->solver_binary_used,&m->solver_semi_used,&m->solver_other_used,&m->solver_fixed);
	if(!m->lbrow || !m->ubrow || !m->relopcol || !m->typerow)goto fail;
	/* Fixed variables are excluded from coefficients. Keep their exported columns
	 * fixed too, and never write these columns back to ASCEND.
	 */
	for(i=0; i<m->vused; ++i){
		if(m->typerow[i] == MPS_FIXED)m->lbrow[i] = m->ubrow[i] = var_value(v[i]);
	}
	lp_real_rhs(m->Ac_mtx,m->relopcol,v,m->rused,m->vused,m->bcol);
	if(!lp_apply_nominal_scaling(m->Ac_mtx,m->lbrow,m->ubrow,m->bcol,m->typerow,
		m->relopcol,m->cap,m->rused,m->vused,m->crow,v,r,obj,
		scale_variables,scale_relations,&m->col_scale,&m->row_scale))goto fail;
	return 0;
fail:
	lp_nuke_pointers(m);
	return 1;
}

void lp_sparse_destroy(lp_sparse_t *p){
	if(!p)return;
	ASC_FREE(p->start); ASC_FREE(p->index); ASC_FREE(p->row_original);
	ASC_FREE(p->value); ASC_FREE(p->cost); ASC_FREE(p->lower); ASC_FREE(p->upper);
	ASC_FREE(p->row_lower); ASC_FREE(p->row_upper); ASC_FREE(p->type);
	memset(p,0,sizeof(*p));
}

int lp_sparse_build(lp_sparse_t *p, const mps_data_t *m,
	struct var_variable **vars, struct rel_relation *obj, real64 minf, real64 pinf){
	int32 i, j, row, col, nnz=0, calc_ok=1;
	int32 *map = NULL;
	mtx_coord_t nz;
	mtx_range_t range;
	double a;
	lp_sparse_destroy(p);
	if(!m || !m->Ac_mtx || !vars || !obj || minf >= pinf)return 1;
	p->num_col = m->vused;
	map = ASC_NEW_ARRAY(int32,m->rused);
	p->row_original = ASC_NEW_ARRAY(int32,m->rused);
	if(!map || !p->row_original)goto fail;
	for(i=0; i<m->rused; ++i){
		map[i] = -1;
		if(m->relopcol[i]){
			map[i] = p->num_row;
			p->row_original[p->num_row++] = i;
		}
	}
	/* Iterate the full permuted matrix: objective/constraint rows can have
	 * moved during output assignment. Filter using original row indices.
	 */
	for(i=0; i<m->vused; ++i){
		nz.col = mtx_org_to_col(m->Ac_mtx,i); nz.row = mtx_FIRST;
		while((a=mtx_next_in_col(m->Ac_mtx,&nz,mtx_range(&range,0,m->cap-1))), nz.row != mtx_LAST){
			row = mtx_row_to_org(m->Ac_mtx,nz.row);
			if(row < m->rused && map[row]>=0){
				if(nnz == INT_MAX)goto fail;
				++nnz;
			}
		}
	}
#define ALLOC(F,T,N) do { p->F=ASC_NEW_ARRAY(T,(N)>0?(N):1); if(!p->F)goto fail; } while(0)
	ALLOC(start,int32,m->vused+1); ALLOC(index,int32,nnz); ALLOC(value,real64,nnz);
	ALLOC(cost,real64,m->vused); ALLOC(lower,real64,m->vused); ALLOC(upper,real64,m->vused);
	ALLOC(type,char,m->vused); ALLOC(row_lower,real64,p->num_row); ALLOC(row_upper,real64,p->num_row);
#undef ALLOC
	for(i=0; i<m->vused; ++i){
		p->cost[i]=0;
		p->type[i]=m->typerow[i];
		p->lower[i]=m->lbrow[i] <= minf ? -HUGE_VAL : m->lbrow[i];
		p->upper[i]=m->ubrow[i] >= pinf ? HUGE_VAL : m->ubrow[i];
		if(isnan(p->lower[i]) || isnan(p->upper[i]))goto fail;
	}
	for(i=0; i<p->num_row; ++i){
		row=p->row_original[i];
		if(!isfinite(m->bcol[row]))goto fail;
		switch(m->relopcol[row]){
		case rel_TOK_less: p->row_lower[i]=-HUGE_VAL; p->row_upper[i]=m->bcol[row]; break;
		case rel_TOK_greater: p->row_lower[i]=m->bcol[row]; p->row_upper[i]=HUGE_VAL; break;
		case rel_TOK_equal: p->row_lower[i]=p->row_upper[i]=m->bcol[row]; break;
		default: goto fail;
		}
	}
	for(i=0; i<m->vused; ++i){
		col=mtx_org_to_col(m->Ac_mtx,i);
		p->start[i]=p->num_nz;
		nz.col=col; nz.row=mtx_FIRST;
		while((a=mtx_next_in_col(m->Ac_mtx,&nz,mtx_range(&range,0,m->cap-1))), nz.row != mtx_LAST){
			if(!isfinite(a))goto fail;
			row=mtx_row_to_org(m->Ac_mtx,nz.row);
			if(row == m->crow)p->cost[i]=a;
			else if(row<m->rused && map[row]>=0){
				j=p->num_nz++;
				p->index[j]=map[row]; p->value[j]=a;
			}
		}
	}
	p->start[m->vused]=p->num_nz;
	p->maximize=relman_obj_direction(obj)==1;
	p->objective_offset=relman_eval(obj,&calc_ok,1);
	if(!calc_ok || !isfinite(p->objective_offset))goto fail;
	for(i=0; i<m->vused; ++i)
		p->objective_offset -= p->cost[i]*var_value(vars[i])/(m->col_scale ? m->col_scale[i] : 1.0);
	if(!isfinite(p->objective_offset))goto fail;
	ASC_FREE(map);
	return 0;
fail:
	ASC_FREE(map);
	lp_sparse_destroy(p);
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Failed to construct finite sparse LP coefficients.");
	return 1;
}

int lp_write_solution(slv_system_t sys, const mps_data_t *m, const real64 *values, int safe){
	struct var_variable **v=slv_get_solvers_var_list(sys);
	struct rel_relation **r=slv_get_solvers_rel_list(sys), *obj=slv_get_obj_relation(sys);
	int32 i, calc_ok=1;
	int ok=1;
	/* Validate all values before changing the model. */
	for(i=0; v[i]; ++i){
		int32 col=var_sindex(v[i]);
		if(col<0 || col>=m->vused)return 1;
		if(lp_free_inc_var_filter(v[i]) && !isfinite(values[col]*(m->col_scale ? m->col_scale[col] : 1)))return 1;
	}
	for(i=0; v[i]; ++i){
		int32 col=var_sindex(v[i]);
		if(lp_free_inc_var_filter(v[i]))var_set_value(v[i],values[col]*(m->col_scale ? m->col_scale[col] : 1));
	}
	if(obj){ relman_eval(obj,&calc_ok,safe); if(!calc_ok)ok=0; }
	for(i=0; r[i]; ++i){
		if(lp_inc_rel_filter(r[i])){ relman_eval(r[i],&calc_ok,safe); if(!calc_ok)ok=0; }
	}
	return ok ? 0 : 1;
}

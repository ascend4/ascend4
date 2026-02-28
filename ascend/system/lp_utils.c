#include "lp_utils.h"

#include <math.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/general/mathmacros.h>
#include <ascend/general/mem.h>
#include <ascend/general/tm_time.h>
#include <ascend/utilities/error.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/symtab.h>

#include "calc.h"
#include "relman.h"
#include "bnd.h"

#define LP_TOK_NONINCIDENT 0

#define destroy_array(p) if((p) != NULL) ascfree((p))
#define create_zero_array(len,type) ((len) > 0 ? ASC_NEW_ARRAY_CLEAR(type,len) : NULL)
#define create_array(len,type) ((len) > 0 ? ASC_NEW_ARRAY(type,len) : NULL)

boolean lp_free_inc_var_filter(struct var_variable *var){
	var_filter_t vfilter;
	vfilter.matchbits = (VAR_FIXED | VAR_ACTIVE);
	vfilter.matchvalue = VAR_ACTIVE;
	return var_apply_filter(var,&vfilter);
}

boolean lp_inc_rel_filter(struct rel_relation *rel){
	rel_filter_t rfilter;
	rfilter.matchbits = (REL_INCLUDED | REL_ACTIVE);
	rfilter.matchvalue = (REL_INCLUDED | REL_ACTIVE);
	return rel_apply_filter(rel,&rfilter);
}

void lp_nuke_pointers(mps_data_t *mps){
	if(mps == NULL)return;

	if(mps->Ac_mtx != NULL){
		mtx_destroy(mps->Ac_mtx);
		mps->Ac_mtx = NULL;
	}
	if(mps->lbrow != NULL){
		destroy_array(mps->lbrow);
		mps->lbrow = NULL;
	}
	if(mps->ubrow != NULL){
		destroy_array(mps->ubrow);
		mps->ubrow = NULL;
	}
	if(mps->bcol != NULL){
		destroy_array(mps->bcol);
		mps->bcol = NULL;
	}
	if(mps->col_scale != NULL){
		destroy_array(mps->col_scale);
		mps->col_scale = NULL;
	}
	if(mps->row_scale != NULL){
		destroy_array(mps->row_scale);
		mps->row_scale = NULL;
	}
	if(mps->typerow != NULL){
		destroy_array(mps->typerow);
		mps->typerow = NULL;
	}
	if(mps->relopcol != NULL){
		destroy_array(mps->relopcol);
		mps->relopcol = NULL;
	}
}

boolean lp_calc_c(mtx_matrix_t mtx, int32 org_row, struct rel_relation *obj){
	var_filter_t vfilter;
	mtx_coord_t coord;
	real64 *derivs = NULL;
	int32 *vars = NULL;
	int32 len, count, i;
	int32 row;
	int status;
	int safe = 0;

	if((mtx == NULL) || (obj == NULL)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"routine was passed a NULL pointer.");
		return FALSE;
	}

	vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
	vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

	row = mtx_org_to_row(mtx,org_row);
	if(row < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"invalid objective row index %d.", (int)org_row);
		return FALSE;
	}

	len = rel_n_incidences(obj);
	if(len <= 0){
		return TRUE;
	}

	derivs = ASC_NEW_ARRAY_OR_NULL(real64,len);
	vars = ASC_NEW_ARRAY_OR_NULL(int32,len);
	if((derivs == NULL) || (vars == NULL)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"memory allocation failed.");
		if(derivs)ascfree(derivs);
		if(vars)ascfree(vars);
		return FALSE;
	}

	count = 0;
	status = relman_diff2(obj,&vfilter,derivs,vars,&count,safe);
	if(status != 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed to evaluate objective gradient.");
		ascfree(derivs);
		ascfree(vars);
		return FALSE;
	}

	coord.row = org_row;
	for(i = 0; i < count; ++i){
		if(vars[i] < 0 || vars[i] >= mtx_order(mtx)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"objective column index %d out of range.", (int)vars[i]);
			ascfree(derivs);
			ascfree(vars);
			return FALSE;
		}
		coord.col = vars[i];
		mtx_fill_org_value(mtx,&coord,derivs[i]);
	}

	ascfree(derivs);
	ascfree(vars);
	return TRUE;
}

real64 *lp_calc_bounds(struct var_variable **vlist, int32 vused, boolean upper){
	real64 *tmp_array_origin;
	int32 col;

	if(vlist == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"routine was passed a NULL variable list pointer.");
		return NULL;
	}

	tmp_array_origin = create_zero_array(vused,real64);
	if(tmp_array_origin == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"memory allocation failed.");
		return NULL;
	}

	for(; *vlist != NULL; ++vlist){
		col = var_sindex(*vlist);
		if((col < 0) || (col >= vused)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"variable index %d out of range.",(int)col);
			ascfree(tmp_array_origin);
			return NULL;
		}
		if(upper){
			tmp_array_origin[col] = var_upper_bound(*vlist);
		}else{
			tmp_array_origin[col] = var_lower_bound(*vlist);
		}
	}
	return tmp_array_origin;
}

char *lp_calc_reloplist(struct rel_relation **rlist, int32 rused){
	char *reloplist;
	int32 row;

	reloplist = create_zero_array(rused,char);
	if(reloplist == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"memory allocation failed.");
		return NULL;
	}

	for(; *rlist != NULL; rlist++){
		row = rel_sindex(*rlist);
		if((row < 0) || (row >= rused)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"relation index %d out of range.",(int)row);
			ascfree(reloplist);
			return NULL;
		}
		if(lp_inc_rel_filter(*rlist)){
			switch(rel_relop(*rlist)) {
			case e_rel_less:
			case e_rel_lesseq:
				reloplist[row] = rel_TOK_less;
				break;
			case e_rel_equal:
				reloplist[row] = rel_TOK_equal;
				break;
			case e_rel_greater:
			case e_rel_greatereq:
				reloplist[row] = rel_TOK_greater;
				break;
			default:
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"unknown relation type.");
				ascfree(reloplist);
				return NULL;
			}
		}else{
			reloplist[row] = LP_TOK_NONINCIDENT;
		}
	}

	return reloplist;
}

char *lp_calc_svtlist(
	struct var_variable **vlist,
	int32 vused,
	int *solver_var_used,
	int *solver_relaxed_used,
	int *solver_int_used,
	int *solver_binary_used,
	int *solver_semi_used,
	int *solver_other_used,
	int *solver_fixed
){
	struct TypeDescription *type;
	struct TypeDescription *solver_var_type;
	struct TypeDescription *solver_int_type;
	struct TypeDescription *solver_binary_type;
	struct TypeDescription *solver_semi_type;
	char *svtlist;
	int32 orgcol;

	if((solver_var_type = FindType(AddSymbol(MPS_VAR_STR))) == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"type '%s' not defined; MPS export will not work.", MPS_VAR_STR);
		return NULL;
	}
	if((solver_int_type = FindType(AddSymbol(MPS_INT_STR))) == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"type '%s' not defined; MPS export will not work.", MPS_INT_STR);
		return NULL;
	}
	if((solver_binary_type = FindType(AddSymbol(MPS_BINARY_STR))) == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"type '%s' not defined; MPS export will not work.", MPS_BINARY_STR);
		return NULL;
	}
	if((solver_semi_type = FindType(AddSymbol(MPS_SEMI_STR))) == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"type '%s' not defined; MPS export will not work.", MPS_SEMI_STR);
		return NULL;
	}

	svtlist = create_array(vused,char);
	if(svtlist == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"memory allocation failed for solver var type list.");
		return NULL;
	}

	*solver_var_used = 0;
	*solver_relaxed_used = 0;
	*solver_int_used = 0;
	*solver_binary_used = 0;
	*solver_semi_used = 0;
	*solver_other_used = 0;
	*solver_fixed = 0;
	for(orgcol = 0; orgcol < vused; ++orgcol){
		svtlist[orgcol] = MPS_FIXED;
	}

	for(; *vlist != NULL; ++vlist){
		orgcol = var_sindex(*vlist);
		if((orgcol < 0) || (orgcol >= vused)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"variable index %d out of range.",(int)orgcol);
			ascfree(svtlist);
			return NULL;
		}
		if(lp_free_inc_var_filter(*vlist)){
			type = InstanceTypeDesc(var_instance(*vlist));

			if(type == MoreRefined(type,solver_binary_type)){
				if(var_relaxed(*vlist)){
					svtlist[orgcol] = MPS_RELAXED;
					(*solver_relaxed_used)++;
				}else{
					svtlist[orgcol] = MPS_BINARY;
					(*solver_binary_used)++;
				}
			}else if(type == MoreRefined(type,solver_int_type)){
				if(var_relaxed(*vlist)){
					svtlist[orgcol] = MPS_RELAXED;
					(*solver_relaxed_used)++;
				}else{
					svtlist[orgcol] = MPS_INT;
					(*solver_int_used)++;
				}
			}else if(type == MoreRefined(type,solver_semi_type)){
				if(var_relaxed(*vlist)){
					svtlist[orgcol] = MPS_RELAXED;
					(*solver_relaxed_used)++;
				}else{
					svtlist[orgcol] = MPS_SEMI;
					(*solver_semi_used)++;
				}
			}else if(type == MoreRefined(type,solver_var_type)){
				svtlist[orgcol] = MPS_VAR;
				(*solver_var_used)++;
			}else{
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"unknown solver_var type encountered.");
			}
		}else{
			svtlist[orgcol] = MPS_FIXED;
			(*solver_fixed)++;
		}
	}

	return svtlist;
}

mtx_matrix_t lp_calc_matrix(
	int32 cap,
	int32 rused,
	int32 vused,
	struct rel_relation **rlist,
	struct rel_relation *obj,
	int32 crow,
	slv_status_t *s,
	int32 *rank,
	real64 **rhs_orig
){
	mtx_matrix_t mtx;
	var_filter_t vfilter;
	double time0;
	struct rel_relation **rp;
	int32 orgrow;
	int status;
	int safe = 0;

	if(obj == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"system must have an objective.");
		return NULL;
	}

	time0 = tm_cpu_time();
	s->calc_ok = TRUE;

	mtx = mtx_create();
	mtx_set_order(mtx,cap);

	vfilter.matchbits = (VAR_FIXED | VAR_INCIDENT | VAR_ACTIVE);
	vfilter.matchvalue = (VAR_INCIDENT | VAR_ACTIVE);

	*rhs_orig = create_zero_array(rused,real64);
	if(*rhs_orig == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"memory allocation for right-hand side failed.");
		return NULL;
	}

	for(rp = rlist; *rp != NULL; ++rp){
		if(lp_inc_rel_filter(*rp)){
			orgrow = rel_sindex(*rp);
			if((orgrow < 0) || (orgrow >= rused)){
				s->calc_ok = FALSE;
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"relation index %d out of range.",(int)orgrow);
				destroy_array(*rhs_orig);
				mtx_destroy(mtx);
				return NULL;
			}
			status = relman_diffs(*rp,&vfilter,mtx,&((*rhs_orig)[orgrow]),safe);
			if(status != 0){
				s->calc_ok = FALSE;
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"error while calculating A matrix.");
				destroy_array(*rhs_orig);
				mtx_destroy(mtx);
				return NULL;
			}
		}
	}

	mtx_output_assign(mtx,crow,vused);
	if(!mtx_output_assigned(mtx)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"output assignment for rank calculation failed.");
		mtx_destroy(mtx);
		destroy_array(*rhs_orig);
		return NULL;
	}
	*rank = mtx_symbolic_rank(mtx);

	if(*rank < 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"symbolic rank calculation failed; matrix may be bad.");
		return mtx;
	}

	if(!lp_calc_c(mtx,crow,obj)){
		s->calc_ok = FALSE;
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"error calculating objective coefficients.");
		mtx_destroy(mtx);
		destroy_array(*rhs_orig);
		return NULL;
	}

	{
		struct slv__block_status_structure *block = slv_status_block_rw(s);
		if(block != NULL){
			block->jactime = tm_cpu_time() - time0;
		}
	}
	return mtx;
}

void lp_real_rhs(
	mtx_matrix_t Ac_mtx,
	char relopcol[],
	struct var_variable **vlist,
	int32 rused,
	int32 vused,
	real64 rhs[]
){
	real64 a;
	mtx_coord_t nz;
	mtx_range_t range;
	int32 currow;
	int orgrow;
	int orgcol;
	double rowval;

	(void)vused;

	if(rhs == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"routine was passed a NULL rhs pointer.");
		return;
	}

	for(currow = 0; currow < rused; currow++){
		orgrow = mtx_row_to_org(Ac_mtx, currow);
		if(relopcol[orgrow] != LP_TOK_NONINCIDENT){
			nz.col = mtx_FIRST;
			nz.row = currow;
			rowval = 0.0;
			a = mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,vused));
			while(nz.col != mtx_LAST){
				orgcol = mtx_col_to_org(Ac_mtx,nz.col);
				rowval += a * var_value(vlist[orgcol]);
				a = mtx_next_in_row(Ac_mtx,&nz,mtx_range(&range,0,vused));
			}
			rhs[orgrow] = rowval - rhs[orgrow];
		}
	}
}

void lp_ensure_bounds(FILE *mif, slv_system_t slv, struct var_variable *var){
	char *varname = NULL;
	real64 val, low, high;
	(void)mif;

	low = var_lower_bound(var);
	high = var_upper_bound(var);
	val = var_value(var);
	varname = var_make_name(slv,var);
	if(varname == NULL)varname = ASC_STRDUP("<unknown>");

	if(low > high){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING
			,"Bounds for variable '%s' are inconsistent [%g,%g]; swapping."
			,varname,low,high
		);
		var_set_upper_bound(var, low);
		var_set_lower_bound(var, high);
		low = var_lower_bound(var);
		high = var_upper_bound(var);
	}

	if(low > val){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING
			,"Variable '%s' was initialized below its lower bound; moved to lower bound."
			,varname
		);
		var_set_value(var, low);
	}else if(val > high){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING
			,"Variable '%s' was initialized above its upper bound; moved to upper bound."
			,varname
		);
		var_set_value(var, high);
	}
	ASC_FREE(varname);
}

static real64 lp_safe_nominal(real64 raw){
	real64 s = fabs(raw);
	if(!isfinite(s) || s < 1e-12){
		return 1.0;
	}
	return s;
}

boolean lp_apply_nominal_scaling(
	mtx_matrix_t Ac_mtx,
	real64 lbrow[],
	real64 ubrow[],
	real64 bcol[],
	char typerow[],
	char relopcol[],
	int32 cap,
	int32 rused,
	int32 vused,
	int32 crow,
	struct var_variable **vlist,
	struct rel_relation **rlist,
	struct rel_relation *obj,
	boolean varnom_scale,
	boolean relnom_scale,
	real64 **col_scale_out,
	real64 **row_scale_out
){
	real64 *col_scale = NULL;
	real64 *row_scale = NULL;
	real64 *row_auto = NULL;
	int32 i;
	struct var_variable **vp;
	struct rel_relation **rp;

	mtx_coord_t nz;
	mtx_range_t range;
	real64 a;
	int32 orgcol;
	int32 curcol;
	int32 orgrow;
	real64 cscale;
	real64 rscale;

	if(
		Ac_mtx == NULL || lbrow == NULL || ubrow == NULL || bcol == NULL
		|| typerow == NULL || relopcol == NULL
		|| vlist == NULL || rlist == NULL || cap <= 0 || rused < 0 || vused < 0
	){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"lp_apply_nominal_scaling called with invalid arguments.");
		return FALSE;
	}
	(void)obj;
	(void)crow;

	col_scale = create_array(vused,real64);
	row_scale = create_array(cap,real64);
	if(col_scale == NULL || row_scale == NULL){
		if(col_scale != NULL)ascfree(col_scale);
		if(row_scale != NULL)ascfree(row_scale);
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed allocating LP/MIP scaling arrays.");
		return FALSE;
	}
	if(relnom_scale){
		row_auto = create_zero_array(rused,real64);
		if(row_auto == NULL){
			ascfree(col_scale);
			ascfree(row_scale);
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"failed allocating LP/MIP row auto-scale array.");
			return FALSE;
		}
	}

	for(i = 0; i < vused; ++i)col_scale[i] = 1.0;
	for(i = 0; i < cap; ++i)row_scale[i] = 1.0;

	if(varnom_scale){
		for(vp = vlist; *vp != NULL; ++vp){
			real64 s;
			int32 col = var_sindex(*vp);
			if(col < 0 || col >= vused)continue;
			switch(typerow[col]){
				case MPS_INT:
				case MPS_BINARY:
				case MPS_SEMI:
					col_scale[col] = 1.0;
					break;
				default:
					s = lp_safe_nominal(var_nominal(*vp));
					col_scale[col] = s;
					break;
			}
		}
	}

	if(relnom_scale){
		for(rp = rlist; *rp != NULL; ++rp){
			real64 s;
			int32 row = rel_sindex(*rp);
			if(row < 0 || row >= rused)continue;
			if(relopcol[row] == LP_TOK_NONINCIDENT)continue;
			s = lp_safe_nominal(relman_scale(*rp));
			row_scale[row] = s;
		}

		for(i = 0; i < rused; ++i){
			row_auto[i] = fabs(bcol[i]);
		}
		for(orgcol = 0; orgcol < vused; ++orgcol){
			curcol = mtx_org_to_col(Ac_mtx,orgcol);
			if(curcol < 0)continue;
			cscale = col_scale[orgcol];
			if(cscale <= 0.0)cscale = 1.0;

			nz.col = curcol;
			nz.row = mtx_FIRST;
			a = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,mtx_order(Ac_mtx)-1));
			while(nz.row != mtx_LAST){
				orgrow = mtx_row_to_org(Ac_mtx,nz.row);
				if(orgrow >= 0 && orgrow < rused){
					real64 abs_coeff = fabs(a * cscale);
					if(abs_coeff > row_auto[orgrow]){
						row_auto[orgrow] = abs_coeff;
					}
				}
				a = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,mtx_order(Ac_mtx)-1));
			}
		}
		for(i = 0; i < rused; ++i){
			real64 rs = lp_safe_nominal(row_auto[i]);
			if(rs > row_scale[i]){
				row_scale[i] = rs;
			}
		}
	}

	for(i = 0; i < vused; ++i){
		cscale = col_scale[i];
		if(cscale <= 0.0)cscale = 1.0;
		lbrow[i] /= cscale;
		ubrow[i] /= cscale;
	}

	for(i = 0; i < rused; ++i){
		rscale = row_scale[i];
		if(rscale <= 0.0)rscale = 1.0;
		bcol[i] /= rscale;
	}

	for(orgcol = 0; orgcol < vused; ++orgcol){
		curcol = mtx_org_to_col(Ac_mtx,orgcol);
		if(curcol < 0)continue;
		cscale = col_scale[orgcol];
		if(cscale <= 0.0)cscale = 1.0;

		nz.col = curcol;
		nz.row = mtx_FIRST;
		a = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,mtx_order(Ac_mtx)-1));
		while(nz.row != mtx_LAST){
			orgrow = mtx_row_to_org(Ac_mtx,nz.row);
			if(orgrow < 0){
				a = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,mtx_order(Ac_mtx)-1));
				continue;
			}
			if(orgrow >= 0 && orgrow < cap){
				rscale = row_scale[orgrow];
			}else{
				rscale = 1.0;
			}
			if(rscale <= 0.0)rscale = 1.0;
			mtx_set_value(Ac_mtx,&nz,a * cscale / rscale);
			a = mtx_next_in_col(Ac_mtx,&nz,mtx_range(&range,0,mtx_order(Ac_mtx)-1));
		}
	}

	if(col_scale_out != NULL){
		*col_scale_out = col_scale;
	}else{
		ascfree(col_scale);
	}
	if(row_scale_out != NULL){
		*row_scale_out = row_scale;
	}else{
		ascfree(row_scale);
	}
	if(row_auto != NULL){
		ascfree(row_auto);
	}

	return TRUE;
}

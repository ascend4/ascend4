/*	ASCEND modelling environment
	Copyright (C) 2006-2011 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
	Analysis routines for the ASCEND wrapper of the IDA integrator.
	These functions perform sorting of variables and relations and create
	additional lists of variables as required for use by our ida.c code.
*/
#include "idaanalyse.h"
#include "idatypes.h"
#include "idaio.h"

#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

#include <ascend/linear/linsolqr.h>

#include <ascend/system/diffvars.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/block.h>
#include <ascend/system/diffvars.h>
#include <ascend/system/diffvars_impl.h>
#include <ascend/system/jacobian.h>
#include <ascend/system/cond_config.h>
#include <ascend/solver/slvDOF.h>

#ifndef IDA_DEBUG
# define IDA_DEBUG 0
#endif
#if !IDA_DEBUG
# undef CONSOLE_DEBUG
# define CONSOLE_DEBUG(...) ((void)0)
#endif

#if IDA_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(...)
#endif

/* #define ANALYSE_DEBUG */

/*
	define DERIV_WITHOUT_DIFF to enable experimental handling of derivatives
	for which corresponding differential vars were not found to be incident.
*/
#define DERIV_WITHOUT_DIFF

#define VARMSG(msg) \
	varname = var_make_name(integ->system,v); \
	MSG(msg,varname); \
	ASC_FREE(varname)

//static int integrator_ida_check_partitioning(IntegratorSystem *integ);
static int integrator_ida_check_diffindex(IntegratorSystem *integ);
/* static int integrator_ida_rebuild_diffindex(IntegratorSystem *integ); */

const var_filter_t integrator_ida_nonderiv = {
	VAR_SVAR | VAR_ACTIVE | VAR_FIXED | VAR_DERIV | VAR_DISCRETE,
	VAR_SVAR | VAR_ACTIVE | 0         | 0         | 0
};

const var_filter_t integrator_ida_deriv = {
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | VAR_FIXED | VAR_DERIV | VAR_DISCRETE,
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0         | VAR_DERIV | 0
};

const rel_filter_t integrator_ida_rel = {
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE,
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE
};

const var_filter_t integrator_ida_active_free_incident = {
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | VAR_FIXED,
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0
};

static const char *integrator_ida_dof_status_name(int status){
	switch(status){
	case 1:
		return "underspecified";
	case 2:
		return "square";
	case 3:
		return "structurally singular";
	case 4:
		return "overspecified";
	default:
		return "analysis error";
	}
}

static int integrator_ida_get_dof_status(IntegratorSystem *integ, int *status, int *dof){
	int local_status = 5;
	int local_dof = 0;

	if(!slvDOF_status(integ->system, &local_status, &local_dof)){
		return 0;
	}
	if(status != NULL){
		*status = local_status;
	}
	if(dof != NULL){
		*dof = local_dof;
	}
	return 1;
}

static void integrator_ida_report_diagnostics(IntegratorSystem *integ){
	int dof_status = 5;
	int dof = 0;
	int active_rels;
	int active_free_vars;
	int total_vars;
	int total_rels;
	int total_bnds;
	int algebraic_vars;
	int algebraic_rels;
	int extra_active_vars;

	if(!integrator_ida_get_dof_status(integ, &dof_status, &dof)){
		dof_status = 5;
		dof = 0;
	}

	active_rels = slv_count_solvers_rels(integ->system, &integrator_ida_rel);
	active_free_vars = slv_count_solvers_vars(integ->system, &integrator_ida_active_free_incident);
	total_vars = slv_get_num_solvers_vars(integ->system);
	total_rels = slv_get_num_solvers_rels(integ->system);
	total_bnds = slv_get_num_solvers_bnds(integ->system);

	algebraic_vars = integ->n_y - integ->n_ydot;
	if(algebraic_vars < 0){
		algebraic_vars = 0;
	}
	algebraic_rels = active_rels - integ->n_diffeqs;
	if(algebraic_rels < 0){
		algebraic_rels = 0;
	}
	extra_active_vars = active_free_vars - (integ->n_y + integ->n_ydot);
	if(extra_active_vars < 0){
		extra_active_vars = 0;
	}

	ERROR_REPORTER_NOLINE(ASC_USER_NOTE,
		"IDA preflight: structure=%s, dof=%d, active rels=%d, free incident vars=%d, solver vars=%d, solver rels=%d",
		integrator_ida_dof_status_name(dof_status), dof, active_rels, active_free_vars, total_vars, total_rels
	);
	ERROR_REPORTER_NOLINE(ASC_USER_NOTE,
		"IDA partition: y=%d (%d differential, %d algebraic), ydot=%d, differential rels=%d, algebraic rels=%d, extra active vars=%d, boundaries=%d",
		integ->n_y, integ->n_ydot, algebraic_vars, integ->n_ydot, integ->n_diffeqs, algebraic_rels, extra_active_vars, total_bnds
	);
}
static int integrator_ida_var_in_list(struct var_variable **list, int n, struct var_variable *var){
	int i;
	for(i = 0; i < n; ++i){
		if(list[i] == var){
			return 1;
		}
	}
	return 0;
}

static int integrator_ida_rebuild_var_order(IntegratorSystem *integ, int *ny1, int *nydot){
	const SolverDiffVarCollection *diffvars;
	struct var_variable **oldvars, **mastervars, **newvars, *v;
	SolverDiffVarSequence seq;
	int i, oldn, newn, count_y, count_ydot;

	diffvars = system_get_diffvars(integ->system);
	if(diffvars == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}

	oldvars = slv_get_solvers_var_list(integ->system);
	mastervars = slv_get_master_var_list(integ->system);
	oldn = slv_get_num_solvers_vars(integ->system);
	newvars = ASC_NEW_ARRAY(struct var_variable *, oldn + diffvars->nseqs + 1);
	if(newvars == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory while rebuilding IDA variable ordering");
		return 1;
	}

	newn = 0;
	count_y = 0;
	for(i = 0; i < diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];
		if(!var_apply_filter(v, &integrator_ida_nonderiv)){
			continue;
		}
		if(!integrator_ida_var_in_list(newvars, newn, v)){
			newvars[newn++] = v;
		}
		count_y++;
	}

	count_ydot = 0;
	for(i = 0; i < diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		if(!var_apply_filter(seq.vars[0], &integrator_ida_nonderiv)){
			continue;
		}
		if(seq.n > 1 && var_apply_filter(seq.vars[1], &integrator_ida_deriv)){
			v = seq.vars[1];
			if(!integrator_ida_var_in_list(newvars, newn, v)){
				newvars[newn++] = v;
			}
			count_ydot++;
		}
	}

	for(i = 0; i < oldn; ++i){
		v = oldvars[i];
		if(!integrator_ida_var_in_list(newvars, newn, v)){
			newvars[newn++] = v;
		}
	}

	newvars[newn] = NULL;
	for(i = 0; i < newn; ++i){
		var_set_sindex(newvars[i], i);
	}

	slv_set_solvers_var_list(integ->system, newvars, newn);
	if(oldvars != NULL && oldvars != mastervars){
		ascfree(oldvars);
	}
	*ny1 = count_y;
	*nydot = count_ydot;
	return 0;
}

/**
	This is the first step in the DAE analysis process. We inspect the
	derivative chains from the solver's analyse() routine.
	Check derivative chains: if a var or derivative is inelligible,
	follow down the chain fixing & setting zero any derivatives.
*/
static int integrator_ida_check_vars(IntegratorSystem *integ){
	const SolverDiffVarCollection *diffvars;
	char *varname;
	int n_y = 0;
	int i, j;
	struct var_variable *v;
	SolverDiffVarSequence seq;
	int vok;

#ifdef ANALYSE_DEBUG
	MSG("BEFORE CHECKING VARS");
	system_diffvars_debug(integ->system,stderr);
#endif

	/* we shouldn't have allocated these yet: just be sure */
	asc_assert(integ->y==NULL);
	asc_assert(integ->ydot==NULL);
	asc_assert(integ->n_y==0);

	/* get the the dervative chains from the system */
	diffvars = system_get_diffvars(integ->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	system_var_list_debug(integ->system);
#endif

	/* add the variables from the derivative chains */
	for(i=0; i<diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];

		/* update the var_fixed flag */
		(void)var_fixed(v);
		vok = var_apply_filter(v,&integrator_ida_nonderiv);

		if(vok && !var_incident(v)){
			/* VARMSG("good var '%s' is not incident"); */
			/* var meets our filter, but perhaps it's not incident? */
			if(seq.n == 1 || var_apply_filter(seq.vars[1],&integrator_ida_nonderiv)){
				/* VARMSG("DEACTIVATING NON-INCIDENT VAR '%s' (NO DERIVATIVE)"); */
				var_set_active(v,0);
				vok = 0;
			}else{
				MSG("Including non-incident state because its derivative is present in the DAE system.");
#ifdef DERIV_WITHOUT_DIFF
				VARMSG("'%s' has a derivative present, so needs to be included in the system");
				MSG("That var %s active",(var_active(v) ? "is" : "is NOT"));
				var_set_incident(v,1);
#else
				return 1;
#endif
			}
		}

		if(!vok){
			/*VARMSG("'%s' fails non-deriv filter");
			if(var_fixed(v)){
				MSG("(var is fixed");
			}
			MSG("passes nonderiv? %s (flags = 0x%x)"
				, (var_apply_filter(v,&integrator_ida_nonderiv) ? "TRUE" : "false")
				, var_flags(v)
			);*/
			for(j=1;j<seq.n;++j){
				v = seq.vars[j];
				var_set_active(v,FALSE);
				var_set_value(v,0);
				/* VARMSG("Derivative '%s' SET ZERO AND INACTIVE"); */
			}
			continue;
		}

		/* VARMSG("We will use var '%s'"); */
		if(seq.n > 2){
			varname = var_make_name(integ->system,v);
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Too-long derivative chain with var '%s'",varname);
			ASC_FREE(varname);
			return 2;
		}else if(seq.n==2){
			/* diff var */
			if(var_apply_filter(seq.vars[1],&integrator_ida_deriv)){
				/* add the diff & deriv vars to the lists */
				n_y++;
				/* VARMSG("Added diff var '%s'");
				v = seq.vars[1]; VARMSG("and its derivative '%s'"); */
				continue;
			}
			/* VARMSG("Diff var '%s' being converted to alg var...");
			v = seq.vars[1]; VARMSG("...because deriv var '%s' fails filter"); */
			/* fall through */
		}

		/* VARMSG("Adding '%s' to algebraic"); */
		n_y++;
	}

#ifdef ANALYSE_DEBUG
	system_var_list_debug(integ->system);
#endif

	/* we assert that all vars in y meet the integrator_ida_nonde6: v_wind
riv filter */
	/* we assert that all vars in ydot meet the integrator_ida_deriv filter */

#ifdef ANALYSE_DEBUG
	MSG("Found %d good non-derivative vars", n_y);
#endif
	integ->n_y = n_y;

	return 0;
}

/**
	Flag relations that contain derivatives as 'REL_DIFFERENTIAL'

	@TODO what to do about relations that make reference to 't'?
*/
static int integrator_ida_flag_rels(IntegratorSystem *integ){
	int i, n, c, nd=0;
	struct rel_relation **rels;
	n = slv_get_num_solvers_rels(integ->system);
	rels = slv_get_solvers_rel_list(integ->system);
	for(i=0;i<n;++i){
		c = rel_classify_differential(rels[i]);
		if(c){
			nd++;
			/* MSG("Rel %d is DIFFERENTIAL", i); */
		}
	}
	MSG("Found %d differential equations (so %d algebraic)",nd, n - nd);
	integ->n_diffeqs = nd;
	return 0;
}

/**
	Sort the lists. First we will put the non-derivative vars, then we will
	put the derivative vars. Then we will put all the others.

	See http://ascendwiki.cheme.cmu.edu/File:Diffvars.png
*/
static int integrator_ida_sort_rels_and_vars(IntegratorSystem *integ){
	int ny1, nydot, nr;


#ifdef ANALYSE_DEBUG
	MSG("BEFORE SORTING RELS AND VARS");
	system_diffvars_debug(integ->system,stderr);
#endif

	/* we should not have allocated y or ydot yet */
	asc_assert(integ->y==NULL && integ->ydot==NULL);

	/* but we should have found some variables (and know how many) */
	asc_assert(integ->n_y);

	if(integrator_ida_rebuild_var_order(integ, &ny1, &nydot)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem ordering IDA variables");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	MSG("Cut %d non-derivative vars to start of list. cf integ->n_y = %d",ny1,integ->n_y);
#endif
	if(ny1 != integ->n_y){
		ERROR_REPORTER_HERE(ASC_PROG_ERR
			,"Unable to order IDA variables consistently (expected %d state/algebraic vars, found %d incident vars)."
			, integ->n_y, ny1
		);
		return 2;
	}

	MSG("moving derivs to start of remainder");

	if(system_cut_rels(integ->system, 0, &integrator_ida_rel, &nr)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem cutting derivs");
		return 1;
	}

	if(ny1 != nr){
		int dof_status = 5;
		int dof = 0;
		int have_dof_status = integrator_ida_get_dof_status(integ, &dof_status, &dof);

		if(have_dof_status && dof_status != 2){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,
				"Model is not in a consistent first-order DAE form for IDA (variables=%d, differential relations=%d)."
				" The active integration problem is %s."
				" Check DOF/free-variable diagnostics first."
				" If startup later fails, also check that INITIAL equations and FIX/FREE settings are square."
				, ny1
				, nr
				, integrator_ida_dof_status_name(dof_status)
			);
			if(dof_status == 1 || dof_status == 4){
				ERROR_REPORTER_NOLINE(ASC_USER_ERROR, "Active-problem DOF = %d", dof);
			}
		}else if(have_dof_status && dof_status == 2){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,
				"Model is not in a consistent first-order DAE form for IDA (variables=%d, differential relations=%d)."
				" The active integration problem appears square, so this is more likely a genuine high-index DAE"
				" or constrained system. Investigate with Pantelides / index-reduction diagnostics."
				, ny1, nr
			);
		}else{
			ERROR_REPORTER_HERE(ASC_USER_ERROR,
				"Model is not in a consistent first-order DAE form for IDA (variables=%d, differential relations=%d)."
				" Unable to classify squareness automatically."
				" Check DOF/free-variable diagnostics first, then Pantelides / index-reduction diagnostics."
				, ny1, nr
			);
		}
		return 3;
	}

	return 0;
}

/**
	Build up the lists 'ydot' and 'y_id.
	'ydot' allows us to find the derivative of a variable given its sindex.
	'y_id' allows us to locate the variable of a derivative given its sindex.

	Hence
	y_id[var_sindex(derivvar)-integ->n_y] = var_sindex(diffvar)
	ydot[var_sindex(diffvar)] = derivvar (or NULL if diffvar is algebraic)

	Note that there is 'shifting' required when looking up y_id.

	Note that we want to get rid of 'y' but for the moment we are also assigning
	that. We want to be rid of it because if the vars are ordered correctly
	it shouldn't be needed.
*/
static int integrator_ida_create_lists(IntegratorSystem *integ){
	const SolverDiffVarCollection *diffvars;
	int i, j, n_good;
	struct var_variable *v;

	SolverDiffVarSequence seq;

	asc_assert(integ->y==NULL);
	asc_assert(integ->ydot==NULL);
	asc_assert(integ->y_id== NULL);

	/* get the the dervative chains from the system */
	diffvars = system_get_diffvars(integ->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}

	/* allocate space (more than enough) */
	integ->y = ASC_NEW_ARRAY(struct var_variable *,integ->n_y);
	integ->ydot = ASC_NEW_ARRAY_CLEAR(struct var_variable *,integ->n_y);
	integ->n_ydot = 0;
	for(i=0;i<integ->n_y;++i){
		asc_assert(integ->ydot[i] == 0);
	}

	#ifdef ANALYSE_DEBUG
	MSG("Passing through chains...");
	#endif
	n_good = 0;
	/* create the lists y and ydot, ignoring 'bad' vars */
	for(i=0; i<diffvars->nseqs; ++i){
		/* MSG("i = %d",i); */

		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];
		j = var_sindex(v);

		if(!var_apply_filter(v,&integrator_ida_nonderiv)){
			continue;
		}

		integ->y[j] = v;
		n_good++;
		/* VARMSG("'%s' is good non-deriv"); */

		if(seq.n > 1 && var_apply_filter(seq.vars[1],&integrator_ida_deriv)){
			v = seq.vars[1];
			asc_assert(var_sindex(v) >= integ->n_y);
			asc_assert(var_sindex(v)-integ->n_y < integ->n_y);

			integ->ydot[j] = v;
			integ->n_ydot++;
			/* VARMSG("'%s' is good deriv"); */
		}else{
			asc_assert(integ->ydot[j]==NULL);
		}
	}

	#ifdef ANALYSE_DEBUG
	MSG("Found %d good non-derivs",n_good);
	#endif
	/* create the list y_id by looking at non-NULLs from ydot */
	integ->y_id = ASC_NEW_ARRAY(int,integ->n_ydot);
	for(i=0,j=0; i <  integ->n_y; ++i){
		if(integ->ydot[i]==NULL)continue;
		/* v = integ->ydot[i]; VARMSG("deriv '%s'..."); */
		/* v = integ->y[i]; VARMSG("diff '%s'..."); */
		integ->y_id[var_sindex(integ->ydot[i]) - integ->n_y] = i;
		j++;
	}

	asc_assert(j==integ->n_ydot);

	return 0;
}

/**
	Check for index-1 DAE system. We plan to do this by checking that df/dyd'
	and dg/dya are both non-singular (and of course square). Valid systems
	can (we think?) be written that don't meet these requirements but they
	will have index problems and may not solve with IDA.
*/
int integrator_ida_check_index(IntegratorSystem *integ){
#if 1
	linsolqr_system_t L;
	mtx_range_t range;
	mtx_region_t R;
	int res, r, index_error;
	struct SystemJacobianStruct df_dydp, dg_dya;

	MSG("system has total of %d rels and %d vars"
		,slv_get_num_solvers_rels(integ->system)
		,slv_get_num_solvers_vars(integ->system)
	);

	MSG("VAR_DERIV = 0x%x = %d",VAR_DERIV, VAR_DERIV);
	MSG("system_vfilter_deriv.matchbits = 0x%x",system_vfilter_deriv.matchbits);
	MSG("system_vfilter_deriv.matchvalue= 0x%x",system_vfilter_deriv.matchvalue);

	asc_assert(system_vfilter_deriv.matchbits & VAR_DERIV);
	asc_assert(system_vfilter_deriv.matchvalue & VAR_DERIV);

	MSG("system has %d vars matching deriv filter",slv_count_solvers_vars(integ->system, &system_vfilter_deriv));

	res = system_jacobian(integ->system
		, &system_rfilter_diff
		, &system_vfilter_deriv
		, 1 /* safe evaluation */
		, &df_dydp
	);

	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error calculating df/dyd'");
		return 1;
	}
	MSG("df/dyd': nr = %d, nv = %d",df_dydp.n_rels,df_dydp.n_vars);

	res = system_jacobian(integ->system
		, &system_rfilter_algeb
		, &system_vfilter_algeb
		, 1 /* safe evaluation */
		, &dg_dya
	);

	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error calculating dg/dya");
		ASC_FREE(df_dydp.vars);
		ASC_FREE(df_dydp.rels);
		if(df_dydp.M)mtx_destroy(df_dydp.M);
		return 1;
	}
	MSG("dg/dya: nr = %d, nv = %d",dg_dya.n_rels,dg_dya.n_vars);

	if((df_dydp.n_rels == 0) ^ (df_dydp.n_vars == 0)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"df/dyd' is a bit ambiguous");
	}

	index_error = 0;

	if(dg_dya.n_rels <= 0){
		MSG("No algebraic equations were found in the DAE system.");
	}else if(dg_dya.n_rels != dg_dya.n_vars){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"The algebraic part of the DAE jacobian, dg/dya, is not square!");
		index_error = 1;
	}else{
		/* check the rank */
		range.low = 0; range.high = mtx_order(dg_dya.M) - 1;
		R.row = range; R.col = range;

		L = linsolqr_create_default();
		linsolqr_set_matrix(L,dg_dya.M);
		linsolqr_set_region(L,R);
		linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
		linsolqr_reorder(L, &R, linsolqr_rmethod(L));
		linsolqr_factor(L,linsolqr_fmethod(L));
		r = linsolqr_rank(L);

		linsolqr_destroy(L);

		if(r != dg_dya.n_rels){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Your DAE system has an index problem: the matrix dg/dya is not full rank");
			index_error = 1;
		}
	}

	ASC_FREE(dg_dya.vars);
	ASC_FREE(dg_dya.rels);
	if(dg_dya.M)mtx_destroy(dg_dya.M);

	if(df_dydp.n_rels <= 0){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"No differential equations were found in the DAE system!");
	}else if(df_dydp.n_rels != df_dydp.n_vars){
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"The differential part of the the jacobian dg/dya is not square!");
		ASC_FREE(df_dydp.vars);
		ASC_FREE(df_dydp.rels);
		if(df_dydp.M)mtx_destroy(df_dydp.M);
		return 1;
	}else{
		/* check the rank */
		range.low = 0; range.high = mtx_order(df_dydp.M) - 1;
		R.row = range; R.col = range;

		L = linsolqr_create_default();
		linsolqr_set_matrix(L,df_dydp.M);
		linsolqr_set_region(L,R);
		linsolqr_prep(L,linsolqr_fmethod_to_fclass(linsolqr_fmethod(L)));
		linsolqr_reorder(L, &R, linsolqr_rmethod(L));
		linsolqr_factor(L,linsolqr_fmethod(L));
		r = linsolqr_rank(L);

		linsolqr_destroy(L);

		if(r != df_dydp.n_rels){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Your DAE system has an index problem: the matrix df/dyd' is not full rank");
			index_error = 1;
		}
	}

	if(df_dydp.n_rels + dg_dya.n_rels == 0){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Both df/dyd' and dg/dya were empty!");
	}

	ASC_FREE(df_dydp.vars);
	ASC_FREE(df_dydp.rels);
	if(df_dydp.M)mtx_destroy(df_dydp.M);
	return index_error;
#else
	MSG("check_index disabled");
	return 0;
#endif
}

/*------------------------------------------------------------------------------
  ANALYSIS ROUTINE (new implementation)
*/

/**
	Perform additional problem analysis to prepare problem for integration with
	IDA.

	We assume that the analyse_generate_diffvars routine has been called, so
	that we just need to call slv_get_diffvars to access the derivative
	chains.

	We can also assume that the independent variable has been found.

	See mailing list, ~Jan 2007.

	Note, the stuff for identifying the static and output sub-problems should
	be part of integrator.c, not this file. We will assume this is handled

	@return 0 on success
	@see integrator_analyse
*/
int integrator_ida_analyse(IntegratorSystem *integ){
	int res;
	const SolverDiffVarCollection *diffvars;
	int i;
#ifdef ANALYSE_DEBUG
	char *varname;
#endif

	asc_assert(ASC_INTEG_ENGINE_IS_IDA_FAMILY(integ));

	MSG("System contains a total of %d bnds and %d rels"
		,slv_get_num_solvers_bnds(integ->system)
		,slv_get_num_solvers_rels(integ->system)
	);

	/* set the active flags on  variables depending on the state of WHENs */
	MSG("Currently %d rels active",slv_count_solvers_rels(integ->system, &integrator_ida_rel));

	reanalyze_solver_lists(integ->system);

	MSG("After analysing WHENs, there are %d rels active"
		,slv_count_solvers_rels(integ->system, &integrator_ida_rel)
	);


#ifdef ANALYSE_DEBUG
	MSG("Starting IDA analysis");
#endif

	/* set the flags on differential and derivative and algebraic vars */
	res = integrator_ida_check_vars(integ);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem with vars");
		return 1;
	}

	res = integrator_ida_flag_rels(integ);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem classifying differential equations");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	MSG("Sorting rels and vars");
#endif

	res = integrator_ida_sort_rels_and_vars(integ);
	if(res){
		if(res != 3){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem sorting rels and vars");
		}
		return 1;
	}

#ifdef ANALYSE_DEBUG
	MSG("Creating lists");

	MSG("BEFORE MAKING LISTS");
	integrator_ida_debug(integ,stderr);
#endif

	res = integrator_ida_create_lists(integ);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem creating lists");
		return 1;
	}

	MSG("After ida_create_lists, there are %d rels active"
		,slv_count_solvers_rels(integ->system, &integrator_ida_rel)
	);

#ifdef ANALYSE_DEBUG
	MSG("Checking lists");

	asc_assert(integ->y);
	asc_assert(integ->ydot);
	asc_assert(integ->y_id);

	integrator_ida_debug(integ,stderr);
#endif

	if(integrator_ida_check_diffindex(integ)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error with diffindex");
		return 360;
	}


	MSG("After ida_check_diffindex, there are %d rels active"
		,slv_count_solvers_rels(integ->system, &integrator_ida_rel)
	);

	/* the following causes TestIDA.testincidence3 to fail:
	if(integ->n_y != slv_get_num_solvers_rels(integ->system)){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Problem is not square");
		return 2;
	}*/

#if 0
	/* check structural singularity for the two IDACalcIC scenarios */

	/* ...(1) FIX the derivatives */
	MSG("Checking system with derivatives fixed...");
	for(i=0;i<integ->n_y;++i){
		if(integ->ydot[i])var_set_fixed(integ->ydot[i],1);
	}

	slv_block_partition(integ->system);
	res = integrator_ida_block_check(integ);
	if(res)return 100 + res;

	/* ...(2) FREE the derivatives, FIX the diffvars */
	MSG("Checking system with differential variables fixed...");
	for(i=0;i<integ->n_y;++i){
		if(integ->ydot[i]){
			var_set_fixed(integ->ydot[i],0);
			var_set_fixed(integ->y[i],1);
		}
	}
	slv_block_partition(integ->system);
	res = integrator_ida_block_check(integ);
	if(res)return 200 + res;

	/* ...(3) restore as it was, FREE the diffvars */
	for(i=0;i<integ->n_y;++i){
		if(integ->ydot[i]){
			var_set_fixed(integ->y[i],0);
		}
	}
#endif

	res = integrator_ida_check_index(integ);
	if(res){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Your DAE system has an index problem");
		return 100 + res;
	}

	MSG("After ida_check_index, there are %d rels active"
		,slv_count_solvers_rels(integ->system, &integrator_ida_rel)
	);


	/* get the the dervative chains from the system */
	diffvars = system_get_diffvars(integ->system);

	/* check the indep var is same as was located earlier */
	if(diffvars->nindep>1){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Multiple variables specified as independent (ode_type=-1)");
		return 3;
	}else if(diffvars->nindep<1){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Independent var not set (ode_type=-1)");
		return 4;
	}else if(diffvars->indep[0]!=integ->x){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Indep var doesn't match");
		return 5;
	}

#ifdef ANALYSE_DEBUG
	MSG("Collecting observed variables");
#endif

	/* get the observations */
	/** @TODO this should use a 'problem provider' API instead of static data
	from the system object */
	integ->n_obs = diffvars->nobs;
	integ->obs = ASC_NEW_ARRAY(struct var_variable *,integ->n_obs);
	for(i=0;i<integ->n_obs;++i){
		/* we get them all, regardless of flags etc */
		integ->obs[i] = diffvars->obs[i];
#ifdef ANALYSE_DEBUG
		varname = var_make_name(integ->system,integ->obs[i]);
		MSG("'%s' is observation",varname);
		ASC_FREE(varname);
#endif
	}

	MSG("rels matchbits:  0x%x",integrator_ida_rel.matchbits);
	MSG("rels matchvalue: 0x%x",integrator_ida_rel.matchvalue);

	MSG("At the end of ida_analyse, there are %d rels active"
		,slv_count_solvers_rels(integ->system, &integrator_ida_rel)
	);

	return 0;
}

#if 0 /* disused function, apparently -- JP */
static int integrator_ida_check_partitioning(IntegratorSystem *integ){
	long i, nv;
	int err=0;
	char *varname;
	struct var_variable **vlist, *v;
	nv = slv_get_num_solvers_vars(integ->system);
	vlist = slv_get_solvers_var_list(integ->system);
	var_filter_t vf = {VAR_SVAR|VAR_ACTIVE|VAR_INCIDENT|VAR_FIXED
					  ,VAR_SVAR|VAR_ACTIVE|VAR_INCIDENT|0 };
	for(i=0;i<nv;++i){
		v=vlist[i];
		if(!var_apply_filter(v,&vf))continue;
		varname = var_make_name(integ->system,v);
		if(!var_deriv(v)){
#ifdef ANALYSE_DEBUG
			fprintf(stderr,"vlist[%ld] = '%s' (nonderiv)\n",i,varname);
#endif
			if(i>=integ->n_y){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"non-deriv var '%s' is not at the start",varname);
				err++;
			}
		}else{
#ifdef ANALYSE_DEBUG
			fprintf(stderr,"vlist[%ld] = '%s' (derivative)\n",i,varname);
#endif
			if(i<integ->n_y){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"deriv var '%s' is not at the end (n_y = %d, i = %d)"
					,varname, integ->n_y, i
				);
				err++;
			}
		}
		ASC_FREE(varname);
	}
	if(err){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error in IDA partitioning");
		return 1;
	}
	return 0;
}
#endif

int integrator_ida_block_check(IntegratorSystem *integ){
	int res;
 	int dof;
#ifdef ANALYSE_DEBUG
	int *vlist, *vp, i, nv, nv_ok;
	char *varname;
	struct var_variable **solversvars;

	var_filter_t vfilt = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_FIXED,
		VAR_ACTIVE | VAR_INCIDENT | 0
	};

	nv = slv_get_num_solvers_vars(integ->system);
	solversvars = slv_get_solvers_var_list(integ->system);
	MSG("-------------- nv = %d -------------",nv);
	for(nv_ok=0, i=0; i < nv; ++i){
		if(var_apply_filter(solversvars[i],&vfilt)){
			varname = var_make_name(integ->system,solversvars[i]);
			fprintf(stderr,"%s\n",varname);
			ASC_FREE(varname);
			nv_ok++;
		}
	}
	MSG("----------- got %d ok -------------",nv_ok);
#endif

	if(!integrator_ida_get_dof_status(integ, &res, &dof)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to determine DOF status");
		return -1;
	}
	switch(res){
		case 1: MSG("System is underspecified (%d degrees of freedom)",dof);break;
		case 2: MSG("System is square"); return 0; /* all OK */
		case 3: MSG("System is structurally singular"); break;
		case 4: MSG("System is overspecified"); break;
		default:
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unrecognised slfDOF_status");
			return -2;
	}

#ifdef ANALYSE_DEBUG
	/* if it was underspecified, what vars could be fixed? */
	if(res==1){
		MSG("Need to FIX %d of the following vars:",dof);
		solversvars = slv_get_solvers_var_list(integ->system);
		if(!slvDOF_eligible(integ->system, &vlist)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to det slvDOF_eligble list");
			return -3;
		}
		for(vp=vlist;*vp!=-1;++vp){
			varname = var_make_name(integ->system, solversvars[*vp]);
			MSG("Fixable var: %s",varname);
			ASC_FREE(varname);
		}
		MSG("(Found %d fixable vars)",(int)(vp-vlist));
		return 1;
	}
#endif

	return res;
}

/* return 0 on succes */
static int check_dups(IntegratorSystem *integ, struct var_variable **list,int n,int allownull){
	int i,j;
	struct var_variable *v;
#ifdef ANALYSE_DEBUG
	char *varname;
#endif
	for(i=0; i< n; ++i){
		v=list[i];
		if(v==NULL){
			if(allownull)continue;
			else return 2;
		}
		asc_assert(v!=(void *)0x31);
		for(j=0; j<i-1;++j){
			if(list[j]==NULL)continue;
			if(v==list[j]){
#ifdef ANALYSE_DEBUG
				varname = var_make_name(integ->system,v);
				if(varname){
					MSG("Duplicate of '%s' found",varname);
					ASC_FREE(varname);
				}else{
					MSG("Duplicate found (couldn't retrieve name)");
				}
				ASC_FREE(varname);
#endif
				return 1;
			}
		}
	}
	return 0;
}

/*
	We are going to check that

	  - n_y in range (0,n_vars)
	  - no duplicates anywhere in the varlist
	  - no duplicates in non-NULL elements of ydot

	  - first n_y vars in solver's var list match those in vector y and match the non-deriv filter.
	  - var_sindex for first n_y vars match their position the the solver's var list

	  - ydot contains n_ydot non-NULL elements, each match the deriv filter.
	  - vars in solver's var list positions [n_y,n_y+n_ydot) all match deriv filter

	  - y_id contains n_ydot elements, with int values in range [0,n_y)
	  - var_sindex(ydot[y_id[i]]) - n_y == i for i in [0,n_ydot)

	  - all the vars [n_y+n_ydot,n_vars) fail the non-deriv filter and fail the deriv filter.
*/
static int integrator_ida_check_diffindex(IntegratorSystem *integ){
	int i, n_vars, n_ydot=0;
	struct var_variable **list, *v;
	char *varname;
	const char *msg;

#ifdef ANALYSE_DEBUG
	MSG("Checking diffindex vector");
#endif

	if(integ->y_id == NULL || integ->y == NULL || integ->ydot == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"list(s) NULL");
		return 1;
	}

	list = slv_get_solvers_var_list(integ->system);
	n_vars = slv_get_num_solvers_vars(integ->system);

	if(check_dups(integ, list, n_vars,FALSE)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"duplicates or NULLs in solver's var list");
		return 1;
	}

	if(check_dups(integ, integ->ydot, integ->n_y,TRUE)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"duplicates in ydot vector");
		return 1;
	}

	/* check n_y in range */
	if(integ->n_y <= 0 || integ->n_y >= n_vars){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"n_y = %d invalid (n_vars = %d)",integ->n_y, n_vars);
		return 1;
	}

	/* check first n_y vars */
	for(i=0; i < integ->n_y; ++i){
		v = list[i];
		if(!var_apply_filter(v, &integrator_ida_nonderiv)){
			msg = "'%s' (in first n_y vars) fails non-deriv filter"; goto finish;
		}else if(var_sindex(v) != i){
			msg = "'%s' has wrong var_sindex"; goto finish;
		}else if(v != integ->y[i]){
			msg = "'%s' not matched in y vector"; goto finish;
		}
		/* meets filter, matches in y vector, has correct var_sindex. */
	}

	/* count non-NULLs in ydot */
	for(i=0; i < integ->n_y; ++i){
		v = integ->ydot[i];
		if(v!=NULL){
			if(var_sindex(v) < integ->n_y){
				msg = "'%s' has var_sindex < n_y"; goto finish;
			}else if(!var_apply_filter(v,&integrator_ida_deriv)){
				msg = "'%s' (in next n_ydot vars) fails deriv filter"; goto finish;
			}
			/* lies beyond first n_y vars, match deriv filter */
			n_ydot++;
		}
	}

	/* check that vars [n_y, n_y+n_ydot) in solver's var list match the deriv filter */
	for(i=integ->n_y; i< integ->n_y + n_ydot; ++i){
		v = list[i];
		if(!var_apply_filter(v,&integrator_ida_deriv)){
			msg = "'%s', in range [n_y,n_y+n_ydot), fails deriv filter"; goto finish;
		}
	}

	/* check values in y_id are ints int range [0,n_y), and that they point to correct vars */
	for(i=0; i<n_ydot; ++i){
		if(integ->y_id[i] < 0 || integ->y_id[i] >= integ->n_y){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"value of y_id[%d] is out of range",i);
			return 1;
		}
		v = integ->ydot[integ->y_id[i]];
		if(var_sindex(v) - integ->n_y != i){
			msg = "'%s' not index correctly from y_id"; goto finish;
		}
	}

	/* check remaining vars fail both filters */
	for(i=integ->n_y + n_ydot; i<n_vars; ++i){
		v = list[i];
		if(var_apply_filter(v,&integrator_ida_nonderiv)){
			msg = "Var '%s' at end meets non-deriv filter, but shouldn't"; goto finish;
		}
		if(var_apply_filter(v,&integrator_ida_deriv)){
			MSG("position = %d",i);
			msg = "Var '%s' at end meets deriv filter, but shouldn't"; goto finish;
		}
	}

	return 0;
finish:
	varname = var_make_name(integ->system,v);
	ERROR_REPORTER_HERE(ASC_PROG_ERR,msg,varname);
	ASC_FREE(varname);
	return 1;
}

/**
	Given a derivative variable, return the index of its corresponding differential
	variable in the y vector (and equivalently the var_sindex of the diff var)
*/
int integrator_ida_diffindex(const IntegratorSystem *integ, const struct var_variable *deriv){
	asc_assert(var_sindex(deriv) >= integ->n_y);
	asc_assert(var_sindex(deriv) < integ->n_y + integ->n_ydot);
	return integ->y_id[var_sindex(deriv) - integ->n_y];
}

/**
	A less assertive version of diffindex...
*/
int integrator_ida_diffindex1(const IntegratorSystem *integ, const struct var_variable *deriv){
	if(var_sindex(deriv) >= integ->n_y)return -1;
	if(var_sindex(deriv) < integ->n_y + integ->n_ydot)return -2;
	return integ->y_id[var_sindex(deriv) - integ->n_y];
}

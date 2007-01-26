#include "idaanalyse.h"

#include <utilities/ascPanic.h>

#ifdef ASC_IDA_NEW_ANALYSE
# include <solver/diffvars.h>
# include <solver/slv_stdcalls.h>
# include <solver/block.h>
#include <solver/slvDOF.h>
#endif

#define ANALYSE_DEBUG

static int integrator_ida_check_partitioning(IntegratorSystem *sys);
static int integrator_ida_check_diffindex(IntegratorSystem *sys);
static int integrator_ida_rebuild_diffindex(IntegratorSystem *sys);

const var_filter_t integrator_ida_nonderiv = {
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | VAR_FIXED | VAR_DERIV,
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0         | 0
};

const var_filter_t integrator_ida_deriv = {
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | VAR_FIXED | VAR_DERIV,
	VAR_SVAR | VAR_INCIDENT | VAR_ACTIVE | 0         | VAR_DERIV
};

const rel_filter_t integrator_ida_rel = {
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE,
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE 
};

/**
	Check derivative chains: if a var or derivative is inelligible,
	follow down the chain fixing & setting zero any derivatives.
*/
static int integrator_ida_check_vars(IntegratorSystem *sys){
	const SolverDiffVarCollection *diffvars;
	char *varname, *derivname;
	int n_y = 0;
	int i, j;
	struct var_variable *v;

	SolverDiffVarSequence seq;

	/* we shouldn't have allocated these yet: just be sure */
	asc_assert(sys->y==NULL);
	asc_assert(sys->ydot==NULL);
	asc_assert(sys->n_y==0);

	/* get the the dervative chains from the system */
	diffvars = slv_get_diffvars(sys->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}
	
	/* add the variables from the derivative chains */
	for(i=0; i<diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];
		if(!var_apply_filter(v,&integrator_ida_nonderiv)){
			varname = var_make_name(sys->system,v);
			CONSOLE_DEBUG("'%s' fails non-deriv filter",varname);
			ASC_FREE(varname);			
			for(j=1;j<seq.n;++j){
				v = seq.vars[j];
				var_set_active(v,FALSE);
				var_set_value(v,0);
				varname = var_make_name(sys->system,v);				
				CONSOLE_DEBUG("Derivative '%s' SET ZERO AND INACTIVE",varname);
				ASC_FREE(v);
			}
			continue;
		}

		varname = var_make_name(sys->system,v);
		CONSOLE_DEBUG("We will use var '%s'",varname);
		ASC_FREE(varname);
		if(seq.n > 2){
			varname = var_make_name(sys->system,v);				
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Too-long derivative chain with var '%s'");
			ASC_FREE(varname);
			return 2;
		}else if(seq.n==2){
			/* diff var */
			if(var_apply_filter(seq.vars[1],&integrator_ida_deriv)){
				/* add the diff & deriv vars to the lists */
				n_y++;
				varname = var_make_name(sys->system,v);
				derivname = var_make_name(sys->system,seq.vars[1]);
				CONSOLE_DEBUG("Added '%s' and '%s'",varname,derivname);
				ASC_FREE(varname);
				ASC_FREE(derivname);
				continue;
			}
			varname = var_make_name(sys->system,v);
			derivname = var_make_name(sys->system,seq.vars[1]);
			CONSOLE_DEBUG("Derivative '%s' of '%s' fails filter; convert '%s' to algebraic",derivname,varname,varname);
			ASC_FREE(varname);
			ASC_FREE(derivname);
			/* fall through */
		}

		varname = var_make_name(sys->system,v);
		CONSOLE_DEBUG("Adding '%s' to algebraic",varname);
		ASC_FREE(varname);
		n_y++;
	}

	/* we assert that all vars in y meet the integrator_ida_nonderiv filter */
	/* we assert that all vars in ydot meet the integrator_ida_deriv filter */

	CONSOLE_DEBUG("Found %d good non-derivative vars", n_y);
	sys->n_y = n_y;
	return 0;
}


/**
	Sort the lists. First we will put the non-derivative vars, then we will
	put the derivative vars. Then we will put all the others.
*/
static int integrator_ida_sort_rels_and_vars(IntegratorSystem *sys){
	int ny1, nydot, nr;

	/* we should not have allocated y or ydot yet */
	asc_assert(sys->y==NULL && sys->ydot==NULL);

	/* but we should have found some variables (and know how many) */
	asc_assert(sys->n_y);

	if(system_cut_vars(sys->system, 0, &integrator_ida_nonderiv, &ny1)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem cutting non-derivs");
		return 1;
	}

	CONSOLE_DEBUG("cut_vars: ny1 = %d, sys->n_y = %d",ny1,sys->n_y);
	asc_assert(ny1 == sys->n_y);

	if(system_cut_vars(sys->system, ny1, &integrator_ida_deriv, &nydot)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem cutting derivs");
		return 1;
	}

	if(system_cut_rels(sys->system, 0, &integrator_ida_rel, &nr)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem cutting derivs");
		return 1;
	}

	if(ny1 != nr){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem is not square (ny = %d, nr = %d)",ny1,nr);
		return 2;
	}

	return 0;
}

/**
	Build up the lists 'ydot' and 'y_id.
	'ydot' allows us to find the derivative of a variable given its sindex.
	'y_id' allows us to locate the variable of a derivative given its sindex.

	Hence
	y_id[var_sindex(derivvar)-sys->n_y] = var_sindex(diffvar)
	ydot[var_sindex(diffvar)] = derivvar (or NULL if diffvar is algebraic)

	Note that there is 'shifting' required when looking up y_id.

	Note that we want to get rid of 'y' but for the moment we are also assigning
	that. We want to be rid of it because if the vars are ordered correctly
	it shouldn't be needed.
*/
static int integrator_ida_create_lists(IntegratorSystem *sys){
	const SolverDiffVarCollection *diffvars;
	int i, j;
	struct var_variable *v;

	SolverDiffVarSequence seq;

	asc_assert(sys->y==NULL);
	asc_assert(sys->ydot==NULL);
	asc_assert(sys->y_id== NULL);

	/* get the the dervative chains from the system */
	diffvars = slv_get_diffvars(sys->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}
	
	/* allocate space (more than enough) */
	sys->y = ASC_NEW_ARRAY(struct var_variable *,sys->n_y);
	sys->y_id = ASC_NEW_ARRAY_CLEAR(int,sys->n_y);
	sys->ydot = ASC_NEW_ARRAY_CLEAR(struct var_variable *,sys->n_y);
	j = 0;

	/* add the variables from the derivative chains */
	for(i=0; i<diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];
		if(!var_apply_filter(v,&integrator_ida_nonderiv)){
			continue;
		}

		if(seq.n > 1 && var_apply_filter(seq.vars[1],&integrator_ida_deriv)){
			asc_assert(var_sindex(seq.vars[1])-sys->n_y >= 0);
			asc_assert(var_sindex(seq.vars[1])-sys->n_y < sys->n_y);
			sys->y_id[var_sindex(seq.vars[1])-sys->n_y] = j;
			sys->ydot[j] = seq.vars[1];
		}else{
			asc_assert(sys->ydot[j]==NULL);
		}

		sys->y[j] = v;
		j++;
	}

	return 0;
}

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
int integrator_ida_analyse(IntegratorSystem *sys){
	int res;
	const SolverDiffVarCollection *diffvars;
	char *varname;
	int i;

	asc_assert(sys->engine==INTEG_IDA);

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Starting IDA analysis");
#endif

	res = integrator_ida_check_vars(sys);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem with vars");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Sorting rels and vars");
#endif

	res = integrator_ida_sort_rels_and_vars(sys);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem sorting rels and vars");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Creating lists");
#endif

	res = integrator_ida_create_lists(sys);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem creating lists");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Checking");
#endif

	asc_assert(sys->y);
	asc_assert(sys->ydot);
	asc_assert(sys->y_id);

	integrator_ida_analyse_debug(sys,stderr);

	if(integrator_ida_check_diffindex(sys)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error with diffindex");
		return 360;
	}

	/* the following causes TestIDA.testincidence3 to fail:
	if(sys->n_y != slv_get_num_solvers_rels(sys->system)){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Problem is not square");
		return 2;
	}*/

#if 0
	/* check structural singularity for the two IDACalcIC scenarios */

	/* ...(1) FIX the derivatives */
	CONSOLE_DEBUG("Checking system with derivatives fixed...");
	for(i=0;i<n_y;++i){
		if(sys->ydot[i])var_set_fixed(sys->ydot[i],1);
	}

	slv_block_partition(sys->system);
	res = integrator_ida_block_check(sys);
	if(res)return 100 + res;

	/* ...(2) FREE the derivatives, FIX the diffvars */
	CONSOLE_DEBUG("Checking system with differential variables fixed...");
	for(i=0;i<n_y;++i){
		if(sys->ydot[i]){
			var_set_fixed(sys->ydot[i],0);
			var_set_fixed(sys->y[i],1);
		}
	}
	slv_block_partition(sys->system);
	res = integrator_ida_block_check(sys);
	if(res)return 200 + res;
	
	/* ...(3) restore as it was, FREE the diffvars */
	for(i=0;i<n_y;++i){
		if(sys->ydot[i]){
			var_set_fixed(sys->y[i],0);
		}
	}	
#endif

	/* get the the dervative chains from the system */
	diffvars = slv_get_diffvars(sys->system);

	/* check the indep var is same as was located elsewhere */
	if(diffvars->nindep>1){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Multiple variables specified as independent (ode_type=-1)");
		return 3;
	}else if(diffvars->nindep<1){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Independent var not set (ode_type=-1)");
		return 4;
	}else if(diffvars->indep[0]!=sys->x){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Indep var doesn't match");
		return 5;
	}

	/* get the observations */
	/** @TODO this should use a 'problem provider' API instead of static data
	from the system object */
	sys->n_obs = diffvars->nobs;
	sys->obs = ASC_NEW_ARRAY(struct var_variable *,sys->n_obs);
	for(i=0;i<sys->n_obs;++i){
		sys->obs[i] = diffvars->obs[i];
		varname = var_make_name(sys->system,sys->obs[i]);
		CONSOLE_DEBUG("'%s' is observation",varname);
		ASC_FREE(varname);
	}

	/*   - 'y' list as [ya|yd] */
	/*   - sparsity pattern for dF/dy and dF/dy' */
	/*   - sparsity pattern for union of above */
	/*   - block decomposition based on above */
    /*   - block decomposition results in reordering of y and y' */
	/*   - boundaries (optional) */
	/* ERROR_REPORTER_HERE(ASC_PROG_ERR,"Implementation incomplete");
	return -1; */

	return 0;
}

/*------------------------------------------------------------------------------
  ANALYSIS ROUTINE (new implementation)
*/

static int integrator_ida_check_partitioning(IntegratorSystem *sys){
	long i, nv;
	int err=0;
	char *varname;
	struct var_variable **vlist, *v;
	nv = slv_get_num_solvers_vars(sys->system);
	vlist = slv_get_solvers_var_list(sys->system);
	var_filter_t vf = {VAR_SVAR|VAR_ACTIVE|VAR_INCIDENT|VAR_FIXED
					  ,VAR_SVAR|VAR_ACTIVE|VAR_INCIDENT|0 };
	for(i=0;i<nv;++i){
		v=vlist[i];
		if(!var_apply_filter(v,&vf))continue;
		varname = var_make_name(sys->system,v);
		if(!var_deriv(v)){
			fprintf(stderr,"vlist[%ld] = '%s' (nonderiv)\n",i,varname);
			if(i>=sys->n_y){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"non-deriv var '%s' is not at the start",varname);
				err++;
			}
		}else{
			fprintf(stderr,"vlist[%ld] = '%s' (derivative)\n",i,varname);
			if(i<sys->n_y){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"deriv var '%s' is not at the end (n_y = %d, i = %d)"
					,varname, sys->n_y, i
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

int integrator_ida_block_check(IntegratorSystem *sys){
	int res;
 	int dof;
#ifdef ANALYSE_DEBUG
	long *vlist, *vp, i, nv, nv_ok;
	char *varname;
	struct var_variable **solversvars;

	var_filter_t vfilt = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_FIXED,
		VAR_ACTIVE | VAR_INCIDENT | 0
	};
			
	nv = slv_get_num_solvers_vars(sys->system);
	solversvars = slv_get_solvers_var_list(sys->system);
	CONSOLE_DEBUG("-------------- nv = %ld -------------",nv);
	for(nv_ok=0, i=0; i < nv; ++i){
		if(var_apply_filter(solversvars[i],&vfilt)){
			varname = var_make_name(sys->system,solversvars[i]);
			fprintf(stderr,"%s\n",varname);
			ASC_FREE(varname);
			nv_ok++;
		}
	}
	CONSOLE_DEBUG("----------- got %ld ok -------------",nv_ok);
#endif

	if(!slvDOF_status(sys->system, &res, &dof)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to determine DOF status");
		return -1;
	}
	switch(res){
		case 1: CONSOLE_DEBUG("System is underspecified (%d degrees of freedom)",dof);break;
		case 2: CONSOLE_DEBUG("System is square"); return 0; /* all OK */
		case 3: CONSOLE_DEBUG("System is structurally singular"); break;
		case 4: CONSOLE_DEBUG("System is overspecified"); break;
		default: 
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unrecognised slfDOF_status");
			return -2;
	}

#ifdef ANALYSE_DEBUG
	/* if it was underspecified, what vars could be fixed? */
	if(res==1){
		CONSOLE_DEBUG("Need to FIX %d of the following vars:",dof);
		solversvars = slv_get_solvers_var_list(sys->system);
		if(!slvDOF_eligible(sys->system, &vlist)){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to det slvDOF_eligble list");
			return -3;
		}
		for(vp=vlist;*vp!=-1;++vp){
			varname = var_make_name(sys->system, solversvars[*vp]);
			CONSOLE_DEBUG("Fixable var: %s",varname);
			ASC_FREE(varname);
		}
		CONSOLE_DEBUG("(Found %d fixable vars)",(int)(vp-vlist));
		return 1;
	}
#endif

	return res;
}

static int integrator_ida_check_diffindex(IntegratorSystem *sys){
	int i, nv, err = 0;
	struct var_variable **vlist;
	int diffindex;

	CONSOLE_DEBUG("Checking diffindex vector");

	if(sys->y_id == NULL){
		CONSOLE_DEBUG("y_id not allocated");
		return 1;
	}

	vlist = slv_get_solvers_var_list(sys->system);
	nv = slv_get_num_solvers_vars(sys->system);

	for(i=0; i<nv; ++i){
		if(var_deriv(vlist[i])){
			if(i < sys->n_y){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Deriv in wrong position");
				err++;
			}else{
				if(var_sindex(vlist[i])!=i){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Deriv var with incorrect sindex");
					err++;
				}
				diffindex = sys->y_id[i - sys->n_y];
				if(diffindex >= sys->n_y){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Diff y_id[%d] value too big",i-sys->n_y);
					err++;
				}
				if(var_sindex(vlist[diffindex])!=diffindex){
					ERROR_REPORTER_HERE(ASC_PROG_ERR,"Diff var with incorrect sindex");
					err++;
				}
			}
		}else{
			if(i!=var_sindex(vlist[i])){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"var_sindex incorrect");
				err++;
			}
		}
	}
	if(err){
		CONSOLE_DEBUG("Errors found in diffindex");
	}
	return err;
}

/**
	@TODO provide a macro implementation of this
*/
int integrator_ida_diffindex(const IntegratorSystem *sys, const struct var_variable *deriv){
	int diffindex;
#ifdef DIFFINDEX_DEBUG
	asc_assert( var_deriv    (deriv));
	asc_assert( var_active   (deriv));
	asc_assert( var_incident (deriv));
	asc_assert( var_svar     (deriv));

	asc_assert(!var_fixed    (deriv));

	asc_assert(var_sindex(deriv) >= sys->n_y);
	asc_assert(diffindex == var_sindex(sys->y[diffindex]));
#endif
	asc_assert(var_sindex(deriv) >= sys->n_y);
	diffindex = sys->y_id[var_sindex(deriv) - sys->n_y];

	return diffindex;
}


int integrator_ida_analyse_debug(const IntegratorSystem *sys,FILE *fp){
	return analyse_diffvars_debug(sys->system,fp);
}


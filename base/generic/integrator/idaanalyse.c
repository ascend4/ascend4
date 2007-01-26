#include "idaanalyse.h"

#include <utilities/ascPanic.h>

#ifdef ASC_IDA_NEW_ANALYSE
# include <solver/diffvars.h>
# include <solver/slv_stdcalls.h>
# include <solver/block.h>
#include <solver/slvDOF.h>
#endif

static int integrator_ida_check_partitioning(IntegratorSystem *sys);
static int integrator_ida_check_diffindex(IntegratorSystem *sys);
static int integrator_ida_rebuild_diffindex(IntegratorSystem *sys);

#ifdef ASC_IDA_NEW_ANALYSE
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
/*	struct var_variable **solversvars;
	unsigned long nsolversvars;
	struct rel_relation **solversrels;
	unsigned long nsolversrels;*/
	const SolverDiffVarCollection *diffvars;
	SolverDiffVarSequence seq;
	long i, j, n_y, n_ydot, n_skipped_diff, n_skipped_alg, n_skipped_deriv;
	/* int res; */

	struct var_variable *v;
	char *varname;

	asc_assert(sys->engine==INTEG_IDA);

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Starting IDA analysis");
#endif

#if 0
	/* partition into static, dynamic and output problems */
	CONSOLE_DEBUG("Block partitioning system...");
	if(slv_block_partition(sys->system)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Unable to block-partition system");
		return 1;
	}
#endif

	/* check that we have some relations */
	if(0==slv_get_num_solvers_rels(sys->system)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"There are no relations!");
		return -1;
	}

	/* get the vars and mark those which are not incident/active etc */

	/* set up the dynamic problem */
	CONSOLE_DEBUG("Setting up the dynamic problem");

	diffvars = slv_get_diffvars(sys->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 6;
	}

	CONSOLE_DEBUG("Got %ld chains, maxorder = %ld",diffvars->nseqs,diffvars->maxorder);
	
	if(diffvars->maxorder > 2){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"System higher-order derivatives. You must manually reduce the"
			" system to a first-order system of DAEs. (maxorder=%d)"
			,diffvars->maxorder
		);
		return 1;
	};

	asc_assert(sys->y == NULL);

	sys->y = ASC_NEW_ARRAY(struct var_variable *,diffvars->nalg + diffvars->ndiff);
	sys->ydot = ASC_NEW_ARRAY(struct var_variable *,diffvars->nalg + diffvars->ndiff);
	n_y = 0; n_ydot = 0;
	n_skipped_alg = 0; n_skipped_diff = 0; n_skipped_deriv = 0;
	
	/* add the variables from the derivative chains */
	CONSOLE_DEBUG("Counting vars (%ld)",n_y);
	for(i=0; i<diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		if(seq.n == 1){
			v = seq.vars[0];
			varname = var_make_name(sys->system,v);
			if(var_fixed(v)){
				CONSOLE_DEBUG("'%s' is fixed",varname);
				ASC_FREE(varname);
				n_skipped_alg++;
				continue;
			}else if(!var_incident(v)){
				CONSOLE_DEBUG("'%s' is not incident",varname);
				ASC_FREE(varname);
				n_skipped_alg++;
				continue;
			}
			CONSOLE_DEBUG("'%s' is algebraic (%ld)",varname,n_y);
		}else{
			v = seq.vars[0];
			varname = var_make_name(sys->system,v);
			asc_assert(var_active(v));
			if(var_fixed(v) || !var_incident(v)){
				CONSOLE_DEBUG("Differential var '%s' is fixed/not incident",varname);
				n_skipped_diff++;
				ASC_FREE(varname);
				for(j=1; j<seq.n; ++j){
					v = seq.vars[j];
					varname = var_make_name(sys->system,v);
					var_set_active(v,FALSE);
					var_set_value(v,0);
					CONSOLE_DEBUG("Derivative '%s' SET INACTIVE",varname);
					ASC_FREE(varname);
					n_skipped_deriv++;
				}
				continue;
			}else if(var_fixed(seq.vars[1])){
				/* diff var with fixed derivative */
				CONSOLE_DEBUG("Derivative of var '%s' is fixed; CONVERTING TO ALGEBRAIC",varname);
				n_skipped_deriv++;
			}else{
				/* seq.n > 1, var is not fixed, deriv is not fixed */
				asc_assert(var_active(seq.vars[1]));
				asc_assert(seq.n == 2);
				sys->y[n_y] = v;
				CONSOLE_DEBUG("'%s' is differential (%ld)",varname,n_y);
				ASC_FREE(varname);
				v = seq.vars[1];
				sys->ydot[n_y] = v;
				/* we don't assigned y_id because the vars may yet be reorderd */
				n_ydot++;
				varname = var_make_name(sys->system,v);
				CONSOLE_DEBUG("'%s' is derivative (%ld)",varname,n_y);
				ASC_FREE(varname);
				n_y++;
				continue;
			}
		}
		/* fall through: v is algebraic */
		ASC_FREE(varname);
		asc_assert(var_active(v));
		sys->y[n_y] = v;
		sys->ydot[n_y] = NULL;
		n_y++;
		continue;
	}
	sys->n_y = n_y;

	CONSOLE_DEBUG("Got %ld non-derivative vars and %ld derivative vars", n_y, n_ydot);

	CONSOLE_DEBUG("Creating DAE partitioning...");
	if(block_sort_dae_rels_and_vars(sys->system, &(sys->n_y))){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error sorting rels and vars");
		return 250;
	}

	if(integrator_ida_check_partitioning(sys)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error with partitioning");
		return 300;
	}
	CONSOLE_DEBUG("DAE partitioning is OK");

	/* alloce space for the deriv-to-diff lookup */
	asc_assert(sys->y_id==NULL);
	if(sys->y_id == NULL){
		sys->y_id = ASC_NEW_ARRAY_CLEAR(int, slv_get_num_solvers_vars(sys->system));
		CONSOLE_DEBUG("Allocated y_id (n_y = %ld)",sys->n_y);
	}

	if(integrator_ida_rebuild_diffindex(sys)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error updating var_sindex values for derivative variables");
		return 350;
	}

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
#endif


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
		CONSOLE_DEBUG("(Found %ld fixable vars)",(int)(vp-vlist));
		return 1;
	}
#endif

	return res;
}

/**
	Assuming that the sys->y and sys->ydot lists are now prepared,
	we make a reverse index from var_sindex(ydot)--->var_sindex(y)
*/
static int integrator_ida_rebuild_diffindex(IntegratorSystem *sys){
	int i;

	CONSOLE_DEBUG("Rebuilding diffindex vector");
	asc_assert(sys->ydot);
	asc_assert(sys->y_id);

	for(i=0; i<sys->n_y; ++i){
		if(sys->ydot[i]!=NULL){
			sys->y_id[var_sindex(sys->ydot[i])]=i;
		}
	}

	CONSOLE_DEBUG("Completed integrator_ida_rebuild_diffindex");
	return 0;
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
			}
			if(var_sindex(vlist[i])!=i){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Deriv var with incorrect sindex");
				err++;
			}
			diffindex = sys->y_id[i];
			if(diffindex >= sys->n_y){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Diff var in wrong position");
				err++;
			}
			if(var_sindex(vlist[diffindex])!=diffindex){
				ERROR_REPORTER_HERE(ASC_PROG_ERR,"Diff var with incorrect sindex");
				err++;
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
	diffindex = sys->y_id[var_sindex(deriv)];
#ifdef DIFFINDEX_DEBUG
	asc_assert( var_deriv    (deriv));
	asc_assert( var_active   (deriv));
	asc_assert( var_incident (deriv));
	asc_assert( var_svar     (deriv));

	asc_assert(!var_fixed    (deriv));

	asc_assert(var_sindex(deriv) >= sys->n_y);
	asc_assert(diffindex == var_sindex(sys->y[diffindex]));
#endif
	return diffindex;
}


int integrator_ida_analyse_debug(const IntegratorSystem *sys,FILE *fp){
	return analyse_diffvars_debug(sys->system,fp);
}


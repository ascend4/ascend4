#include "idaanalyse.h"

#include <utilities/ascPanic.h>

#ifdef ASC_IDA_NEW_ANALYSE
# include <solver/diffvars.h>
# include <solver/slv_stdcalls.h>
# include <solver/block.h>
# include <solver/diffvars_impl.h>
#include <solver/slvDOF.h>
#endif

#define ANALYSE_DEBUG

#define VARMSG(MSG) \
	varname = var_make_name(sys->system,v); \
	CONSOLE_DEBUG(MSG,varname); \
	ASC_FREE(varname)

static int integrator_ida_check_partitioning(IntegratorSystem *sys);
static int integrator_ida_check_diffindex(IntegratorSystem *sys);
/* static int integrator_ida_rebuild_diffindex(IntegratorSystem *sys); */

/**
	A var can be non-incident. If it *is* non incident and we're going to
	keep it, it will have to have derivative that *is* incident, and that
	meets the following filter.

	If it doesn't have a valid derivative (eg the derivative is fixed, or
	the variable doesn't HAVE a derivative), we will mark the non-deriv
	var non-ACTIVE, so anyway it will end up meeting this filter after we've
	run	integrator_ida_check_vars.
*/
const var_filter_t integrator_ida_nonderiv = {
	VAR_SVAR | VAR_ACTIVE | VAR_FIXED | VAR_DERIV,
	VAR_SVAR | VAR_ACTIVE | 0         | 0
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
	This is the first step in the DAE analysis process. We inspect the
	derivative chains from the solver's analyse() routine.
	Check derivative chains: if a var or derivative is inelligible,
	follow down the chain fixing & setting zero any derivatives.
*/
static int integrator_ida_check_vars(IntegratorSystem *sys){
	const SolverDiffVarCollection *diffvars;
	char *varname;
	int n_y = 0;
	int i, j;
	struct var_variable *v;
	SolverDiffVarSequence seq;
	int vok;

	CONSOLE_DEBUG("BEFORE CHECKING VARS");
	integrator_ida_analyse_debug(sys,stderr);

	/* we shouldn't have allocated these yet: just be sure */
	asc_assert(sys->y==NULL);
	asc_assert(sys->ydot==NULL);
	asc_assert(sys->n_y==0);

	/* get the the dervative chains from the system */
	diffvars = system_get_diffvars(sys->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}

	system_var_list_debug(sys->system);
	
	/* add the variables from the derivative chains */
	for(i=0; i<diffvars->nseqs; ++i){
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];

		/* update the var_fixed flag */
		(void)var_fixed(v);
		vok = var_apply_filter(v,&integrator_ida_nonderiv);
		
		if(vok && !var_incident(v)){
			VARMSG("good var '%s' is not incident");
			/* var meets our filter, but perhaps it's not incident? */
			if(seq.n == 1 || var_apply_filter(seq.vars[1],&integrator_ida_nonderiv)){
				VARMSG("DEACTIVATING NON-INCIDENT VAR '%s' (NO DERIVATIVE)");
				var_set_active(v,0);
				vok = 0;
			}else{
				VARMSG("'%s' has a derivative that's OK");
				ERROR_REPORTER_HERE(ASC_USER_ERROR,"Non-incident var with an incident derivative. ASCEND can't handle this case at the moment, but we hope to fix it.");
				return 1;
			}
		}		

		if(!vok){
			VARMSG("'%s' fails non-deriv filter");
			if(var_fixed(v)){
				CONSOLE_DEBUG("(var is fixed");
			}
			CONSOLE_DEBUG("passes nonderiv? %s (flags = 0x%x)"
				, (var_apply_filter(v,&integrator_ida_nonderiv) ? "TRUE" : "false")
				, var_flags(v));

			for(j=1;j<seq.n;++j){
				v = seq.vars[j];
				var_set_active(v,FALSE);
				var_set_value(v,0);
				VARMSG("Derivative '%s' SET ZERO AND INACTIVE");
			}
			continue;
		}		

		/* VARMSG("We will use var '%s'"); */
		if(seq.n > 2){
			varname = var_make_name(sys->system,v);				
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Too-long derivative chain with var '%s'",varname);
			ASC_FREE(varname);
			return 2;
		}else if(seq.n==2){
			/* diff var */
			if(var_apply_filter(seq.vars[1],&integrator_ida_deriv)){
				/* add the diff & deriv vars to the lists */
				n_y++;
				VARMSG("Added diff var '%s'");
				v = seq.vars[1]; VARMSG("and its derivative '%s'");
				continue;
			}
			VARMSG("Diff var '%s' being converted to alg var...");
			v = seq.vars[1]; VARMSG("...because deriv var '%s' fails filter");
			/* fall through */
		}

		VARMSG("Adding '%s' to algebraic");
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


	CONSOLE_DEBUG("BEFORE SORTING RELS AND VARS");
	integrator_ida_analyse_debug(sys,stderr);

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
	char *varname;

	SolverDiffVarSequence seq;

	asc_assert(sys->y==NULL);
	asc_assert(sys->ydot==NULL);
	asc_assert(sys->y_id== NULL);

	/* get the the dervative chains from the system */
	diffvars = system_get_diffvars(sys->system);
	if(diffvars==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Derivative structure is empty");
		return 1;
	}
	
	/* allocate space (more than enough) */
	sys->y = ASC_NEW_ARRAY(struct var_variable *,sys->n_y);
	sys->ydot = ASC_NEW_ARRAY_CLEAR(struct var_variable *,sys->n_y);
	sys->n_ydot = 0;

	CONSOLE_DEBUG("Passing through chains...");

	/* create the lists y and ydot, ignoring 'bad' vars */
	for(i=0; i<diffvars->nseqs; ++i){
		CONSOLE_DEBUG("i = %d",i);
			
		seq = diffvars->seqs[i];
		asc_assert(seq.n >= 1);
		v = seq.vars[0];
		j = var_sindex(v);

		if(!var_apply_filter(v,&integrator_ida_nonderiv)){
			continue;
		}

		sys->y[j] = v;
		VARMSG("'%s' is good non-deriv");

		if(seq.n > 1 && var_apply_filter(seq.vars[1],&integrator_ida_deriv)){
			v = seq.vars[1];
			asc_assert(var_sindex(v) >= sys->n_y);
			asc_assert(var_sindex(v)-sys->n_y < sys->n_y);

			sys->ydot[j] = v;
			sys->n_ydot++;
			VARMSG("'%s' is good deriv");
		}else{
			asc_assert(sys->ydot[j]==NULL);
		}
	}

	CONSOLE_DEBUG("Found %d good non-derivs",j);

	/* create the list y_id by looking at non-NULLs from ydot */
	sys->y_id = ASC_NEW_ARRAY(int,sys->n_ydot);
	for(i=0,j=0; i <  sys->n_y; ++i){
		if(sys->ydot[i]==NULL)continue;
		v = sys->ydot[i]; VARMSG("deriv '%s'...");
		v = sys->y[i]; VARMSG("diff '%s'...");
		sys->y_id[var_sindex(sys->ydot[i]) - sys->n_y] = i;
		j++;
	}
	
	asc_assert(j==sys->n_ydot);

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

	CONSOLE_DEBUG("BEFORE MAKING LISTS");
	integrator_ida_debug(sys,stderr);

	res = integrator_ida_create_lists(sys);
	if(res){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Problem creating lists");
		return 1;
	}

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Checking lists");
#endif

	asc_assert(sys->y);
	asc_assert(sys->ydot);
	asc_assert(sys->y_id);

	integrator_ida_debug(sys,stderr);

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
	diffvars = system_get_diffvars(sys->system);

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

#ifdef ANALYSE_DEBUG
	CONSOLE_DEBUG("Collecting observed variables");
#endif

	/* get the observations */
	/** @TODO this should use a 'problem provider' API instead of static data
	from the system object */
	sys->n_obs = diffvars->nobs;
	sys->obs = ASC_NEW_ARRAY(struct var_variable *,sys->n_obs);
	for(i=0;i<sys->n_obs;++i){
		/* we get them all, regardless of flags etc */
		sys->obs[i] = diffvars->obs[i];
		varname = var_make_name(sys->system,sys->obs[i]);
		CONSOLE_DEBUG("'%s' is observation",varname);
		ASC_FREE(varname);
	}

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
	int *vlist, *vp, i, nv, nv_ok;
	char *varname;
	struct var_variable **solversvars;

	var_filter_t vfilt = {
		VAR_ACTIVE | VAR_INCIDENT | VAR_FIXED,
		VAR_ACTIVE | VAR_INCIDENT | 0
	};
			
	nv = slv_get_num_solvers_vars(sys->system);
	solversvars = slv_get_solvers_var_list(sys->system);
	CONSOLE_DEBUG("-------------- nv = %d -------------",nv);
	for(nv_ok=0, i=0; i < nv; ++i){
		if(var_apply_filter(solversvars[i],&vfilt)){
			varname = var_make_name(sys->system,solversvars[i]);
			fprintf(stderr,"%s\n",varname);
			ASC_FREE(varname);
			nv_ok++;
		}
	}
	CONSOLE_DEBUG("----------- got %d ok -------------",nv_ok);
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

/* return 0 on succes */
static int check_dups(struct var_variable **list,int n,int allownull){
	int i,j;
	struct var_variable *v;
	for(i=0; i< n; ++i){
		v=list[i];
		if(v==NULL){
			if(allownull)continue;
			else return 2;
		}
		for(j=0; j<i-1;++j){
			if(list[j]==NULL)continue;
			if(v==list[j])return 1;
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
static int integrator_ida_check_diffindex(IntegratorSystem *sys){
	int i, n_vars, n_ydot=0;
	struct var_variable **list, *v;
	char *varname;
	int diffindex;
	const char *msg;

	CONSOLE_DEBUG("Checking diffindex vector");

	if(sys->y_id == NULL || sys->y == NULL || sys->ydot == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"list(s) NULL");
		return 1;
	}
	
	list = slv_get_solvers_var_list(sys->system);
	n_vars = slv_get_num_solvers_vars(sys->system);
	
	if(check_dups(list,n_vars,FALSE)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"duplicates or NULLs in solver's var list");
		return 1;
	}

	if(check_dups(sys->ydot,n_vars,TRUE)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"duplicates in ydot vector");
		return 1;
	}

	/* check n_y in range */
	if(sys->n_y <= 0 || sys->n_y >= n_vars){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"n_y = %d invalid (n_vars = %d)",sys->n_y, n_vars);
		return 1;
	}

	/* check first n_y vars */
	for(i=0; i < sys->n_y; ++i){
		v = list[i];
		if(!var_apply_filter(v, &integrator_ida_nonderiv)){
			msg = "'%s' (in first n_y vars) fails non-deriv filter"; goto finish;
		}else if(var_sindex(v) != i){
			msg = "'%s' has wrong var_sindex"; goto finish;
		}else if(v != sys->y[i]){
			msg = "'%s' not matched in y vector"; goto finish;
		}
		/* meets filter, matches in y vector, has correct var_sindex. */
	}

	/* count non-NULLs in ydot */
	for(i=0; i < sys->n_y; ++i){
		v = sys->ydot[i];
		if(v!=NULL){
			if(var_sindex(v) < sys->n_y){
				msg = "'%s' has var_sindex < n_y"; goto finish;
			}else if(!var_apply_filter(v,&integrator_ida_deriv)){
				msg = "'%s' (in next n_ydot vars) fails deriv filter"; goto finish;
			}
			/* lies beyond first n_y vars, match deriv filter */
			n_ydot++;
		}
	}

	/* check that vars [n_y, n_y+n_ydot) in solver's var list match the deriv filter */
	for(i=sys->n_y; i< sys->n_y + n_ydot; ++i){
		v = list[i];
		if(!var_apply_filter(v,&integrator_ida_deriv)){
			msg = "'%s', in range [n_y,n_y+n_ydot), fails deriv filter"; goto finish;
		}
	}

	/* check values in y_id are ints int range [0,n_y), and that they point to correct vars */
	for(i=0; i<n_ydot; ++i){
		if(sys->y_id[i] < 0 || sys->y_id[i] >= sys->n_y){
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"value of y_id[%d] is out of range",i);
			return 1;
		}
		v = sys->ydot[sys->y_id[i]];
		if(var_sindex(v) - sys->n_y != i){
			msg = "'%s' not index correctly from y_id"; goto finish;
		}
	}

	/* check remaining vars fail both filters */
	for(i=sys->n_y + n_ydot; i<n_vars; ++i){
		v = list[i];
		if(var_apply_filter(v,&integrator_ida_nonderiv)){
			msg = "Var '%s' at end meets non-deriv filter, but shouldn't"; goto finish;
		}
		if(var_apply_filter(v,&integrator_ida_deriv)){
			CONSOLE_DEBUG("position = %d",i);
			msg = "Var '%s' at end meets deriv filter, but shouldn't"; goto finish;
		}
	}

	return 0;		
finish:
	varname = var_make_name(sys->system,v);
	ERROR_REPORTER_HERE(ASC_PROG_ERR,msg,varname);
	ASC_FREE(varname);
	return 1;			
}

/**
	Given a derivative variable, return the index of its corresponding differential
	variable in the y vector (and equivalently the var_sindex of the diff var)
*/
int integrator_ida_diffindex(const IntegratorSystem *sys, const struct var_variable *deriv){
	asc_assert(var_sindex(deriv) >= sys->n_y);
	asc_assert(var_sindex(deriv) < sys->n_y + sys->n_ydot);
	return sys->y_id[var_sindex(deriv) - sys->n_y];
}


/**
	This function will output the data structures provided to use BY THE
	SYSTEM -- not the ones we're working with here IN THE SOLVER.
*/
int integrator_ida_analyse_debug(const IntegratorSystem *sys,FILE *fp){
	return system_diffvars_debug(sys->system,fp);
}


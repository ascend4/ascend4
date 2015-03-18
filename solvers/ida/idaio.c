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
*//**
	@file
	Access to the IDA integrator for ASCEND. IDA is a DAE solver that comes
	as part of the GPL-licensed SUNDIALS solver package from LLNL.

	IDA provides the non-linear parts, as well as a number of pluggable linear
	solvers: dense, banded and krylov types.

	We also implement here an EXPERIMENTAL direct sparse linear solver for IDA
	using the ASCEND linsolqr routines.

	@see http://www.llnl.gov/casc/sundials/
*//*
	by John Pye, May 2006
*/

//#define _GNU_SOURCE

#include "idaio.h"
#include "idalinear.h"
#include "idaanalyse.h"
#include "idatypes.h"
#include "idaprec.h"
#include "idacalc.h"

#include <signal.h>
#include <setjmp.h>
#include <fenv.h>
#include <math.h>


#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascSignal.h>
#include <ascend/general/panic.h>
#include <ascend/compiler/instance_enum.h>

#include <ascend/system/slv_client.h>
#include <ascend/system/relman.h>
#include <ascend/system/block.h>
#include <ascend/system/slv_stdcalls.h>
#include <ascend/system/jacobian.h>
#include <ascend/system/bndman.h>
#include <ascend/system/diffvars.h>

#include <ascend/utilities/config.h>
#include <ascend/integrator/integrator.h>

/**
	This routine just outputs the stats to the CONSOLE_DEBUG routine.

	@TODO provide a GUI way of stats reporting from IDA.
*/
void integrator_ida_write_stats(IntegratorIdaStats *stats){
# define SL(N) CONSOLE_DEBUG("%s = %ld",#N,stats->N)
# define SI(N) CONSOLE_DEBUG("%s = %d",#N,stats->N)
# define SR(N) CONSOLE_DEBUG("%s = %f",#N,stats->N)
		SL(nsteps); SL(nrevals); SL(nlinsetups); SL(netfails);
		SI(qlast); SI(qcur);
		SR(hinused); SR(hlast); SR(hcur); SR(tcur);
# undef SL
# undef SI
# undef SR
}

/*------------------------------------------------------------------------------
  OUTPUT OF INTERNALS: JACOBIAN / INCIDENCE MATRIX / DEBUG INFO
*/

/**
	Here we construct the local transfer matrix. It's a bit of a naive
	approach; probably far more efficient ways can be worked out. But it will
	hopefully be a useful way to examine stability of some spatial
	discretisation schemes for PDAE systems.

	http://ascend4.org/IDA#Stability
*/
int integrator_ida_transfer_matrix(const IntegratorSystem *integ, struct SystemJacobianStruct *J){
#ifdef STILL_NEED_TO_IMPLEMENT_THIS
	int i=0, res;
	enum submat{II_GA=0, II_GD, II_FA, II_FD, II_FDP, II_NUM};

	const var_filter_t *matvf[II_NUM] = {
		&system_vfilter_algeb
		,&system_vfilter_diff
		,&system_vfilter_algeb
		,&system_vfilter_diff
		,&system_vfilter_deriv
	};

	const rel_filter_t *matrf[II_NUM] = {
		&system_rfilter_algeb
		,&system_rfilter_algeb
		,&system_rfilter_diff
		,&system_rfilter_diff
		,&system_rfilter_diff
	};

	struct SystemJacobianStruct D[II_NUM];

	for(i=0;i<II_NUM;++i){
		res = system_jacobian(integ->system, matrf[i], matvf[i], 1/*safe*/ ,&(D[i]));
	}

	/* compute inverses for matrices that need it */
#endif

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return 1;
}

/**
	Our task here is to write the matrices that IDA *should* be seeing. We
	are actually making calls to relman_diff in order to do that, so we're
	really going back to the variables in the actual system and computing
	row by row what the values are. This should mean just a single call to
	each blackbox present in the system (if blackbox caching is working
	correctly).
*/
int integrator_ida_write_matrix(const IntegratorSystem *integ, FILE *f, const char *type){
	/* IntegratorIdaData *enginedata; */
	struct SystemJacobianStruct J = {NULL,NULL,NULL,0,0};
	int status=1;
	mtx_region_t R;

	if(type==NULL)type = "dx'/dx";

	if(0==strcmp(type,"dg/dz")){
		CONSOLE_DEBUG("Calculating dg/dz...");
		status = system_jacobian(integ->system
			, &system_rfilter_algeb, &system_vfilter_algeb
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dg/dx")){
		CONSOLE_DEBUG("Calculating dg/dx...");
		status = system_jacobian(integ->system
			, &system_rfilter_algeb, &system_vfilter_diff
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"df/dx'")){
		CONSOLE_DEBUG("Calculating df/dx'...");
		status = system_jacobian(integ->system
			, &system_rfilter_diff, &system_vfilter_deriv
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"df/dz")){
		CONSOLE_DEBUG("Calculating df/dz...");
		status = system_jacobian(integ->system
			, &system_rfilter_diff, &system_vfilter_algeb
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"df/dx")){
		CONSOLE_DEBUG("Calculating df/dx...");
		status = system_jacobian(integ->system
			, &system_rfilter_diff, &system_vfilter_diff
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dF/dy")){
		CONSOLE_DEBUG("Calculating dF/dy...");
		status = system_jacobian(integ->system
			, &system_rfilter_all, &system_vfilter_nonderiv
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dF/dy'")){
		CONSOLE_DEBUG("Calculating dF/dy'...");
		status = system_jacobian(integ->system
			, &system_rfilter_all, &system_vfilter_deriv
			, 1 /* safe */
			, &J
		);
	}else if(0==strcmp(type,"dx'/dx")){
		/* system state transfer matrix dyd'/dyd */
		status = integrator_ida_transfer_matrix(integ, &J);
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid matrix type '%s'",type);
		return 1;
	}

	if(status){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error calculating matrix");
	}else{
		/* send the region explicitly, so that we handle non-square correctly */
		R.row.low = 0; R.col.low = 0;
		R.row.high = J.n_rels - 1; R.col.high = J.n_vars - 1;
		/* note that we're not fussy about empty matrices here... */
		mtx_write_region_mmio(f,J.M,&R);
	}

	if(J.vars)ASC_FREE(J.vars);
	if(J.rels)ASC_FREE(J.rels);
	if(J.M)mtx_destroy(J.M);

	return status;
}

/**
	This routine outputs matrix structure in a crude text format, for the sake
	of debugging.
*/
void integrator_ida_write_incidence(IntegratorSystem *integ){
	int i, j;
	struct rel_relation **relptr;
	IntegratorIdaData *enginedata = integ->enginedata;
	double *derivatives;
	struct var_variable **variables;
	int count, status;
	char *relname;

	if(enginedata->nrels > 100){
		CONSOLE_DEBUG("Ignoring call (matrix size too big = %d)",enginedata->nrels);
		return;
	}

	variables = ASC_NEW_ARRAY(struct var_variable *, integ->n_y * 2);
	derivatives = ASC_NEW_ARRAY(double, integ->n_y * 2);

	CONSOLE_DEBUG("Outputting incidence information to console...");

	for(i=0, relptr = enginedata->rellist;
			i< enginedata->nrels && relptr != NULL;
			++i, ++relptr
	){
		relname = rel_make_name(integ->system, *relptr);

		/* get derivatives for this particular relation */
		status = relman_diff3(*relptr, &enginedata->vfilter, derivatives, variables, &count, enginedata->safeeval);
		if(status){
			CONSOLE_DEBUG("ERROR calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			break;
		}

		fprintf(stderr,"%3d:%-15s:",i,relname);
		ASC_FREE(relname);

		for(j=0; j<count; ++j){
			if(var_deriv(variables[j])){
				fprintf(stderr," %p:ydot[%d]",variables[j],integrator_ida_diffindex(integ,variables[j]));
			}else{
				fprintf(stderr," %p:y[%d]",variables[j],var_sindex(variables[j]));
			}
		}
		fprintf(stderr,"\n");
	}
	ASC_FREE(variables);
	ASC_FREE(derivatives);
}

/* @return 0 on success */
int integrator_ida_debug(const IntegratorSystem *integ, FILE *fp){
	char *varname, *relname;
	struct var_variable **vlist, *var;
	struct rel_relation **rlist, *rel;
	long vlen, rlen;
	long i;
	long di;

	fprintf(fp,"THERE ARE %d VARIABLES IN THE INTEGRATION SYSTEM\n\n",integ->n_y);

	/* if(integrator_sort_obs_vars(integ))return 10; */

	if(integ->y && integ->ydot){
		fprintf(fp,"CONTENTS OF THE 'Y' AND 'YDOT' LISTS\n\n");
		fprintf(fp,"index\t%-15s\tydot\n","y");
		fprintf(fp,"-----\t%-15s\t-----\n","-----");
		for(i=0;i<integ->n_y;++i){
			varname = var_make_name(integ->system, integ->y[i]);
			fprintf(fp,"%ld\t%-15s\t",i,varname);
			if(integ->ydot[i]){
				ASC_FREE(varname);
				varname = var_make_name(integ->system, integ->ydot[i]);
				fprintf(fp,"%s\n",varname);
				ASC_FREE(varname);
			}else{
				fprintf(fp,".\n");
				ASC_FREE(varname);
			}
		}
	}else{
		fprintf(fp,"'Y' and 'YDOT' LISTS ARE NOT SET!\n");
	}

	fprintf(fp,"\n\nCONTENTS OF THE VAR_FLAGS AND VAR_SINDEX\n\n");
	fprintf(fp,"sindex\t%-15s\ty    \tydot \n","name");
	fprintf(fp,"------\t%-15s\t-----\t-----\n","----");


	/* visit all the slv_system_t master var lists to collect vars */
	/* find the vars mostly in this one */
	vlist = slv_get_solvers_var_list(integ->system);
	vlen = slv_get_num_solvers_vars(integ->system);
	for(i=0;i<vlen;i++){
		var = vlist[i];

		varname = var_make_name(integ->system, var);
		fprintf(fp,"%ld\t%-15s\t",i,varname);

		if(var_fixed(var)){
			// it's fixed, so not really a DAE var
			fprintf(fp,"(fixed)\n");
		}else if(!var_active(var)){
			// inactive
			fprintf(fp,"(inactive)\n");
		}else if(!var_incident(var)){
			// not incident
			fprintf(fp,"(not incident)\n");
		}else{
			if(var_deriv(var)){
				if(integ->y_id){
					di = integrator_ida_diffindex1(integ,var);
					if(di>=0){
						ASC_FREE(varname);
						varname = var_make_name(integ->system,vlist[di]);
						fprintf(fp,".\tdiff(%ld='%s')\n",di,varname);
					}else{
						fprintf(fp,".\tdiff(???,err=%ld)\n",di);
					}
				}else{
					fprintf(fp,".\tderiv... of??\n");
				}
			}else{
				fprintf(fp,"%d\t.\n",var_sindex(var));
			}
		}
		ASC_FREE(varname);
	}

	/* let's write out the relations too */
	rlist = slv_get_solvers_rel_list(integ->system);
	rlen = slv_get_num_solvers_rels(integ->system);

	fprintf(fp,"\nALL RELATIONS IN THE SOLVER'S LIST (%ld)\n\n",rlen);
	fprintf(fp,"index\tname\n");
	fprintf(fp,"-----\t----\n");
	for(i=0; i<rlen; ++i){
		rel = rlist[i];
		relname = rel_make_name(integ->system,rel);
		fprintf(fp,"%ld\t%s\n",i,relname);
		ASC_FREE(relname);
	}

	/* write out the derivative chains */
	fprintf(fp,"\nDERIVATIVE CHAINS\n");
	if(system_diffvars_debug(integ->system,stderr)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error getting diffvars debug info");
		return 340;
	}
	fprintf(fp,"\n");

	/* and lets write block debug output */
	system_block_debug(integ->system, fp);

	return 0; /* success */
}

/*----------------------------------------------
  ERROR REPORTING
*/
/**
	Error message reporter function to be passed to IDA. All error messages
	will trigger a call to this function, so we should find everything
	appearing on the console (in the case of Tcl/Tk) or in the errors/warnings
	panel (in the case of PyGTK).
*/
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
){
	error_severity_t sev;

	/* cast back the IntegratorSystem, just in case we need it */
#ifdef WE_DONT_NEED_THIS_YET
	IntegratorSystem *integ;
	integ = (IntegratorSystem *)eh_data;
#endif

	/* severity depends on the sign of the error_code value */
	if(error_code <= 0){
		sev = ASC_PROG_ERR;
	}else{
		sev = ASC_PROG_WARNING;
	}

	/* use our all-purpose error reporting to get stuff back to the GUI */
	error_reporter(sev,module,0,function,"%s (error %d)",msg,error_code);
}


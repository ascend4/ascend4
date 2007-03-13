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
*/
#include "jacobian.h"
#include "relman.h"
#include "slv_client.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>
#include <general/mathmacros.h>

/* #define JACOBIAN_DEBUG */

int system_jacobian(slv_system_t sys
	, const rel_filter_t *rfilter, const var_filter_t *vfilter, const int safe
	, struct SystemJacobianStruct *sysjac
){
	int i,j,n,nsr,nsv,nr,nv;
	struct var_variable **svars;
	struct rel_relation **srels;
	int *vartocol;
	int res, err=0;
	double *derivvals;
	int *derivvars;
	mtx_coord_t coord;
	char *relname;
#ifdef JACOBIAN_DEBUG
	char *varname;
#endif
	
	/* first count the rels */
	nsr = slv_get_num_solvers_rels(sys);
	nr = slv_count_solvers_rels(sys,rfilter);
	srels = slv_get_solvers_rel_list(sys);

	/* and vars */
	nsv = slv_get_num_solvers_vars(sys);
	nv = slv_count_solvers_vars(sys,vfilter);
	svars = slv_get_solvers_var_list(sys);

	/* allocate space for lists */
	sysjac->vars = ASC_NEW_ARRAY(struct var_variable*, nv);
	sysjac->rels = ASC_NEW_ARRAY(struct rel_relation*, nr);
	vartocol = ASC_NEW_ARRAY(int,nsv);

	/* now create a the lists of vars and rels, and temp mapping array */
	n = 0;
	for(i=0;i<nsr;++i){
		if(rel_apply_filter(srels[i],rfilter)){
			sysjac->rels[n++] = srels[i];
		}
	}
	asc_assert(n==nr);

	n = 0;
	for(i=0;i<nsv;++i){
		if(var_apply_filter(svars[i],vfilter)){			
			vartocol[i]=n;
			sysjac->vars[n++] = svars[i];
		}else{
			vartocol[i]=-1;
		}
	}
	asc_assert(n==nv);

	CONSOLE_DEBUG("nr = %d",nr);
	CONSOLE_DEBUG("nv = %d",nv);

	derivvals = ASC_NEW_ARRAY(double,nv);
	derivvars = ASC_NEW_ARRAY(int,nv);

	/* now create a matrix */
	sysjac->M = mtx_create();
	mtx_set_order(sysjac->M, MAX(nv,nr));
	
	for(i=0;i<nr;++i){
#ifdef JACOBIAN_DEBUG
		relname = rel_make_name(sys,sysjac->rels[i]);
		CONSOLE_DEBUG("rel '%s'",relname);
		ASC_FREE(relname);
#endif
	
		res = relman_diff2(sysjac->rels[i], vfilter, derivvals, derivvars, &n, safe);
		for(j=0;j<n;++j){
#ifdef JACOBIAN_DEBUG
			asc_assert(var_apply_filter(svars[derivvars[j]],vfilter));
			varname = var_make_name(sys,svars[derivvars[j]]);
			CONSOLE_DEBUG("var '%s' (var_deriv = %d)",varname,var_deriv(svars[derivvars[j]]));
			ASC_FREE(varname);
#endif
			mtx_set_value(sysjac->M,mtx_coord(&coord,i,vartocol[derivvars[j]]),derivvals[j]);
		}
		if(res){
			relname = rel_make_name(sys,sysjac->rels[i]);
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error calculating derivatives for relation '%s'",relname);
			ASC_FREE(relname);
			err = 1;
		}
	}

	sysjac->n_rels = nr;
	sysjac->n_vars = nv;

	ASC_FREE(derivvals);
	ASC_FREE(derivvars);

	return err;
}

/* these are the filters that will partitiong our DAE system */


const rel_filter_t system_rfilter_algeb = {
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE | REL_DIFFERENTIAL,
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE | 0
};

const rel_filter_t system_rfilter_diff = {
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE | REL_DIFFERENTIAL,
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE | REL_DIFFERENTIAL
};

const rel_filter_t system_rfilter_all = {
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE,
	REL_INCLUDED | REL_EQUALITY | REL_ACTIVE
};


const var_filter_t system_vfilter_algeb = {
	VAR_SVAR | VAR_ACTIVE | VAR_FIXED | VAR_DERIV | VAR_DIFF,
	VAR_SVAR | VAR_ACTIVE | 0         | 0         | 0
};

const var_filter_t system_vfilter_diff = {
	VAR_SVAR | VAR_ACTIVE | VAR_FIXED | VAR_DERIV | VAR_DIFF,
	VAR_SVAR | VAR_ACTIVE | 0         | 0         | VAR_DIFF
};

const var_filter_t system_vfilter_deriv = {
	VAR_SVAR | VAR_ACTIVE | VAR_FIXED | VAR_DERIV ,
	VAR_SVAR | VAR_ACTIVE | 0         | VAR_DERIV
};

const var_filter_t system_vfilter_nonderiv = {
	VAR_SVAR | VAR_ACTIVE | VAR_FIXED | VAR_DERIV ,
	VAR_SVAR | VAR_ACTIVE | 0         | 0        
};

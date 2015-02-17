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
*//** @file
	Routines for creating 'derivative chains' during system build for 
	models which use the der() syntax.

	This file is almost a copy of diffvars.c, but it uses DerInfo instead
	of ode_type and ode_id for creating the derivative chains.

*/
#include "diffvars.h"
#include "k12_diffvars.h"
#include "diffvars_impl.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/link.h>
#include <ascend/compiler/vlist.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/deriv.h>
#include <ascend/compiler/instquery.h>

#include "analyse_impl.h"
#include "analyze.h"
#include "k12_analyze.h"
#include "system_impl.h"

/* #define DIFFVARS_DEBUG */

/*------------------------------------------------------------------------------
	DERIVATIVES & DIFFERENTIAL VARIABLES
*/

/**
	This function steals a lot of what was in integrator.c, but we try to make
	it more general and capable of extracting info about high-order derivative
	chains. One eye open to the possibility of doing Pantelides (or other?) 
	style index reduction.

	Also, note that we try to only use data from the struct problem_t, NOT
	to go back to the instance hierarchy. The idea is that we have already got
	what we need in classify_instance, etc.

	@return 0 on success
*/
int k12_system_generate_diffvars(slv_system_t sys, struct problem_t *prob){

	SolverDiffVarCollection *diffvars = NULL;
	struct solver_ipdata *vip;
	SolverDiffVarSequence *seq;
	struct gl_list_t *seqs;
	long i, nalg, ndiff, len, c;
	short j;
	struct Instance *indep;

	asc_assert(prob);
	if(gl_length(prob->diffvars)==0){
		sys->diffvars = NULL;
		return 0;
	}
	CONSOLE_DEBUG("Differential variables were seen. Generating diffvars data.");		
	seqs = gl_create(prob->nr);	
	/* add the list of algebraic variables to the structure too */
	asc_assert(prob->algebvars);
	nalg = gl_length(prob->algebvars);
	for(i=0; i<nalg; ++i){
		vip = (struct solver_ipdata *)gl_fetch(prob->algebvars,i+1);	
		seq = ASC_NEW(SolverDiffVarSequence);
		seq->n = 1;
		seq->ode_id = 0;
		seq->vars = ASC_NEW_ARRAY(struct var_variable*,1);
		seq->vars[0] = vip->u.v.data;
		gl_append_ptr(seqs,(void *)seq);
	}
	indep = ((struct solver_ipdata *)gl_fetch(prob->indepvars,1))->i;
	ndiff = 0;
	len = gl_length(prob->diffvars);
	for(c=1;c<=len;c++){
	  vip = (struct solver_ipdata *)gl_fetch(prob->diffvars,c);
	  for (i=1;i<=len;i++) {
	    if (IsState(vip->i) && !IsDeriv(vip->i)) {
#ifdef DIFFVARS_DEBUG
	      CONSOLE_DEBUG("Beginning of a derivative chain: %s",WriteInstanceNameString(vip->i,NULL));
#endif
	      seq = ASC_NEW(SolverDiffVarSequence);
	      seq->n = 2;
	      seq->ode_id = ndiff;
	      seq->vars = ASC_NEW_ARRAY(struct var_variable*,seq->n);
#ifdef DIFFVARS_DEBUG
	      CONSOLE_DEBUG("Saving sequence ode_id = %ld, n = %d",seq->ode_id,seq->n);
#endif
	      for(j=0;j<seq->n;++j){
#ifdef DIFFVARS_DEBUG
		CONSOLE_DEBUG("Add %s to a derivative chain.",WriteInstanceNameString(vip->i,NULL));
#endif
	        seq->vars[j]=vip->u.v.data;
	        /* set the VAR_ALGEB flag as req */
	        var_set_diff(seq->vars[j],j==0 && seq->n > 1);
	        if(FindDerByArgs(vip->i,indep)) {
                  if(j!=1) vip = GetInterfacePtr(FindDerByArgs(vip->i,indep));
                  else ERROR_REPORTER_HERE(ASC_USER_ERROR,"Higher order derivatives are not implemented.");
                }
	      }
	      gl_append_ptr(seqs,(void *)seq);
	      ndiff++;
#ifdef DIFFVARS_DEBUG
	      CONSOLE_DEBUG("Completed seq %ld",gl_length(seqs));
#endif
	    }	  
	  }
        }
#ifdef DIFFVARS_DEBUG
	CONSOLE_DEBUG("Identified %ld derivative chains ",gl_length(seqs));
#endif
	diffvars = ASC_NEW(SolverDiffVarCollection);
	diffvars->maxorder = 2;
	diffvars->nseqs = gl_length(seqs);
	diffvars->seqs = ASC_NEW_ARRAY(SolverDiffVarSequence,diffvars->nseqs);
	for(i=0;i<diffvars->nseqs;++i){
		diffvars->seqs[i] = *(SolverDiffVarSequence *)gl_fetch(seqs,i+1);
	}
	gl_free_and_destroy(seqs);
	diffvars->nalg = nalg;
	diffvars->ndiff = ndiff;

	diffvars->nindep = gl_length(prob->indepvars);

	diffvars->indep = ASC_NEW_ARRAY(struct var_variable *,diffvars->nindep);
	for(i=0;i<diffvars->nindep;++i){
		vip = (struct solver_ipdata *)gl_fetch(prob->indepvars,i+1);
		diffvars->indep[i] = vip->u.v.data;
	}
#ifdef DIFFVARS_DEBUG
	CONSOLE_DEBUG("Identified %ld indep vars",diffvars->nindep);
#endif

	diffvars->nobs = gl_length(prob->obsvars);
	diffvars->obs = ASC_NEW_ARRAY(struct var_variable *,diffvars->nobs);
	for(i=0;i<diffvars->nobs;++i){
		vip = (struct solver_ipdata *)gl_fetch(prob->obsvars,i+1);
		diffvars->obs[i] = vip->u.v.data;
	}

	diffvars->ndobs = gl_length(prob->dobsvars);
	diffvars->dobs = ASC_NEW_ARRAY(struct dis_discrete *,diffvars->ndobs);
	for(i=0;i<diffvars->ndobs;++i){
		vip = (struct solver_ipdata *)gl_fetch(prob->dobsvars,i+1);
		diffvars->dobs[i] = vip->u.dv.data;
	}
	diffvars->npres = gl_length(prob->prevars);
	diffvars->pres = ASC_NEW_ARRAY(struct var_variable *, diffvars->npres);
	for(i=0;i<diffvars->npres;++i){
		vip = (struct solver_ipdata *)gl_fetch(prob->prevars,i+1);
		diffvars->pres[i] = vip->u.v.data;
	}
	CONSOLE_DEBUG("There were %ld pres",diffvars->npres);

	slv_set_diffvars(sys,(void *)diffvars);

	return 0;
}

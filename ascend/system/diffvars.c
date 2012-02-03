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
	Routines for creating 'derivative chains' during system build.
*//*
	By John Pye, Jan 2007
*/
#include "diffvars.h"
#include "diffvars_impl.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/link.h>
#include <ascend/compiler/vlist.h>
#include <ascend/compiler/name.h>

#include "analyse_impl.h"
#include "analyze.h"
#include "system_impl.h"

/* #define DIFFVARS_DEBUG */

/*------------------------------------------------------------------------------
	DERIVATIVES & DIFFERENTIAL VARIABLES
*/

SolverDiffVarCollection *system_get_diffvars(slv_system_t sys){
	return sys->diffvars;
}

/**<DS: compare the names of the instances from the problem_t that will be sent to the solver and a symchar, on success return the pointer to the respective solver_ipdata */
static
struct solver_ipdata *FindVarIPdata(struct problem_t *prob,symchar *varName){
	int i,len;
	struct solver_ipdata *ip;
	len = gl_length(prob->algebvars);
	for(i=1;i<=len;i++){
		ip = (struct solver_ipdata *) gl_fetch(prob->algebvars,i);
		if(strcmp(SCP(varName),WriteInstanceNameString(ip->i,prob->root)) == 0){
			return ip;
		}
	}
	return NULL;
}

static 
int CmpDiffVars(const struct solver_ipdata *a, const struct solver_ipdata *b){
	if(a->u.v.odeid < b->u.v.odeid)return -1;
	if(a->u.v.odeid > b->u.v.odeid)return 1;
	/* now, the ode_id is equal: next level of sorting... */
	if(a->u.v.deriv < b->u.v.deriv)return -1;
	if(a->u.v.deriv > b->u.v.deriv)return 1;
	return 0;
}

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
int system_generate_diffvars(slv_system_t sys, struct problem_t *prob){

	
	

	SolverDiffVarCollection *diffvars = NULL;
	struct solver_ipdata *vip, *vipnext;
	SolverDiffVarSequence *seq;
	struct gl_list_t *seqs;
	long i, seqstart, nalg, ndiff;
	short j;
	char cont;
	short maxorder = 0;

	asc_assert(prob);
	

	if(gl_length(prob->diffvars)==0){
		CONSOLE_DEBUG("No differential variables were seen. Skipping generation of diffvars struct.");
		return 0;
	}

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
#ifdef DIFFVARS_DEBUG
	CONSOLE_DEBUG("Added %ld algebraic vars to chains",nalg);
	CONSOLE_DEBUG("Sorting %ld differential & derivative vars...",gl_length(prob->diffvars));
#endif
	
	/* first sort the list of diffvars */
	gl_sort(prob->diffvars, (CmpFunc)CmpDiffVars);
	
	/* scan the list for derivs that are missing vars */
	i = 1; cont = TRUE; seqstart =1; ndiff = 0;
	vip = (struct solver_ipdata *)gl_fetch(prob->diffvars,i);
	if(vip->u.v.deriv > 1){
		ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
		FPRINTF(ASCERR,"Missing ode_type %d for ode_id %d (check var '",vip->u.v.deriv+1,vip->u.v.odeid);
		WriteInstanceName(ASCERR,vip->i,prob->root);
		FPRINTF(ASCERR,"')");
		error_reporter_end_flush();
		return 2;
	}
	while(cont){
		/* CONSOLE_DEBUG("Working, seqstart=%ld",seqstart); */
		if(i >= gl_length(prob->diffvars))cont = FALSE;
		else vipnext = (struct solver_ipdata *)gl_fetch(prob->diffvars,i+1);

		if(cont && vipnext->u.v.odeid == vip->u.v.odeid){
			/* same sequence, check that it's the next derivative */
			asc_assert(vip->u.v.deriv >= vip->u.v.deriv);

			if(vipnext->u.v.deriv == vip->u.v.deriv){
				ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
				FPRINTF(ASCERR,"Repeated ode_type %d for ode_id %d (var '",vip->u.v.deriv,vip->u.v.odeid);
				WriteInstanceName(ASCERR,vip->i,prob->root);
				FPRINTF(ASCERR,"')");
				error_reporter_end_flush();
				return 1;
			}else if(vipnext->u.v.deriv > vip->u.v.deriv + 1){
				ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
				FPRINTF(ASCERR,"Missing ode_type %d for ode_id %d (check var '",vip->u.v.deriv+1,vip->u.v.odeid);
				WriteInstanceName(ASCERR,vip->i,prob->root);
				FPRINTF(ASCERR,"')");
				error_reporter_end_flush();
				return 2;
			}

			/* allow this var into the sequence, loop again */
			vip = vipnext; ++i; continue;
		}

		/* close the current sequence */
		if(i == seqstart){
			ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
			FPRINTF(ASCERR,"Variable '");
			WriteInstanceName(ASCWAR,vip->i,prob->root);
			FPRINTF(ASCERR,"' declared differential without derivative being identified (ode_id=%d)",vip->u.v.odeid);
			error_reporter_end_flush();
			return 3;
		}
		seq = ASC_NEW(SolverDiffVarSequence);
		seq->n = i - seqstart + 1;
		seq->ode_id = vip->u.v.odeid;
		seq->vars = ASC_NEW_ARRAY(struct var_variable*,seq->n);
#ifdef DIFFVARS_DEBUG
		CONSOLE_DEBUG("Saving sequence ode_id = %ld, n = %d",seq->ode_id,seq->n);
#endif
		for(j=0;j<seq->n;++j){
			vip = (struct solver_ipdata *)gl_fetch(prob->diffvars,seqstart+j);
			seq->vars[j]=vip->u.v.data;
			/* set the VAR_ALGEB flag as req */
			var_set_diff(seq->vars[j],j==0 && seq->n > 1);
#ifdef DIFFVARS_DEBUG
			CONSOLE_DEBUG("seq, j=%d: is %s",j
				,(var_diff(seq->vars[j]) ? "DIFF" : 
					(var_deriv(seq->vars[j]) ? "deriv" : "alg")
				) 
			);
#endif
		}
		gl_append_ptr(seqs,(void *)seq);
		ndiff++;
#ifdef DIFFVARS_DEBUG
		CONSOLE_DEBUG("Completed seq %ld",gl_length(seqs));
#endif

		if(vip->u.v.deriv > maxorder){
			maxorder = vip->u.v.deriv;
			/* CONSOLE_DEBUG("maxorder increased to %d",maxorder); */
		}

		++i; seqstart=i; vip=vipnext;
		if(cont && vip->u.v.deriv != 1){
			ERROR_REPORTER_START_NOLINE(ASC_USER_ERROR);
			FPRINTF(ASCERR,"Missing undifferentiated form of var '");
			WriteInstanceName(ASCWAR,vip->i,prob->root);
			FPRINTF(ASCERR,"' (ode_id = %d, ode_type = %d)",vip->u.v.odeid,vip->u.v.deriv);
			error_reporter_end_flush();
			return 4;
		}
		continue;
	}
	
	


#ifdef DIFFVARS_DEBUG
	CONSOLE_DEBUG("Identified %ld derivative chains, maximum length %d...",gl_length(seqs),maxorder);
#endif

	diffvars = ASC_NEW(SolverDiffVarCollection);
	diffvars->maxorder = maxorder;
	diffvars->nseqs = gl_length(seqs);
	diffvars->seqs = ASC_NEW_ARRAY(SolverDiffVarSequence,diffvars->nseqs);
	for(i=0;i<diffvars->nseqs;++i){
		diffvars->seqs[i] = *(SolverDiffVarSequence *)gl_fetch(seqs,i+1);
	}
	gl_destroy(seqs);
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

	/* I know, I know, this might not be the best place for this... */
	diffvars->nobs = gl_length(prob->obsvars);
	diffvars->obs = ASC_NEW_ARRAY(struct var_variable *,diffvars->nobs);
	for(i=0;i<diffvars->nobs;++i){
		vip = (struct solver_ipdata *)gl_fetch(prob->obsvars,i+1);
		diffvars->obs[i] = vip->u.v.data;
	}
#ifdef DIFFVARS_DEBUG
	CONSOLE_DEBUG("Identified %ld obs vers",diffvars->nobs);
	CONSOLE_DEBUG("There were %ld rels",prob->nr);
#endif

	slv_set_diffvars(sys,(void *)diffvars);

	return 0;
}


int system_diffvars_debug(slv_system_t sys,FILE *fp){
	int i, j;
	char *varname;
	const SolverDiffVarCollection *diffvars;
	SolverDiffVarSequence seq;
	diffvars = sys->diffvars;
	if(diffvars==NULL){
		fprintf(fp,"NO DIFFVARS (NULL)");
		return 0;
	}
	fprintf(fp,"Derivative chains in slv_system...\n");
	for(i=0; i<diffvars->nseqs;++i){
		seq = diffvars->seqs[i];
		fprintf(fp,"%d: ",i);
		for(j=0; j<seq.n; ++j){
			if(j)fprintf(fp," <-- ");
			fprintf(fp,"%d: (%p)",var_sindex(seq.vars[j]),seq.vars[j]);
			varname = var_make_name(sys,seq.vars[j]);
			fprintf(fp,"'%s'",varname);
			ASC_FREE(varname);
		}
		fprintf(fp,"\n");
	}
	return 0;
}


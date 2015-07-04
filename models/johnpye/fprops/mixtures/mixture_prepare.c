/*	ASCEND modelling environment 
	Copyright (C) Carnegie Mellon University 

	This program is free software; you can redistribute it and/or modify 
	it under the terms of the GNU General Public License as published by 
	the Free Software Foundation; either version 2, or (at your option) 
	any later version.

	This program is distributed in the hope that it will be useful, but 
	WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	General Public License for more details.

	You should have received a copy of the GNU General Public License 
	along with this program; if not, write to the Free Software 
	Foundation --

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA 02111-1307, USA.
*//*
	by Jacob Shealy, June 25-, 2015

	Functions to prepare structs that represent mixtures when modeling mixtures
 */

#include "mixture_generics.h"
#include "mixture_prepare.h"
#include "mixture_struct.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"
#include <stdio.h>

/**
	Specify a mixture, without specifying state (temperature, pressure, or 
	densities) of the mixture.

	@param npure number of pure components in the mixture
	@param Xs array of mass fractions
	@param fluids names of the pure components
	@param type name of the correlation type (equation of state) used to model 
	the components
	@param source name of sources for component data
	
	@return a mixture specification (struct MixtureSpec)
 */
void mixture_specify(MixtureSpec *MS, unsigned npure, double *Xs, void **fluids, char *type, char **source, MixtureError *merr){
	unsigned i;
	double X_sum=0.0;

	for(i=0;i<npure;i++){
		X_sum += Xs[i];
	}
	if(fabs(X_sum - 1.0) > MIX_XTOL){
		*merr = MIXTURE_XSUM_ERROR;
	}

	if(0==strcmp(type, "ideal")){ /* model fluids with ideal-gas equation of state */
		EosData **ig_fluids = (EosData **)fluids;
		ReferenceState ref = {FPROPS_REF_REF0};

		for(i=0;i<npure;i++){
			MS->PF[i] = ideal_prepare(ig_fluids[i], &ref);
		}
	}else{ /* use some other equation of state */
		char **fluid_names = (char **)fluids;

		for(i=0;i<npure;i++){
			MS->PF[i] = fprops_fluid(fluid_names[i],type,source[i]);
			/* printf("\nprepared fluid %s", fluid_names[i]); */
		}
	}

	/* *MS = ASC_NEW(MixtureSpec);
	MS->pures = npure;
	MS->Xs = Xs;
	MS->PF = PF; */

	for(i=0;i<npure;i++){
		printf("\nfluid number %u at %p is %s, modeled with %u"
				, i, MS->PF[i], MS->PF[i]->name, MS->PF[i]->type);
	}
}

void mixture_fluid_spec(MixtureSpec *MS, unsigned npure, void **fluids, char *type, char **source, MixtureError *merr){
	unsigned i;
	/* double X_sum=0.0; */

	if(0==strcmp(type, "ideal")){ /* model fluids with ideal-gas equation of state */
		EosData **ig_fluids = (EosData **)fluids;
		ReferenceState ref = {FPROPS_REF_REF0};

		for(i=0;i<npure;i++){
			MS->PF[i] = ideal_prepare(ig_fluids[i], &ref);
		}
	}else{ /* use some other equation of state */
		char **fluid_names = (char **)fluids;

		for(i=0;i<npure;i++){
			MS->PF[i] = fprops_fluid(fluid_names[i],type,source[i]);
			printf("\nprepared fluid %s", fluid_names[i]);
		}
	}

	/* for(i=0;i<npure;i++){
		printf("\nfluid number %u at %p is %s, modeled with %u"
				, i, MS->PF[i], MS->PF[i]->name, MS->PF[i]->type);
	} */
}

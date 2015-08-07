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
	by Jacob Shealy, July 31-,2015

	Functions to initialize some of the structs defined in the header file
 */

#include "mixture_struct.h"

/* Functions to create new structs with all members declared, but minimal data filled in */
MixtureSpec *new_MixtureSpec(unsigned npure){
	MixtureSpec *M = ASC_NEW(MixtureSpec);
	M->pures = npure;
	M->Xs    = ASC_NEW_ARRAY(double,npure);
	M->PF    = ASC_NEW_ARRAY(PureFluid *,npure);

	return M;
}

Phase *new_Phase(unsigned npure){
	Phase *P = ASC_NEW(Phase);
	P->pures = npure;
	P->c     = ASC_NEW_ARRAY(unsigned,npure);
	P->Xs    = ASC_NEW_ARRAY(double,npure);
	P->xs    = ASC_NEW_ARRAY(double,npure);
	P->PF    = ASC_NEW_ARRAY(PureFluid *,npure);
	P->rhos  = ASC_NEW_ARRAY(double,npure);
	
	return P;
}

PhaseSpec *new_PhaseSpec(unsigned npure, unsigned nphase){
	unsigned i;
	PhaseSpec *P = ASC_NEW(PhaseSpec);

	P->phases  = 0;
	P->ph_type = ASC_NEW_ARRAY(PhaseName,nphase);
	P->ph_frac = ASC_NEW_ARRAY(double,nphase);
	P->PH      = ASC_NEW_ARRAY(Phase *,nphase);

	for(i=0;i<nphase;i++){
		P->PH[i] = new_Phase(npure);
	}

	return P;
}

PhaseMixState *new_PhaseMixState(unsigned npure, unsigned nphase, double T, double p){
	PhaseMixState *P = ASC_NEW(PhaseMixState);
	P->T  = T;
	P->p  = p;
	P->MS = new_MixtureSpec(npure);
	P->PS = new_PhaseSpec(npure,nphase);

	return P;
}

/* Functions to create and fill new structs with all members */
MixtureSpec *fill_MixtureSpec(unsigned npure, double *X, PureFluid **P){
	MixtureSpec *MS = new_MixtureSpec(npure);
	MS->Xs = X;
	MS->PF = P;
	return MS;
}

PhaseMixState *fill_PhaseMixState(double T, double p, PhaseSpec *P, MixtureSpec *M){
	PhaseMixState *PM = ASC_NEW(PhaseMixState);
	PM->T = T;
	PM->p = p;
	PM->PS = P;
	PM->MS = M;
	return PM;
}

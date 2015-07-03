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

	Enumerations and structures used in modeling mixtures
 */

#ifndef MIX_STRUCT_HEADER
#define MIX_STRUCT_HEADER

#include "../rundata.h"

#define MIX_XTOL 1e-6

typedef double SecantSubjectFunction(double, void *user_data);

/* Enumerations to hold phase-equilibrium conditions and names of phases */
/* typedef enum PhaseEquilibrium_Enum {
	GAS_PHASE, LIQ_PHASE, VLE_PHASE, LLE_PHASE
} PhaseEqb; */

typedef enum PhaseNames_Enum {
	SUPERCRIT, GAS, VAPOR, LIQUID, SOLID/* , SAT_VLE */
} PhaseName;

/*
	Possible errors which may be encountered in performing mixture and 
	phase-equilibrium calculations

	This enum will continue to be expanded as more error conditions are 
	identified.
 */
typedef enum MixPhaseError_Enum {
	MIXTURE_NO_ERROR = 0
	, MIXTURE_XSUM_ERROR /* mass fractions do not sum to one */
	, MIXTURE_PSUM_ERROR /* phase fractions do not sum to one */
	/* ,  */
} MixtureError;

/* Structures to represent mixtures and phases */
/*
	Bare specification of a mixture, with just the components and overall mass 
	fractions
 */
typedef struct MixtureSpec_Struct {
	unsigned pures; /* number of components */
	double *Xs;     /* mass fractions of components */
	PureFluid **PF; /* pure fluid characteristics of components */
} MixtureSpec;

/*
	Specification of the phases of a mixture, including the number of phases, 
	identities (gas, liquid, solid, etc.) of each phase, and mass fractions and 
	densities of components within each phase.

	This allows the phase data to be hidden within this structure in the 
	PhaseMixState structure below
 */
typedef struct PhaseSpec_Struct {
	unsigned phases;    /* number of phases */
	PhaseName *ph_name; /* type of each phase */
	double *ph_frac;    /* fraction of total mass in each phase */
	double **Xs;        /* mass fractions of components in each phase */
	double **rhos;      /* densities of components in each phase */
} PhaseSpec;

/*
	This is the bare-bones representation of the mixture state, without any 
	phase data.  Therefore, this structure SHOULD NOT be used in production 
	code.
 */
typedef struct MixtureState_Struct {
	double T;       /* mixture temperature */
	double *rhos;   /* (current) mass densities of components */
	MixtureSpec *X; /* specification of pure-component members of mixture */
} MixtureState;

/*
	Representation of mixture with phases
 */
typedef struct MixturePhaseState_Struct {
	double T;           /* mixture temperature */
	double **rhos;      /* (current) mass densities of components */
	MixtureSpec *X;     /* specification of pure-component members of mixture */
	unsigned phases;    /* number of phases */
	PhaseName *ph_name; /* type of each phase */
	double *ph_frac;    /* fraction of mass in each phase */
	double **Xs;        /* mass fractions within each phase */
} MixturePhaseState;

/*
	This is an alternate representation of mixture state with phases -- I 
	suspect it may prove superior to the MixturePhaseState structure above.
 */
typedef struct PhaseMixtureState_Struct {
	double T;        /* temperature */
	double p;        /* pressure */
	PhaseSpec *PH;   /* specification of phases */
	MixtureSpec *MX; /* specification of mixture */
} PhaseMixState;

#endif

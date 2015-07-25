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
	by Jacob Shealy, June 25-July 22, 2015

	Enumerations and structures used in modeling mixtures
 */

#ifndef MIX_STRUCT_HEADER
#define MIX_STRUCT_HEADER

#include "../rundata.h"

#if 0
#define MIX_XSUM_ERROR MIX_ERROR "the sum over all mass fractions, which should be exactly 1.00, is %.10f\n"
#define MIX_COMPR_ERROR MIX_ERROR "the compressibility has assumed a non-physical value"
#else
#define MIX_XSUM_ERROR "Sum over all mass fractions, which should be exactly 1.00, is %.10f\n"
#define MIX_COMPR_ERROR "Compressibility has assumed a non-physical value"
#endif

#define MIX_PI M_PI
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
	Specification of one single phase of a mixture, including number of components 
	present in the phase, indices of the components, etc.
 */
typedef struct Phase_Struct {
	unsigned ncomps; /* number of components in the phase */
	unsigned *c;     /* index within a MixtureSpec of each component present in the phase */
	double *Xs;      /* mass fractions of components in the phase */
	double *xs;      /* mole (NOT mass) fractions of components in the phase */
	PureFluid **PF;  /* pure fluid characteristics of components */
	double *rhos;    /* densities of components in the phase */
} Phase;

/*
	Specification of the phases of a mixture, including the number of phases, 
	identities (gas, liquid, solid, etc.) of each phase, and an array of phase 
	specifications.

	This allows the phase data to be hidden within this structure in the 
	PhaseMixState structure below
 */
typedef struct PhaseSpec_Struct {
	unsigned phases;    /* number of phases */
	PhaseName *ph_type; /* type of each phase */
	double *ph_frac;    /* fraction of total moles in each phase */
	Phase **PH;         /* specification of each phase */
} PhaseSpec;

/*
	A capable representation of mixture state with phases; this structure and 
	the three above are what you shoul use in code intended for actual use.
 */
typedef struct PhaseMixtureState_Struct {
	double T;        /* temperature */
	double p;        /* pressure */
	PhaseSpec *PS;   /* specification of phases */
	MixtureSpec *MX; /* specification of mixture */
} PhaseMixState;

/*
	WARNING: the following two structures were written for testing purposes and 
	SHOULD NOT be used in code intended for actual use.
 */
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

#endif

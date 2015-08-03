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
#include "../color.h"

#define MIX_PI M_PI
#define MIX_XTOL 1e-6
#define MIX_XSUM_ERROR "Sum over all mass fractions, which should be exactly 1.00, is %.10f\n"
#define MIX_PSUM_ERROR "Sum over all phase mass fractions, which should be exactly 1.00, is %.10f\n"
#define MIX_COMPR_ERROR "Compressibility has assumed a non-physical value"

#ifndef ASC_NEW
#define ASC_NEW(TYPE) (TYPE*)malloc(sizeof(TYPE))
#define ASC_NEW_ARRAY(TYPE,COUNT) (TYPE*)malloc(sizeof(TYPE)*(COUNT))
#endif

#define MIX_DEBUG
#define MIX_ERROR

#ifdef MIX_DEBUG
#define MSG FPROPS_MSG
#define MSG_MARK(MARK) MSG("mark " MARK)
#else
#define MSG(ARGS...) ((void)0)
#define MSG_MARK(ARGS...) ((void)0)
#endif

#ifdef MIX_ERROR
#define ERRMSG FPROPS_ERRMSG
#define ERRMSG_XSUM(SUM) FPROPS_ERRMSG(MIX_XSUM_ERROR, SUM)
#else
#define ERRMSG(ARGS...) ((void)0)
#define ERRMSG_XSUM(ARGS...) ((void)0)
#endif

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
	unsigned pures; /* number of components in the phase */
	unsigned *c;    /* index within a MixtureSpec of each component present in the phase */
	double *Xs;     /* mass fractions of components in the phase */
	double *xs;     /* mole (NOT mass) fractions of components in the phase */
	PureFluid **PF; /* pure fluid characteristics of components */
	double *rhos;   /* densities of components in the phase */
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
	MixtureSpec *MS; /* specification of mixture */
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

/*
	Macros to initialize MixtureSpec, PhaseSpec, and PhaseMixState structs
 */
#if 0
#define CREATE_NEW_MIX_SPEC(NAME,N_PURE) \
	NAME = ASC_NEW(MixtureSpec); \
	NAME##->pures = N_PURE; \
	NAME->Xs    = ASC_NEW_ARRAY(double,N_PURE); \
	NAME->PF    = ASC_NEW_ARRAY(PureFluid *,N_PURE);

#define CREATE_NEW_PHASE(NAME,N_PURE) \
	NAME = ASC_NEW(Phase); \
	NAME->pures = N_PURE; \
	NAME->c      = ASC_NEW_ARRAY(unsigned,N_PURE); \
	NAME->Xs     = ASC_NEW_ARRAY(double,N_PURE); \
	NAME->xs     = ASC_NEW_ARRAY(double,N_PURE); \
	NAME->PF     = ASC_NEW_ARRAY(PureFluid *,N_PURE); \
	NAME->rhos   = ASC_NEW_ARRAY(double,N_PURE);

#define CREATE_NEW_PHASE_SPEC(NAME,N_PURE,N_PHASE) \
	NAME = ASC_NEW(PhaseSpec); \
	NAME->phases  = 0; \
	NAME->ph_type = ASC_NEW_ARRAY(PhaseName,N_PHASE); \
	NAME->ph_frac = ASC_NEW_ARRAY(double,N_PHASE); \
	NAME->PH      = ASC_NEW_ARRAY(Phase *,N_PHASE); \
#ifndef i_ph_spec \
	unsigned i_ph_spec; \
#endif \
	for(i_ph_spec=0;i_ph_spec<N_PHASE;i_ph_spec++){ \
		CREATE_NEW_PHASE( NAME->PH[i_ph_spec], N_PURE ); \
	}

#define CREATE_NEW_PHASE_MIX_SPEC(NAME,N_PURE,N_PHASE,TEMP,PRESSURE) \
	NAME = ASC_NEW(PhaseMixSpec); \
	NAME->T = TEMP; \
	NAME->p = PRESSURE; \
	CREATE_NEW_MIX_SPEC(NAME->MX,N_PURE); \
	CREATE_NEW_PHASE_SPEC(NAME->PS,N_PURE,N_PHASE);
#endif

MixtureSpec *new_MixtureSpec(unsigned npure);
Phase *new_Phase(unsigned npure);
PhaseSpec *new_PhaseSpec(unsigned npure, unsigned nphase);
PhaseMixState *new_PhaseMixState(unsigned npure, unsigned nphase, double T, double P);

#endif

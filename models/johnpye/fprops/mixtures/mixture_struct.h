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
*//** @file
	Common macros, enumerations, and structs used in modeling mixtures.  Also 
	defines functions to initialize the structs defined.
*//*
	by Jacob Shealy, June 25-August 21, 2015
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
#define MIX_PHASE_STRING(PTYPE) (PTYPE==SUPERCRIT) ? "supercritical" : \
									   (PTYPE==VAPOR) ? "vapor" : "liquid"

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

/*----------------------------------------------------------------------
	PHASE ENUMERATIONS
 */
/**
	Possible phase types
 */
typedef enum PhaseNames_Enum {
	SUPERCRIT /**< supercritical phase.  In practical terms this is used instead of `GAS` */
	, GAS     /**< gaseous phase: above critical temperature, but below critical pressure */
	, VAPOR   /**< vapor phase */
	, LIQUID  /**< liquid phase */
	, SOLID   /**< solid phase -- unlikely to ever be encountered */
} PhaseName;

/**
	Possible errors which may be encountered in performing mixture and 
	phase-equilibrium calculations.  (Not used much)

	This enum may continue to be expanded as more error conditions are 
	identified.
 */
typedef enum MixPhaseError_Enum {
	MIXTURE_NO_ERROR = 0 /**< no error */
	, MIXTURE_XSUM_ERROR /**< mass fractions do not sum to one */
	, MIXTURE_PSUM_ERROR /**< phase fractions do not sum to one */
} MixtureError;


/*----------------------------------------------------------------------
	PHASE STRUCTS
 */
/**
	Minimal specification of a mixture, just components and overall mass 
	fractions.
 */
typedef struct MixtureSpec_Struct {
	unsigned pures; /**< number of components */
	double *Xs;     /**< mass fractions of components */
	PureFluid **PF; /**< pure fluid characteristics of components */
} MixtureSpec;

/**
	Specification of a single mixture phase
 */
typedef struct Phase_Struct {
	unsigned pures; /**< number of components in the phase */
	unsigned *c;    /**< index within a MixtureSpec of each component present in the phase */
	double *Xs;     /**< mass fractions of components in the phase */
	double *xs;     /**< mole (NOT mass) fractions of components in the phase */
	PureFluid **PF; /**< pure fluid characteristics of components */
	double *rhos;   /**< densities of components in the phase */
} Phase;

/**
	Specification of all phases in a mixture.  The phase data can be hidden 
	within this structure in the `PhaseMixState` structure
 */
typedef struct PhaseSpec_Struct {
	unsigned phases;    /**< number of phases */
	PhaseName *ph_type; /**< type of each phase */
	double *ph_frac;    /**< fraction of total moles in each phase */
	Phase **PH;         /**< specification of each phase */
} PhaseSpec;

/**
	A full representation of mixture state with its phases
 */
typedef struct PhaseMixtureState_Struct {
	double T;        /**< temperature */
	double p;        /**< pressure */
	PhaseSpec *PS;   /**< specification of phases */
	MixtureSpec *MS; /**< specification of mixture */
} PhaseMixState;


/*----------------------------------------------------------------------
	FORWARD DECLARATIONS
 */
/**
	Create a new MixtureSpec
	@param npure [in] number of components to initialize the MixtureSpec with
	@return Pointer to the new MixtureSpec
 */
MixtureSpec *new_MixtureSpec(unsigned npure);

/**
	Create and fill a new MixtureSpec from predefined members

	@param npure [in] the number of components
	@param X [in] an array of mass fractions
	@param P [in] an array of PureFluid structs which represent the components
	@return Pointer to the new MixtureSpec
 */
MixtureSpec *fill_MixtureSpec(unsigned npure, double *X, PureFluid **P);

/**
	Build a MixtureSpec from scratch, initializing the PureFluid structs

	@param npure [in] the number of components
	@param Xs [in] an array of mass fractions
	@param fluids [in] an array of names of the fluids
	@param type [in] the name of the equation of state to use
	@param source [in] an array of sources to use in setting fluid parameters
		Typically this will contain simply a series of NULL pointers
	@param merr [in] a MixtureError to record if any errors occur
	@return Pointer to the new MixtureSpec
 */
MixtureSpec *build_MixtureSpec(unsigned npure, double *Xs, void **fluids, char *type, char **source, MixtureError *merr);

/**
	Create a new Phase struct
	@param npure [in] the number of components to initialize the Phase with
	@return Pointer to the new Phase
 */
Phase *new_Phase(unsigned npure);

/**
	Create a new PhaseSpec struct

	@param npure [in] the (maximum) number of components in each member Phases
	@param nphase [in] the (maximum) number of phases (does not set the `phases` member)
	@return Pointer to the new PhaseSpec
 */
PhaseSpec *new_PhaseSpec(unsigned npure, unsigned nphase);

/**
	Create a new PhaseMixState struct

	@param npure [in] the number of pures in the mixture
	@param nphase [in] the (maximum) number of phases
	@param T [in] the mixture temperature
	@param P [in] the mixture pressure
	@return Pointer to the new PhaseMixState
 */
PhaseMixState *new_PhaseMixState(unsigned npure, unsigned nphase, double T, double P);

/**
	Create and fill a new PhaseMixState from predefined members

	@param T [in] the mixture temperature
	@param p [in] the mixture pressure
	@param P [in] the PhaseSpec struct to be used
	@param M [in] the MixtureSpec struct to be used
 */
PhaseMixState *fill_PhaseMixState(double T, double p, PhaseSpec *P, MixtureSpec *M);

#endif

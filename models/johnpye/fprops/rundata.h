/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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
*//** @file
This file contains declarations of the data structures passed to
functions that EVALUATE fluid properties. We allow from some preprocessing of
data loaded from input files, if deisred/needed.

Data declarations as provided in input files are given in filedata.h
*/

#ifndef FPROPS_RUNDATA_H
#define FPROPS_RUNDATA_H

#include "common.h"

/* TODO remove this dependency eventually (some helmholtz data objects are not yet being copied into new structures*/
#include "filedata.h"

typedef struct PureFluid_struct PureFluid;

/** Power terms for phi0 (including polynomial) */
typedef struct Phi0RunPowTerm_struct{
	double a;
	double p;
} Phi0RunPowTerm;

/** Planck-Einstein aka 'exponential' terms for phi0 */
typedef struct Phi0RunExpTerm_struct{
	double n;
	double gamma;
} Phi0RunExpTerm;

/**
	Runtime data for ideal gas properties, which are stored in the
	form of reduced ideal-gas compnent of helmholtz energy (see http://fprops.org)

	There is no 'R' or 'cp0star' in this structure. If cp0star != R in the filedata, that
	difference will be corrected for when this structure is created.
*/
typedef struct 	Phi0RunData_struct{
	double c;       /**< second integration constant in phi0, value determined by reference point for entropy */
	double m;       /**< first integration constant in phi0, value determined by reference point for enthalpy */

	unsigned np;    /**< number of power terms */
	Phi0RunPowTerm *pt; /**< power term data, may be NULL if np == 0 */
	unsigned ne;    /**< number of Planck-Einstein aka 'exponential' terms */
	Phi0RunExpTerm *et; /**< exponential term data, maybe NULL if ne == 0 */
} Phi0RunData;

typedef struct HelmholtzRunData_struct{
	double rho_star;/**< normalisation density, kg/m3 */
	double T_star;  /**< normalisation temperature, K */

	//REMOVED: double p_t; /**< triple-point pressure */

   	unsigned np;                 /**< number of power terms in residual equation */
	const HelmholtzPowTerm *pt;  /**< power term data for residual eqn, maybe NULL if np == 0 */
	unsigned ng;                 /**< number of critical terms of the first kind */
	const HelmholtzGausTerm *gt; /**< critical terms of the first kind */
	unsigned nc;                 /**< number of critical terms of the second kind */
	const HelmholtzCritTerm *ct; /**< critical terms of the second kind */
} HelmholtzRunData;

typedef struct PengrobRunData_struct{
	double aTc;   /**< value of 'a' when evaluated at T =  T_c */
	double b;     /**< coeficient 'b' in PR EOS */
	double kappa; /** parameter used in a(T) */
} PengrobRunData;

typedef struct IncompRunData_struct{
	DensityData rho;
	const Cp0Data *cp0;
	double const_h;
	double const_s;
} IncompRunData;

typedef union CorrelationUnion_union{
	HelmholtzRunData *helm;
	PengrobRunData *pengrob;
	IncompRunData *incomp;
	/* maybe more later */
} CorrelationUnion;

/** All runtime 'core' data for all possible correlations, with exception of
correlation-type-ID, function pointers and metadata (URLs, publications etc)

TODO FluidData (or PureFluid?) could/should be extended to include the following
frequently-calculated items:
	- fluid properties at triple point (rhoft, rhogt, pt...)
	- fluid properties at critical point (hc, ...)
	- accurate saturation curve data (interpolation/spline/something like that)
	- solutions of iterative solver results, eg (p,h) pairs.

This data would be held at this level unless it is correlation-specific in
nature, in which case it would belong in lower-level rundata structures.

For fluids without phase change (incompressible, ideal), we
	- set T_c to zero,
	- use a value of 1 K for Tstar
	- provide a _sat SatEvalFn that always returns an error.
...but maybe there's a better way. It's up to the particular PropEvalFn to
make use of Tstar or T_c as desired, but this data is stored here
*/
typedef struct FluidData_struct{
	/* common data across all correlations */
	double R;     /**< specific gas constant */
	double M;     /**< molar mass, kg/kmol */
	double T_t;   /**< triple-point temperature */
	double T_min; /**< minimium temperature, Tmin in RPP4 */
	double T_f;   /**< normal freezing temperature (1 atm), Tfp in RPP4 */
	double T_c;   /**< critical temperature */
	double p_c;   /**< critical pressure */
	double rho_c; /**< critical density */
	double omega; /**< acentric factor (possibly calculated from correlation data)*/
	double Tstar;   /**< reference for reduced temperature */
	double rhostar; /**< reference for reduced density */
	Phi0RunData *cp0; /* data for ideal component of Helmholtz energy */
	ReferenceState ref0;
	/* correlation-specific stuff here */
	CorrelationUnion corr;
} FluidData;



/**
	State object for FPROPS. This struct allows user-friendly API in a similar
	way to in freesteam, but supports different fluid types and correlations,
	and might be extensible to support fluid mixtures.

	TODO if we've got a saturated state then we almost certainly have calculated
	rhof, rhog, p. They're expensive, so we should save them in this state
	struct, too, or else find some other way to cache them.

	TODO perhaps eventually we can different different correlations using
	different independent variables, in which case this state could be modified/
	expanded/improved.
*/

#if 0
typedef enum FluidStateType_enum{
	FPROPS_STATE_TRHO /* helholtz, pengrob, ideal... compressible fluids */
	,FPROPS_STATE_TP /* needed for incompressible fluids */
} FluidStateType;
#endif

typedef struct FluidStateTrho_struct{
	double T;
	double rho;
}FluidStateTrho;

typedef struct FluidStateTp_struct{
	double T;
	double p;
}FluidStateTp;

/** Union of all possible EOS data structures */
typedef union FluidStateUnion_union{
	FluidStateTrho Trho;
	FluidStateTp Tp; // applicable to incompressible mixtures
	/* maybe eventually this will also be able to contain mixture states...? */
	/* NOTE that this union doesn't know which data it is storing; the form of data depends on the type of FluidData. */
} FluidStateUnion;

/**
	An updated FluidState struct that stores different state variables
	depending on the EOS currently in use. Ideally, the user shouldn't need
	to know what's inside this struct, and should use 'accessor functions'
	instead.
*/
typedef struct FluidState2_struct{
	FluidStateUnion vals;
	const PureFluid *fluid; ///< pointer to fluid description and associated functions
} FluidState2;

/* Definition of a fluid property function pointer */
typedef double PropEvalFn2(FluidStateUnion vals,const FluidData *data, FpropsError *err);

//typedef double PropEvalFn(double T, double rho, const FluidData *data, FpropsError *err);

/** @return psat */
typedef double SatEvalFn(double T,double *rhof, double *rhog, const FluidData *data, FpropsError *err);

typedef int SetRefStateFn(PureFluid *data, const ReferenceState *ref);

typedef enum PhaseBehaviour_enum{
	FPROPS_SINGLEPHASE
	,FPROPS_VLE
} PhaseBehaviour;

/**
	Structure containing all the necessary data and metadata for run-time
	calculation of fluid properties.
*/
typedef struct PureFluid_struct{
    const char *name;
	const char *source;
	EosType type;
	FluidData *data; // everything we need at runtime in the following functions should be in here
	//Pointers to evaluation functions, which are specific for the EoSType selected.
	PropEvalFn2 *T_fn; // should be used externally in preference to reading the FluidState2
	PropEvalFn2 *rho_fn; // should be used externally in preference to reading the FluidState2
	PropEvalFn2 *p_fn;
	PropEvalFn2 *u_fn;
	PropEvalFn2 *h_fn;
	PropEvalFn2 *s_fn;
	PropEvalFn2 *a_fn;
	PropEvalFn2 *cv_fn;
	PropEvalFn2 *cp_fn;
	PropEvalFn2 *w_fn;
	PropEvalFn2 *g_fn;
	PropEvalFn2 *alphap_fn;
	PropEvalFn2 *betap_fn;
	PropEvalFn2 *dpdrho_T_fn; // this derivative is required for saturation properties by Akasaka method
	SatEvalFn *sat_fn; // function to return {psat,rhof,rhog}(T) for this pure fluid;
	SetRefStateFn *setref_fn; // function to set reference state for this pure fluid

	const ViscosityData *visc; // TODO should it be here? or inside FluidData?? probably yes, but needs review.
	const ThermalConductivityData *thcond; // TODO should it be here? probably yes, but needs review.
} PureFluid;

#endif

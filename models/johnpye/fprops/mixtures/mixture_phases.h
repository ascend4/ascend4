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
	by Jacob Shealy, July 26-, 2015 (GSOC 2015)

	Headers & some macro definitions for ideal-solution mixture functions
 */

#ifndef MIX_PHASE_HEADER
#define MIX_PHASE_HEADER

#include "mixture_generics.h"
#include "mixture_struct.h"

typedef struct RachRiceData_Struct {
	unsigned pures;
	double *zs;
	double *Ks;
} RRData;
/**
	Structure to pass constant parameters into the Rachford-Rice function
 */

typedef struct DewBubbleData_Struct {
	MixtureSpec *MS;  /* components and mass fractions */
	double T;         /* temperature */
	double *p_sat;    /* saturation pressures for all components */
	/* double *rho_v; */ /* vapor densities at dew pressure for all components */
	/* double *rho_l; */ /* liquid densities at dew pressure for all components */
	double *rhos;     /* vapor-phase densities at dew pressure */
	double *sat_rhos; /* liquid saturation densities for all components */
	double tol;       /* tolerance to which to solve */
	FpropsError *err; /* error enumeration */
} DBData;
/**
	Structure to pass constant parameters into the dew-point and bubble-point 
	functions
 */

SecantSubjectFunction rachford_rice;
SecantSubjectFunction dew_p_error;
SecantSubjectFunction bubble_p_error;

double dew_pressure(MixtureSpec *MS, double T, FpropsError *err);
/**
	Find the dew pressure using the function 'dew_p_error'.  This should only be 
	called after determining which components in a mixture are subcritical.

	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature of the mixture
	@param err an FpropsError struct to pass to other FPROPS functions

	@return the dew pressure at T
 */

double bubble_pressure(MixtureSpec *MS, double T, FpropsError *err);
/**
	Find the bubble pressure using the function 'bubble_p_error'.  This should 
	only be called after determining which components in a mixture are 
	subcritical.

	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature of the mixture
	@param err an FpropsError struct to pass to other FPROPS functions

	@return the bubble pressure of the mixture
 */

double pengrob_phi_pure(PureFluid *PF, double T, double P, PhaseName type, FpropsError *err);
/**
	Find the fugacity coefficient (phi) of a chemical species, using the 
	Peng-Robinson equation of state.

	@param PF a PureFluid struct representing the chemical species
	@param T the temperature
	@param P the pressure
	@param type a PhaseName struct representing the species phase (liquid/vapor)
	@param err an FpropsError struct to pass to other FPROPS functions

	@return the value of the fugacity coefficient at the given conditions
 */

double poynting_factor(PureFluid *PF, double T, double P, FpropsError *err);
/**
	Find the Poynting Factor of a chemical species

	@param PF a PureFluid struct representing the chemical species
	@param T the temperature
	@param P the pressure
	@param err an FpropsError struct to pass to other FPROPS functions

	@return the value of the Poynting Factor at the given conditions
 */

void mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, FpropsError *err);
/**
	Find what phases a mixture splits into at a given temperature and pressure, 
	and the mass/mole fractions of each phase and each component within each 
	phase.

	@param PS a PhaseSpec struct to hold the description of the phases
	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature
	@param P the pressure
	@param err an FpropsError struct to pass to other FPROPS functions
 */

#endif

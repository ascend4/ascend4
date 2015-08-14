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
	double tol;       /* tolerance to which to solve */
	FpropsError *err; /* error enumeration */
} DBData;
/**
	Struct to pass constant parameters into the dew-pressure and bubble-pressure 
	functions.  This only works in the functions that calculate pressures.
 */

typedef struct DewBubbleTemperatureData_Struct {
	MixtureSpec *MS;  /* components and mass fractions */
	double p;         /* pressure */
	double tol;       /* tolerance to which to solve */
	FpropsError *err; /* error enumeration */
} DBTempData;
/**
	Struct to pass constant parameters into the dew-temperature and 
	bubble-temperature functions.  This only works in the functions that 
	calculate temperatures.
 */

SecantSubjectFunction rachford_rice;
SecantSubjectFunction dew_p_error;
SecantSubjectFunction bubble_p_error;
SecantSubjectFunction dew_T_error;
SecantSubjectFunction bubble_T_error;

double dew_pressure(MixtureSpec *MS, double T, double tol, FpropsError *err);
/**
	Find the dew pressure using the function 'dew_p_error'.  This should only be 
	called after determining which components in a mixture are subcritical.

	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature of the mixture
	@param tol the tolerance to which to find the dew pressure
	@param err an FpropsError struct to pass to other FPROPS functions

	@return the dew pressure at T
 */

double bubble_pressure(MixtureSpec *MS, double T, double tol, FpropsError *err);
/**
	Find the bubble pressure using the function 'bubble_p_error'.  This should 
	only be called after determining which components in a mixture are 
	subcritical.

	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature of the mixture
	@param tol the tolerance to which to find the bubble pressure
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

int mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, double tol, FpropsError *err);
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

int mixture_dew_pressure(double *p_d, MixtureSpec *MS, double T, double tol, FpropsError *err);
/**
	Find the mixture dew pressure using the function 'dew_p_error'.  This 
	checks for the presence of subcritical pure components, so it can be called 
	directly on a mixture.

	@param p_d a double which will hold the dew pressure
	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature of the mixture
	@param err an FpropsError struct to pass to other FPROPS functions

	@return whether the calculation succeeded or failed
 */

int mixture_bubble_pressure(double *p_b, MixtureSpec *MS, double T, double tol, FpropsError *err);
/**
	Find the mixture bubble pressure using the function 'bubble_p_error'.  This 
	checks for the presence of subcritical pure components, so it can be called 
	directly on a mixture.

	@param p_b a double which will hold the bubble pressure
	@param MS a MixtureSpec struct which describes the mixture composition
	@param T the temperature of the mixture
	@param err an FpropsError struct to pass to other FPROPS functions

	@return whether the calculation succeeded or failed
 */

int mixture_dew_temperature(double *T_d, MixtureSpec *MS, double p, double tol, FpropsError *err);
/**
	Find the mixture dew temperature using the function 'dew_T_error'.  Checks 
	for the presence of subcritical components, so it can be called directly on 
	a mixture.

	@param T_d a double which will hold the dew temperature
	@param MS a MixtureSpec struct which describes the mixture composition
	@param p the pressure of the mixture
	@param err an FpropsError struct to pass to other FPROPS functions

	@return whether the calculation succeeded or failed
 */

int mixture_bubble_temperature(double *T_b, MixtureSpec *MS, double p, double tol, FpropsError *err);
/**
	Find the mixture bubble temperature using the function 'dew_T_error'.  
	Checks for the presence of subcritical components, so it can be called 
	directly on a mixture.

	@param T_b a double which will hold the bubble temperature
	@param MS a MixtureSpec struct which describes the mixture composition
	@param p the pressure of the mixture
	@param err an FpropsError struct to pass to other FPROPS functions

	@return whether the calculation succeeded or failed
 */

#endif

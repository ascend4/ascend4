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
	Functions that calculate phase conditions within a mixture.

	a
*//*
	by Jacob Shealy, July 26-August 21, 2015
 */

#ifndef MIX_PHASE_HEADER
#define MIX_PHASE_HEADER

#include "mixture_generics.h"
#include "mixture_struct.h"


/*----------------------------------------------------------------------
	ESSENTIAL STRUCTURES
 */
/**
	Passes constant parameters into the Rachford-Rice function
 */
typedef struct RachRiceData_Struct {
	unsigned pures; /**< number of pure components */
	double *zs;     /**< overall mole fractions */
	double *Ks;     /**< K-factors (ratio between vapor and liquid mole fractions) */
} RRData;

/**
	Passes constant parameters into the functions that find the error in 
	dew-pressure and bubble-pressure.  This only works in the functions that 
	calculate pressures, not in the dew- or bubble-temperature functions.
 */
typedef struct DewBubbleData_Struct {
	MixtureSpec *MS;  /**< components and mass fractions */
	double T;         /**< temperature */
	double *p_sat;    /**< saturation pressures for all components */
	double tol;       /**< tolerance to which to solve */
	FpropsError *err; /**< error enumeration */
} DBData;

/**
	Passes constant parameters into the functions that find the error in 
	dew-temperature and bubble-temperature.  This only works for the functions 
	that calculate temperatures, not in the dew- or bubble-pressure functions.
 */
typedef struct DewBubbleTemperatureData_Struct {
	MixtureSpec *MS;  /**< components and mass fractions */
	double p;         /**< pressure */
	double tol;       /**< tolerance to which to solve */
	FpropsError *err; /**< error enumeration */
} DBTempData;


/*----------------------------------------------------------------------
	ERROR FUNCTIONS
 */
/**
	Finds the value of the Rachford-Rice equation, 
	\f[
	F = \sum\limits_i y_i - x_i
	= \sum\limits_i \frac{z_i (K_i - )}{1 + \mathcal{V} (K_i - 1)}
	\f]
	which equals zero when in vapor-liquid phase equilibrium.

	@param V [in] the current vapor mole fraction of the mixture
	@param user_data [in] constant parameters for the function; original type RRData
	@return Value of the Rachford-Rice equation.
 */
SecantSubjectFunction rachford_rice;

/**
	Finds the dew pressure at conditions given, including current dew pressure 
	(dew pressure appears in some terms used to find dew pressure).  Returns the 
	difference (error) between the given and calculated dew pressures.

	@param P_D [in] the current dew pressure
	@param user_data [in] constant parameters for the function; original type DBData
	@return Difference between the given and calculated dew pressures.
 */
SecantSubjectFunction dew_p_error;

/**
	Finds the bubble pressure at conditions given (including current bubble 
	pressure), and the difference between given and calculated bubble pressure.

	@param P_B [in] the current bubble pressure
	@param user_data [in] constant parameters for the function; original type DBData
	@return Difference (error) between the given and calculated bubble pressures.
 */
SecantSubjectFunction bubble_p_error;

/**
	Finds the dew temperature from conditions given (including a current 
	dew temperature), and the difference between given and calculated dew 
	temperatures.

	@param T_D [in] the current dew temperature
	@param user_data [in] constant parameters for the function; original type DBTempData
	@return Difference (error) between given and calculated dew temperatures.
 */
SecantSubjectFunction dew_T_error;

/**
	Finds the bubble temperature from conditions given (including a current 
	bubble temperature), and the difference between given and calculated bubble 
	temperatures.

	@param T_B [in] the current bubble temperature
	@param user_data [in] constant parameters for the function; original type DBTempData
	@return Difference (error) between given and calculated bubble temperatures.
 */
SecantSubjectFunction bubble_T_error;


/*----------------------------------------------------------------------
	PHASE-EQUILIBRIUM FUNCTIONS
 */
/**
	Find the dew pressure using `dew_p_error`.  This should only be 
	called after determining which components in a mixture are subcritical.

	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param T [in] the temperature of the mixture
	@param tol [in] the tolerance to which to find the dew pressure
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions

	@return Dew pressure of the mixture at T
 */
double dew_pressure(MixtureSpec *MS, double T, double tol, FpropsError *err);

/**
	Find the bubble pressure using the function `bubble_p_error`.  This should 
	only be called after determining which components in a mixture are 
	subcritical.

	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param T [in] the temperature of the mixture
	@param tol [in] the tolerance to which to find the bubble pressure
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions

	@return Bubble pressure of the mixture at T
 */
double bubble_pressure(MixtureSpec *MS, double T, double tol, FpropsError *err);

/**
	Find the fugacity coefficient (phi) of a chemical species, using the 
	Peng-Robinson equation of state.

	@param PF [in] a PureFluid struct representing the chemical species
	@param T [in] the temperature
	@param P [in] the pressure
	@param type [in] a PhaseName struct representing the species phase (liquid/vapor)
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return Value of the fugacity coefficient
 */
double pengrob_phi_pure(PureFluid *PF, double T, double P, PhaseName type, FpropsError *err);

/**
	Finds the Poynting Factor of a chemical species.

	@param PF [in] a PureFluid struct representing the chemical species
	@param T [in] the temperature
	@param P [in] the pressure
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return Value of the Poynting Factor
 */
double poynting_factor(PureFluid *PF, double T, double P, FpropsError *err);

/**
	Finds what phases a mixture splits into at a given temperature and pressure, 
	and the mass/mole fractions of each phase and each component within each 
	phase.

	@param PS [in,out] a PhaseSpec struct to hold the description of the phases
	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param T [in] the temperature
	@param P [in] the pressure
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return 0 on success
 */
int mixture_flash(PhaseSpec *PS, MixtureSpec *MS, double T, double P, double tol, FpropsError *err);

/**
	Finds the mixture dew pressure using the function 'dew_p_error'.  This 
	checks for the presence of subcritical pure components, so this function can 
	be called directly on a mixture.

	@param p_d [out] a double which will hold the dew pressure
	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param T [in] the temperature of the mixture
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return 0 on success
 */
int mixture_dew_pressure(double *p_d, MixtureSpec *MS, double T, double tol, FpropsError *err);

/**
	Finds the mixture bubble pressure using the function 'bubble_p_error'.  This 
	checks for the presence of subcritical pure components, so this function can 
	be called directly on a mixture.

	@param p_b [out] a double which will hold the bubble pressure
	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param T [in] the temperature of the mixture
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return 0 on success
 */
int mixture_bubble_pressure(double *p_b, MixtureSpec *MS, double T, double tol, FpropsError *err);

/**
	Finds the mixture dew temperature using the function 'dew_T_error'.  Checks 
	for the presence of subcritical components, so this function can be called 
	directly on a mixture.

	@param T_d [out] a double which will hold the dew temperature
	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param p [in] the pressure of the mixture
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return 0 on success
 */
int mixture_dew_temperature(double *T_d, MixtureSpec *MS, double p, double tol, FpropsError *err);

/**
	Finds the mixture bubble temperature using the function 'dew_T_error'.  
	Checks for the presence of subcritical components, so this function can be 
	called directly on a mixture.

	@param T_b [out] a double which will hold the bubble temperature
	@param MS [in] a MixtureSpec struct which describes the mixture composition
	@param p [in] the pressure of the mixture
	@param err [in,out] an FpropsError struct to pass to other FPROPS functions
	@return 0 on success
 */
int mixture_bubble_temperature(double *T_b, MixtureSpec *MS, double p, double tol, FpropsError *err);

#endif

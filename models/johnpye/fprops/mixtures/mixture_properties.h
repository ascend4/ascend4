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
	Functions that calculate mixture properties.

	The functions here fall into two broad categories.  First, those that are 
	used to set or determine the state of the mixture.  These are represented by 
	`mixture_rhos_sat` and `mixture_T_ph`, and by the secant subject functions 
	`pressure_rho_error` and `enthalpy_T_error` which they use.
	
	All other functions fall into the second category, those which calculate 
	mixture properties for each phase of a mixture and for the whole mixture.  
	In each case, whole-mixture and by-phase properties are calculated by a 
	single function.
*//*
	by Jacob Shealy, July 30-August 21, 2015
 */

#ifndef MIX_PROPERTIES_HEADER
#define MIX_PROPERTIES_HEADER

#include "mixture_struct.h"
#include "../helmholtz.h"
#include "../fluids.h"
#include "../fprops.h"
#include "../refstate.h"
#include "../sat.h"

#include <stdio.h>
#include <math.h>

typedef double MixPropertyFunc(PhaseMixState *PM, double *p_phases, FpropsError *err);


/*----------------------------------------------------------------------
	ESSENTIAL STRUCTURES 
 */
/**
	Passes constant parameters into the function that finds the error in 
	pressure at a given density.
 */
typedef struct PressureRhoData_Struct{
	double T;         /**< mixture temperature */
	double p;         /**< mixture pressure */
	PureFluid *pfl;   /**< pure fluid from the mixture */
	FpropsError *err; /**< necessary error variable */
} PRData;

/**
	Passes constant parameters into the function that finds the error in 
	temperature at a given enthalpy.
 */
typedef struct EnthalpyTData_Struct{
	double h;         /**< the whole-mixture enthalpy being sought */
	double p;         /**< the pressure of the mixture */
	MixtureSpec *MS;  /**< specification of mixture composition */
    double tol;       /**< error to be used in solving flash condition for mixture */
	FpropsError *err; /**< necessary error variable */
} HTData;

/*----------------------------------------------------------------------
	SET/FIND MIXTURE STATE
 */
/**
	Finds the error in the current density value, for finding density from 
	pressure.

	@param rho [in] a conjectural value for the density
	@param user_data [in] extra (constant) data used to calculate the density
	@return Difference between the conjectured and calculated densities
 */
SecantSubjectFunction pressure_rho_error;

/**
	Finds the error in the current temperature value, for finding temperature 
	from enthalpy.

	@param T [in] a conjectural value for the temperature
	@param user_data [in] extra (constant) data used to calculate the temperature
	@return Difference between the conjectured and calculated temperatures
 */
SecantSubjectFunction enthalpy_T_error;

/**
	Find the density of each component within each phase of a mixture, at which 
	the temperature and pressure are as given.

	@param PS [out] a PhaseSpec struct representing the mixture with phases
	@param T [in] the mixture temperature
	@param P [in] the mixture pressure
	@param tol [in] a tolerance for the root-finding function that seeks the density
	@param err [in] an error variable necessary to calculate component properties

	@return 0 on success
 */
int mixture_rhos_sat(PhaseSpec *PS, double T, double P, double tol, FpropsError *err);

/**
	Find the temperature at which the pressure and enthalpy are as given.

	@param T [out] the mixture temperature
	@param MS [in] a MixtureSpec struct representing the mixture composition
	@param p [in] the mixture pressure
	@param h [in] the mixture enthalpy
	@param tol [in] a tolerance for the root-finding function that seeks the temperature
	@param err [in] an error variable necessary to calculate component properties

	@return 0 on success
 */
int mixture_T_ph(double *T, MixtureSpec *MS, double p, double h, double tol, FpropsError *err);


/*----------------------------------------------------------------------
	MIXTURE PROPERTY FUNCTIONS
 */
/**
	Calculate the density of the whole mixture, and density of each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase density
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture density
 */
double mixture_rho(PhaseMixState *PM, double *rhos);

/**
	Calculates the internal energy of the whole mixture, and internal energy of 
	each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase internal energy
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture internal energy
 */
MixPropertyFunc mixture_u;

/**
	Calculates the enthalpy of the whole mixture, and enthalpy of each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase enthalpy
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture enthalpy
 */
MixPropertyFunc mixture_h;

/**
	Calculates the constant-pressure heat capacity of the whole mixture, and 
	constant-pressure heat capacity of each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase heat capacity
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture constant-pressure heat capacity
 */
MixPropertyFunc mixture_cp;

/**
	Calculates the constant-volume heat capacity of the whole mixture, and 
	constant-volume heat capacity of each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase heat capacity
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture constant-volume heat capacity
 */
MixPropertyFunc mixture_cv;

/**
	Calculates the entropy of the whole mixture, and entropy of each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase entropy
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture entropy
 */
MixPropertyFunc mixture_s;

/**
	Calculates the Gibbs energy of the whole mixture, and Gibbs energy of each 
	phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an array to hold the by-phase Gibbs energy
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture Gibbs energy
 */
MixPropertyFunc mixture_g;

/**
	Calculates the Helmholtz energy of the whole mixture, and Helmholtz energy 
	of each phase.

	@param PM [in] a PhaseMixState struct representing the mixture, with phases
	@param p_phases [out] an output array that will hold the by-phase Helmholtz energy
	@param err [in] an error variable necessary to calculate component properties

	@return Mixture Helmholtz energy
 */
MixPropertyFunc mixture_a;

#endif

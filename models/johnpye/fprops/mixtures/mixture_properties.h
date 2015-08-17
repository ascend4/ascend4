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
	by Jacob Shealy, July 30-,2015

	Headers for functions that calculate mixture properties.
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

SecantSubjectFunction pressure_rho_error;
/**
	Finds the error in the current density value, for finding density from 
	pressure.

	@param rho a conjectural value for the density
	@param user_data extra (constant) data used to calculate the density

	@return the difference between the conjectured and calculated densities
 */

SecantSubjectFunction enthalpy_T_error;
/**
	Finds the error in the current temperature value, for finding temperature 
	from enthalpy.

	@param T a conjectural value for the temperature
	@param user_data extra (constant) data used to calculate the temperature

	@return the difference between the conjectured and calculated temperatures
 */

int mixture_rhos_sat(PhaseSpec *PS, double T, double P, double tol, FpropsError *err);
/**
	Find the density of each component within each phase of a mixture, at which 
	the temperature and pressure are as given.

	@param PS a PhaseSpec struct representing the mixture with phases
	@param T the mixture temperature
	@param P the mixture pressure
	@param tol a tolerance for the root-finding function that seeks the density
	@param err an error variable necessary to calculate component properties

	@return whether setting the densities succeeded or failed
 */

int mixture_T_ph(double *T, MixtureSpec *MS, double p, double h, double tol, FpropsError *err);
/**
	Find the temperature at which the pressure and enthalpy are as given.

	@param T an output, the mixture temperature
	@param MS a MixtureSpec struct representing the mixture composition
	@param p the mixture pressure
	@param h the mixture enthalpy
	@param tol a tolerance for the root-finding function which seeks the temperature
	@param err an error variable necessary to calculate component properties

	@return whether setting the temperature succeeded or failed
 */

double mixture_rho(PhaseMixState *PM, double *rhos);
/**
	Calculate the density of the whole mixture, and density of each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase density
	@param err an error variable necessary to calculate component properties

	@return the mixture density
 */

MixPropertyFunc mixture_u;
/**
	Calculates the internal energy of the whole mixture, and internal energy of 
	each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase internal energy
	@param err an error variable necessary to calculate component properties

	@return the mixture internal energy
 */

MixPropertyFunc mixture_h;
/**
	Calculates the enthalpy of the whole mixture, and enthalpy of each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase enthalpy
	@param err an error variable necessary to calculate component properties

	@return the mixture enthalpy
 */

MixPropertyFunc mixture_cp;
/**
	Calculates the constant-pressure heat capacity of the whole mixture, and 
	constant-pressure heat capacity of each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase heat capacity
	@param err an error variable necessary to calculate component properties

	@return the mixture constant-pressure heat capacity
 */

MixPropertyFunc mixture_cv;
/**
	Calculates the constant-volume heat capacity of the whole mixture, and 
	constant-volume heat capacity of each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase heat capacity
	@param err an error variable necessary to calculate component properties

	@return the mixture constant-volume heat capacity
 */

MixPropertyFunc mixture_s;
/**
	Calculates the entropy of the whole mixture, and entropy of each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase entropy
	@param err an error variable necessary to calculate component properties

	@return the mixture entropy
 */

MixPropertyFunc mixture_g;
/**
	Calculates the Gibbs energy of the whole mixture, and Gibbs energy of each 
	phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase Gibbs energy
	@param err an error variable necessary to calculate component properties

	@return the mixture Gibbs energy
 */

MixPropertyFunc mixture_a;
/**
	Calculates the Helmholtz energy of the whole mixture, and Helmholtz energy 
	of each phase.

	@param PM a PhaseMixState struct representing the mixture, with phases
	@param p_phases an output array that will hold the by-phase Helmholtz energy
	@param err an error variable necessary to calculate component properties

	@return the mixture Helmholtz energy
 */

#endif

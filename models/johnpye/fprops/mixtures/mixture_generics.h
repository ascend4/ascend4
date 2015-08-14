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

	Function headers for generic functions used in modeling mixtures
 */

#ifndef MIX_GENERICS_HEADER
#define MIX_GENERICS_HEADER

#include "mixture_struct.h"
#include "../zeroin.h"

#include <stdio.h>
#include <math.h>

/*
	Maxima and Minima
 */
double min_element(unsigned nelems, double *nums);
/**
	Finds the minimum element within an array of doubles

	@param nelems the length of the array to search
	@param nums the array of doubles to search

	@return minimum element of 'nums'
 */

int min_positive_elem(double *min, unsigned nelems, double *nums);
/**
	Finds the minimum positive element within an array of doubles

	@param min variable to hold the value of the minimum positve element
	@param nelems length of the array to search
	@param nums array of doubles to search

	@return whether a qualifying element was found (array contents might all be
	negative...)
 */

double max_element(unsigned nelems, double *nums);
/**
	Find the maximum element in an array of doubles

	@param nelems the length of the array to search
	@param nums the array of doubles to search

	@return maximum element of 'nums'
 */

double sum_elements(unsigned nelems, double *nums);
/**
	Sum element of an array of doubles

	@param nelems the length of the array to sum
	@param nums the array of doubles to sum

	@return sum of the elements of 'nums'
 */

unsigned index_of_min(unsigned nelems, double *nums);
/**
	Find the index of the minimum element in an array of doubles

	@param nelems the length of the array to search
	@param nums the array of doubles to search

	@return index of the minimum element within 'nums'
 */

unsigned index_of_max(unsigned nelems, double *nums);
/**
	Find the index of the maximum element in an array of doubles

	@param nelems the length of the array to search
	@param nums the array of doubles to search

	@return index of the maximum element within 'nums'
 */

/*
	Finding the Roots of Equations
 */
int secant_solve(SecantSubjectFunction *func, void *user_data, double x[2], double tol);
/**
	Find the intersection (root) of a function by the secant method (like 
	Newton's method, but using secants rather than derivatives)

	@param func the function for which to find the root
	@param user_data extra data (not varied by secant_solve) used by 'func'
	@param x the starting values of the variable being zeroed; x[0] will be set to the approximate zero point of 'func'
	@param tol how close 'func' has to be to zero to declare the solution a success

	@return whether the root-finding succeeded
 */

int cubic_solution(double coef[4], double *roots);
/**
	Find only the real roots of a cubic equation

	@param coef an array of equation coefficients
	@param roots an array which will hold the real roots

	@return the number of roots (1-3)
 */

/*
	Finding and Interconverting Fractions
 */
void mole_fractions(unsigned npure, double *x_mole, double *X_mass, PureFluid **PF);
/**
	Find the mole fractions of a mixture from the mass fractions

	@param npure the number of pure components
	@param x_mole an output array of mole fractions
	@param X_mass an array of mass fractions
	@param PF an array of fluids, the mixture components
 */

void mass_fractions(unsigned npure, double *X_mass, double *x_mole, PureFluid **PF);
/**
	Find the mass fractions of a mixture from the mole fractions

	@param npure the number of pure components
	@param X_mass an output array of mass fractions
	@param x_mole an array of mole fractions
	@param PF an array of fluids, the mixture components
 */

void mixture_x_props(unsigned npure, double *Xs, double *props);
/**
	Find fractions (e.g. mass or mole fractions) that sum to 1.0 and bear a 
	particular set of ratios to each other.

	@param npure the number of fractions
	@param Xs an output array of fractions, which sum to 1.0
	@param props an array of numbers, which bear the same ratios to each other as the members of 'Xs' do
 */

double mixture_x_fill_in(unsigned npure, double *Xs);
/**
	Find the last of a set of fractions that sum to 1.0

	@param npure the number of fractions
	@param Xs the fractions, which must sum to less than 1.0

	@return the difference between 1.0 and the sum over all elements of 'Xs'
 */

double mixture_M_avg(unsigned npure, double *Xs, PureFluid **PF);
/**
	Find the average molar mass of a mixture

	@param npure the number of pure components
	@param Xs an array of mass fractions
	@param PF an array of fluids, the components in the mixture

	@return the average molar mass of the mixture so specified
 */

double mixture_x_ln_x(unsigned npure, double *x_mass, PureFluid **PF);
/**
	Find the sum over all mole fractions, of mole fraction times natural 
	logarithm of the mole fraction:
		\sum\limits_i x_i \ln(x_i)

	This quantity is used in calculating second-law properties.

	@param npure the number of pure components
	@param Xs an array of mass fractions
	@param PF an array of PureFluid structs

	@return sum of mole fraction x_i times ln(x_i)
 */

#endif

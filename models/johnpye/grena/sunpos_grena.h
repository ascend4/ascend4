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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

/* WARNING: this is a FIRST DRAFT of the code and HAS NOT BEEN CHECKED yet. */

/** @FILE
	Code reproduced with minor cosmetic changes from:
	R Grena (2008), An algorithm for the computation of the solar position, 
	Solar Energy (82), pp 462-470.

	This header file contains the declaration of a class which includes all the
	input and output data, and the function that performs the calculation.

	To calculate the sun position, follow these steps:

	1. include this file.
	2. declare a variable of type SunCoord.
	3. Initialise the variable given the 9 input quantities required.
	   This can be done in the declaration, listing the quantities between 
	   commas, or calling the function SetCoord(). In both cases, only the first
	   four quantities are required; the others default to standard values
	   (pressure = 1 atm, T = 20 °C, 0 for all the other quantities) if omitted.
	   Longitude and latitude must be given in RADIANS, pressure in ATM,
	   temperature in °C.
	   Day, Month and Year are integer, UT in decimal hour from 0 to 24 (e.g.
	   3:30 pm becomes 15.5).
	4. Call the Calculate() method of the SunCoord object.

	Example:
	(see the original publication)

	Warning: in order to improve accessibility and efficiency, there is not 
	access control in the class. The user is free to directly access and 
	modify all the data and there is not any control of consistency. Some
	caution in the use of the class is advisable.
*/
#ifndef SUNPOS_GRENA_H
#define SUNPOS_GRENA_H

#include <iostream>
#include <cmath>

#ifndef PI
# define PI 3.14159265358979
#endif

/**
	Structure to hold sun position data, both input data as well as calculated
	output data.

	FIXME convert fields p, T to base SI units Pa and K.
*/
typedef struct SunPos_struct{
	// input data
	double t; ///< Ephemeris Julian Day, offset such that 0 = noon 1 Jan 2003.
	double latitude; ///< Latitude (N = positive??), in RADIANS.
	double longitude; ///< Longitude (E = positive??), in RADIANS.
	double p; ///< Pressure, in ATM (used for refraction calculation)
	double T; ///< Temperature, in °C (used for refraction calculation)
} SunPos;

// functions

/** Calculate time given the input date fields and store it in the SunPos object.
	@param UT fractional universal time (GMT) in hours from midnight (or fractional hours as required)
	@param Day Day of the month, starting at 1??
	@param Month Month of the year, starting at 1??
	@param Year Year, eg 2011.
	@param Delta_t Difference between UT and Terrestrial Time, in seconds.
*/
void SunPos_calc_time(SunPos *S, double UT, int Day, int Month, int Year, double Delta_t);

/**	Set time directly in days since noon 1 Jan 2003 UTC. */
void SunPos_set_time(SunPos *S, double t);

/** Set location of observer on Earth
	@param latitude latitude in RADIANS!
	@param longitude longitude in RADIANS!
*/
void SunPos_set_lat_long(SunPos *S, double latitude, double longitude);

/** Set local atmospheric conditions 
	@param p Pressure in ATM
	@param T Temperature in °C
*/
void SunPos_set_pressure_temp(SunPos *S, double p, double T)

/**
	Calculate the sun position in local spherical coordinates.
	@param S sun position input data object (set using above functions)
	@param zenith zenith angle in radians (output)
	@param azimuth azimuth angle in radians (output)
*/
void SunPos_calc_zen_azi(SunPos *S, double *zenith, double *azimuth);


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
*//** @FILE
	This file's code is based on the source code given in the publication
	R Grena (2008), An algorithm for the computation of the solar position, 
	Solar Energy (82), pp 462-470.

	The original code was in C++ and returned several intermediate results.
	This modified version returns only the zenith and azimuth angles for 
	given date/time.
*/

#include "sunpos_grena.h"

/* we have converted this file from C++ to C. not so classy any more ;-) */

void SunPos_calc_time(SunPos *S, double UT, int Day, int Month, int Year, double Delta_t){

	// calculation of JD and JDE
	double dYear, dMonth;
	if(Month <= 2){
		dYear = (double)Year - 1.;
		dMonth = (double)Month + 12.;
	}else{
		dYear = (double)Year;
		dMonth = (double)Month;
	}

	// WARNING: my testing shows that following equation gives correct values 
	// only in the range 2001 to 2099. By not correcting for leap years on 100
	// and 1000 year multiples, it gradually gets out of accord. -- J Pye. 

	// universal time
	S->t_G = (double)trunc(365.25 * (dYear - 2000))
		+ (double)trunc(30.6001 * (dMonth + 1))
		+ (double)Day + UT/24. - 1158.5;

	S->Delta_t = Delta_t;
}


void SunPos_set_time(SunPos *S, double t_G, double Delta_t){
	S->t_G = t_G;
	S->Delta_t = Delta_t;
}


void SunPos_set_lat_long(SunPos *S, double latitude, double longitude){
	S->latitude = latitude;
	S->longitude = longitude;
}


void SunPos_set_press_temp(SunPos *S, double p, double T){
	S->p = p;
	S->T = T;
}

/*
	Note: there is some confusion in nomenclature in the paper regarding
	'JD_t' and 't_G' which are assumed to be the same time. This seems to John
	to be correct.
*/
void SunPos_calc_zen_azi(SunPos *S, double *zenith, double *azimuth){
	double HourAngle;
	double TopocRightAscension;
	double TopocDeclination;
	double TopocHourAngle;
	double Elevation_no_refrac;
	double RefractionCorrection;

	// HELIOCENTRIC LONGITUDE

	double t = S->t_G + S->Delta_t / 86400;

	// linear increase + annual harmonic

	double ang = 1.72019e-2 * t - 0.0563;
	double HeliocLongitude = 1.740940 + 1.7202768683e-2 * t + 3.34118e-2 * sin(ang) + 3.488e-4 * sin(2*ang);

	// moon perturbation

	HeliocLongitude += 3.13e-5 * sin(2.127730e-1*t - 0.585);

	// harmonic correction

	HeliocLongitude += 1.26e-5 * sin(4.243e-3 * t + 1.46)	
		+ 2.35e-5 * sin(1.0727e-2 * t + 0.72)
		+ 2.76e-5 * sin(1.5799e-2 * t + 2.35)
		+ 2.75e-5 * sin(2.1551e-2 * t - 1.98)
		+ 1.26e-5 * sin(3.1490e-2 * t - 0.80);

	// polynomial correction

	double t2 = t/1000;
	HeliocLongitude += ((( -2.30796e-7 * t2 + 3.7976e-6) * t2 - 2.0458e-5) * t
		+ 3.976e-5) * t2*t2;

	// to obtain obtain Heliocentric longitude in the range [0,2pi] uncomment:
	// HeliocLongitude = fmod(HeliocLongitude, 2*PI);

	// END HELIOCENTRIC LONGITUDE CALCULATION

	// Correction to longitude due to nutation

	double delta_psi = 8.33e-5 * sin(9.252e-4 * t - 1.173);

	// Earth axis inclination

	double epsilon = -6.21e-9 * t + 0.409086 + 4.46e-5 * sin(9.252e-4 * t + 0.397);

	// Geocentric global solar coordinates:

	double GeocSolarLongitude = HeliocLongitude + PI + delta_psi - 9.932e-5;

	double s_lambda = sin(GeocSolarLongitude);

	double RightAscension = atan2(s_lambda * cos(epsilon), cos(GeocSolarLongitude));

	double Declination = asin(sin(epsilon) * s_lambda);

	// local hour angle of the sun

	HourAngle = 6.30038809903 * S->t_G + 4.8824623 + delta_psi * 0.9174 
		+ S->longitude - RightAscension;

	// to obtain the local hour angle in the range [0,2pi] uncomment:
	//HourAngle = fmod(HourAngle,2*PI);

	double c_lat = cos(S->latitude);
	double s_lat = sin(S->latitude);
	double c_H = cos(HourAngle);
	double s_H = sin(HourAngle);

	// parallax correction to right ascension

	double d_alpha = -4.26e-5 * c_lat * s_H;
	TopocRightAscension = RightAscension + d_alpha;
	TopocHourAngle = HourAngle - d_alpha;

	// parallax correction to declination:

	TopocDeclination = Declination - 4.26e-5 * (s_lat - Declination * c_lat);
	double s_delta_corr = sin(TopocDeclination);
	double c_delta_corr = cos(TopocDeclination);
	double c_H_corr = c_H + d_alpha * s_H;
	double s_H_corr = s_H - d_alpha * c_H;

	// solar elevation angle, without refraction correction
	Elevation_no_refrac =  asin(s_lat * s_delta_corr + c_lat * c_delta_corr * c_H_corr);

	// refraction correction: it is calculated only
	// if Elevation_no_refract > elev_min

	const double elev_min = -0.01;
	
	if(Elevation_no_refrac > elev_min){
		RefractionCorrection = 0.084217 * S->p / (273 + S->T) 
			/ tan(Elevation_no_refrac + 0.0031376 / (Elevation_no_refrac + 0.089186));
	}else{
		RefractionCorrection = 0;
	}

	// local coordinates of the sun

	*zenith = PI/2 - Elevation_no_refrac - RefractionCorrection;

	*azimuth = atan2(s_H_corr, c_H_corr*s_lat - s_delta_corr/c_delta_corr*c_lat);
}


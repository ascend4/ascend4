/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

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
	Implementation of the Modified Benedict-Webb-Rubin (MBWR) equation of state.

John Pye, 29 Jul 2008.
*/

#include "mbwr.h"

#include <math.h>

/**
	Function to calculate pressure from MBWR correlation, given temperature
	and molar density.

	@param T temperature in K
	@param rhob molar density in mol/L
	@return pressure in Pa
*/
double mbwr_p(double T, double rhob, MbwrData *data){
	int i;
	double p = 0;
	double Ti, Ti2, Ti3, Ti4;
	double rhob2, rhobpow, sum10, rhob_r;
	double alpha[15];

	/* precalculate powers of T^-1 for faster evaluation */
	Ti =1. / T;
	Ti2 = Ti*Ti;
	Ti3 = Ti2*Ti;
	Ti4 = Ti3*Ti;

	/* values of alpha in MBWR are functions of temperature */
	double *B = data->beta;
#define R data->R
	alpha[0] = R * T;
	alpha[1] = B[0]*T + B[1]*sqrt(T) + B[2] + B[3]*Ti + B[4]*Ti2;
	alpha[2] = B[5]*T + B[6] + B[7]*Ti + B[8]*Ti2;
	alpha[3] = B[9]*T + B[10] + B[11]*Ti;
	alpha[4] = B[12];
	alpha[5] = B[13]*Ti + B[14]*Ti2;
	alpha[6] = B[15]*Ti;
	alpha[7] = B[16]*Ti + B[17]*Ti2;
	alpha[8] = B[18]*Ti2;
	alpha[9] = B[19]*Ti2 + B[20]*Ti3;
	alpha[10] = B[21]*Ti2 + B[22]*Ti4;
	alpha[11] = B[23]*Ti2 + B[24]*Ti3;
	alpha[12] = B[25]*Ti2 + B[26]*Ti4;
	alpha[13] = B[27]*Ti2 + B[28]*Ti3;
	alpha[14] = B[29]*Ti2 + B[30]*Ti3 + B[31]*Ti4;
#undef R

	/* add up the first sum in the MBWR correlation */
	rhobpow = 1;
	for(i=0;i<9;++i){
		rhobpow *= rhob;
		p += alpha[i]* rhobpow;
	}

	/* work out the second sum in the MBWR correlation */
	sum10 = 0;
	rhobpow = rhob;
	rhob2 = rhob*rhob;
	for(i=9; i<15; ++i){
		rhobpow *= rhob2;
		sum10 += alpha[i] * rhobpow;
	}

	/* calculate the exponential term and add it to 'p' */
#define RHO_C data->rhob_c
	rhob_r = rhob/RHO_C;
	p += exp(rhob_r*rhob_r) * sum10;
#undef RHO_C

	return p * 1e-5; /* convert bar to Pa on return */
}



/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

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

#ifndef FPROPS_HELMHOLTZ_H
#define FPROPS_HELMHOLTZ_H

#include "ideal.h"

/**
	Data structure for rows of the coefficient and exponent table (allows
	the data to be represented more concisely when declaring a fluid from
	C code.
*/
typedef struct HelmholtzPowTerm_struct{
	double a; /* coefficient */
	double t; /* exponent of tau */
	int d; /* exponent of delta */
	unsigned l; /* exponent X in exp(-del^X) */
} HelmholtzPowTerm;

/**
	Data structure for 'exponential terms' in the residual expression.
	These terms are of the form used in Span et al, 1998, as cited in
	the file 'nitrogen.c'.
*/
typedef struct HelmholtzExpTerm_struct{
	double a; /* coefficient */
	double t; /* exponent of tau */
	int d; /* exponent of delta */
	int phi;
	int beta;
	double gamma;
} HelmholtzExpTerm;

/*
	Data structure for Gaussian terms in the residual expression, for
	improvement of correlation in the critical region. These terms are of the
	form as utilised in the correlations for water (see water.c) and hydrogen
	(see hydrogen.c). According to Leachman, these terms are due to Setzmann
	and Wagner (J Phys Chem Ref Data, 1966).

	Using the nomenclature of IAPWS-95 (see water.c), terms here for the reduced
	helmholtz energy are:

		n * del^d * tau^t * exp[-alpha*(delta-epsilon)^2 - beta*(tau-gamma)^2]

	NOTE the minus signs preceeding 'alpha' and 'beta' and note that this is
	in conflict with the sign convention of Leachman, who assumes a plus sign
	in front of the corresponding parameters in his equation.
*/
typedef struct HelmholtzGausTerm_struct{
	double n; /**< coefficient */
	double t; /**< power of tau */
	double d; /**< power of delta */
	double alpha,beta,gamma,epsilon;
} HelmholtzGausTerm;

/*
	Data structure for 'critical terms' in the residual expression. These
	terms are of the form described in the IAPWS-95 document, as cited in 
	the file 'water.c'.

	This structure is for the second kind, with A, B, C, D.
*/
typedef struct HelmholtzCritTerm_struct{
	double n; /**< coefficient */
	double a,b,beta,A,B,C,D;
} HelmholtzCritTerm;

/**
	Data structure for fluid-specific data for the Helmholtz free energy EOS.
	See Tillner-Roth 1993 for information about 'atd' and 'a0' data.
*/
typedef struct HelmholtzData_struct{
	double R; /**< specific gas constant */
	double M; /**< molar mass, kg/kmol */
	double rho_star; /**< normalisation density, kg/m³ */
	double T_star; /* normalisation temperature, K */
	
	const IdealData *ideal; /* data for ideal component of Helmholtz energy */

	unsigned np; /* number of power terms in residual equation */
	const HelmholtzPowTerm *pt; /* power term data for residual eqn, maybe NULL if np == 0 */
	unsigned ne; /* number of exponential terms (a la Span et al 1998) for residual eqn */
	const HelmholtzExpTerm *et; /* exponential term data, maybe NULL if ne == 0 */

	unsigned ng; /* number of critical terms of the first kind */
	const HelmholtzGausTerm *gt; /* critical terms of the first kind */
	unsigned nc; /* number of critical terms of the second kind */
	const HelmholtzCritTerm *ct; /* critical terms of the second kind */
} HelmholtzData;

double helmholtz_p(double T, double rho, const HelmholtzData *data);
double helmholtz_u(double T, double rho, const HelmholtzData *data);
double helmholtz_h(double T, double rho, const HelmholtzData *data);
double helmholtz_s(double T, double rho, const HelmholtzData *data);
double helmholtz_a(double T, double rho, const HelmholtzData *data);
double helmholtz_cp0(double T, const HelmholtzData *data);

#endif


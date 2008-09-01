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
#ifndef FPROPS_IDEAL_H
#define FPROPS_IDEAL_H

/* 
	Data types for declaration of ideal fluid components of property
	correlations. Frequently property correlations for 'real' fluids are
	given in terms of ideal gas components plus 'residual' or 'real'
	components. For these cases, this file defines the data structures
	used to declare those ideal component curves.
*/

/*
	Terms containing powers of 'tau', like
	
	a0 * tau^t0
*/
typedef struct IdealPowTerm_struct{
	double a0;
	double t0;
} IdealPowTerm;

/*
	Terms containing exponential expressions with tau, like

	b x^2 exp(-x) / [1-exp(-x)]^2

	where x = beta / T. Instead of representing in terms of beta, we ask the
	user to provide (b,beta) in the form of

		b,B

	where B = beta / T*, with T* being the normalisation temperature used
	to calculate tau in the main 'residual' correlation.

	See J R Cooper 'Representation of the Ideal-Gas Thermaldynamic
	Properties of Water', Int J Thermophys v 3 no 1, 1982 and also
	Span, Lemmon, Jacobsen & Wagner 'A Reference Quality Equation of State 
	for Nitrogen' 1998.

	From the above expression, the term appearing in the reduced
	Helmholtz function equation becomes

		b_i ln [ 1 - exp( -B_i tau ) ]

	This is the component in tau, there is also a constant term added,

		- b_i ln [ 1 - exp( -B_i tau0 ) ]

*/
typedef struct IdealExpTerm_struct{
	double b;
	double B;
} IdealExpTerm;

typedef struct 	IdealData_struct{
	double c;
	double m;
	unsigned np; /* number of power terms */
	const IdealPowTerm *pt; /* power term data, may be NULL if np == 0 */
	unsigned ne; /* number of 'exponential' terms */
	const IdealExpTerm *et; /* exponential term data, maybe NULL if ne == 0 */
} IdealData;

#endif


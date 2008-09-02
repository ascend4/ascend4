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
	When cp0(T) values are not provided, eg in Tillner-Roth (ammonia.c) we
	are given the terms of the ideal component of the Helmholtz function.
	This macro converts power terms from the Helmholtz function back into
	terms for the cp0 function. The Helmholtz terms are assumed to be
	of the form

		A0_I * tau^P
*/
#define IDEALPOWTERM_FROM_HELM0(A0_I, P) {-(A0_I)*(-(P))*(-(P)+1), -(P)}


/*
	Terms containing exponential expressions with tau, like

	b x^2 exp(-x) / [1-exp(-x)]^2

	where x = beta / T. Instead of representing in terms of beta, we ask the
	user to provide (b,beta) in the form of

		b,beta

	We also need to know T* where tau = T* / T, the normalisation temperature
	used to calculate tau in the main 'residual' correlation.

	See J R Cooper 'Representation of the Ideal-Gas Thermaldynamic
	Properties of Water', Int J Thermophys v 3 no 1, 1982 and also
	Span, Lemmon, Jacobsen & Wagner 'A Reference Quality Equation of State 
	for Nitrogen' 1998.

	From the above expression, the term appearing in the reduced
	Helmholtz function equation becomes

		b_i ln [ 1 - exp( -beta*tau/Tstar ) ]

	As well as this term, there is a constant term and a term linear in tau
	that we can ignore because the values of those constants can be determined
	from fixing the value of h0 and s0.

*/
typedef struct IdealExpTerm_struct{
	double b;
	double beta;
} IdealExpTerm;

typedef struct 	IdealData_struct{
	double c;
	double m;
	double Tstar; /* normalisation temperature used in residual correlation */
	double cp0star; /* reducing parameter used for cp0 */
	unsigned np; /* number of power terms */
	const IdealPowTerm *pt; /* power term data, may be NULL if np == 0 */
	unsigned ne; /* number of 'exponential' terms */
	const IdealExpTerm *et; /* exponential term data, maybe NULL if ne == 0 */
} IdealData;

#endif


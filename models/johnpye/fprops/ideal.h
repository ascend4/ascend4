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
	See J R Cooper 'Representation of the Ideal-Gas Thermaldynamic
	Properties of Water', Int J Thermophys v 3 no 1, 1982 and also
	Span, Lemmon, Jacobsen & Wagner 'A Reference Quality Equation of State 
	for Nitrogen' 1998.

	The form of exponential terms appearing in the reduced
	Helmholtz function equation is

		b ln [ 1 - exp( -B tau ) ]

	where B is defined as beta / tau in the nomenclature of Cooper.
*/
typedef struct IdealExpTerm_struct{
	double b;
	double B;
} IdealExpTerm;

typedef struct 	IdealData_struct{
	double c; /* constant value in phi_0 expression */
	double m; /* linear coefficient in phi_0 expression */
	unsigned np; /* number of power terms */
	const IdealPowTerm *pt; /* power term data, may be NULL if np == 0 */
	unsigned ne; /* number of 'exponential' terms */
	const IdealExpTerm *et; /* exponential term data, maybe NULL if ne == 0 */
} IdealData;

#endif


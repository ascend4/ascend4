#ifndef FPROPS_HELMHOLTZ_H
#define FPROPS_HELMHOLTZ_H

/*
	Terms containing powers of 'tau'
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
} HelmholtzData;

double helmholtz_p(double T, double rho, const HelmholtzData *data);
double helmholtz_u(double T, double rho, const HelmholtzData *data);
double helmholtz_h(double T, double rho, const HelmholtzData *data);
double helmholtz_s(double T, double rho, const HelmholtzData *data);

#endif


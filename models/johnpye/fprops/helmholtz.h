#ifndef FPROPS_HELMHOLTZ_H
#define FPROPS_HELMHOLTZ_H

/**
	Data structure for rows of the coefficient and exponent table (allows
	the data to be represented more concisely when declaring a fluid from
	C code.
*/
typedef struct HelmholtzATDL_struct{
	double a; /* coefficient */
	double t; /* exponent of tau */
	int d; /* exponent of delta */
	unsigned l; /* exponent X in exp(-del^X) */
} HelmholtzATDL;

/**
	Data structure for fluid-specific data for the Helmholtz free energy EOS.
	See Tillner-Roth 1993 for information about 'atd' and 'a0' data.
*/
typedef struct HelmholtzData_struct{
	double R; /**< specific gas constant */
	double M; /**< molar mass, kg/kmol */
	double rho_star; /**< normalisation density, kg/m³ */
	double T_star; /* normalisation temperature, K */
	
	unsigned ni; /* number of coefficients in ideal equation */
	const double *a0; /* coefficients for the ideal component of the fund eqn */

	unsigned nr; /* number of coefficients in residual equation */
	const HelmholtzATDL *atdl; /* coefficients and exponents for residual component of fund eqn */
} HelmholtzData;

double helmholtz_p(double T, double rho, const HelmholtzData *data);
double helmholtz_u(double T, double rho, const HelmholtzData *data);
double helmholtz_h(double T, double rho, const HelmholtzData *data);
double helmholtz_s(double T, double rho, const HelmholtzData *data);

#endif


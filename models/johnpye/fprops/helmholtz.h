
/**
	Data structure for rows of the coefficient and exponent table (allows
	the data to be represented more concisely when declaring a fluid from
	C code.
*/
typedef struct HelmholtzATD_struct{
	double a; /* coefficient */
	double t; /* exponent of tau */
	int d; /* exponent of delta */
} HelmholtzATD;

/**
	Data structure for fluid-specific data for the Helmholtz free energy EOS.
*/
typedef struct HelmholtzData_struct{
	double R; /* specific gas constant */
	double rho_star; /* normalisation density */
	double T_star; /* normalisation temperature */
	
	double a0[6]; /* coefficients for the ideal component of the fund eqn */

	HelmholtzATD atd[21];
} HelmholtzData;

double helmholtz_p(double T, double rho, const HelmholtzData *data);
double helmholtz_u(double T, double rho, const HelmholtzData *data);
double helmholtz_h(double T, double rho, const HelmholtzData *data);
double helmholtz_s(double T, double rho, const HelmholtzData *data);

const HelmholtzData helmholtz_data_ammonia;

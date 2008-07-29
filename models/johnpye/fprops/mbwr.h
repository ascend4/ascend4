/**
	Data structure for fluid-specific data for the MBWR EOS.
*/
typedef struct MbwrData_struct{
	double R; /**< ??? we should put this somewhere else */
	double rhob_c; /**< critical molar density in mol/L */
	double beta[32]; /**< constants in MBWR for the fluid in question */
} MbwrData;

double mbwr_p(double T, double rhob, MbwrData *data);


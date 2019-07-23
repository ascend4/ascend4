#include <fprops.h>
#include <sat.h>
#include <fluids.h>
#include <solve_ph.h>

//#define USE_MODELICA_ERROR
#ifdef USE_MODELICA_ERROR
# include <omc/c/ModelicaUtilities.h>
#else
# include <stdio.h>
#endif

// probably we can find a way to pass the fluid name as a parameter...?
double Tsat_p_Na(double p);
double T_ph_Na(double p,double h);

double Tsat_p_Na(double p){
	FpropsError err = FPROPS_NO_ERROR;
	ReferenceState R = {FPROPS_REF_IIR};
	
	const PureFluid *D = fprops_fluid("carbondioxide","helmholtz",NULL);

	double T_sat, rho_f, rho_g;
	fprops_sat_p(p, &T_sat, &rho_f, &rho_g, D, &err);
	if(err){
		// fprops_destroy(...)
#ifdef USE_MODELICA_ERROR		
		ModelicaError(fprops_error(err)); // does not return
#else
		fprintf(stderr,"ERROR: %s",fprops_error(err));
#endif
	}

	return T_sat;
}

double T_ph_Na(double p, double h){
	FpropsError err = FPROPS_NO_ERROR;
	ReferenceState R = {FPROPS_REF_IIR};
	
	const PureFluid *D = fprops_fluid("carbondioxide","helmholtz",NULL);

	double T, rho;
	fprops_solve_ph(p,h, &T, &rho, 0, D, &err);

	if(err){
		// fprops_destroy(...)
#ifdef USE_MODELICA_ERROR		
		ModelicaError(fprops_error(err)); // does not return
#else
		fprintf(stderr,"ERROR: %s",fprops_error(err));
#endif
	}

	return T;
}





#ifdef TEST
#include <stdio.h>
int main(void){
	double p = 1e5;
	double h = 500e3;
	fprintf(stderr,"Tsat(%f bar) = %f °C\n", p/1e5, Tsat_p_Na(p)-273.15);
	fprintf(stderr,"T(p=%f bar,h=%f kJ/kg) = %f °C\n", p/1e5, h/1e3, T_ph_Na(p,h)-273.15);
	return 0;
}
#endif


#include <fprops.h>
#include <sat.h>
#include <fluids.h>

#ifdef USE_MODELICA_ERROR
# include <omc/c/ModelicaUtilities.h>
#else
# include <stdio.h>
#endif

// probably we can find a way to pass the fluid name as a parameter...?
double Tsat_p_Na(double p);

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

#ifdef TEST
#include <stdio.h>
int main(void){
	double p = 1e5;
	fprintf(stderr,"Tsat(%f bar) = %f °C\n", p/1e5, Tsat_p_Na(p));
	return 0;
}
#endif


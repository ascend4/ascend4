#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../eqm_phase.h"

/* Evaluate the spinel phase-entry residual for supplied element potentials.
   Usage:
     eqm_spinel_entry_runner <spinel-source> <T[K]> <P[Pa]> <lambda_Fe[J/mol]> <lambda_O[J/mol]>
*/

int main(int argc, char *argv[]){
	const char *source;
	double T;
	double P;
	double lambda_fe;
	double lambda_o;
	double lambda[FPROPS_EQM_PHASE_MAX_ELEMS];
	double y[FPROPS_EQM_PHASE_MAX_VARS];
	double phi = NAN;
	FpropsEqmPhaseModel phase;
	int i;
	int i_fe;
	int i_o;

	if(argc != 6){
		fprintf(stderr, "USAGE: %s <spinel-source> <T[K]> <P[Pa]> <lambda_Fe[J/mol]> <lambda_O[J/mol]>\n", argv[0]);
		return 2;
	}
	source = argv[1];
	T = atof(argv[2]);
	P = atof(argv[3]);
	lambda_fe = atof(argv[4]);
	lambda_o = atof(argv[5]);
	if(!(T > 0.0) || !(P > 0.0) || !isfinite(lambda_fe) || !isfinite(lambda_o)){
		fprintf(stderr, "Invalid input\n");
		return 2;
	}
	if(!fprops_eqm_phase_resolve("spinel", source, &phase)){
		fprintf(stderr, "Could not resolve spinel source '%s'\n", source);
		return 1;
	}
	for(i = 0; i < FPROPS_EQM_PHASE_MAX_ELEMS; ++i){
		lambda[i] = 0.0;
	}
	for(i = 0; i < FPROPS_EQM_PHASE_MAX_VARS; ++i){
		y[i] = NAN;
	}
	i_fe = fprops_eqm_phase_find_element(&phase, "Fe");
	i_o = fprops_eqm_phase_find_element(&phase, "O");
	if(i_fe < 0 || i_o < 0){
		fprintf(stderr, "Spinel phase lacks Fe/O element indices\n");
		return 1;
	}
	lambda[i_fe] = lambda_fe;
	lambda[i_o] = lambda_o;
	if(!fprops_eqm_phase_entry_residual(&phase, T, P, lambda, &phi, y)){
		fprintf(stderr, "Entry residual evaluation failed\n");
		return 1;
	}
	printf("{\"source\":\"%s\",\"T\":%.17g,\"P\":%.17g,\"lambda_Fe\":%.17g,\"lambda_O\":%.17g,\"phi\":%.17g,\"y\":[", source, T, P, lambda_fe, lambda_o, phi);
	for(i = 0; i < phase.nvar; ++i){
		if(i > 0) printf(",");
		if(isfinite(y[i])){
			printf("%.17g", y[i]);
		}else{
			printf("null");
		}
	}
	printf("]}\n");
	return isfinite(phi) ? 0 : 1;
}

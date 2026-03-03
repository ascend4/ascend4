#include <stdio.h>
#include <stdlib.h>

#include "../eqm.h"

/* How to run:
   1) Build:
        scons models/johnpye/fprops/test/eqm_mu0_runner -j4
   2) Evaluate mixed-provider mu0(T,P0) for selected species:
        ./models/johnpye/fprops/test/eqm_mu0_runner "Moran and Shapiro" 1000 100000 \
          hydrogen oxygen water carbonmonoxide carbondioxide Fe FeO Fe2O3 Fe3O4
*/

int main(int argc, char *argv[]){
	const char *source;
	double T;
	double P0;
	int i;
	int nfail = 0;

	if(argc < 5){
		fprintf(stderr, "USAGE: %s <source> <T[K]> <P0[Pa]> <species...>\n", argv[0]);
		return 2;
	}
	source = argv[1];
	T = atof(argv[2]);
	P0 = atof(argv[3]);
	if(!(T > 0.0) || !(P0 > 0.0)){
		fprintf(stderr, "Invalid T/P0\n");
		return 2;
	}

	printf("{\"source\":\"%s\",\"T\":%.17g,\"P0\":%.17g", source, T, P0);
	printf(",\"species\":[");
	for(i = 4; i < argc; ++i){
		if(i > 4) printf(",");
		printf("\"%s\"", argv[i]);
	}
	printf("],\"mu0\":[");
	for(i = 4; i < argc; ++i){
		double mu0 = 0.0;
		int ok = eqm_mu0_source(argv[i], source, T, P0, &mu0);
		if(i > 4) printf(",");
		if(ok){
			printf("%.17g", mu0);
		}else{
			printf("null");
			++nfail;
		}
	}
	printf("]}\n");
	return nfail ? 1 : 0;
}

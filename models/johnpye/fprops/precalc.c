#include "ideal.h"
#include <math.h>
#include <stdio.h>

extern IdealPhi0Data precalc_data;

int main(void){
	fprintf(stdout,"\t/* cp0 POWER TERMS AUTOMATICALLY PRE-CALCULATED FROM phi0 */\n");
	fprintf(stdout,"\t, %d\n",precalc_data.np);
	fprintf(stdout,"\t, (const IdealPowTerm[]){\n");
	unsigned i;
	for(i=0; i<precalc_data.np; ++i){
		double a = precalc_data.pt[i].a0;
		double t = precalc_data.pt[i].t0;
		double c = - a * t * (t-1.) * pow(precalc_data.T_star, t);
		double p = -t;
		
		fprintf(stdout,"\t\t%s{%0.15e, %0.15e}\n",(i>0?", ":""), c, p);
	}
	fprintf(stdout,"\t}\n");
	fprintf(stdout,"\t, 0, (const IdealExpTerm *)0 /* no exponential terms */\n");
	fprintf(stderr,"\t/* END OF PRE-CALCULATED VALUES */\n");
}

	

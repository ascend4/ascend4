#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../eqm.h"

/* How to run:
   1) Build:
        scons models/johnpye/fprops/test/eqm_case_runner -j4
   2) List available cases:
        ./models/johnpye/fprops/test/eqm_case_runner list 300 100000
   3) Run one case (JSON output):
        ./models/johnpye/fprops/test/eqm_case_runner wgs 1000 101325 \
          auto_nullspace "Moran and Shapiro"
*/

#define MAX_NS 8
#define MAX_NE 4

typedef struct EqmCase{
	const char *name;
	int ns;
	const char *species[MAX_NS];
	int ne;
	const char *elements[MAX_NE];
	double b[MAX_NE];
	double nu[MAX_NS];
} EqmCase;

static const EqmCase CASES[] = {
	{
		"h2o_dissociation",
		3,
		{"hydrogen", "oxygen", "water"},
		2,
		{"H", "O"},
		{2.0, 1.0},
		{1.0, 0.5, -1.0}
	},
	{
		"co2_dissociation",
		3,
		{"carbonmonoxide", "oxygen", "carbondioxide"},
		2,
		{"C", "O"},
		{1.0, 2.0},
		{1.0, 0.5, -1.0}
	},
	{
		"wgs",
		4,
		{"carbonmonoxide", "water", "carbondioxide", "hydrogen"},
		3,
		{"C", "O", "H"},
		{1.0, 2.0, 2.0},
		{1.0, 1.0, -1.0, -1.0}
	},
	{
		"wgs_inert_n2",
		5,
		{"carbonmonoxide", "water", "carbondioxide", "hydrogen", "nitrogen"},
		4,
		{"C", "O", "H", "N"},
		{1.0, 2.0, 2.0, 2.0},
		{1.0, 1.0, -1.0, -1.0, 0.0}
	},
	{
		"co_co2_h2o_h2_o2",
		5,
		{"carbonmonoxide", "carbondioxide", "water", "hydrogen", "oxygen"},
		3,
		{"C", "O", "H"},
		{1.0, 2.0, 2.0},
		{1.0, -1.0, 1.0, -1.0, 0.0} /* report WGS log10K for this mixed system */
	}
};

static const int NCASES = (int)(sizeof(CASES) / sizeof(CASES[0]));
static const double P0 = 1e5;

static int eqm_status_ok(int status){
	return status == 0 || status == 1 || status == 6;
}

static const EqmCase *find_case(const char *name){
	int i;
	for(i = 0; i < NCASES; ++i){
		if(0 == strcmp(name, CASES[i].name)){
			return &CASES[i];
		}
	}
	return NULL;
}

static void print_case_list(void){
	int i;
	for(i = 0; i < NCASES; ++i){
		printf("%s\n", CASES[i].name);
	}
}

static double log10K_from_n(const double *n, const double *nu, int ns, double P){
	double ntot = 0.0;
	double sum = 0.0;
	int i;
	for(i = 0; i < ns; ++i){
		if(!(n[i] > 0.0)){
			return NAN;
		}
		ntot += n[i];
	}
	if(!(ntot > 0.0)){
		return NAN;
	}
	for(i = 0; i < ns; ++i){
		double yi = n[i] / ntot;
		double a = yi * P / P0;
		if(!(a > 0.0)){
			return NAN;
		}
		sum += nu[i] * log10(a);
	}
	return sum;
}

static void print_json_result(const EqmCase *C, double T, double P, const char *source,
		const char *algorithm, int status, const double *n){
	int i;
	printf("{");
	printf("\"case\":\"%s\"", C->name);
	printf(",\"source\":\"%s\"", source);
	printf(",\"algorithm\":\"%s\"", algorithm);
	printf(",\"T\":%.17g", T);
	printf(",\"P\":%.17g", P);
	printf(",\"status\":%d", status);
	printf(",\"species\":[");
	for(i = 0; i < C->ns; ++i){
		if(i) printf(",");
		printf("\"%s\"", C->species[i]);
	}
	printf("]");
	if(eqm_status_ok(status)){
		double log10K = log10K_from_n(n, C->nu, C->ns, P);
		double ntot = 0.0;
		printf(",\"n\":[");
		for(i = 0; i < C->ns; ++i){
			if(i) printf(",");
			printf("%.17g", n[i]);
			ntot += n[i];
		}
		printf("]");
		printf(",\"y\":[");
		for(i = 0; i < C->ns; ++i){
			if(i) printf(",");
			printf("%.17g", n[i] / ntot);
		}
		printf("]");
		if(isfinite(log10K)){
			printf(",\"log10K\":%.17g", log10K);
		}else{
			printf(",\"log10K\":null");
		}
	}else{
		printf(",\"n\":null");
		printf(",\"y\":null");
		printf(",\"log10K\":null");
	}
	printf("}\n");
}

int main(int argc, char *argv[]){
	const EqmCase *C;
	const char *algorithm = "auto_nullspace";
	const char *source = "Moran and Shapiro";
	const char *names[MAX_NS] = {0};
	const char *elements[MAX_NE] = {0};
	double T;
	double P;
	double n[MAX_NS] = {0.0};
	int status;
	int i;

	if(argc < 4){
		fprintf(stderr, "USAGE: %s <case|list> <T[K]> <P[Pa]> [algorithm] [source]\n", argv[0]);
		return 2;
	}
	if(0 == strcmp(argv[1], "list")){
		print_case_list();
		return 0;
	}
	T = atof(argv[2]);
	P = atof(argv[3]);
	if(argc >= 5){
		algorithm = argv[4];
	}
	if(argc >= 6){
		source = argv[5];
	}
	C = find_case(argv[1]);
	if(!C){
		fprintf(stderr, "Unknown case '%s'\n", argv[1]);
		return 2;
	}
	if(!(T > 0.0) || !(P > 0.0)){
		fprintf(stderr, "Invalid T/P\n");
		return 2;
	}
	for(i = 0; i < C->ns; ++i){
		names[i] = C->species[i];
	}
	for(i = 0; i < C->ne; ++i){
		elements[i] = C->elements[i];
	}

	status = eqm_solve_elements((const char **)names, C->ns, (const char **)elements, C->ne, C->b,
		source, T, P, algorithm, NULL, n);
	print_json_result(C, T, P, source, algorithm, status, n);
	return 0;
}

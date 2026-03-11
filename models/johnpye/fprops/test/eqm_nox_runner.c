#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../eqm.h"

#define MAX_NS 9

typedef struct EqmCase{
	const char *name;
	int ns;
	const char *species[MAX_NS];
	double n0[MAX_NS];
} EqmCase;

static const EqmCase CASES[] = {
	{
		"no_air",
		4,
		{"nitrogen", "oxygen", "argon", "nitric_oxide"},
		{0.78084, 0.20946, 0.00970, 0.0}
	},
	{
		"no2_air",
		5,
		{"nitrogen", "oxygen", "argon", "nitric_oxide", "nitrogen_dioxide"},
		{0.78084, 0.20946, 0.00970, 0.0, 0.0}
	},
	{
		"humid_air_no",
		8,
		{"nitrogen", "oxygen", "argon", "water", "carbondioxide",
			"nitric_oxide", "carbonmonoxide", "hydrogen"},
		{
			0.78050661145600002,
			0.20937052904000001,
			0.0096958595039999996,
			0.015,
			0.00043000000000000002,
			0.0,
			0.0,
			0.0
		}
	},
	{
		"humid_air_no2",
		9,
		{"nitrogen", "oxygen", "argon", "water", "carbondioxide",
			"nitric_oxide", "nitrogen_dioxide", "carbonmonoxide", "hydrogen"},
		{
			0.78050661145600002,
			0.20937052904000001,
			0.0096958595039999996,
			0.015,
			0.00043000000000000002,
			0.0,
			0.0,
			0.0,
			0.0
		}
	},
	{
		"humid_air_nox",
		9,
		{"nitrogen", "oxygen", "argon", "water", "carbondioxide",
			"nitric_oxide", "nitrogen_dioxide", "carbonmonoxide", "hydrogen"},
		{
			0.78050661145600002,
			0.20937052904000001,
			0.0096958595039999996,
			0.015,
			0.00043000000000000002,
			0.0,
			0.0,
			0.0,
			0.0
		}
	},
	{
		"co2_h2o_trace_air",
		7,
		{"nitrogen", "oxygen", "argon", "water", "carbondioxide", "carbonmonoxide", "hydrogen"},
		{
			0.78050661145600002,
			0.20937052904000001,
			0.0096958595039999996,
			0.015,
			0.00043000000000000002,
			0.0,
			0.0
		}
	}
};

static const int NCASES = (int)(sizeof(CASES) / sizeof(CASES[0]));
static const double P0 = 1e5;

static double extract_y(const EqmCase *C, const double *n, const char *name){
	double ntot = 0.0;
	int i;
	for(i = 0; i < C->ns; ++i){
		if(!(n[i] > 0.0)){
			return NAN;
		}
		ntot += n[i];
	}
	if(!(ntot > 0.0)){
		return NAN;
	}
	for(i = 0; i < C->ns; ++i){
		if(0 == strcmp(C->species[i], name)){
			return n[i] / ntot;
		}
	}
	return 0.0;
}

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

static double log10Q_simple(const char * const *species, const double *n, int ns, double P,
		const char *num1, double nu1, const char *num2, double nu2, const char *den){
	double ntot = 0.0;
	double y_num1 = -1.0;
	double y_num2 = -1.0;
	double y_den = -1.0;
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
		if(0 == strcmp(species[i], num1)){
			y_num1 = yi;
		}else if(num2 && 0 == strcmp(species[i], num2)){
			y_num2 = yi;
		}else if(0 == strcmp(species[i], den)){
			y_den = yi;
		}
	}
	if(!(y_num1 > 0.0) || !(y_den > 0.0)){
		return NAN;
	}
	if(num2 && !(y_num2 > 0.0)){
		return NAN;
	}
	{
		double sum = nu1 * log10(y_num1 * P / P0) - log10(y_den * P / P0);
		if(num2){
			sum += nu2 * log10(y_num2 * P / P0);
		}
		return sum;
	}
}

static void print_json_result(const EqmCase *C, double T, double P, const char *source,
		const char *algorithm, int status, const double *n, double H_total){
	double ntot = 0.0;
	double y_no = NAN;
	double y_no2 = NAN;
	double y_co = NAN;
	double y_h2 = NAN;
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
		y_no = extract_y(C, n, "nitric_oxide");
		y_no2 = extract_y(C, n, "nitrogen_dioxide");
		y_co = extract_y(C, n, "carbonmonoxide");
		y_h2 = extract_y(C, n, "hydrogen");
		printf(",\"y_no\":%.17g", isfinite(y_no) ? y_no : 0.0);
		printf(",\"y_no2\":%.17g", isfinite(y_no2) ? y_no2 : 0.0);
		printf(",\"y_co\":%.17g", isfinite(y_co) ? y_co : 0.0);
		printf(",\"y_h2\":%.17g", isfinite(y_h2) ? y_h2 : 0.0);
		{
			double log10k_no = log10Q_simple(C->species, n, C->ns, P,
				"nitric_oxide", 1.0, "oxygen", -0.5, "nitrogen");
			double log10k_no2 = log10Q_simple(C->species, n, C->ns, P,
				"nitrogen_dioxide", 1.0, "oxygen", -0.5, "nitric_oxide");
			if(isfinite(log10k_no)){
				printf(",\"log10K_no\":%.17g", log10k_no);
			}else{
				printf(",\"log10K_no\":null");
			}
			if(isfinite(log10k_no2)){
				printf(",\"log10K_no2\":%.17g", log10k_no2);
			}else{
				printf(",\"log10K_no2\":null");
			}
		}
		if(isfinite(H_total)){
			printf(",\"H_total\":%.17g", H_total);
		}else{
			printf(",\"H_total\":null");
		}
	}else{
		printf(",\"n\":null");
		printf(",\"y\":null");
		printf(",\"log10K_no\":null");
		printf(",\"log10K_no2\":null");
		printf(",\"H_total\":null");
	}
	printf("}\n");
}

int main(int argc, char *argv[]){
	const EqmCase *C;
	const char *algorithm = "auto_reduced";
	const char *source = "Moran and Shapiro";
	double T;
	double P;
	double y0[MAX_NS] = {0.0};
	double n[MAX_NS] = {0.0};
	double H_total = NAN;
	double nsum = 0.0;
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
		if(C->n0[i] < 0.0 || !isfinite(C->n0[i])){
			fprintf(stderr, "Invalid feed amount for species %d\n", i);
			return 2;
		}
		nsum += C->n0[i];
	}
	if(!(nsum > 0.0) || !isfinite(nsum)){
		fprintf(stderr, "Invalid feed total\n");
		return 2;
	}
	for(i = 0; i < C->ns; ++i){
		y0[i] = C->n0[i] / nsum;
	}
	status = fprops_eqm_tpy((const char **)C->species, C->ns, y0, source, T, P, algorithm, C->n0, n);
	if(eqm_status_ok(status)){
		(void)fprops_mix_h_tpn((const char **)C->species, C->ns, n, source, T, P, &H_total);
	}
	print_json_result(C, T, P, source, algorithm, status, n, H_total);
	return 0;
}

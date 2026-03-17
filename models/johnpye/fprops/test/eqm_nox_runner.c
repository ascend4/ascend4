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
		"humid_air_nox_demo",
		9,
		{"nitrogen", "oxygen", "argon", "water", "carbondioxide",
			"nitric_oxide", "nitrogen_dioxide", "carbonmonoxide", "hydrogen"},
		{
			0.78050639391839993,
			0.20937051030959999,
			0.0096958557720000001,
			0.015,
			0.00042724000000000001,
			0.0,
			0.0,
			0.0,
			0.0
		}
	},
	{
		"humid_air_nox_demo_ascorder",
		9,
		{"argon", "carbonmonoxide", "carbondioxide", "hydrogen", "water",
			"nitrogen", "nitric_oxide", "nitrogen_dioxide", "oxygen"},
		{
			0.0096958557720000001,
			0.0,
			0.00042724000000000001,
			0.0,
			0.015,
			0.78050639391839993,
			0.0,
			0.0,
			0.20937051030959999
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

typedef enum RunnerMode{
	RUNNER_LEGACY_FEEDINIT = 0,
	RUNNER_PKG_NULLINIT,
	RUNNER_PKG_FEEDINIT,
	RUNNER_PKG_CUSTOMINIT
} RunnerMode;

static const char *runner_mode_label(RunnerMode mode){
	switch(mode){
	case RUNNER_LEGACY_FEEDINIT:
		return "legacy_feedinit";
	case RUNNER_PKG_NULLINIT:
		return "pkg_nullinit";
	case RUNNER_PKG_FEEDINIT:
		return "pkg_feedinit";
	case RUNNER_PKG_CUSTOMINIT:
		return "pkg_custominit";
	default:
		return "unknown";
	}
}

static int parse_init_env(const char *envname, int ns, double *n_init){
	const char *v = getenv(envname);
	char *buf = NULL;
	char *tok = NULL;
	char *saveptr = NULL;
	int i = 0;

	if(!v || !v[0] || !n_init || ns <= 0){
		return 0;
	}
	buf = strdup(v);
	if(!buf){
		return 0;
	}
	for(tok = strtok_r(buf, ",", &saveptr); tok; tok = strtok_r(NULL, ",", &saveptr)){
		char *endptr = NULL;
		double val;
		if(i >= ns){
			free(buf);
			return 0;
		}
		val = strtod(tok, &endptr);
		if(endptr == tok || !isfinite(val) || val < 0.0){
			free(buf);
			return 0;
		}
		n_init[i++] = val;
	}
	free(buf);
	return i == ns;
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
		const char *algorithm, const char *runner_mode, int status, const double *n,
		double H_total){
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
	printf(",\"runner_mode\":\"%s\"", runner_mode);
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

static int run_case_once(const EqmCase *C, double T, double P, const char *algorithm,
		const char *source, RunnerMode mode, FpropsRxnPackage *pkg_reuse){
	double n[MAX_NS] = {0.0};
	double H_total = NAN;
	int status;
	int i;

	if(mode == RUNNER_LEGACY_FEEDINIT){
		double y0[MAX_NS] = {0.0};
		double nsum = 0.0;
		for(i = 0; i < C->ns; ++i){
			nsum += C->n0[i];
		}
		for(i = 0; i < C->ns; ++i){
			y0[i] = C->n0[i] / nsum;
		}
		status = fprops_eqm_tpy((const char **)C->species, C->ns, y0, source, T, P, algorithm, C->n0, n);
		if(eqm_status_ok(status)){
			(void)fprops_mix_h_tpn((const char **)C->species, C->ns, n, source, T, P, &H_total);
		}
	}else{
		FpropsRxnPackage *pkg = pkg_reuse;
		FpropsRxnTPN state = {T, P, C->n0};
		FpropsRxnResult out = {-99, NAN, NAN, n};
		double n_init_buf[MAX_NS] = {0.0};
		const double *n_init = NULL;
		int own_pkg = 0;
		if(mode == RUNNER_PKG_FEEDINIT){
			n_init = C->n0;
		}else if(mode == RUNNER_PKG_CUSTOMINIT){
			if(!parse_init_env("EQM_RUNNER_INIT", C->ns, n_init_buf)){
				fprintf(stderr, "Invalid or missing EQM_RUNNER_INIT for pkg_custominit\n");
				return 1;
			}
			n_init = n_init_buf;
		}
		if(!pkg){
			pkg = fprops_rxn_package_build((const char **)C->species, C->ns, source);
			own_pkg = 1;
		}
		if(!pkg){
			fprintf(stderr, "Failed to build reactive package\n");
			return 1;
		}
		status = fprops_rxn_eqm_tpy(pkg, &state, algorithm, n_init, &out);
		if(eqm_status_ok(status)){
			FpropsRxnTPN state_out = {T, P, n};
			(void)fprops_rxn_mix_h(pkg, &state_out, &H_total);
		}
		if(own_pkg){
			fprops_rxn_package_free(pkg);
		}
	}
	print_json_result(C, T, P, source, algorithm, runner_mode_label(mode), status, n, H_total);
	return 0;
}

int main(int argc, char *argv[]){
	const EqmCase *C;
	const char *algorithm = "auto_reduced";
	const char *source = "Moran and Shapiro";
	RunnerMode mode = RUNNER_LEGACY_FEEDINIT;
	double P;
	char *temps_arg = NULL;
	int i;

	if(argc < 4){
		fprintf(stderr,
			"USAGE: %s <case|list> <T[K]|T1,T2,...> <P[Pa]> [algorithm] [source] [legacy_feedinit|pkg_nullinit|pkg_feedinit|pkg_custominit]\n",
			argv[0]);
		return 2;
	}
	if(0 == strcmp(argv[1], "list")){
		print_case_list();
		return 0;
	}
	P = atof(argv[3]);
	if(argc >= 5){
		algorithm = argv[4];
	}
	if(argc >= 6){
		source = argv[5];
	}
	if(argc >= 7){
		if(0 == strcmp(argv[6], "legacy_feedinit")){
			mode = RUNNER_LEGACY_FEEDINIT;
		}else if(0 == strcmp(argv[6], "pkg_nullinit")){
			mode = RUNNER_PKG_NULLINIT;
		}else if(0 == strcmp(argv[6], "pkg_feedinit")){
			mode = RUNNER_PKG_FEEDINIT;
		}else if(0 == strcmp(argv[6], "pkg_custominit")){
			mode = RUNNER_PKG_CUSTOMINIT;
		}else{
			fprintf(stderr, "Unknown runner mode '%s'\n", argv[6]);
			return 2;
		}
	}
	C = find_case(argv[1]);
	if(!C){
		fprintf(stderr, "Unknown case '%s'\n", argv[1]);
		return 2;
	}
	if(!(P > 0.0)){
		fprintf(stderr, "Invalid P\n");
		return 2;
	}
	for(i = 0; i < C->ns; ++i){
		if(C->n0[i] < 0.0 || !isfinite(C->n0[i])){
			fprintf(stderr, "Invalid feed amount for species %d\n", i);
			return 2;
		}
	}
	temps_arg = strdup(argv[2]);
	if(!temps_arg){
		fprintf(stderr, "Allocation failure\n");
		return 1;
	}
	if(strchr(temps_arg, ',')){
		FpropsRxnPackage *pkg_reuse = NULL;
		char *saveptr = NULL;
		char *tok = strtok_r(temps_arg, ",", &saveptr);
		if(mode != RUNNER_LEGACY_FEEDINIT){
			pkg_reuse = fprops_rxn_package_build((const char **)C->species, C->ns, source);
			if(!pkg_reuse){
				free(temps_arg);
				fprintf(stderr, "Failed to build reactive package\n");
				return 1;
			}
		}
		while(tok){
			double T = atof(tok);
			if(!(T > 0.0)){
				fprintf(stderr, "Invalid T '%s'\n", tok);
				free(temps_arg);
				if(pkg_reuse) fprops_rxn_package_free(pkg_reuse);
				return 2;
			}
			if(run_case_once(C, T, P, algorithm, source, mode, pkg_reuse)){
				free(temps_arg);
				if(pkg_reuse) fprops_rxn_package_free(pkg_reuse);
				return 1;
			}
			tok = strtok_r(NULL, ",", &saveptr);
		}
		if(pkg_reuse){
			fprops_rxn_package_free(pkg_reuse);
		}
	}else{
		double T = atof(temps_arg);
		if(!(T > 0.0)){
			fprintf(stderr, "Invalid T\n");
			free(temps_arg);
			return 2;
		}
		if(run_case_once(C, T, P, algorithm, source, mode, NULL)){
			free(temps_arg);
			return 1;
		}
	}
	free(temps_arg);
	return 0;
}

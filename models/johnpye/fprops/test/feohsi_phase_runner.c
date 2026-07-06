#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../eqm_phase.h"

static double nz(double v){
	return isfinite(v) ? v : 0.0;
}

static void print_phase(const FpropsEqmPhaseResult *r, const char *name, int *first){
	double v = fprops_eqm_phase_amount(r, name);
	if(!*first) printf(",");
	printf("\"%s\":", name);
	if(isfinite(v)) printf("%.17g", v); else printf("null");
	*first = 0;
}

static void print_member(const FpropsEqmPhaseResult *r, const char *phase, const char *member, int *first){
	double v = fprops_eqm_phase_member_amount(r, phase, member);
	if(!*first) printf(",");
	printf("\"%s:%s\":", phase, member);
	if(isfinite(v)) printf("%.17g", v); else printf("null");
	*first = 0;
}

int main(int argc, char *argv[]){
	FpropsEqm eqm;
	FpropsEqmPhaseResult result;
	double T, P, b_fe, b_o, b_si, b_h;
	const char *solver = "slsqp";
	const char *algorithm = "auto";
	int status;
	int first;
	double n_fe_bcc, n_fe_fcc, n_fe2o3, n_sio2, n_fe2sio4;
	double n_h2, n_h2o;
	double fe_fayalite, fe_metal, fe_hematite;
	double si_fayalite, si_free_sio2;
	double god_out;

	if(argc < 7 || argc > 9){
		fprintf(stderr, "USAGE: %s <T[K]> <P[Pa]> <b_Fe> <b_O> <b_Si> <b_H> [solver=slsqp] [algorithm=auto]\n", argv[0]);
		return 2;
	}
	T = atof(argv[1]);
	P = atof(argv[2]);
	b_fe = atof(argv[3]);
	b_o = atof(argv[4]);
	b_si = atof(argv[5]);
	b_h = atof(argv[6]);
	if(argc >= 8) solver = argv[7];
	if(argc >= 9) algorithm = argv[8];
	if(!(T > 0.0) || !(P > 0.0) || !(b_fe >= 0.0) || !(b_o >= 0.0) || !(b_si >= 0.0) || !(b_h >= 0.0)){
		fprintf(stderr, "Invalid input\n");
		return 2;
	}

	fprops_eqm_init(&eqm);
	if(fprops_eqm_add_phase(&eqm, "Fe_bcc=hidayat_2015", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "Fe_fcc=hidayat_2015", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "wustite=hidayat_2015", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "spinel=hidayat_adj1", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "Fe2O3=hidayat_2015", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "SiO2=slag_pragmatic_2026", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "Fe2SiO4=hidayat_2017_feo_fe2o3_sio2", NULL) < 0) return 3;
	if(fprops_eqm_add_phase(&eqm, "gas:ideal(hydrogen,water)=helmholtz+ref0:", NULL) < 0) return 3;
	if(fprops_eqm_set_element(&eqm, "Fe", b_fe) != 0) return 4;
	if(fprops_eqm_set_element(&eqm, "O", b_o) != 0) return 4;
	if(fprops_eqm_set_element(&eqm, "Si", b_si) != 0) return 4;
	if(fprops_eqm_set_element(&eqm, "H", b_h) != 0) return 4;
	if(fprops_eqm_set_TP(&eqm, T, P) != 0) return 4;
	if(fprops_eqm_set_algorithm(&eqm, algorithm) != 0) return 4;
	if(fprops_eqm_set_nlp_solver_name(&eqm, solver) != 0) return 4;

	status = fprops_eqm_solve(&eqm, &result);

	n_fe_bcc = nz(fprops_eqm_phase_amount(&result, "Fe_bcc"));
	n_fe_fcc = nz(fprops_eqm_phase_amount(&result, "Fe_fcc"));
	n_fe2o3 = nz(fprops_eqm_phase_amount(&result, "Fe2O3"));
	n_sio2 = nz(fprops_eqm_phase_amount(&result, "SiO2"));
	n_fe2sio4 = nz(fprops_eqm_phase_amount(&result, "Fe2SiO4"));
	n_h2 = nz(fprops_eqm_phase_member_amount(&result, "gas:ideal", "hydrogen"));
	n_h2o = nz(fprops_eqm_phase_member_amount(&result, "gas:ideal", "water"));
	fe_fayalite = 2.0 * n_fe2sio4;
	fe_metal = n_fe_bcc + n_fe_fcc;
	fe_hematite = 2.0 * n_fe2o3;
	si_fayalite = n_fe2sio4;
	si_free_sio2 = n_sio2;
	god_out = (n_h2 + n_h2o) > 0.0 ? n_h2o / (n_h2 + n_h2o) : NAN;

	printf("{\"T\":%.17g,\"P\":%.17g,\"solver\":\"%s\",\"algorithm\":\"%s\",\"status\":%d,\"solver_status\":%d", T, P, solver, algorithm, status, result.solver_status);
	printf(",\"feed\":{\"Fe\":%.17g,\"O\":%.17g,\"Si\":%.17g,\"H\":%.17g}", b_fe, b_o, b_si, b_h);
	printf(",\"phase_amounts\":{");
	first = 1;
	print_phase(&result, "Fe_bcc", &first);
	print_phase(&result, "Fe_fcc", &first);
	print_phase(&result, "wustite", &first);
	print_phase(&result, "spinel", &first);
	print_phase(&result, "Fe2O3", &first);
	print_phase(&result, "SiO2", &first);
	print_phase(&result, "Fe2SiO4", &first);
	print_phase(&result, "gas:ideal", &first);
	printf("}");
	printf(",\"member_amounts\":{");
	first = 1;
	print_member(&result, "wustite", "Wus_FeO", &first);
	print_member(&result, "wustite", "Wus_FeO1p5", &first);
	print_member(&result, "spinel", "Sp_Fe2_tet", &first);
	print_member(&result, "spinel", "Sp_Fe3_tet", &first);
	print_member(&result, "spinel", "Sp_Fe2_oct", &first);
	print_member(&result, "spinel", "Sp_Fe3_oct", &first);
	print_member(&result, "spinel", "Sp_Va_oct", &first);
	print_member(&result, "gas:ideal", "hydrogen", &first);
	print_member(&result, "gas:ideal", "water", &first);
	printf("}");
	printf(",\"summary\":{\"gas_GOD_out\":");
	if(isfinite(god_out)) printf("%.17g", god_out); else printf("null");
	printf(",\"Fe_metal\":%.17g,\"Fe_fayalite\":%.17g,\"Fe_hematite\":%.17g,\"Si_fayalite\":%.17g,\"Si_free_sio2\":%.17g", fe_metal, fe_fayalite, fe_hematite, si_fayalite, si_free_sio2);
	printf(",\"Fe_metal_frac\":%.17g,\"Fe_fayalite_frac\":%.17g,\"Si_fayalite_frac\":%.17g", b_fe > 0.0 ? fe_metal / b_fe : NAN, b_fe > 0.0 ? fe_fayalite / b_fe : NAN, b_si > 0.0 ? si_fayalite / b_si : NAN);
	printf("}}\n");
	return status == 0 ? 0 : 1;
}

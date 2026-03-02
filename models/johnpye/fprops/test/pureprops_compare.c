#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fluids.h"
#include "../fprops.h"
#include "../eqm.h"
#include "../ideal.h"
#include "../helmholtz.h"

static double to_molar(double specific, const PureFluid *P){
	return specific * P->data->M / 1000.0;
}

typedef struct {
	int ok;
	double rho;
	double v;
	double z;
	double h;
	double s;
	double g;
	double cp;
	double cv;
	double cp0;
	double mu0;
} PropsRow;

static int solve_rho_gas(double T, double p, const PureFluid *P, double *rho_out){
	FpropsError err = FPROPS_NO_ERROR;
	double rho = p / (P->data->R * T);
	int iter;
	if(!(rho > 0.0)){
		return 0;
	}
	if(P->type == FPROPS_IDEAL){
		*rho_out = rho;
		return 1;
	}
	for(iter = 0; iter < 20; ++iter){
		FluidState2 S = fprops_set_Trho(T, rho, P, &err);
		double pcalc, dpdrho, step, rho_new;
		if(err){
			return 0;
		}
		pcalc = fprops_p(S, &err);
		if(err){
			return 0;
		}
		dpdrho = fprops_dpdrho_T(S, &err);
		if(err || !isfinite(dpdrho) || fabs(dpdrho) < 1e-12){
			return 0;
		}
		step = (pcalc - p) / dpdrho;
		rho_new = rho - step;
		if(!(rho_new > 0.0) || !isfinite(rho_new)){
			rho_new = 0.5 * rho;
		}
		if(fabs((rho_new - rho) / rho) < 1e-10){
			*rho_out = rho_new;
			return 1;
		}
		rho = rho_new;
	}
	*rho_out = rho;
	return 1;
}

static PropsRow eval_one(const char *species, const char *label, const PureFluid *P, double T, double p){
	FpropsError err = FPROPS_NO_ERROR;
	FluidState2 S;
	PropsRow row;
	double pchk;
	const char *mu0_source = NULL;

	memset(&row, 0, sizeof(row));
	row.mu0 = NAN;

	if(!P){
		return row;
	}

	if(!solve_rho_gas(T, p, P, &row.rho)){
		return row;
	}
	S = fprops_set_Trho(T, row.rho, P, &err);
	if(err){
		return row;
	}

	row.rho = fprops_rho(S, &err);
	if(err) goto fail;
	pchk = fprops_p(S, &err);
	if(err) goto fail;
	row.h = fprops_h(S, &err);
	if(err) goto fail;
	row.s = fprops_s(S, &err);
	if(err) goto fail;
	row.g = fprops_g(S, &err);
	if(err) goto fail;
	row.cp = fprops_cp(S, &err);
	if(err) goto fail;
	row.cv = fprops_cv(S, &err);
	if(err) goto fail;
	row.cp0 = fprops_cp0(S, &err);
	if(err) goto fail;

	row.v = 1.0 / row.rho;
	row.z = pchk / (row.rho * P->data->R * T);

	if(0 == strcmp(label, "helmholtz")){
		mu0_source = "helmholtz:";
	}else if(0 == strcmp(label, "helm_REF0")){
		mu0_source = "helmholtz+ref0:";
	}else if(0 == strcmp(label, "RPP")){
		mu0_source = "ideal:RPP";
	}else{
		mu0_source = "Moran and Shapiro";
	}

	if(!eqm_mu0_source(species, mu0_source, T, p, &row.mu0)){
		row.mu0 = NAN;
	}

	row.h = to_molar(row.h, P);
	row.s = to_molar(row.s, P);
	row.g = to_molar(row.g, P);
	row.cp = to_molar(row.cp, P);
	row.cv = to_molar(row.cv, P);
	row.cp0 = to_molar(row.cp0, P);
	row.ok = 1;
	return row;

fail:
	return row;
}

static void print_one(const char *species, const char *label, const PureFluid *P, double T, double p){
	PropsRow row = eval_one(species, label, P, T, p);
	if(!row.ok){
		printf("%-8s %-12s %8.2f %12s\n", species, label, T - 273.15, "prop_failed");
		return;
	}

	printf(
		"%-8s %-12s %8.2f %12.6g %12.6g %12.6g %12.6g %12.6g %12.6g %12.6g %12.6g %12.6g %12.6g\n",
		species,
		label,
		T - 273.15,
		row.rho,
		row.v,
		row.z,
		row.h,
		row.s,
		row.g,
		row.cp,
		row.cv,
		row.cp0,
		row.mu0
	);
}

static void print_reaction_row(const char *label, double T, double dg_g, double dg_mu0){
	printf(
		"%-12s %8.2f %16.6g %16.6g\n",
		label,
		T - 273.15,
		dg_g,
		dg_mu0
	);
}

int main(int argc, char *argv[]){
	static const char *species[] = {"water", "hydrogen", "oxygen"};
	static const double default_temps_c[] = {300.0, 600.0, 900.0, 1000.0};
	double p = 1e5;
	double temps_c[64];
	int ntemps = 0;
	int i, j;
	ReferenceState ref0 = {FPROPS_REF_REF0};

	if(argc >= 2){
		p = atof(argv[1]);
	}
	if(argc >= 3){
		for(i = 2; i < argc && ntemps < 64; ++i){
			temps_c[ntemps++] = atof(argv[i]) - 273.15;
		}
	}else{
		for(i = 0; i < 4; ++i){
			temps_c[ntemps++] = default_temps_c[i];
		}
	}

	printf(
		"%-8s %-12s %8s %12s %12s %12s %12s %12s %12s %12s %12s %12s %12s\n",
		"species", "model", "T[C]", "rho[kg/m3]", "v[m3/kg]", "Z",
		"h[J/mol]", "s[J/mol/K]", "g[J/mol]", "cp[J/mol/K]", "cv[J/mol/K]",
		"cp0[J/mol/K]", "mu0[J/mol]"
	);

	for(i = 0; i < ntemps; ++i){
		double T = temps_c[i] + 273.15;
		for(j = 0; j < 3; ++j){
			const char *sp = species[j];
			const PureFluid *Phelm = fprops_fluid(sp, "helmholtz", NULL);
			const EosData *Ehelm = fprops_eos(sp, "helmholtz", NULL);
			const PureFluid *Phelm_ref0 = Ehelm ? helmholtz_prepare(Ehelm, &ref0) : NULL;
			const EosData *Erpp = fprops_eos(sp, "ideal", "RPP");
			const PureFluid *Prpp = Erpp ? ideal_prepare(Erpp, &ref0) : NULL;
			const EosData *Ems = fprops_eos(sp, "ideal", "Moran and Shapiro");
			const PureFluid *Pms = Ems ? ideal_prepare(Ems, &ref0) : NULL;
			print_one(sp, "helmholtz", Phelm, T, p);
			print_one(sp, "helm_REF0", Phelm_ref0, T, p);
			print_one(sp, "RPP", Prpp, T, p);
			print_one(sp, "M&S", Pms, T, p);
		}
	}

	printf("\n");
	printf("%-12s %8s %16s %16s\n", "model", "T[C]", "dG_rxn[g]", "dG_rxn[mu0]");
	for(i = 0; i < ntemps; ++i){
		double T = temps_c[i] + 273.15;
		const PureFluid *Pw_helm = fprops_fluid("water", "helmholtz", NULL);
		const PureFluid *Ph2_helm = fprops_fluid("hydrogen", "helmholtz", NULL);
		const PureFluid *Po2_helm = fprops_fluid("oxygen", "helmholtz", NULL);
		const EosData *Ew_helm = fprops_eos("water", "helmholtz", NULL);
		const EosData *Eh2_helm = fprops_eos("hydrogen", "helmholtz", NULL);
		const EosData *Eo2_helm = fprops_eos("oxygen", "helmholtz", NULL);
		const PureFluid *Pw_helm_ref0 = Ew_helm ? helmholtz_prepare(Ew_helm, &ref0) : NULL;
		const PureFluid *Ph2_helm_ref0 = Eh2_helm ? helmholtz_prepare(Eh2_helm, &ref0) : NULL;
		const PureFluid *Po2_helm_ref0 = Eo2_helm ? helmholtz_prepare(Eo2_helm, &ref0) : NULL;
		const EosData *Ew_rpp = fprops_eos("water", "ideal", "RPP");
		const EosData *Eh2_rpp = fprops_eos("hydrogen", "ideal", "RPP");
		const EosData *Eo2_rpp = fprops_eos("oxygen", "ideal", "RPP");
		const EosData *Ew_ms = fprops_eos("water", "ideal", "Moran and Shapiro");
		const EosData *Eh2_ms = fprops_eos("hydrogen", "ideal", "Moran and Shapiro");
		const EosData *Eo2_ms = fprops_eos("oxygen", "ideal", "Moran and Shapiro");
		const PureFluid *Pw_rpp = Ew_rpp ? ideal_prepare(Ew_rpp, &ref0) : NULL;
		const PureFluid *Ph2_rpp = Eh2_rpp ? ideal_prepare(Eh2_rpp, &ref0) : NULL;
		const PureFluid *Po2_rpp = Eo2_rpp ? ideal_prepare(Eo2_rpp, &ref0) : NULL;
		const PureFluid *Pw_ms = Ew_ms ? ideal_prepare(Ew_ms, &ref0) : NULL;
		const PureFluid *Ph2_ms = Eh2_ms ? ideal_prepare(Eh2_ms, &ref0) : NULL;
		const PureFluid *Po2_ms = Eo2_ms ? ideal_prepare(Eo2_ms, &ref0) : NULL;
		PropsRow w_helm = eval_one("water", "helmholtz", Pw_helm, T, p);
		PropsRow h2_helm = eval_one("hydrogen", "helmholtz", Ph2_helm, T, p);
		PropsRow o2_helm = eval_one("oxygen", "helmholtz", Po2_helm, T, p);
		PropsRow w_helm_ref0 = eval_one("water", "helm_REF0", Pw_helm_ref0, T, p);
		PropsRow h2_helm_ref0 = eval_one("hydrogen", "helm_REF0", Ph2_helm_ref0, T, p);
		PropsRow o2_helm_ref0 = eval_one("oxygen", "helm_REF0", Po2_helm_ref0, T, p);
		PropsRow w_rpp = eval_one("water", "RPP", Pw_rpp, T, p);
		PropsRow h2_rpp = eval_one("hydrogen", "RPP", Ph2_rpp, T, p);
		PropsRow o2_rpp = eval_one("oxygen", "RPP", Po2_rpp, T, p);
		PropsRow w_ms = eval_one("water", "M&S", Pw_ms, T, p);
		PropsRow h2_ms = eval_one("hydrogen", "M&S", Ph2_ms, T, p);
		PropsRow o2_ms = eval_one("oxygen", "M&S", Po2_ms, T, p);
		double dg_g_helm, dg_g_helm_ref0, dg_g_rpp, dg_g_ms;
		double dg_mu0_helm, dg_mu0_helm_ref0, dg_mu0_rpp, dg_mu0_ms;

		if(!(w_helm.ok && h2_helm.ok && o2_helm.ok
				&& w_helm_ref0.ok && h2_helm_ref0.ok && o2_helm_ref0.ok
				&& w_rpp.ok && h2_rpp.ok && o2_rpp.ok
				&& w_ms.ok && h2_ms.ok && o2_ms.ok)){
			continue;
		}

		dg_g_helm = w_helm.g - h2_helm.g - 0.5 * o2_helm.g;
		dg_g_helm_ref0 = w_helm_ref0.g - h2_helm_ref0.g - 0.5 * o2_helm_ref0.g;
		dg_g_rpp = w_rpp.g - h2_rpp.g - 0.5 * o2_rpp.g;
		dg_g_ms = w_ms.g - h2_ms.g - 0.5 * o2_ms.g;
		dg_mu0_helm = w_helm.mu0 - h2_helm.mu0 - 0.5 * o2_helm.mu0;
		dg_mu0_helm_ref0 = w_helm_ref0.mu0 - h2_helm_ref0.mu0 - 0.5 * o2_helm_ref0.mu0;
		dg_mu0_rpp = w_rpp.mu0 - h2_rpp.mu0 - 0.5 * o2_rpp.mu0;
		dg_mu0_ms = w_ms.mu0 - h2_ms.mu0 - 0.5 * o2_ms.mu0;

		print_reaction_row("helmholtz", T, dg_g_helm, dg_mu0_helm);
		print_reaction_row("helm_REF0", T, dg_g_helm_ref0, dg_mu0_helm_ref0);
		print_reaction_row("RPP", T, dg_g_rpp, dg_mu0_rpp);
		print_reaction_row("M&S", T, dg_g_ms, dg_mu0_ms);
	}
	return 0;
}

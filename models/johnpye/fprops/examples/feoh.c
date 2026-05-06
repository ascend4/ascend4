#include "../eqm.h"
#include "../eqm_phase.h"

#include <math.h>
#include <stdio.h>

static double interp(double x, double y0, double x0, double y1, double x1){
	return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

static double log10_from_god(double god){
	return log10(god / (1.0 - god));
}

static double interp_log10_god(double tc, double god0, double tc0, double god1,
		double tc1){
	return log10_from_god(interp(tc, god0, tc0, god1, tc1));
}

static double bg_fe_wustite_log10_h2o_h2_700c(void){
	return interp_log10_god(700.0,
		0.28587264008, 682.801879077, 0.295194336274, 704.894663517);
}

static double bg_wustite_spinel_log10_h2o_h2_700c(void){
	return interp_log10_god(700.0,
		0.5333888167, 698.63992637, 0.553100275071, 707.819009833);
}

static int find_lambda_fe_for_entry(const FpropsEqmPhaseModel *phase,
		double T, double P, double lambda_o, double *lambda_fe_out, double *y_out){
	double lambda[2];
	int i_fe = fprops_eqm_phase_find_element(phase, "Fe");
	int i_o = fprops_eqm_phase_find_element(phase, "O");
	double lo = -1000000.0;
	double hi = 1000000.0;
	double r_lo;
	double r_hi;
	if(i_fe < 0 || i_o < 0){
		return 0;
	}
	// In this equilibrium calculation, lambda is the vector of Lagrange
	// multipliers for the element-balance constraints. Thermodynamically, each
	// entry is the chemical potential contribution of one mole of that element.
	// FPROPS evaluates a phase entry residual as g_phase - sum(lambda_i a_i), so
	// a phase is stable for entry when its minimized residual is zero.
	//
	// The H2/H2O buffer fixes the oxygen multiplier lambda_O for this example
	// feed. We then solve the remaining one-dimensional problem for lambda_Fe,
	// which gives the wustite composition sitting on that buffer.
	lambda[i_o] = lambda_o;
	lambda[i_fe] = lo;
	if(!fprops_eqm_phase_entry_residual(phase, T, P, lambda, &r_lo, y_out)){
		return 0;
	}
	lambda[i_fe] = hi;
	if(!fprops_eqm_phase_entry_residual(phase, T, P, lambda, &r_hi, y_out)){
		return 0;
	}
	if(r_lo * r_hi > 0.0){
		return 0;
	}
	for(int k = 0; k < 80; ++k){
		double mid = 0.5 * (lo + hi);
		double r_mid;
		lambda[i_fe] = mid;
		if(!fprops_eqm_phase_entry_residual(phase, T, P, lambda, &r_mid, y_out)){
			return 0;
		}
		if(fabs(r_mid) < 1e-8){
			*lambda_fe_out = mid;
			return 1;
		}
		if(r_lo * r_mid <= 0.0){
			hi = mid;
			r_hi = r_mid;
		}else{
			lo = mid;
			r_lo = r_mid;
		}
	}
	*lambda_fe_out = 0.5 * (lo + hi);
	return 1;
}

static int add_h2_h2o_feed(FpropsEqm *eqm, double gas_total, double log10_h2o_h2){
	double ratio = pow(10.0, log10_h2o_h2);
	double n_h2o = gas_total * ratio / (1.0 + ratio);
	double n_h2 = gas_total - n_h2o;
	return fprops_eqm_add_comps(eqm, "H2", n_h2, "H2O", n_h2o);
}

static int solve_and_print_case(const char *label, FpropsEqm *eqm, double T){
	FpropsEqmPhaseResult result;
	result.status = -99;
	result.solver_status = -99;

	// Temperature and pressure live in the problem object. The solve call only
	// needs the problem and an output result record.
	int status = fprops_eqm_solve_TP(eqm, T, 101325.0, &result);

	printf("\n%s\n", label);
	printf("  T = %.2f K, P = 101325 Pa\n", T);
	printf("  status = %d, solver_status = %d (%s)\n", status, result.solver_status,
		fprops_eqm_status_text(result.solver_status));
	if(status != 0){
		return 1;
	}

	// The accessors keep the example from relying on the storage layout of
	// FpropsEqmPhaseResult for the common summary quantities.
	printf("  summary:\n");
	printf("    metallic Fe = %.8e mol\n",
		fprops_eqm_phase_amount(&result, "Fe_bcc"));
	printf("    unreduced hematite = %.8e mol\n",
		fprops_eqm_phase_amount(&result, "Fe2O3"));
	printf("    wustite = %.8e mol, x_FeO1p5 = %.8e\n",
		fprops_eqm_phase_amount(&result, "wustite"),
		fprops_eqm_phase_coord(&result, "wustite", "x_FeO1p5"));
	printf("    spinel = %.8e mol, y_tet_fe2 = %.8e, y_oct_fe2 = %.8e\n",
		fprops_eqm_phase_amount(&result, "spinel"),
		fprops_eqm_phase_coord(&result, "spinel", "y_tet_fe2"),
		fprops_eqm_phase_coord(&result, "spinel", "y_oct_fe2"));
	printf("    H2 = %.8e mol, H2O = %.8e mol\n",
		fprops_eqm_phase_member_amount(&result, "gas:ideal", "hydrogen"),
		fprops_eqm_phase_member_amount(&result, "gas:ideal", "water"));

	printf("  phases in package order:\n");
	fprops_eqm_write(stdout, &result, "text");
	return 0;
}

int main(void){
	// This example is intentionally FPROPS-only: it builds the same kind of
	// phase package that ASCEND callers will use, but without any ASCEND model.
	FpropsEqm eqm;
	double y_wus[1] = {0.0};
	double y_sp[2] = {0.40, 0.20};
	double log10_mid;
	double mu_h2;
	double mu_h2o;
	double lambda_o_thermo;
	double lambda_fe;
	int ph_wustite;
	int failed = 0;

	// Resolve each compact phase specification into one reusable problem
	// object. The object stores the phase package, element list, feed totals,
	// temperature, pressure, and solver settings.
	fprops_eqm_init(&eqm);
	if(fprops_eqm_add_phases(&eqm,
			"Fe_bcc=hidayat_2015",
			"wustite=hidayat_2015",
			"spinel=degterov_2001",
			"Fe2O3=hidayat_2015",
			"gas:ideal(hydrogen,water)=helmholtz+ref0:") != 0){
		fprintf(stderr, "failed to add phase package\n");
		return 1;
	}
	ph_wustite = fprops_eqm_find_phase(&eqm, "wustite");
	if(ph_wustite < 0 || fprops_eqm_find_phase(&eqm, "spinel") < 0){
		fprintf(stderr, "failed to find required solution phases\n");
		return 1;
	}

	// Reducing feed: one mole of hematite plus 100 mole H2. Written as element
	// totals, that is [Fe, O, H] = [2, 3, 200].
	fprops_eqm_clear_feed(&eqm);
	if(fprops_eqm_add_comps(&eqm, "Fe2O3", 1, "H2", 100) != 0){
		fprintf(stderr, "failed to set reducing feed\n");
		return 1;
	}
	failed |= solve_and_print_case("reducing: 1 mol Fe2O3 + 100 mol H2",
		&eqm, 1173.15);

	// Intermediate feed: use the midpoint between nearby Baur-Glaessner
	// Fe/wustite and wustite/spinel H2O/H2 buffer ratios at 700 C.
	log10_mid = 0.5 * (bg_fe_wustite_log10_h2o_h2_700c()
		+ bg_wustite_spinel_log10_h2o_h2_700c());

	// Convert that gas ratio into an oxygen element chemical potential from
	// H2 + O = H2O. This is just a convenient way to construct a feed close to a
	// known Fe-O-H buffer; the equilibrium solver itself remains a general
	// element-balance Gibbs minimizer.
	if(!eqm_mu0_source("hydrogen", "helmholtz+ref0:", 973.15, 1e5, &mu_h2)
			|| !eqm_mu0_source("water", "helmholtz+ref0:", 973.15, 1e5, &mu_h2o)){
		fprintf(stderr, "failed to evaluate H2/H2O standard potentials\n");
		return 1;
	}
	lambda_o_thermo = FPROPS_R * 973.15 * log(10.0) * log10_mid
		+ (mu_h2o - mu_h2);

	// Pick one mole of wustite at the chosen buffer composition and let the
	// problem object convert that phase amount into global element totals.
	if(!find_lambda_fe_for_entry(fprops_eqm_phase_model(&eqm, "wustite"), 973.15, 101325.0,
			lambda_o_thermo, &lambda_fe, y_wus)){
		fprintf(stderr, "failed to evaluate wustite buffer composition\n");
		return 1;
	}
	(void)lambda_fe;
	fprops_eqm_clear_feed(&eqm);
	if(fprops_eqm_add_phase_feed(&eqm, "wustite", 1.0, y_wus[0]) != 0){
		fprintf(stderr, "failed to add wustite feed\n");
		return 1;
	}

	// Add a finite H2/H2O gas charge at the same buffer ratio so the solved
	// assemblage contains both condensed phases and a gas phase.
	if(add_h2_h2o_feed(&eqm, 10.0, log10_mid) != 0){
		fprintf(stderr, "failed to add intermediate gas feed\n");
		return 1;
	}
	failed |= solve_and_print_case("intermediate: wustite-buffered feed",
		&eqm, 973.15);

	// Oxidizing feed: start from one mole of spinel at a simple site
	// composition, then add 10 mole gas with a high H2O/H2 ratio.
	fprops_eqm_clear_feed(&eqm);
	if(fprops_eqm_add_phase_feed_vars(&eqm, "spinel", 1.0,
			"y_tet_fe2", y_sp[0], "y_oct_fe2", y_sp[1]) != 0){
		fprintf(stderr, "failed to add spinel feed\n");
		return 1;
	}
	if(add_h2_h2o_feed(&eqm, 10.0, 0.80) != 0){
		fprintf(stderr, "failed to add oxidizing gas feed\n");
		return 1;
	}
	failed |= solve_and_print_case("oxidizing: spinel-buffered feed",
		&eqm, 973.15);
	return failed ? 1 : 0;
}

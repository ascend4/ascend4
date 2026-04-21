#include "wustite_hidayat.h"

#include <math.h>

/*
 * Hidayat et al. (CALPHAD 48, 2015) accepted binary Bragg-Williams model:
 *   (1-x) FeO + x FeO1.5
 * with polynomial excess term from Eq. (8) and Table 1:
 *   g_ex = X_A X_B [q00 + q10 X_A]
 * where X_A = 1 - x and X_B = x.
 * Coefficients are in J/mol and valid over 298-2500 K.
 */

static double wustite_hidayat_g0_a(double T, double p, FpropsError *err){
	(void)p;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T > 0.0)){
		if(err){
			*err = FPROPS_INVALID_REQUEST;
		}
		return NAN;
	}
	return -285203.5
		+ 274.2455 * T
		- 49.19444 * T * log(T)
		- 0.004678477 * T * T
		+ 297568.8 / T
		+ 574.4469 * log(T);
}

static double wustite_hidayat_g0_b(double T, double p, FpropsError *err){
	(void)p;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T > 0.0)){
		if(err){
			*err = FPROPS_INVALID_REQUEST;
		}
		return NAN;
	}
	return -523138.0
		+ 73.37019 * T
		- 26.96809 * T * log(T)
		- 0.008835071 * T * T
		+ 1498519.0 / T
		+ 25471.09 * log(T);
}

static double wustite_hidayat_gex(double T, double x, FpropsError *err){
	const double q00 = -59412.8;
	const double q10 = 42676.8;
	double xa = 1.0 - x;
	(void)T;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(x >= 0.0) || !(x <= 1.0)){
		if(err){
			*err = FPROPS_INVALID_REQUEST;
		}
		return NAN;
	}
	return xa * x * (q00 + q10 * xa);
}

static double wustite_hidayat_dgex_dx(double T, double x, FpropsError *err){
	const double q00 = -59412.8;
	const double q10 = 42676.8;
	(void)T;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(x >= 0.0) || !(x <= 1.0)){
		if(err){
			*err = FPROPS_INVALID_REQUEST;
		}
		return NAN;
	}
	return q00 * (1.0 - 2.0 * x) + q10 * (1.0 - 4.0 * x + 3.0 * x * x);
}

static double wustite_hidayat_d2gex_dx2(double T, double x, FpropsError *err){
	const double q00 = -59412.8;
	const double q10 = 42676.8;
	(void)T;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(x >= 0.0) || !(x <= 1.0)){
		if(err){
			*err = FPROPS_INVALID_REQUEST;
		}
		return NAN;
	}
	return -2.0 * q00 + q10 * (-4.0 + 6.0 * x);
}

static const BinarySolutionModel wustite_model = {
	"wustite_hidayat",
	&wustite_hidayat_g0_a,
	&wustite_hidayat_g0_b,
	&wustite_hidayat_gex,
	&wustite_hidayat_dgex_dx,
	&wustite_hidayat_d2gex_dx2,
	0.0,
	1.0
};

static const char *elements_feo[] = {"Fe", "O"};
static const double stoich_feo[] = {1.0, 1.0};

static const char *elements_feo15[] = {"Fe", "O"};
static const double stoich_feo15[] = {1.0, 1.5};

static const BinarySolutionPhaseDef wustite_phase = {
	"wustite",
	"hidayat_2015",
	"Wus_FeO",
	"Wus_FeO1p5",
	2, elements_feo, stoich_feo,
	2, elements_feo15, stoich_feo15,
	&wustite_model
};

static const BinarySolutionPhaseDef wustite_phase_feoxide_recon = {
	"wustite",
	"feoxide_recon_baseline_2026",
	"Wus_FeO",
	"Wus_FeO1p5",
	2, elements_feo, stoich_feo,
	2, elements_feo15, stoich_feo15,
	&wustite_model
};

const BinarySolutionPhaseDef *wustite_hidayat_phase(void){
	return &wustite_phase;
}

const BinarySolutionPhaseDef *wustite_feoxide_recon_phase(void){
	return &wustite_phase_feoxide_recon;
}

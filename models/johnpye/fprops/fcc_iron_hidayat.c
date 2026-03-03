#include "fcc_iron_hidayat.h"

#include <math.h>

static double fcc_iron_R(void){
	return 8.31446261815324;
}

static double hillert_jarl_A(double p){
	return 518.0 / 1125.0 + (11692.0 / 15975.0) * (1.0 / p - 1.0);
}

static int hillert_jarl_gmag(double T, double Tord, double beta, double p, double *g_out){
	double tau;
	double A;
	double f;
	if(!g_out || !(T > 0.0) || !(Tord != 0.0) || !(beta != -1.0) || !(p > 0.0)){
		return 0;
	}
	tau = T / fabs(Tord);
	A = hillert_jarl_A(p);
	if(!(A > 0.0)){
		return 0;
	}
	if(tau <= 1.0){
		double poly = tau * tau * tau / 6.0
			+ pow(tau, 9.0) / 135.0
			+ pow(tau, 15.0) / 600.0;
		f = 1.0 - (
			(79.0 / (140.0 * p)) / tau
			+ (474.0 / 497.0) * (1.0 / p - 1.0) * poly
		) / A;
	}else{
		f = -(
			pow(tau, -5.0) / 10.0
			+ pow(tau, -15.0) / 315.0
			+ pow(tau, -25.0) / 1500.0
		) / A;
	}
	*g_out = f * fcc_iron_R() * T * log(fabs(beta) + 1.0);
	return isfinite(*g_out);
}

static double fcc_iron_hidayat_ghser_fe(double T, FpropsError *err){
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T >= 298.0) || !(T <= 6000.0)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	if(T <= 1811.0){
		return 1225.7
			+ 124.134 * T
			- 23.5143 * T * log(T)
			- 0.00439752 * T * T
			+ 77359.0 / T
			- 5.8927e-8 * T * T * T;
	}
	return -25383.581
		+ 299.31255 * T
		- 46.0 * T * log(T)
		+ 2.29603e31 * pow(T, -9.0);
}

static double fcc_iron_hidayat_g0_fe(double T, double p, FpropsError *err){
	double gmag = 0.0;
	double g;
	(void)p;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T >= 298.0) || !(T <= 6000.0)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	if(T <= 1811.0){
		double ghser = fcc_iron_hidayat_ghser_fe(T, err);
		if(!isfinite(ghser)){
			if(err){
				*err = FPROPS_RANGE_ERROR;
			}
			return NAN;
		}
		g = ghser
			- 1462.4
			+ 8.282 * T
			- 1.15 * T * log(T)
			+ 6.4e-4 * T * T;
	}else{
		g = -27098.266
			+ 300.25256 * T
			- 46.0 * T * log(T)
			+ 2.78854e31 * pow(T, -9.0);
	}
	if(!hillert_jarl_gmag(T, -201.0, -2.1, 0.28, &gmag)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	return g + gmag;
}

static double fcc_iron_hidayat_g0_o(double T, double p, FpropsError *err){
	(void)p;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T >= 298.0) || !(T <= 2000.0)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	return 120184.8
		+ 139.1406 * T
		- 24.5000 * T * log(T)
		- 9.8420e-4 * T * T
		- 0.12938e-6 * T * T * T
		+ 322517.0 / T;
}

static double fcc_iron_hidayat_gex(double T, double x, FpropsError *err){
	double L;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T >= 298.0) || !(T <= 2000.0) || !(x >= 0.0) || !(x <= 1.0)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	L = -315652.63336 + 27.6144 * T;
	return x * (1.0 - x) * L;
}

static double fcc_iron_hidayat_dgex_dx(double T, double x, FpropsError *err){
	double L;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T >= 298.0) || !(T <= 2000.0) || !(x >= 0.0) || !(x <= 1.0)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	L = -315652.63336 + 27.6144 * T;
	return (1.0 - 2.0 * x) * L;
}

static double fcc_iron_hidayat_d2gex_dx2(double T, double x, FpropsError *err){
	(void)x;
	if(err){
		*err = FPROPS_NO_ERROR;
	}
	if(!(T >= 298.0) || !(T <= 2000.0)){
		if(err){
			*err = FPROPS_RANGE_ERROR;
		}
		return NAN;
	}
	return 2.0 * (315652.63336 - 27.6144 * T);
}

static const BinarySolutionModel fcc_iron_model = {
	"fcc_iron_hidayat",
	&fcc_iron_hidayat_g0_fe,
	&fcc_iron_hidayat_g0_o,
	&fcc_iron_hidayat_gex,
	&fcc_iron_hidayat_dgex_dx,
	&fcc_iron_hidayat_d2gex_dx2,
	0.0,
	1.0
};

static const char *elements_fcc_fe[] = {"Fe"};
static const double stoich_fcc_fe[] = {1.0};

static const char *elements_fcc_o[] = {"O"};
static const double stoich_fcc_o[] = {1.0};

static const BinarySolutionPhaseDef fcc_iron_phase = {
	"fcc_iron",
	"hidayat_2015",
	"Fcc_Fe",
	"Fcc_O",
	1, elements_fcc_fe, stoich_fcc_fe,
	1, elements_fcc_o, stoich_fcc_o,
	&fcc_iron_model
};

const BinarySolutionPhaseDef *fcc_iron_hidayat_phase(void){
	return &fcc_iron_phase;
}
